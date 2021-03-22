//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

#ifndef ZvCmdServer_HPP
#define ZvCmdServer_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuString.hpp>
#include <zlib/ZuByteSwap.hpp>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmPLock.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtString.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiMultiplex.hpp>
#include <zlib/ZiFile.hpp>
#include <zlib/ZiIOBuf.hpp>

#include <zlib/Zfb.hpp>
#include <zlib/Ztls.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvUserDB.hpp>
#include <zlib/ZvTelemetry.hpp>
#include <zlib/ZvTelServer.hpp>

#include <zlib/loginreq_fbs.h>
#include <zlib/loginack_fbs.h>
#include <zlib/userdbreq_fbs.h>
#include <zlib/userdback_fbs.h>
#include <zlib/zcmdreq_fbs.h>
#include <zlib/zcmdack_fbs.h>

#include <zlib/ZvCmdHost.hpp>
#include <zlib/ZvCmdDispatcher.hpp>
#include <zlib/ZvCmdNet.hpp>

template <typename App, typename Link> class ZvCmdServer;

template <typename App_, typename Impl_>
class ZvCmdSrvLink :
    public Ztls::SrvLink<App_, Impl_>,
    public ZiIORx<Ztls::IOBuf> {
public:
  using App = App_;
  using Impl = Impl_;
  using Base = Ztls::SrvLink<App, Impl>;
friend Base;
template <typename, typename> friend class ZvCmdServer;

private:
  using IORx = ZiIORx<Ztls::IOBuf>;
  using User = ZvUserDB::User;
  using Bitmap = ZvUserDB::Bitmap;

public:
  const Impl *impl() const { return static_cast<const Impl *>(this); }
  Impl *impl() { return static_cast<Impl *>(this); }

  struct State {
    enum {
      Down = 0,
      Login,
      LoginFailed,
      Up
    };
  };

  ZvCmdSrvLink(App *app) : Base{app} { }

  void connected(const char *, const char *alpn) {
    scheduleTimeout();

    if (!alpn || strcmp(alpn, "zcmd")) {
      m_state = State::LoginFailed;
      return;
    }

    m_state = State::Login;

    IORx::connected();
  }

  void disconnected() {
    m_state = State::Down;

    cancelTimeout();

    IORx::disconnected();

    this->app()->disconnected(impl());
  }

private:
  int processLogin(const uint8_t *data, unsigned len) {
    using namespace ZvUserDB;
    using namespace Zfb;
    {
      using namespace Load;
      {
	Verifier verifier{data, len};
	if (!fbs::VerifyLoginReqBuffer(verifier)) {
	  m_state = State::LoginFailed;
	  return 0;
	}
      }
      if (int failures = this->app()->processLogin(
	    fbs::GetLoginReq(data), m_user, m_interactive) ||
	  ZuUnlikely(!m_user)) {
	if (failures > 0 && failures <= 3) return -1;
	m_state = State::LoginFailed;
	return 0;
      }
    }
    {
      using namespace Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateLoginAck(m_fbb,
	    m_user->id, str(m_fbb, m_user->name),
	    strVecIter(m_fbb, m_user->roles.length(),
	      [&roles = m_user->roles](unsigned k) {
		return roles[k]->name;
	      }),
	    m_fbb.CreateVector(m_user->perms.data, Bitmap::Words),
	    m_user->flags, 1));
    }
    ZvCmdHdr{m_fbb, ZvCmd::ID::login()};
    this->send_(m_fbb.buf());
    m_state = State::Up;
    return len;
  }
  int processUserDB(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyRequestBuffer(verifier)) return -1;
    }
    m_fbb.Clear();
    this->app()->processUserDB(
	m_user, m_interactive, fbs::GetRequest(data), m_fbb);
    ZvCmdHdr{m_fbb, ZvCmd::ID::userDB()};
    this->send_(m_fbb.buf());
    return len;
  }
  int processCmd(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvCmd;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyRequestBuffer(verifier)) return -1;
    }
    m_fbb.Clear();
    this->app()->processCmd(impl(), m_user, m_interactive,
	fbs::GetRequest(data), m_fbb);
    ZvCmdHdr{m_fbb, ZvCmd::ID::cmd()};
    this->send_(m_fbb.buf());
    return len;
  }
  int processTelReq(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvTelemetry;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyRequestBuffer(verifier)) return -1;
    }
    m_fbb.Clear();
    this->app()->processTelReq(
	impl(), m_user, m_interactive, fbs::GetRequest(data), m_fbb);
    ZvCmdHdr{m_fbb, ZvCmd::ID::telReq()};
    this->send_(m_fbb.buf());
    return len;
  }

