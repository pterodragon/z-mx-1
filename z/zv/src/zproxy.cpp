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

#include <ZuString.hpp>
#include <ZuPrint.hpp>
#include <ZuPolymorph.hpp>

#include <ZmHash.hpp>
#include <ZmList.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmTrap.hpp>
#include <ZmSemaphore.hpp>

#include <ZtString.hpp>
#include <ZtArray.hpp>
#include <ZtRegex.hpp>

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>
#include <ZvCmd.hpp>
#include <ZvMultiplexCf.hpp>

class IOBuf;		// I/O buffer
class Connection;	// ZiConnection, owns queue of IO buffers
class Proxy;		// pair of active connections
class Listener;		// spawns proxies
class App;		// the app (singleton) owns mx, listeners and proxies

#define BufSize (32<<10)

// wrapper for { op, result, errno }
typedef ZuTuple<const char *, int, ZeError> Error;
// overloaded print for various types
template <typename T> struct Print {
  Print(const T &v_) : v(v_) { }

  // Error wrapper
  template <typename S, typename T_ = T>
  inline typename ZuIs<T_, Error>::T print(S &s) const {
    s << v.p1() << "() - " << Zi::resultName(v.p2()) << " - " << v.p3();
  }
  // ZiCxnInfo
  template <typename S, typename T_ = T>
  inline typename ZuIs<T_, ZiCxnInfo>::T print(S &s) const {
    s << (v.type() == ZiCxnType::TCPIn ? "IN  " : "OUT ") <<
      v.localIP << ':' << ZuBoxed(v.localPort) << " -> " <<
      v.remoteIP << ':' << ZuBoxed(v.remotePort);
  }

  const T &v;
};
template <typename T> struct ZuPrint<Print<T> > : public ZuPrintFn { };
template <typename T> inline Print<T> print(const T &v) { return Print<T>(v); }

class IOBuf : public ZmPolymorph {
public:
  inline IOBuf(Connection *connection) :
    m_connection(connection), m_buf(BufSize) { }
  inline IOBuf(Connection *connection, const ZmTime &stamp) :
    m_connection(connection), m_stamp(stamp), m_buf(BufSize) { }

  inline Connection *connection() const { return m_connection; }
  inline void connection(Connection *connection) { m_connection = connection; }

  inline const ZmTime &stamp() const { return m_stamp; }
  inline ZmTime &stamp() { return m_stamp; }

  inline const ZtArray<char> &buf() const { return m_buf; }
  inline ZtArray<char> &buf() { return m_buf; }

  inline const char *data() const { return m_buf.data(); }
  inline char *data() { return m_buf.data(); }
  inline unsigned length() const { return m_buf.length(); }
  template <typename ...Args>
  inline void append(Args &&... args) { m_buf.append(ZuFwd<Args>(args)...); }
  template <typename ...Args>
  inline void splice(Args &&... args) { m_buf.splice(ZuFwd<Args>(args)...); }

  void recv(ZiIOContext *io);
  void recv_(ZiIOContext &io);
  void rcvd_(ZiIOContext &io);
  void send(ZiIOContext *io);
  void send_(ZiIOContext &io);
  void sent_(ZiIOContext &io);

private:
  Connection	*m_connection;
  ZmTime	m_stamp;
  ZtArray<char>	m_buf;
};

typedef ZmList<ZmRef<IOBuf>, ZmListLock<ZmNoLock> > IOList;

class IOQueue : protected IOList {
public:
  inline IOQueue() : m_size(0) { }

  inline uint32_t size() const { return m_size; }

  inline unsigned count() const { return IOList::count(); }

  inline ZmRef<IOBuf> head() const { return IOList::head(); }
  inline ZmRef<IOBuf> tail() const { return IOList::tail(); }

  inline void push(ZmRef<IOBuf> &&ioBuf) {
    m_size += ioBuf->length();
    IOList::push(ZuMv(ioBuf));
  }
  inline void unshift(ZmRef<IOBuf> &&ioBuf) {
    m_size += ioBuf->length();
    IOList::unshift(ZuMv(ioBuf));
  }
  inline ZmRef<IOBuf> pop() {
    ZmRef<IOBuf> ioBuf = IOList::pop();
    if (ioBuf) m_size -= ioBuf->length();
    return ioBuf;
  }
  inline ZmRef<IOBuf> shift() {
    ZmRef<IOBuf> ioBuf = IOList::shift();
    if (ioBuf) m_size -= ioBuf->length();
    return ioBuf;
  }

private:
  uint32_t	m_size;
};

class Connection : public ZiConnection {
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

public:
  enum {
    In		= 0x001,	// incoming
    Hold	= 0x002,	// held
    SuspRecv	= 0x004,	// read suspended
    SuspSend	= 0x008,	// send suspended
    Trace	= 0x010,	// trace
    Drop	= 0x020		// drop
  };

  Connection(Proxy *proxy,
      uint32_t flags, double latency, uint32_t frag,
      uint32_t pack, double delay, const ZiConnectionInfo &ci);

  inline ZiMultiplex *mx() const { return m_mx; }

  inline ZmRef<Proxy> proxy() const { return m_proxy; }
  inline void proxy(Proxy *p) { m_proxy = p; }

  inline Connection *peer() const { return m_peer; }
  inline void peer(Connection *peer) { m_peer = peer; }

  inline uint32_t queueSize() const { return m_queue.size(); }
  inline uint32_t flags() const { return m_flags; }
  inline double latency() const { return m_latency; }
  inline uint32_t frag() const { return m_frag; }
  inline uint32_t pack() const { return m_pack; }
  inline double delay() const { return m_delay; }

