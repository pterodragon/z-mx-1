//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// mbedtls C++ wrapper - main SSL component

#ifndef Ztls_HPP
#define Ztls_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZtlsLib.hpp>

#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cache.h>
#include <mbedtls/ssl_ticket.h>

#include <ZuString.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>
#include <ZiMultiplex.hpp>
#include <ZiIOBuf.hpp>

#include <ZtlsRandom.hpp>

namespace Ztls {

// mbedtls runs sharded within a dedicated thread, without lock contention

// API functions: listen, connect, disconnect/disconnect_, send/send_
// API callbacks: accepted, connected, disconnected, process

// Function Category | I/O Threads |        TLS thread          | App threads
// ------------------|-------------|----------------------------|------------
// Server            | accepted()  | connected() disconnected() | listen()
// Client            |             | connected() disconnected() | connect()
// Disconnect        |             | disconnect_()              | disconnect()
// Transmission (Tx) |             | send_()                    | send()
// Reception    (Rx) |             | process()                  |

// IOBuf buffers are used to transport data between threads (-+> arrows below)

// I/O Threads |                    TLS thread                   | App threads
// ------------|-------------------------------------------------|------------
//     I/O Rx -+> Rx input  -> Decryption -> Rx output -> App Rx |
// ------------|                                                 |
//     I/O Tx <+- Tx output <- Encryption <- Tx input           <+- App Tx
// ------------|-------------------------------------------------|------------

// FIXME - move to uint8_t * for all data ptrs (from char *)

using IOBuf = ZiIOBuf;

template <typename Link_, typename LinkRef>
class LinkTCP : public ZiConnection {
public:
  using Link = Link_;

  inline LinkTCP(LinkRef link, const ZiCxnInfo &ci) :
    ZiConnection(link->app()->mx(), ci), m_link(ZuMv(link)) { }