public:
  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    if (ZuUnlikely(m_state == State::LoginFailed))
      return len; // timeout then disc.

    scheduleTimeout();

    ZuID id;
    int i = IORx::process(data, len,
	[&id](const uint8_t *data, unsigned len) -> int {
	  if (ZuUnlikely(len < sizeof(ZvCmdHdr))) return INT_MAX;
	  auto hdr = reinterpret_cast<const ZvCmdHdr *>(data);
	  id = hdr->id;
	  return sizeof(ZvCmdHdr) + hdr->len;
	},
	[this, &id](const uint8_t *data, unsigned len) -> int {
	  data += sizeof(ZvCmdHdr), len -= sizeof(ZvCmdHdr);
	  int i;
	  if (ZuUnlikely(m_state == State::Login)) {
	    if (id != ZvCmd::ID::login()) return -1;
	    i = processLogin(data, len);
	  } else
	    i = this->app()->dispatch(id, impl(), data, len);
	  if (ZuUnlikely(i <= 0)) return i;
	  return sizeof(ZvCmdHdr) + i;
	});
    if (ZuUnlikely(i < 0)) m_state = State::Down;
    return i;
  }

private:
  void scheduleTimeout() {
    if (this->app()->timeout())
      this->app()->mx()->add(ZmFn<>{ZmMkRef(this), [](ZvCmdSrvLink *link) {
	link->disconnect();
      }}, ZmTimeNow(this->app()->timeout()), &m_timer);
  }
  void cancelTimeout() { this->app()->mx()->del(&m_timer); }

private:
  ZmScheduler::Timer	m_timer;
  int			m_state = State::Down;
  Zfb::IOBuilder	m_fbb;
  ZmRef<User>		m_user;
  bool			m_interactive = false;
};