  void connected(ZiIOContext &io);
  void connected_();
  void disconnected();

  void send(ZmRef<IOBuf> ioBuf);

  void recv();
  void recv(ZiIOContext *io);
  void recv_(ZmRef<IOBuf> ioBuf, ZiIOContext &io);
  void delayedSend();
  void send();
  void send(ZiIOContext *io);
  void send(Guard &guard, ZiIOContext *io);
  void send_(IOBuf *ioBuf, ZiIOContext &io);

  void hold() { m_flags |= Hold; }
  void release();

  void suspRecv() { m_flags |= SuspRecv; }
  void resRecv() { m_flags &= ~SuspRecv; recv(); }
  void suspSend() { m_flags |= SuspSend; }
  void resSend() { m_flags &= ~SuspSend; send(); }

  void trace(bool on) { on ? (m_flags |= Trace) : (m_flags &= ~Trace); }
  void drop(bool on) { on ? (m_flags |= Drop) : (m_flags &= ~Drop); }

  void latency(double n) { m_latency = n; }
  void frag(uint32_t n) { m_frag = n; }
  void pack(uint32_t n) { m_pack = n; }
  void delay(double n) { m_delay = n; }

  template <typename S> void print(S &) const;

private:
  ZiMultiplex		*m_mx;
  ZmRef<Proxy>		m_proxy;
  Connection		*m_peer;
  Lock			m_lock;
    IOQueue		  m_queue;
    bool		  m_sendPending;
  uint32_t		m_flags;
  double		m_latency;
  uint32_t		m_frag;
  uint32_t		m_pack;
  double		m_delay;
};
template <> struct ZuPrint<Connection> : public ZuPrintFn { };

class Proxy : public ZmPolymorph {
public:
  struct SrcPortAccessor;
friend struct SrcPortAccessor;
  struct SrcPortAccessor : public ZuAccessor<Proxy *, int> {
    static unsigned value(Proxy *p);
  };

  Proxy(Listener *listener);
  virtual ~Proxy() { }

  inline ZiMultiplex *mx() const { return m_mx; }
  inline App *app() const { return m_app; }

  inline ZmRef<Listener> listener() const { return m_listener; }

  inline ZmRef<Connection> in() { return m_in; }
  inline ZmRef<Connection> out() { return m_out; }

  inline ZuString tag() const { return m_tag; }

  void connected(Connection *connection);
  void connect2();
  ZiConnection *connected2(const ZiConnectionInfo &ci);
  void failed2(bool transient);
  void disconnected(Connection *connection);

private:
  void status_(ZmStream &) const;
  template <typename S> inline void status_(S &s_) const {
    ZmStream s(s_);
    status_(s);
  }
public:
  struct Status;
friend struct Status;
  struct Status {
    template <typename S> inline void print(S &s) const { p.status_(s); }
    const Proxy &p;
  };
  inline Status status() const { return Status{*this}; }

private:
  ZiMultiplex		*m_mx;
  App			*m_app;
  ZmRef<Listener>	m_listener;
  ZmRef<Connection>	m_in;
  ZmRef<Connection>	m_out;
  ZuString		m_tag;
};
template <> struct ZuPrint<Proxy::Status> : public ZuPrintFn { };

template <typename S> inline void Connection::print(S &s) const
{
  const ZiCxnInfo &info = this->info();
  s << (info.type == ZiCxnType::TCPIn ? "IN  " : "OUT ") <<
    info.localIP << ':' << ZuBoxed(info.localPort) <<
    (this->up() ? " -> " : " !> ") <<
    info.remoteIP << ':' << ZuBoxed(info.remotePort) <<
    " (" << m_proxy->tag() << ") [" <<
    ((m_flags & Connection::Hold) ? 'H' : '-') <<
    ((m_flags & Connection::SuspRecv) ? 'R' : '-') <<
    ((m_flags & Connection::SuspSend) ? 'S' : '-') <<
    ((m_flags & Connection::Trace) ? 'T' : '-') <<
    ((m_flags & Connection::Drop) ? 'D' : '-') << ']';
}

struct ListenerPrintIn;
struct ListenerPrintOut;
class Listener : public ZmObject {
  typedef ZmHash<ZmRef<Proxy>,
	    ZmHashObject<ZuNull> > ProxyHash;

public:
  struct LocalPortAccessor;
friend struct LocalPortAccessor;
  struct LocalPortAccessor : public ZuAccessor<Listener *, int> {
    inline static int value(Listener *listener) {
      return listener->m_localPort;
    }
  };

  Listener(App *app, uint32_t cxnFlags,
      double cxnLatency, uint32_t cxnFrag, uint32_t cxnPack, double cxnDelay,
      ZiIP localIP, unsigned localPort, ZiIP remoteIP, unsigned remotePort,
      ZiIP srcIP, unsigned srcPort, ZuString tag, unsigned reconnectFreq);
  virtual ~Listener() { }

  void add(Proxy *proxy) { m_proxies->add(proxy); }
  void del(Proxy *proxy) { delete m_proxies->del(proxy); }

  inline ZiMultiplex *mx() const { return m_mx; }
  inline App *app() const { return m_app; }
  inline uint32_t cxnFlags() const { return m_cxnFlags; }
  inline double cxnLatency() const { return m_cxnLatency; }
  inline uint32_t cxnFrag() const { return m_cxnFrag; }
  inline uint32_t cxnPack() const { return m_cxnPack; }
  inline double cxnDelay() const { return m_cxnDelay; }
  inline ZiIP localIP() const { return m_localIP; }
  inline unsigned localPort() const { return m_localPort; }
  inline ZiIP remoteIP() const { return m_remoteIP; }
  inline unsigned remotePort() const { return m_remotePort; }
  inline ZiIP srcIP() const { return m_srcIP; }
  inline unsigned srcPort() const { return m_srcPort; }
  inline bool listening() const { return m_listening; }
  inline ZuString tag() const { return m_tag; }
  inline unsigned reconnectFreq() const { return m_reconnectFreq; }