  void connected(ZiIOContext &io) { m_link->connected_(this, io); }
  void disconnected() { m_link->disconnected_(this); }

private:
  LinkRef	m_link;
};

template <typename App, typename Impl, typename TCP_> class Link {
public:
  using TCP = TCP_;
friend TCP;

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  inline Link(App *app) : m_app(app) {
    mbedtls_ssl_init(&m_ssl);
    mbedtls_ssl_setup(&m_ssl, app->conf());
    mbedtls_ssl_set_bio(&m_ssl, this, txOut_, rxIn_, nullptr);
  }
  inline ~Link() {
    mbedtls_ssl_free(&m_ssl);
  }

  ZuInline App *app() const { return m_app; }
  ZuInline TCP *tcp() const { return m_tcp; }

protected:
  ZuInline mbedtls_ssl_context *ssl() { return &m_ssl; }

private:
  void connected_(TCP *tcp, ZiIOContext &io) {
    if (ZuUnlikely(m_tcp && m_tcp != tcp)) m_tcp->close();
    m_tcp = tcp;
    m_rxBuf = new IOBuf(this, 0);
    io.init(ZiIOFn{this, [](Link *link, ZiIOContext &io) -> uintptr_t {
      link->rx(io);
      return 0;
    }}, m_rxBuf->data_, IOBuf::Size, 0);
    impl()->connected__();
  }

  void disconnected_(TCP *tcp) {
    if (m_tcp == tcp) m_tcp = nullptr;
    app()->invoke(impl(), [](Impl *impl) {
      mbedtls_ssl_session_reset(&impl->m_ssl);
      impl->disconnected();
    });
  }

  void rx(ZiIOContext &io) { // I/O Rx thread
    io.offset += io.length;
    m_rxBuf->length = io.offset;
    app()->run([buf = ZuMv(m_rxBuf)]() {
      auto link = static_cast<Link *>(buf->owner);
      link->recv_(ZuMv(buf));
    });
    m_rxBuf = new IOBuf(this, 0);
    io.ptr = m_rxBuf->data_;
    io.length = IOBuf::Size;
    io.offset = 0;
  }

  void recv_(ZmRef<IOBuf> buf) { // TLS thread
    m_rxInBuf = ZuMv(buf); // f_recv reads out of rxInBuf
    m_rxInOffset = 0;
    if (ZuLikely(m_ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER)) {
recv:
      while (recv());
    } else {
      while (handshake())
	if (ZuLikely(m_ssl.state == MBEDTLS_SSL_HANDSHAKE_OVER))
	  goto recv;
    }
    m_rxInBuf = nullptr;
  }

  // ssl bio f_recv function - TLS thread
  static int rxIn_(void *link_, unsigned char *ptr, size_t len) {
    return static_cast<Link *>(link_)->rxIn(ptr, len);
  }
  int rxIn(void *ptr, size_t len) { // TLS thread
    if (!m_rxInBuf) return MBEDTLS_ERR_SSL_WANT_READ;
    unsigned offset = m_rxInOffset;
    int avail = m_rxInBuf->length - offset;
    if (avail <= 0) return MBEDTLS_ERR_SSL_WANT_READ;
    if (len > (size_t)avail) len = avail;
    memcpy(ptr, m_rxInBuf->data_ + offset, len);
    m_rxInOffset = offset + len;
    return len;
  }

protected:
  bool handshake() { // TLS thread
    int n = mbedtls_ssl_handshake(&m_ssl);

    if (n) {
      switch (n) {
	case MBEDTLS_ERR_SSL_WANT_READ:
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
	case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
#endif
	  break;
	case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
	case MBEDTLS_ERR_SSL_CONN_EOF:
	  break;
	case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED: {
	  const char *hostname = m_ssl.hostname;
	  if (!hostname) hostname = "(null)";
	  app()->logError(
	      "server \"", hostname, "\": unable to verify X.509 cert");
	  disconnect_();
	} break;
	default:
	  app()->logError("mbedtls_ssl_handshake() returned ", n);
	  disconnect_();
	  break;
      }
      return false;
    }

    impl()->verify_(); // client only - verify server cert
    impl()->save_(); // client only - save session for subsequent reconnect

    impl()->connected(m_ssl.hostname, mbedtls_ssl_get_alpn_protocol(&m_ssl));

    return recv();
  }

private:
  bool recv() { // TLS thread
    int n = mbedtls_ssl_read(&m_ssl,
	(unsigned char *)(m_rxOutBuf + m_rxOutOffset),
	MBEDTLS_SSL_MAX_CONTENT_LEN - m_rxOutOffset);

    if (n < 0) {
      switch (n) {
	case MBEDTLS_ERR_SSL_WANT_READ:
#ifdef MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS
	case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
#endif
	  break;
	case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
	case MBEDTLS_ERR_SSL_CONN_EOF:
	  break;
	default:
	  app()->logError("mbedtls_ssl_read() returned ", n);
	  disconnect_();
	  break;
      }
      return false;
    }

    m_rxOutOffset += n;

    do {
      n = impl()->process(m_rxOutBuf, m_rxOutOffset);
      if (n < 0) {
	m_rxOutOffset = 0;
	disconnect_();
	return false;
      }
      if (!n) break;
      if (n < m_rxOutOffset)
	memmove(m_rxOutBuf, m_rxOutBuf + n, m_rxOutOffset -= n);
      else
	m_rxOutOffset = 0;
    } while (m_rxOutOffset);

    return true;
  }

public:
  void send(const uint8_t *data, unsigned len) { // App thread(s)
    if (ZuUnlikely(!len)) return;
    unsigned offset = 0;
    do {
      unsigned n = len - offset;
      if (ZuUnlikely(n > IOBuf::Size)) n = IOBuf::Size;
      ZmRef<IOBuf> buf = new IOBuf(this, n);
      memcpy(buf->data_, data + offset, n);
      app()->invoke([buf = ZuMv(buf)]() mutable {
	auto link = static_cast<Link *>(buf->owner);
	link->send_(ZuMv(buf));
      });
      offset += n;
    } while (offset < len);
  }
  void send(ZmRef<IOBuf> buf) {
    if (ZuUnlikely(!buf->length)) return;
    app()->invoke([buf = ZuMv(buf)]() mutable {
      auto link = static_cast<Link *>(buf->owner);
      link->send_(ZuMv(buf));
    });
  }

  void send_(ZmRef<IOBuf> buf) { // TLS thread
    send_(buf->data(), buf->length);
  }
  void send_(const uint8_t *data, unsigned len) { // TLS thread
    if (ZuUnlikely(!len)) return;
    unsigned offset = 0;
    do {
      int n = mbedtls_ssl_write(&m_ssl, data + offset, len - offset);
      if (n < 0) {
	app()->logError("mbedtls_ssl_write() returned ", n);
	return;
      }
      offset += n;
    } while (offset < len);
  }

private:
  // ssl bio f_send function - TLS thread
  static int txOut_(void *link_, const unsigned char *data, size_t len) {
    return static_cast<Link *>(link_)->txOut(data, len);
  }
  int txOut(const uint8_t *data, size_t len) { // TLS thread
    if (ZuUnlikely(!len)) return 0;
    unsigned offset = 0;
    auto mx = app()->mx();
    do {
      unsigned n = len - offset;
      if (ZuUnlikely(n > IOBuf::Size)) n = IOBuf::Size;
      ZmRef<IOBuf> buf = new IOBuf(this, n);
      memcpy(buf->data_, data + offset, n);
      mx->txRun([buf = ZuMv(buf)]() mutable {
	auto link = static_cast<Link *>(buf->owner);
	if (ZuUnlikely(!link->m_tcp)) return;
	link->m_tcp->send_(ZiIOFn{ZuMv(buf),
	  [](IOBuf *buf, ZiIOContext &io) {
	    io.init(ZiIOFn{io.fn.mvObject<IOBuf>(),
	      [](IOBuf *buf, ZiIOContext &io) {
		if (ZuUnlikely((io.offset += io.length) < io.size)) return;
	      }}, buf->data_, buf->length, 0);
	  }});
      });
      offset += n;
    } while (offset < len);
    return len;
  }

public:
  void disconnect() { // App thread(s)
    app()->invoke(this, [](Link *link) { link->disconnect_(); });
  }
  void disconnect_() { // TLS thread
    int n = mbedtls_ssl_close_notify(&m_ssl);
    if (n) app()->logWarning("mbedtls_ssl_close_notify() returned ", n);
    app()->mx()->txInvoke(this, [](Link *link) {
      if (auto tcp = ZuMv(link->m_tcp)) tcp->disconnect();
    });
  }

private:
  App			*m_app = nullptr;
  ZmRef<TCP>		m_tcp;
  mbedtls_ssl_context	m_ssl;

  // I/O Tx thread
  ZmRef<IOBuf>		m_rxBuf;

  // TLS thread
  ZmRef<IOBuf>		m_rxInBuf;
  unsigned		m_rxInOffset;
  uint8_t		m_rxOutBuf[MBEDTLS_SSL_MAX_CONTENT_LEN];
  unsigned		m_rxOutOffset;
};

template <typename App, typename Impl>
class CliLink : public Link<App, Impl, LinkTCP<Impl, Impl *> > {
public:
  using TCP = LinkTCP<Impl>;
  using Base = Link<App, Impl, TCP>;
friend Base;

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  CliLink(App *app) : Base(app) {
    mbedtls_ssl_session_init(&m_session);
  }
  ~CliLink() {
    mbedtls_ssl_session_free(&m_session);
  }