template <typename App_, typename Link_>
class ZvCmdServer :
    public ZvCmdDispatcher,
    public ZvCmdHost,
    public Ztls::Server<App_>,
    public ZvTelemetry::Server<App_, Link_> {
public:
  using App = App_;
  using Link = Link_;
  using Host = ZvCmdHost;
  using Dispatcher = ZvCmdDispatcher;
  using TLS = Ztls::Server<App>;
friend TLS;
  using User = ZvUserDB::User;
  using TelServer = ZvTelemetry::Server<App, Link>;

  enum { OutBufSize = 8000 }; // initial TLS buffer size

  struct UserDB : public ZuObject, public ZvUserDB::Mgr {
    template <typename ...Args>
    ZuInline UserDB(Args &&... args) :
	ZvUserDB::Mgr(ZuFwd<Args>(args)...) { }
  };

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  void init(ZiMultiplex *mx, const ZvCf *cf) {
    static const char *alpn[] = { "zcmd", 0 };

    Host::init();
    Dispatcher::init(); // FIXME - dispatch hash table size

    map(ZvCmd::ID::userDB(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processUserDB(data, len);
	});
    map(ZvCmd::ID::cmd(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processCmd(data, len);
	});
    map(ZvCmd::ID::telReq(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processTelReq(data, len);
	});

    TLS::init(mx,
	cf->get("thread", true), cf->get("caPath", true), alpn,
	cf->get("certPath", true), cf->get("keyPath", true));

    m_ip = cf->get("localIP", false, "127.0.0.1");
    m_port = cf->getInt("localPort", 1, (1<<16) - 1, false, 19400);
    m_nAccepts = cf->getInt("nAccepts", 1, 1024, false, 8);
    m_rebindFreq = cf->getInt("rebindFreq", 0, 3600, false, 0);
    m_timeout = cf->getInt("timeout", 0, 3600, false, 0);
    unsigned passLen = 12, totpRange = 6, keyInterval = 30, maxSize = (10<<20);
    if (ZmRef<ZvCf> mgrCf = cf->subset("userDB")) {
      passLen = mgrCf->getInt("passLen", 6, 60, false, 12);
      totpRange = mgrCf->getInt("totpRange", 0, 100, false, 6);
      keyInterval = mgrCf->getInt("keyInterval", 0, 36000, false, 30);
      maxSize = mgrCf->getInt("maxSize", 0, (10<<24), false, (10<<20));
      m_userDBPath = mgrCf->get("path", true);
      m_userDBMaxAge = mgrCf->getInt("maxAge", 0, INT_MAX, false, 8);
    }
    m_userDB = new UserDB(this, passLen, totpRange, keyInterval, maxSize);

    if (!loadUserDB())
      throw ZtString() << "failed to load \"" << m_userDBPath << '"';

    TelServer::init(mx, cf->subset("telemetry"));
  }

  void final() {
    TelServer::final();

    m_userDB = nullptr;

    TLS::final();

    Dispatcher::final();
    Host::final();
  }

  bool start() {
    this->listen();
    return true;
  }
  void stop() {
    this->stopListening();
    this->run(ZmFn<>{this, [](ZvCmdServer *server) { server->stop_(); }});
  }
private:
  void stop_() {
    this->mx()->del(&m_userDBTimer);
    if (m_userDB->modified()) saveUserDB();
  }

  using TCP = typename Link::TCP;
  TCP *accepted(const ZiCxnInfo &ci) {
    return new TCP(new Link{app()}, ci);
  }
public:
  ZuInline ZiIP localIP() const { return m_ip; }
  ZuInline unsigned localPort() const { return m_port; }
  ZuInline unsigned nAccepts() const { return m_nAccepts; }
  ZuInline unsigned rebindFreq() const { return m_rebindFreq; }
  ZuInline unsigned timeout() const { return m_timeout; }

  ZuInline const ZtString &userDBPath() const { return m_userDBPath; }
  ZuInline unsigned userDBFreq() const { return m_userDBFreq; }
  ZuInline unsigned userDBMaxAge() const { return m_userDBMaxAge; }

  ZuInline int cmdPerm() const { return m_cmdPerm; } // -1 if unset

  ZuInline int findPerm(ZuString name) const {
    return m_userDB->findPerm(name);
  }
  ZuInline bool ok(User *user, bool interactive, unsigned perm) const {
    return m_userDB->ok(user, interactive, perm);
  }

private:
  bool loadUserDB() {
    ZeError e;
    if (m_userDB->load(m_userDBPath, &e) != Zi::OK) {
      this->logWarning("load(\"", m_userDBPath, "\"): ", e);
      ZtString backup{m_userDBPath.length() + 3};
      backup << m_userDBPath << ".1";
      if (m_userDB->load(backup, &e) != Zi::OK) {
	this->logError("load(\"", m_userDBPath, ".1\"): ", e);
	return false;
      }
    }
    m_cmdPerm = m_userDB->findPerm("ZCmd");
    if (m_cmdPerm < 0) {
      this->logError(m_userDBPath, ": ZCmd permission missing");
      return false;
    }
    m_telPerm = m_userDB->findPerm("ZTel"); // optional
    return true;
  }
  bool saveUserDB() {
    ZeError e;
    if (m_userDB->save(m_userDBPath, m_userDBMaxAge, &e) != Zi::OK) {
      this->logError("save(\"", m_userDBPath, "\"): ", e);
      return false;
    }
    return true;
  }

public:
  int processLogin(const ZvUserDB::fbs::LoginReq *login,
      ZmRef<User> &user, bool &interactive) {
    return m_userDB->loginReq(login, user, interactive);
  }
  void processUserDB(User *user, bool interactive,
      const ZvUserDB::fbs::Request *in, Zfb::Builder &fbb) {
    fbb.Finish(m_userDB->request(user, interactive, in, fbb));
    if (m_userDB->modified())
      this->run(
	  ZmFn<>{this, [](ZvCmdServer *server) { server->saveUserDB(); }},
	  ZmTimeNow(m_userDBFreq), ZmScheduler::Advance, &m_userDBTimer);
  }
  void processCmd(Link *link, User *user, bool interactive,
      const ZvCmd::fbs::Request *in, Zfb::Builder &fbb) {
    if (m_cmdPerm < 0 || !m_userDB->ok(user, interactive, m_cmdPerm)) {
      fbb.Finish(ZvCmd::fbs::CreateReqAck(fbb,
	    in->seqNo(), __LINE__,
	    Zfb::Save::str(fbb, "permission denied\n")));
      return;
    }
    auto cmd_ = in->cmd();
    ZtArray<ZtString> args;
    args.length(cmd_->size());
    Zfb::Load::all(cmd_,
	[&args](unsigned i, auto arg_) { args[i] = Zfb::Load::str(arg_); });
    ZvCmdContext ctx{
      .app_ = app(), .link_ = link, .user_ = user,
      .interactive = interactive
    };
    Host::processCmd(&ctx, args);
    fbb.Finish(ZvCmd::fbs::CreateReqAck(
	  fbb, in->seqNo(), ctx.code, Zfb::Save::str(fbb, ctx.out)));
  }
  void processTelReq(Link *link, User *user, bool interactive,
      const ZvTelemetry::fbs::Request *in, Zfb::Builder &fbb) {
    using namespace ZvTelemetry;
    if (m_telPerm < 0 || !m_userDB->ok(user, interactive, m_telPerm)) {
      using namespace Zfb::Save;
      fbb.Finish(fbs::CreateReqAck(fbb, in->seqNo(), false));
      return;
    }
    TelServer::process(link, in);
    using namespace Zfb::Save;
    fbb.Finish(fbs::CreateReqAck(fbb, in->seqNo(), true));
  }

  void disconnected(Link *link) {
    TelServer::disconnected(link);
  }

  // ZvCmdHost virtual functions
  Dispatcher *dispatcher() {
    return static_cast<Dispatcher *>(this);
  }
  void send(void *link, ZmRef<ZiIOBuf<>> buf) {
    return static_cast<Link *>(link)->send(ZuMv(buf));
  }

private:
  ZiIP			m_ip;
  uint16_t		m_port = 0;
  unsigned		m_nAccepts = 0;
  unsigned		m_rebindFreq = 0;
  unsigned		m_timeout = 0;

  ZtString		m_userDBPath;
  unsigned		m_userDBFreq = 60;	// checkpoint frequency
  unsigned		m_userDBMaxAge = 8;

  ZmScheduler::Timer	m_userDBTimer;

  ZuRef<UserDB>		m_userDB;
  int			m_cmdPerm = -1;		// "ZCmd"
  int			m_telPerm = -1;		// "ZTel"
};

#endif /* ZvCmdServer_HPP */