  int start();
  void stop();

  ZiConnection *accepted(const ZiCxnInfo &ci);

private:
  void status_(ZmStream &) const;
  template <typename S> inline void status_(S &s_) const {
    ZmStream s(s_);
    status_(s);
  }
public:
  struct Status;
friend struct Status;
  struct Status {
    template <typename S> inline void print(S &s) const { l.status_(s); }
    const Listener &l;
  };
  inline Status status() const { return Status{*this}; }

  template <typename S> inline void print(S &s) const {
    s << (m_listening ? "LISTEN " : "STOPPED") << " (" << m_tag << ") " <<
      m_localIP << ':' << ZuBoxed(m_localPort) << " = " <<
      m_srcIP << ':' << ZuBoxed(m_srcPort) << " -> " <<
      m_remoteIP << ':' << ZuBoxed(m_remotePort);
  }

  ListenerPrintIn printIn() const;
  ListenerPrintOut printOut() const;

private:
  void ok(const ZiListenInfo &);
  void failed(bool transient);

friend struct ListenerPrintIn;
friend struct ListenerPrintOut;
  template <typename S> inline void printIn_(S &s) const {
    s << "NC:NC !> " << m_localIP << ':' << ZuBoxed(m_localPort) <<
      " (" << m_tag << ')';
  }
  template <typename S> inline void printOut_(S &s) const {
    s << m_srcIP << ':' << ZuBoxed(m_srcPort) << " !> " <<
      m_remoteIP << ':' << ZuBoxed(m_remotePort) << " (" << m_tag << ')';
  }

  ZiMultiplex		*m_mx;
  App			*m_app;
  ZmSemaphore		m_started;
  ZmRef<ProxyHash>	m_proxies;
  uint32_t		m_cxnFlags;
  double		m_cxnLatency;
  uint32_t		m_cxnFrag;
  uint32_t		m_cxnPack;
  double		m_cxnDelay;
  ZiIP			m_localIP;
  unsigned		m_localPort;
  ZiIP			m_remoteIP;
  unsigned		m_remotePort;
  ZiIP			m_srcIP;
  unsigned		m_srcPort;
  bool			m_listening;
  ZtString		m_tag;
  unsigned		m_reconnectFreq;
};
template <> struct ZuPrint<Listener::Status> : public ZuPrintFn { };

template <> struct ZuPrint<Listener> : public ZuPrintFn { };
struct ListenerPrintIn {
  inline ListenerPrintIn(const Listener &l_) : l(l_) { }
  template <typename S> inline void print(S &s) const { l.printIn_(s); }
  const Listener &l;
};
template <> struct ZuPrint<ListenerPrintIn> : public ZuPrintFn { };
struct ListenerPrintOut {
  inline ListenerPrintOut(const Listener &l_) : l(l_) { }
  template <typename S> inline void print(S &s) const { l.printOut_(s); }
  const Listener &l;
};
template <> struct ZuPrint<ListenerPrintOut> : public ZuPrintFn { };
inline ListenerPrintIn Listener::printIn() const {
  return ListenerPrintIn(*this);
}
inline ListenerPrintOut Listener::printOut() const {
  return ListenerPrintOut(*this);
}

bool validateTag(ZuString s) {
  const char *t = s.data();
  return !(s.length() < 2 || !*t || *t != '#');
}

template <typename S>
void parseAddr(const S &s, ZiIP &ip, uint16_t &port) {
  const auto &colon = ZtStaticRegexUTF8(":");
  const auto &nonDigit = ZtStaticRegexUTF8("\\D");
  ZtRegex::Captures c;
  if (!s) {
    ip = ZiIP();
    port = 0;
  } else if (colon.m(s, c)) {
    ip = c[0];
    port = ZuBox<unsigned>(c[2]);
  } else if (nonDigit.m(s)) {
    ip = s;
    port = 0;
  } else {
    ip = ZiIP();
    port = ZuBox<unsigned>(s);
  }
}

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

namespace Side {
  ZtEnumValues(In, Out, Both);
  ZtEnumNames("in", "out", "*");
}

namespace IOOp {
  ZtEnumValues(Send, Recv, Both);
  ZtEnumNames("send", "recv", "*");
}

class App : public ZmPolymorph, public ZvCmdServer {

  class Mx : public ZuObject, public ZiMultiplex {
  public:
    inline Mx() : ZiMultiplex(ZvMxParams()) { }
    inline Mx(ZvCf *cf) : ZiMultiplex(ZvMxParams(cf)) { }
  };

  typedef ZmHash<ZmRef<Listener>,
	    ZmHashIndex<Listener::LocalPortAccessor,
	      ZmHashObject<ZuNull> > > ListenerHash;

  typedef ZmHash<ZmRef<Proxy>,
	    ZmHashIndex<Proxy::SrcPortAccessor,
	      ZmHashObject<ZuNull> > > ProxyHash;

public:
  App() : m_verbose(false) {
    m_listeners = new ListenerHash(ZmHashParams().bits(4).loadFactor(1.0));
    m_proxies = new ProxyHash(ZmHashParams().bits(8).loadFactor(1.0));
  }