  bool connect(ZuString server, uint16_t port) { // App thread(s)
    thread_local ZmSemaphore sem;
    bool ok;
    this->app()->invoke([&, sem = &sem]() mutable {
      ok = connect_(server, port);
      sem->post();
    });
    sem.wait();
    return ok;
  }

  bool connect_(ZuString server, uint16_t port) { // TLS thread
    ZiIP ip = server;
    if (!ip) {
      this->app()->logError('"', server, "\": hostname lookup failure");
      return false;
    }
    {
      int n;
      n = mbedtls_ssl_set_hostname(this->ssl(), server);
      if (n) {
	this->app()->logError(
	    "mbedtls_ssl_set_hostname(\"", server, "\") returned ", n);
	return false;
      }
    }

    this->load_(); // load any session saved from a previous connection

    this->app()->mx()->connect(
	ZiConnectFn{impl(), [](Impl *impl, const ZiCxnInfo &ci) -> uintptr_t {
	  return (uintptr_t)(new TCP(impl, ci));
	}},
	ZiFailFn{impl(), [](Impl *impl, bool transient) {
	  impl->connectFailed(transient);
	}},
	ZiIP(), 0, ip, port);

    return true;
  }

private:
  void save_() {
    int n = mbedtls_ssl_get_session(this->ssl(), &m_session);
    if (n) {
      this->app()->logError("mbedtls_ssl_get_session() returned ", n);
      return;
    }
    m_saved = true;
  }

  void load_() {
    if (!m_saved) return;
    int n = mbedtls_ssl_set_session(this->ssl(), &m_session);
    if (n) {
      this->app()->logWarning("mbedtls_ssl_set_session() returned ", n);
      return;
    }
  }

  void connected__() { // Tx thread
    this->app()->invoke(this, [](CliLink *link) {
      while (link->handshake());
    });
  }