  void init(ZvCf *cf) {
    // cf->set("mx:debug", "1");
    m_mx = new Mx(cf->subset("mx", true));
    ZvCmdServer::init(m_mx, cf->subset("cmd", true));
    m_verbose = cf->getInt("verbose", 0, 1, false, 0);
    addCmd("proxy",
	"tag { type scalar } "
	"suspend { type flag } "
	"hold { type flag } "
	"trace { type flag } "
	"drop { type flag } "
	"latency { type scalar } "
	"frag { type scalar } "
	"pack { type scalar } "
	"delay { type scalar } "
	"reconnect { type scalar }",
	ZvCmdFn::Member<&App::proxy>::fn(this),
	"establish TCP proxy",
	"usage: proxy [LOCALIP:]LOCALPORT [REMOTEIP:]REMOTEPORT "
	    "[[SRCIP:][SRCPORT]] [OPTIONS]\n\n"
	    "Options:\n"
	    "  --tag=TAG\t- apply name tag (\"#default\" if unspecified)\n"
	    "  --suspend\t- suspend I/O initially\n"
	    "  --hold\t- hold connections open until released\n"
	    "  --trace\t- hex dump traffic\n"
	    "  --drop\t- drop (discard) incoming traffic\n"
	    "  --latency=N\t- introduce latency of N seconds\n"
	    "  --frag=N\t- fragment packets into N fragments\n"
	    "  --pack=N\t- consolidate packets into N bytes\n"
	    "  --delay=N\t- delay each receive by N seconds\n"
	    "  --reconnect=N\t- retry connect every N seconds (0 - disabled)");
    addCmd("stop", "",
	ZvCmdFn::Member<&App::stopListening>::fn(this),
	"stop listening (do not disconnect open connections)",
	"usage: stop #TAG|LOCALPORT");
    addCmd("hold", "",
	ZvCmdFn::Member<&App::hold>::fn(this),
	"hold [one side] open",
	"usage: hold SRCPORT|#TAG|all [in|out]");
    addCmd("release", "",
	ZvCmdFn::Member<&App::release>::fn(this),
	"release [one side], permit disconnect\n"
	"Note: remote-initiated disconnects always occur regardless",
	"usage: release SRCPORT|#TAG|all [in|out]");
    addCmd("disc", "",
	ZvCmdFn::Member<&App::disc>::fn(this),
	"disconnect SRCPORT",
	"disc SRCPORT|#TAG|all");
    addCmd("suspend", "",
	ZvCmdFn::Member<&App::suspend>::fn(this),
	"suspend I/O",
	"usage: suspend SRCPORT|#TAG|all [in|out [send|recv]]");
    addCmd("resume", "",
	ZvCmdFn::Member<&App::resume>::fn(this),
	"resume I/O",
	"resume SRCPORT|#TAG|all [in|out [send|recv]]");
    addCmd("trace", "",
	ZvCmdFn::Member<&App::trace>::fn(this),
	"hex dump traffic (0 - off, 1 - on)",
	"trace SRCPORT|#TAG|all [0|1 [in|out]]");
    addCmd("drop", "",
	ZvCmdFn::Member<&App::drop>::fn(this),
	"drop (discard) incoming traffic (0 - off, 1 - on)",
	"drop SRCPORT|#TAG|all [0|1 [in|out]]");
    addCmd("verbose", "",
	ZvCmdFn::Member<&App::verboseCmd>::fn(this),
	"log connection setup and teardown (0 - off, 1 - on)",
	"verbose 0|1");
    addCmd("status", "",
	ZvCmdFn::Member<&App::status>::fn(this),
	"list listeners and open connections (including queue sizes)",
	"status [#TAG]");
    addCmd("quit", "",
	ZvCmdFn::Member<&App::quit>::fn(this),
	"shutdown and exit", "");
  }
  void final() {
    ZvCmdServer::final();
    m_listeners->clean();
    m_proxies->clean();
  }

  inline Mx *mx() const { return m_mx; }
  inline bool verbose() const { return m_verbose; }

  int start() {
    int r;
    if ((r = m_mx->start()) != Zi::OK) return r;
    ZvCmdServer::start();
    return Zi::OK;
  }

  void stop() {
    ZvCmdServer::stop();
    m_mx->stop(true);
  }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

  void add(Proxy *proxy) {
    m_proxies->add(proxy);
  }
  void del(Proxy *proxy) {
    delete m_proxies->del(Proxy::SrcPortAccessor::value(proxy));
  }

  void proxy(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &outMsg) {
    ZmRef<Listener> listener;
    ZiIP localIP, remoteIP, srcIP;
    ZuString tag;
    uint16_t localPort, remotePort, srcPort;
    uint32_t cxnFlags = 0;
    double cxnLatency = 0;
    uint32_t cxnFrag = 0;
    uint32_t cxnPack = 0;
    double cxnDelay = 0;
    unsigned reconnectFreq = 0;
    try {
      parseAddr(args->get("1"), localIP, localPort);
      if (!localPort) throw ZeError();
      parseAddr(args->get("2"), remoteIP, remotePort);
      if (!remotePort) throw ZeError();
      if (!remoteIP) remoteIP = "127.0.0.1";
      parseAddr(args->get("3"), srcIP, srcPort);
      tag = args->get("tag");
      if (tag) {
	if (!validateTag(tag)) throw ZeError();
      } else
	tag = "#default";
      if (args->getInt("suspend", 0, 1, false, 0))
	cxnFlags |= Connection::SuspRecv | Connection::SuspSend;
      if (args->getInt("hold", 0, 1, false, 0))
	cxnFlags |= Connection::Hold;
      if (args->getInt("trace", 0, 1, false, 0))
	cxnFlags |= Connection::Trace;
      if (args->getInt("drop", 0, 1, false, 0))
	cxnFlags |= Connection::Drop;
      cxnLatency = args->getDbl("latency", 0, 3600, false, 0);
      cxnFrag = args->getInt("frag", INT_MIN, INT_MAX, false, 0);
      cxnPack = args->getInt("pack", INT_MIN, INT_MAX, false, 0);
      cxnDelay = args->getDbl("delay", 0, 3600, false, 0);
      reconnectFreq = args->getInt("reconnect", 0, 3600, false, 0);
    } catch (...) {
      throw ZvCmdUsage();
    }
    outMsg = new ZvCmdMsg();
    auto &out = outMsg->cmd();
    if (m_listeners->findKey(localPort)) {
      outMsg->code(1);
      out << "already listening on port " << ZuBoxed(localPort) << '\n';
      return;
    }
    listener = new Listener(
	this, cxnFlags, cxnLatency, cxnFrag, cxnPack, cxnDelay,
	localIP, localPort, remoteIP, remotePort, srcIP, srcPort, tag,
	reconnectFreq);
    if (listener->start() == Zi::OK)
      m_listeners->add(listener);
    else
      outMsg->code(1);
    out << listener->status() << '\n';
  }

  void stopListening(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    ZmRef<Listener> listener;
    unsigned localPort;
    bool isTag = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else
        localPort = args->getInt("1", 1, 65535, true);
    } catch (...) {
      throw ZvCmdUsage();
    }
    outMsg = new ZvCmdMsg();
    auto &out = outMsg->cmd();
    if (isTag) {
      auto i = m_listeners->iterator();
      while (ZmRef<Listener> listener = i.iterateKey()) {
        if (listener->tag() != tag) continue;
        listener->stop();
        delete m_listeners->del(listener->localPort());
        out << listener->status() << '\n';
        status(cxn, args, ZuMv(inMsg), outMsg);
      }
    } else {
      ZmRef<Listener> listener = m_listeners->findKey(localPort);
      if (!listener) {
	outMsg->code(1);
	out << "no listener on port " << ZuBoxed(localPort) << '\n';
	return;
      }
      listener->stop();
      out << listener->status() << '\n';
      delete m_listeners->del(localPort);
    }
  }

  void hold(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    unsigned srcPort;
    int side;
    bool allProxies = false, isTag = false;
    ZuString tag;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      side = args->getEnum<Side::Map>("2", false, Side::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in()))
	  connection->hold();
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out()))
	  connection->hold();
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in()))
	connection->hold();
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out()))
	connection->hold();
      out << proxy->status() << '\n';
    }
  }

  void release(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    int side;
    bool isTag = false, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      side = args->getEnum<Side::Map>("2", false, Side::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in()))
	  connection->release();
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out()))
	  connection->release();
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in()))
	connection->release();
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out()))
	connection->release();
      out << proxy->status() << '\n';
    }
  }

  void disc(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    bool isTag = false, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if (connection = proxy->in()) connection->disconnect();
	if (connection = proxy->out()) connection->disconnect();
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if (connection = proxy->in()) connection->disconnect();
      if (connection = proxy->out()) connection->disconnect();
      out << proxy->status() << '\n';
    }
  }

  void suspend(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    int side, op;
    bool isTag, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      side = args->getEnum<Side::Map>("2", false, Side::Both);
      op = args->getEnum<IOOp::Map>("3", false, IOOp::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in())) {
	  if (op == IOOp::Send || op == IOOp::Both)
	    connection->suspSend();
	  if (op == IOOp::Recv || op == IOOp::Both)
	    connection->suspRecv();
	}
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out())) {
	  if (op == IOOp::Send || op == IOOp::Both)
	    connection->suspSend();
	  if (op == IOOp::Recv || op == IOOp::Both)
	    connection->suspRecv();
	}
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in())) {
	if (op == IOOp::Send || op == IOOp::Both) connection->suspSend();
	if (op == IOOp::Recv || op == IOOp::Both) connection->suspRecv();
      }
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out())) {
	if (op == IOOp::Send || op == IOOp::Both) connection->suspSend();
	if (op == IOOp::Recv || op == IOOp::Both) connection->suspRecv();
      }
      out << proxy->status() << '\n';
    }
  }

  void resume(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    int side, op;
    bool isTag = false, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      side = args->getEnum<Side::Map>("2", false, Side::Both);
      op = args->getEnum<IOOp::Map>("3", false, IOOp::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in())) {
	  if (op == IOOp::Send || op == IOOp::Both) connection->resSend();
	  if (op == IOOp::Recv || op == IOOp::Both) connection->resRecv();
	}
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out())) {
	  if (op == IOOp::Send || op == IOOp::Both) connection->resSend();
	  if (op == IOOp::Recv || op == IOOp::Both) connection->resRecv();
	}
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in())) {
	if (op == IOOp::Send || op == IOOp::Both) connection->resSend();
	if (op == IOOp::Recv || op == IOOp::Both) connection->resRecv();
      }
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out())) {
	if (op == IOOp::Send || op == IOOp::Both) connection->resSend();
	if (op == IOOp::Recv || op == IOOp::Both) connection->resRecv();
      }
      out << proxy->status() << '\n';
    }
  }

  void trace(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    int side;
    bool on;
    bool isTag = false, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      on = args->getInt("2", 0, 1, false, 1);
      side = args->getEnum<Side::Map>("3", false, Side::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in()))
	  connection->trace(on);
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out()))
	  connection->trace(on);
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in()))
	connection->trace(on);
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out()))
	connection->trace(on);
      out << proxy->status() << '\n';
    }
  }

  void drop(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    unsigned srcPort;
    int side;
    bool on;
    bool isTag = false, allProxies = false;
    try {
      tag = args->get("1");
      if (validateTag(tag))
        isTag = true;
      else if (args->get("1") == "all")
	allProxies = true;
      else
	srcPort = args->getInt("1", 1, 65535, true);
      on = args->getInt("2", 0, 1, false, 1);
      side = args->getEnum<Side::Map>("3", false, Side::Both);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (allProxies || isTag) {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	ZmRef<Connection> connection;
        if (isTag && proxy->tag() != tag)
          continue;
	if ((side == Side::In || side == Side::Both) &&
	    (connection = proxy->in()))
	  connection->drop(on);
	if ((side == Side::Out || side == Side::Both) &&
	    (connection = proxy->out()))
	  connection->drop(on);
      }
      status(cxn, args, ZuMv(inMsg), outMsg);
    } else {
      outMsg = new ZvCmdMsg();
      auto &out = outMsg->cmd();
      ZmRef<Proxy> proxy = m_proxies->findKey(srcPort);
      if (!proxy) {
	outMsg->code(1);
	out << "no proxy on source port " << ZuBoxed(srcPort) << '\n';
	return;
      }
      ZmRef<Connection> connection;
      if ((side == Side::In || side == Side::Both) &&
	  (connection = proxy->in()))
	connection->drop(on);
      if ((side == Side::Out || side == Side::Both) &&
	  (connection = proxy->out()))
	connection->drop(on);
      out << proxy->status() << '\n';
    }
  }

  void verboseCmd(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &outMsg) {
    bool on;
    try {
      on = args->getInt("1", 0, 1, false, 1);
    } catch (...) {
      throw ZvCmdUsage();
    }
    m_verbose = on;
    outMsg = new ZvCmdMsg();
    auto &out = outMsg->cmd();
    out = on ? "verbose on\n" : "verbose off\n";
  }

  void status(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &outMsg) {
    ZuString tag;
    bool isTag = false;
    try {
      tag = args->get("1");
      isTag = validateTag(tag);
    } catch (...) {
      throw ZvCmdUsage();
    }
    if (!outMsg) outMsg = new ZvCmdMsg();
    auto &out = outMsg->cmd();
    {
      auto i = m_listeners->iterator();
      while (ZmRef<Listener> listener = i.iterateKey()) {
        if (isTag && listener->tag() != tag)
          continue;
	if (out.length()) out << '\n';
	out << listener->status();
      }
    }
    {
      auto i = m_proxies->readIterator();
      while (ZmRef<Proxy> proxy = i.iterateKey()) {
	if (out.length()) out << '\n';
	out << proxy->status();
      }
    }
  }

  void quit(ZvCmdServerCxn *cxn,
      ZvCf *args, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &outMsg) {
    post();
    outMsg = new ZvCmdMsg();
    auto &out = outMsg->cmd();
    out = "shutting down\n";
  }