  void verify_() {
    if (this->ssl()->conf->endpoint == MBEDTLS_SSL_IS_CLIENT) {
      uint32_t flags;
      if (flags = mbedtls_ssl_get_verify_result(this->ssl())) {
	ZtString s;
	s << "server \"" << this->ssl()->hostname
	  << "\": X.509 cert verification failure: ";
	static const char *errors[] = {
	  "validity has expired",
	  "revoked (is on a CRL)",
	  "CN does not match with the expected CN",
	  "not correctly signed by the trusted CA",
	  "CRL is not correctly signed by the trusted CA",
	  "CRL is expired",
	  "certificate missing",
	  "certificate verification skipped",
	  "unspecified/other",
	  "validity starts in the future",
	  "CRL is from the future",
	  "usage does not match the keyUsage extension",
	  "usage does not match the extendedKeyUsage extension",
	  "usage does not match the nsCertType extension",
	  "signed with an bad hash",
	  "signed with an bad PK alg (e.g. RSA vs ECDSA)",
	  "signed with bad key (e.g. bad curve, RSA too short)",
	  "CRL signed with an bad hash",
	  "CRL signed with bad PK alg (e.g. RSA vs ECDSA)",
	  "CRL signed with bad key (e.g. bad curve, RSA too short)"
	};
	{
	  bool comma = false;
	  for (unsigned i = 0; i < sizeof(errors) / sizeof(errors[0]); i++)
	    if (flags & (1<<i)) {
	      if (comma) s << ", ";
	      comma = true;
	      s << errors[i];
	    }
	}
	this->app()->logError(s);
	this->disconnect_();
      }
    }
  }

private:
  mbedtls_ssl_session	m_session;
  bool			m_saved = false;
};

template <typename App, typename Impl>
class SrvLink : public Link<App, Impl, LinkTCP<Impl, ZmRef<Impl> > > {
public:
  using TCP = LinkTCP<Impl>;
  using Base = Link<App, Impl, TCP>;
friend Base;

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  SrvLink(App *app) : Base(app) { }

private:
  void save_() { } // unused
  void load_() { } // unused
  void connected__() { } // unused
  void verify_() { } // unused
};

template <typename App_> class Engine : public Random {
public:
  using App = App_;
template <typename, typename, typename> friend class Link;
template <typename, typename> friend class CliLink;
template <typename, typename> friend class SrvLink;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  inline Engine() {
    mbedtls_x509_crt_init(&m_cacert);
    mbedtls_ssl_config_init(&m_conf);
  }
  inline ~Engine() {
    mbedtls_ssl_config_free(&m_conf);
    mbedtls_x509_crt_free(&m_cacert);
  }

  template <typename L>
  bool init(ZiMultiplex *mx, ZuString thread, L l) {
    m_mx = mx;
    if (!(m_thread = m_mx->tid(thread))) {
      logError("invalid Rx thread ID \"", thread, '"');
      return false;
    }
    thread_local ZmSemaphore sem;
    bool ok;
    invoke([&, l = ZuMv(l), sem = &sem]() mutable {
      ok = init_(ZuMv(l));
      sem->post();
    });
    sem.wait();
    return ok;
  }
private:
  template <typename L>
  bool init_(L l) {
    mbedtls_ssl_conf_dbg(&m_conf, [](
	  void *app_, int level,
	  const char *file, int line, const char *message) {
      auto app = static_cast<App *>(app_);
      int sev;
      switch (level) {
	case 0: sev = Ze::Error;
	case 1: sev = Ze::Warning;
	case 2: sev = Ze::Info;
	case 3: sev = Ze::Info;
	default: sev = Ze::Debug;
      }
      app->exception(new ZeEvent(sev, file, line, "", ZtString{message}));
    }, app());
    if (!Random::init()) {
      app()->logError("mbedtls_ctr_drbg_seed() failed");
      return false;
    }
    mbedtls_ssl_conf_rng(&m_conf, mbedtls_ctr_drbg_random, ctr_drbg());
    mbedtls_ssl_conf_renegotiation(&m_conf, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
    return l();
  }

public:
  void final() { }

  ZuInline ZiMultiplex *mx() const { return m_mx; }

  template <typename ...Args> ZuInline void run(Args &&... args)
    { m_mx->run(m_thread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void invoke(Args &&... args)
    { m_mx->invoke(m_thread, ZuFwd<Args>(args)...); }

protected:
  // TLS thread
  ZuInline mbedtls_ssl_config *conf() { return &m_conf; }

  void exception(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); } // default

  inline static void log__(ZmStream &s) { }
  template <typename Arg0, typename ...Args>
  inline static void log__(ZmStream &s, Arg0 &&arg0, Args &&... args) {
    s << ZuFwd<Arg0>(arg0);
    log__(s, ZuMv(args)...);
  }
  template <int Level, typename ...Args> void log_(Args &&... args) {
    app()->exception(ZeEVENT_(Level, (
      [args...]
#if 0
	// LATER - uncomment this when C++20 P0780R2 is implemented
      [...args = ZuMv(args)]
#endif
	  (const ZeEvent &, ZmStream &s) mutable {
	log__(s, ZuFwd<Args>(args)...);
      })));
  }
  template <typename ...Args> void logInfo(Args &&... args)
    { log_<Ze::Info>(ZuFwd<Args>(args)...); }
  template <typename ...Args> void logWarning(Args &&... args)
    { log_<Ze::Warning>(ZuFwd<Args>(args)...); }
  template <typename ...Args> void logError(Args &&... args)
    { log_<Ze::Error>(ZuFwd<Args>(args)...); }
  template <typename ...Args> void logFatal(Args &&... args)
    { log_<Ze::Fatal>(ZuFwd<Args>(args)...); }

  // Arch/Ubuntu/Debian/SLES - /etc/ssl/certs
  // Fedora/CentOS/RHEL - /etc/pki/tls/certs
  // Android - /system/etc/security/cacerts
  // FreeBSD - /usr/local/share/certs
  // NetBSD - /etc/openssl/certs
  // AIX - /var/ssl/certs
  // Windows - ROOT certificate store (using Cert* API)
  inline bool loadCA(ZuString path) {
    int n;
    const char *function;
    if (ZiFile::isdir(path)) {
      function = "mbedtls_x509_crt_parse_path";
      n = mbedtls_x509_crt_parse_path(&m_cacert, path);
    } else {
      function = "mbedtls_x509_crt_parse_file";
      n = mbedtls_x509_crt_parse_file(&m_cacert, path);
    }
    if (n < 0) {
      logError(function, "() returned ", n);
      return false;
    }
    mbedtls_ssl_conf_ca_chain(&m_conf, &m_cacert, nullptr);
    return true;
  }

private:
  ZiMultiplex			*m_mx = nullptr;
  unsigned			m_thread = 0;

  mbedtls_x509_crt		m_cacert;
  mbedtls_ssl_config		m_conf;
};

// CRTP - implementation must conform to the following interface:
#if 0
  struct App : public Client<App> {
    void exception(ZmRef<ZeEvent>); // optional

    struct Link : public CliLink<App, Link> {
      // TLS thread - handshake completed
      void connected(const char *hostname, const char *alpn);

      void disconnected(); // TLS thread
      void connectFailed(bool transient); // I/O Tx thread
      
      // process() should return:
      // +ve - the number of bytes processed
      // 0   - more data needed - continue appending to Rx output buffer
      // -ve - disconnect, abandon any remaining Rx dats
      int process(const char *data, unsigned len); // process received data
    };
  };
#endif
template <typename App> class Client : public Engine<App> {
public:
  using Base = Engine<App>;
friend Base;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  inline bool init(ZiMultiplex *mx, ZuString thread,
      ZuString caPath, const char **alpn) {
    return Base::init(mx, thread, [&]() -> bool {
      mbedtls_ssl_config_defaults(this->conf(),
	  MBEDTLS_SSL_IS_CLIENT,
	  MBEDTLS_SSL_TRANSPORT_STREAM,
	  MBEDTLS_SSL_PRESET_DEFAULT);

      mbedtls_ssl_conf_session_tickets(this->conf(),
	  MBEDTLS_SSL_SESSION_TICKETS_ENABLED);

      mbedtls_ssl_conf_authmode(this->conf(), MBEDTLS_SSL_VERIFY_REQUIRED);
      if (!this->loadCA(caPath)) return false;
      if (alpn) mbedtls_ssl_conf_alpn_protocols(this->conf(), alpn);
      return true;
    });
  }

  inline void final() { Base::final(); }
};

// CRTP - implementation must conform to the following interface:
#if 0
  struct App : public Server<App> {
    void exception(ZmRef<ZeEvent>); // optional

    struct Link : public SrvLink<App, Link> {
      // TLS thread - handshake completed
      void connected(const char *, const char *alpn);

      void disconnected(); // TLS thread
      void connectFailed(bool transient); // I/O Tx thread
      
      // process() should return:
      // +ve - the number of bytes processed
      // 0   - more data needed - continue appending to Rx output buffer
      // -ve - disconnect, abandon any remaining Rx dats
      int process(const char *data, unsigned len); // process received data
    };

    inline Link::TCP *accepted(const ZiCxnInfo &ci) {
      // ... potentially return nullptr if too many open connections
      ZmRef<Link> link = new Link(this);
      auto tcp = new Link::TCP(link, ci);
      // ... store link in server's container
      return tcp;
    }

    unsigned nAccepts(); // optional
    void listening(const ZiListenInfo &info); // optional
    void listenFailed(bool transient); // optional - can re-schedule listen()
  };
#endif
template <typename App> class Server : public Engine<App> {
public:
  using Base = Engine<App>;
friend Base;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  inline Server() {
    mbedtls_x509_crt_init(&m_cert);
    mbedtls_pk_init(&m_key);
    mbedtls_ssl_cache_init(&m_cache);
    mbedtls_ssl_ticket_init(&m_ticket_ctx);
  }
  inline ~Server() {
    mbedtls_ssl_ticket_free(&m_ticket_ctx);
    mbedtls_ssl_cache_free(&m_cache);
    mbedtls_pk_free(&m_key);
    mbedtls_x509_crt_free(&m_cert);
  }

  inline bool init(ZiMultiplex *mx, ZuString thread,
      ZuString caPath, const char **alpn, ZuString certPath,
      ZuString keyPath, int cacheMax = -1, int cacheTimeout = -1) {
    return Base::init(mx, thread, [&]() -> bool {
      mbedtls_ssl_config_defaults(this->conf(),
	  MBEDTLS_SSL_IS_SERVER,
	  MBEDTLS_SSL_TRANSPORT_STREAM,
	  MBEDTLS_SSL_PRESET_DEFAULT);

      if (cacheMax >= 0) // library default is 50
	mbedtls_ssl_cache_set_max_entries(&m_cache, cacheMax);
      if (cacheTimeout >= 0) // library default is 86400, i.e. one day
	mbedtls_ssl_cache_set_timeout(&m_cache, cacheTimeout);

      mbedtls_ssl_conf_session_cache(this->conf(), &m_cache,
	  mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);

      if (mbedtls_ssl_ticket_setup(&m_ticket_ctx,
	    mbedtls_ctr_drbg_random, this->ctr_drbg(),
	    MBEDTLS_CIPHER_AES_256_GCM,
	    cacheTimeout)) return false;
      mbedtls_ssl_conf_session_tickets_cb(this->conf(),
	  mbedtls_ssl_ticket_write,
	  mbedtls_ssl_ticket_parse,
	  &m_ticket_ctx);

      mbedtls_ssl_conf_authmode(this->conf(), MBEDTLS_SSL_VERIFY_REQUIRED);
      if (!this->loadCA(caPath)) return false;
      if (alpn) mbedtls_ssl_conf_alpn_protocols(this->conf(), alpn);

      if (mbedtls_x509_crt_parse_file(&m_cert, certPath)) return false;
      if (mbedtls_pk_parse_keyfile(&m_key, keyPath, "")) return false;
      if (mbedtls_ssl_conf_own_cert(this->conf(), &m_cert, &m_key))
	return false;
      return true;
    });
  }

  inline void final() { Base::final(); }

  void listen(ZiIP ip, unsigned port) {
    this->mx()->listen(
	  ZiListenFn(app(), [](App *app, const ZiListenInfo &info) {
	    app->listening(info);
	  }),
	  ZiFailFn(app(), [](App *app, bool transient) {
	    app->listenFailed(transient);
	  }),
	  ZiConnectFn(app(), [](App *app, const ZiCxnInfo &ci) -> uintptr_t {
	    return (uintptr_t)app->accepted(ci);
	  }),
	  ip, port, app()->nAccepts(), ZiCxnOptions());
  }

protected:
  inline unsigned nAccepts() { return 8; } // default
  inline void listening(const ZiListenInfo &info) { // default
    this->logInfo("listening(", info.ip, ':', info.port, ')');
  }
  inline void listenFailed(bool transient) { // default
    this->logWarning("listen() failed ", transient ? "(transient)" : "");
  }

private:
  mbedtls_x509_crt		m_cert;
  mbedtls_pk_context		m_key;
  mbedtls_ssl_cache_context	m_cache;
  mbedtls_ssl_ticket_context	m_ticket_ctx;
};

}

#endif /* Ztls_HPP */