private:
  ZmRef<Mx>		m_mx;
  ZmSemaphore		m_done;
  ZmRef<ListenerHash>	m_listeners;
  ZmRef<ProxyHash>	m_proxies;
  bool			m_verbose;
};

void IOBuf::recv(ZiIOContext *io) {
  if (!io)
    m_connection->ZiConnection::recv(
	ZiIOFn::Member<&IOBuf::recv_>::fn(ZmMkRef(this)));
  else
    recv_(*io);
}
void IOBuf::recv_(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&IOBuf::rcvd_>::fn(ZmMkRef(this)),
      m_buf.data(), m_buf.size(), 0);
}
void IOBuf::rcvd_(ZiIOContext &io)
{
  m_buf.length(io.offset += io.length);
  m_stamp = ZmTimeNow();
  m_connection->recv_(this, io);
}

void IOBuf::send(ZiIOContext *io)
{
  if (!io)
    m_connection->ZiConnection::send(
	ZiIOFn::Member<&IOBuf::send_>::fn(ZmMkRef(this)));
  else
    send_(*io);
}
void IOBuf::send_(ZiIOContext &io)
{
  io.init(ZiIOFn::Member<&IOBuf::sent_>::fn(ZmMkRef(this)),
      m_buf.data(), m_buf.length(), 0);
}
void IOBuf::sent_(ZiIOContext &io)
{
  if ((io.offset += io.length) >= io.size)
    m_connection->send_(this, io);
}

Connection::Connection(Proxy *proxy, uint32_t flags,
    double latency, uint32_t frag, uint32_t pack, double delay,
    const ZiConnectionInfo &ci) :
  ZiConnection(proxy->mx(), ci),
  m_mx(proxy->mx()), m_proxy(proxy), m_peer(0),
  m_flags(flags), m_latency(latency),
  m_frag(frag), m_pack(pack), m_delay(delay)
{
}

void Connection::connected(ZiIOContext &io)
{
  io.complete();
  m_mx->add(ZmFn<>::Member<&Connection::connected_>::fn(this));
}

void Connection::connected_()
{
  if (ZmRef<Proxy> proxy = m_proxy) proxy->connected(this);
}

void Connection::disconnected()
{
  if (m_peer && m_peer->up() && !(m_peer->m_flags & Hold)) {
    if (ZuBoxed(m_latency).fgt(0)) {
      // double the latency to avoid overtaking pending delayed sends
      ZmTime next(ZmTime::Now, m_latency * 2.0);
      m_mx->add(ZmFn<>::Member<&ZiConnection::disconnect>::fn(m_peer), next);
    } else
      m_peer->disconnect();
  }
  if (ZmRef<Proxy> proxy = m_proxy) proxy->disconnected(this);
}

void Connection::recv() { recv(0); }

void Connection::recv(ZiIOContext *io)
{
  if (ZuUnlikely(m_flags & SuspRecv)) {
    if (io) io->complete();
    return;
  }

  if (ZuUnlikely(ZuBoxed(m_delay).fgt(0))) {
    if (io) io->complete();
    m_mx->add(
	ZmFn<>::Member<static_cast<void (Connection::*)()>(
	  &Connection::recv)>::fn(this),
	ZmTime(ZmTime::Now, m_delay));
    return;
  }

  ZmRef<IOBuf> ioBuf = new IOBuf(this);
  ioBuf->recv(io);
}

void Connection::recv_(ZmRef<IOBuf> ioBuf, ZiIOContext &io)
{
  if (m_flags & Trace) {
    ZeLOG(Info, ZtHexDump(ZtString() << *this,
	  ioBuf->data(), ioBuf->length()));
  }

  if (!(m_flags & Drop)) m_peer->send(ZuMv(ioBuf));

  recv(&io);
}

void Connection::send(ZmRef<IOBuf> ioBuf)
{
  Guard guard(m_lock);

  if (uint32_t frag = m_frag) {
    if (!(frag = ioBuf->length() / frag)) frag = 1;
    while (ioBuf->length() > frag) {
      ZmRef<IOBuf> ioBuf_ = new IOBuf(this, ioBuf->stamp());
      ioBuf->splice(ioBuf_->buf(), 0, frag);
      m_queue.push(ZuMv(ioBuf_));
    }
  }
    
  if (ioBuf->length()) m_queue.push(ZuMv(ioBuf));

  if (m_sendPending) return;

  if (ZuBoxed(m_latency).fgt(0)) {
    ZmTime now(ZmTime::Now);
    ZmTime next = m_queue.tail()->stamp() + m_latency;
    if (next > now) {
      m_sendPending = true;
      m_mx->add(ZmFn<>::Member<&Connection::delayedSend>::fn(this), next);
      return;
    }
  }

  send(guard, 0);
}

void Connection::delayedSend()
{
  Guard guard(m_lock);
  m_sendPending = false;
  send(guard, 0);
}

void Connection::send() { send(0); }

void Connection::send(ZiIOContext *io)
{
  Guard guard(m_lock);
  send(guard, io);
}

void Connection::send(Guard &guard, ZiIOContext *io)
{
  if (m_sendPending) return;

  if (m_flags & SuspSend) return;

  ZmRef<IOBuf> ioBuf = m_queue.shift();

  if (!ioBuf) return;

  if (m_pack)
    while (ioBuf->length() < m_pack)
      if (ZmRef<IOBuf> ioBuf_ = m_queue.shift()) {
	unsigned length = m_pack - ioBuf->length();
	if (length > ioBuf_->length()) length = ioBuf_->length();
	ioBuf->append(ioBuf_->data(), length);
	if (length < ioBuf_->length()) {
	  ioBuf_->splice(0, length);
	  m_queue.push(ZuMv(ioBuf_));
	  break;
	}
      } else
	break;

  m_sendPending = true;

  ioBuf->connection(this);

  ioBuf->send(io);
}

void Connection::send_(IOBuf *ioBuf, ZiIOContext &io)
{
  if (m_flags & Trace) {
    ZeLOG(Info, ZtHexDump(ZtString() << *this,
	  ioBuf->data(), ioBuf->length()));
  }

  Guard guard(m_lock);

  m_sendPending = false;

  if (m_flags & SuspSend) { io.complete(); return; }

  send(guard, &io);
}

void Connection::release()
{
  m_flags &= ~Hold;
  if (!up()) {
    if (m_peer && m_peer->up()) m_peer->disconnect();
    if (ZmRef<Proxy> proxy = m_proxy) proxy->disconnected(this);
  } else {
    if (m_peer && !m_peer->up()) disconnect();
  }
}

inline unsigned Proxy::SrcPortAccessor::value(Proxy *proxy)
{
  if (!proxy->m_out) return 0;
  return proxy->m_out->info().localPort;
}

Proxy::Proxy(Listener *listener) :
    m_mx(listener->mx()), m_app(listener->app()), m_listener(listener),
    m_tag(listener->tag())
{
}

void Proxy::connected(Connection *connection)
{
  if (connection->flags() & Connection::In) {
    m_in = connection;
    connect2();
  } else {
    m_out = connection;
    m_app->add(this);
    m_listener->del(this);
    if (m_app->verbose()) { ZeLOG(Info, status()); }
    m_in->peer(m_out);
    m_out->peer(m_in);
    m_in->recv();
    m_out->recv();
  }
}

void Proxy::connect2()
{
  if (!m_listener) return;
  if (m_app->verbose()) { ZeLOG(Info, status()); }
  m_mx->connect(
      ZiConnectFn::Member<&Proxy::connected2>::fn(this),
      ZiFailFn::Member<&Proxy::failed2>::fn(this),
      m_listener->srcIP(), m_listener->srcPort(),
      m_listener->remoteIP(), m_listener->remotePort());
}

void Proxy::failed2(bool transient)
{
  if (transient) {
    m_mx->add(
	ZmFn<>::Member<&Proxy::connect2>::fn(this),
	ZmTimeNow((int)m_listener->reconnectFreq()));
  } else {
    if (m_app->verbose()) { ZeLOG(Info, this->status()); }
    m_in->proxy(0);
    m_in->disconnect();
    m_listener->del(this);
    m_listener = 0;
  }
}

ZiConnection *Proxy::connected2(const ZiConnectionInfo &ci)
{
  return new Connection(this, m_listener->cxnFlags(),
      m_listener->cxnLatency(),
      m_listener->cxnFrag(), m_listener->cxnPack(),
      m_listener->cxnDelay(), ci);
}

void Proxy::disconnected(Connection *connection)
{
  if ((!m_in || !m_in->up()) && (!m_out || !m_out->up())) {
    if (m_in) m_in->proxy(0);
    if (m_out) m_out->proxy(0);
    m_app->del(this);
    if (m_listener) {
      m_listener->del(this);
      m_listener = 0;
    }
  }
}

void Proxy::status_(ZmStream &s) const
{
  if (ZmRef<Connection> cxn = m_in) {
    s << *cxn;
    if (ZuBoxed(cxn->latency()).fgt(0))
      s << " (latency=" << ZuBoxed(cxn->latency()) << ')';
    if (uint32_t frag = cxn->frag())
      s << " (frag=" << ZuBoxed(frag) << ')';
    if (uint32_t pack = cxn->pack())
      s << " (pack=" << ZuBoxed(pack) << ')';
    if (ZuBoxed(cxn->delay()).fgt(0))
      s << " (delay=" << ZuBoxed(cxn->delay()) << ')';
    if (uint32_t queueSize = cxn->queueSize())
      s << " (" << ZuBoxed(queueSize) << " queued)";
  } else {
    if (m_listener)
      s << m_listener->printIn();
    else
      s << "NC:NC !> NC:NC (" << m_tag << ')';
  }
  s << " =\n\t";
  if (ZmRef<Connection> cxn = m_out) {
    s << *cxn;
    if (uint32_t queueSize = cxn->queueSize())
      s << " (" << ZuBoxed(queueSize) << " queued)";
  } else {
    if (m_listener)
      s << m_listener->printOut();
    else
      s << "NC:NC !> NC:NC (" << m_tag << ')';
  }
}

Listener::Listener(App *app, uint32_t cxnFlags,
    double cxnLatency, uint32_t cxnFrag, uint32_t cxnPack, double cxnDelay,
    ZiIP localIP, unsigned localPort, ZiIP remoteIP, unsigned remotePort,
    ZiIP srcIP, unsigned srcPort, ZuString tag, unsigned reconnectFreq) :
  m_mx(app->mx()), m_app(app),
  m_cxnFlags(cxnFlags), m_cxnLatency(cxnLatency),
  m_cxnFrag(cxnFrag), m_cxnPack(cxnPack), m_cxnDelay(cxnDelay),
  m_localIP(localIP), m_localPort(localPort),
  m_remoteIP(remoteIP), m_remotePort(remotePort),
  m_srcIP(srcIP), m_srcPort(srcPort), m_listening(false), m_tag(tag),
  m_reconnectFreq(reconnectFreq)
{
  m_proxies = new ProxyHash();
}

int Listener::start()
{
  m_mx->listen(
      ZiListenFn::Member<&Listener::ok>::fn(this),
      ZiFailFn::Member<&Listener::failed>::fn(this),
      ZiConnectFn::Member<&Listener::accepted>::fn(this),
      m_localIP, m_localPort, 8);
  m_started.wait();
  return m_listening ? Zi::OK : Zi::IOError;
}

void Listener::ok(const ZiListenInfo &)
{
  m_listening = true;
  m_started.post();
}

void Listener::failed(bool transient)
{
  m_listening = false;
  m_started.post();
}

void Listener::stop()
{
  m_mx->stopListening(m_localIP, m_localPort);
  m_listening = false;
}

ZiConnection *Listener::accepted(const ZiCxnInfo &ci)
{
  ZmRef<Proxy> proxy = new Proxy(this);
  if (m_app->verbose()) { ZeLOG(Info, this->status()); }
  add(proxy);
  return new Connection(proxy, Connection::In | m_cxnFlags,
      m_cxnLatency, m_cxnFrag, m_cxnPack, m_cxnDelay, ci);
}

void Listener::status_(ZmStream &s) const
{
  s << *this;
  auto i = m_proxies->readIterator();
  while (ZmRef<Proxy> proxy = i.iterateKey())
    s << "\n" << proxy->status();
}

int main(int argc, char **argv)
{
  static ZvOpt opts[] = {
    { "verbose", "v", ZvOptFlag },
    { "cmd:localIP", "i", ZvOptScalar, },
    { "cmd:localPort", "p", ZvOptScalar },
    { "cmd:nAccepts", "n", ZvOptScalar },
    { "mx:nThreads", "t", ZvOptScalar },
    { 0 }
  };

  static const char *usage =
    "usage: zproxy [OPTIONS]\n"
    "\n"
    "Options:\n"
    "  -v --verbose\t- log connection setup and teardown events\n"
    "  -i --cmd:localIP=IP\t- set local IP for zcmd\n"
    "  -p --cmd:localPort=PORT\t- set local port for zcmd\n"
    "  -n --cmd:nAccepts=N\t- set number of simultaneous accepts for zcmd\n"
    "  -t --mx:nThreads=N\t- set number of threads\n";

  ZeLog::init("zproxy");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  ZmRef<App> app = new App();
  ZmRef<ZvCf> args = new ZvCf();

  try {
    if (args->fromArgs(opts, argc, argv) != 1) {
      std::cerr << usage << std::flush;
      return 1;
    }
    app->init(args);
  } catch (...) {
    std::cerr << usage << std::flush;
    return 1;
  }

  ZmTrap::sigintFn(ZmFn<>::Member<&App::post>::fn(app));
  ZmTrap::trap();

  app->start();

  app->wait();

  app->stop();

  ZeLog::stop();

  app->final();

  return 0;
}
