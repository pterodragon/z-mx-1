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
#include <zlib/zcmd_fbs.h>
#include <zlib/zcmdreq_fbs.h>
#include <zlib/zcmdack_fbs.h>

#include <zlib/ZvCmdHost.hpp>
#include <zlib/ZvCmdNet.hpp>

template <typename App_>
class ZvCmdSrvLink :
    public Ztls::SrvLink<App_, ZvCmdSrvLink<App_> >,
    public ZiIORx<Ztls::IOBuf> {
public:
  using App = App_;
  using Base = Ztls::SrvLink<App, ZvCmdSrvLink<App_> >;
friend Base;

private:
  using IORx = ZiIORx<Ztls::IOBuf>;
  using User = ZvUserDB::User;
  using Bitmap = ZvUserDB::Bitmap;

public:
  struct State {
    enum {
      Down = 0,
      Login,
      LoginFailed,
      Up
    };
  };

  ZvCmdSrvLink(App *app) : Base(app) { }

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

    this->app()->disconnected(this);
  }

  // send app message
  void sendApp(Zfb::IOBuilder &fbb) {
    using namespace ZvCmd;
    fbb.PushElement(ZvCmd_mkHdr(fbb.GetSize(), fbs::MsgType_App));
    this->send(fbb.buf());
  }

private:
  int processLogin(const uint8_t *data, unsigned len) {
    using namespace ZvUserDB;
    using namespace Zfb;
    {
      using namespace Load;
      {
	Verifier verifier(data, len);
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
    m_fbb.PushElement(ZvCmd_mkHdr(m_fbb.GetSize(), ZvCmd::fbs::MsgType_Login));
    this->send_(m_fbb.buf());
    m_state = State::Up;
    return len;
  }

  // userDB, zcmd and telemetry requests are synchronously
  // responded within the TLS thread
  int processMsg(int type, const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    switch (type) {
      default:
	return -1;
      case ZvCmd::fbs::MsgType_UserDB: {
	using namespace ZvUserDB;
	{
	  Verifier verifier(data, len);
	  if (!fbs::VerifyRequestBuffer(verifier)) return -1;
	}
	m_fbb.Clear();
	int i = this->app()->processUserDB(
	    m_user, m_interactive, fbs::GetRequest(data), m_fbb);
	if (ZuUnlikely(i < 0)) return -1;
	if (!i) return len;
	m_fbb.PushElement(ZvCmd_mkHdr(i, ZvCmd::fbs::MsgType_UserDB));
	this->send_(m_fbb.buf());
      } break;
      case ZvCmd::fbs::MsgType_Cmd: {
	using namespace ZvCmd;
	{
	  Verifier verifier(data, len);
	  if (!fbs::VerifyRequestBuffer(verifier)) return -1;
	}
	m_fbb.Clear();
	int i = this->app()->processCmd(this, m_user, m_interactive,
	    fbs::GetRequest(data), m_fbb);
	if (ZuUnlikely(i < 0)) return -1;
	if (!i) return len;
	m_fbb.PushElement(ZvCmd_mkHdr(i, ZvCmd::fbs::MsgType_Cmd));
	this->send_(m_fbb.buf());
      } break;
      case ZvCmd::fbs::MsgType_Telemetry: {
	using namespace ZvTelemetry;
	{
	  Verifier verifier(data, len);
	  if (!fbs::VerifyRequestBuffer(verifier)) return -1;
	}
	m_fbb.Clear();
	int i = this->app()->processTel(
	    this, m_user, m_interactive, fbs::GetRequest(data), m_fbb);
	if (ZuUnlikely(i < 0)) return -1;
	if (!i) return len;
	m_fbb.PushElement(ZvCmd_mkHdr(i, ZvCmd::fbs::MsgType_Telemetry));
	this->send_(m_fbb.buf());
      } break;
      case ZvCmd::fbs::MsgType_App: {
	int i = this->app()->processApp(this, m_user, m_interactive,
	    ZuArray<const uint8_t>(data, len));
	if (ZuUnlikely(i < 0)) return -1;
      } break;
    }
    return len;
  }

public:
  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    if (ZuUnlikely(m_state == State::LoginFailed))
      return len; // timeout then disc.

    scheduleTimeout();

    int type = -1;
    int i = IORx::process(data, len,
	[&type](const uint8_t *data, unsigned len) -> int {
	  if (ZuUnlikely(len < ZvCmd_hdrLen())) return INT_MAX;
	  uint32_t hdr_ = ZvCmd_getHdr(data);
	  type = ZvCmd_bodyType(hdr_);
	  return ZvCmd_hdrLen() + ZvCmd_bodyLen(hdr_);
	},
	[this, &type](const uint8_t *data, unsigned len) -> int {
	  data += ZvCmd_hdrLen(), len -= ZvCmd_hdrLen();
	  int i;
	  if (ZuUnlikely(m_state == State::Login)) {
	    if (type != ZvCmd::fbs::MsgType_Login) return -1;
	    i = processLogin(data, len);
	  } else if (ZuUnlikely(type < 0))
	    return -1;
	  else
	    i = processMsg(type, data, len);
	  if (ZuUnlikely(i < 0)) return i;
	  if (ZuUnlikely(!i)) return i;
	  return ZvCmd_hdrLen() + i;
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

template <typename App_>
class ZvCmdServer :
    public ZvCmdHost,
    public Ztls::Server<App_>,
    public ZvTelemetry::Server<App_, ZvCmdSrvLink<App_> > {
public:
  using App = App_;
  using TLS = Ztls::Server<App>;
friend TLS;
  using Link = ZvCmdSrvLink<App>;
  using User = ZvUserDB::User;
  using TelServer = ZvTelemetry::Server<App, Link>;

  enum { OutBufSize = 8000 }; // initial TLS buffer size

  struct UserDB : public ZuObject, public ZvUserDB::Mgr {
    template <typename ...Args>
    ZuInline UserDB(Args &&... args) :
	ZvUserDB::Mgr(ZuFwd<Args>(args)...) { }
  };

  struct Context {
    App		*app;
    Link	*link;
    User	*user;
    bool	interactive;
  };

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  void init(ZiMultiplex *mx, ZvCf *cf) {
    static const char *alpn[] = { "zcmd", 0 };
    ZvCmdHost::init();
    TLS::init(mx,
	cf->get("thread", true), cf->get("caPath", true), alpn,
	cf->get("certPath", true), cf->get("keyPath", true));
    m_ip = cf->get("localIP", false, "127.0.0.1");
    m_port = cf->getInt("localPort", 1, (1<<16) - 1, false, 19400);
    m_nAccepts = cf->getInt("nAccepts", 1, 1024, false, 8);
    m_rebindFreq = cf->getInt("rebindFreq", 0, 3600, false, 0);
    m_timeout = cf->getInt("timeout", 0, 3600, false, 0);
    unsigned passLen = 12, totpRange = 6;
    if (ZmRef<ZvCf> mgrCf = cf->subset("userDB", false, true)) {
      passLen = mgrCf->getInt("passLen", 6, 60, false, 12);
      totpRange = mgrCf->getInt("totpRange", 0, 100, false, 6);
      m_userDBPath = mgrCf->get("path", true);
      m_userDBMaxAge = mgrCf->getInt("maxAge", 0, INT_MAX, false, 8);
    }
    m_userDB = new UserDB(this, passLen, totpRange);
    if (!loadUserDB())
      throw ZtString() << "failed to load \"" << m_userDBPath << '"';
    TelServer::init(mx, cf->subset("telemetry", false));
  }

  void final() {
    TelServer::final();
    m_userDB = nullptr;
    TLS::final();
    ZvCmdHost::final();
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
  inline TCP *accepted(const ZiCxnInfo &ci) {
    return new TCP(new Link(app()), ci);
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

  int processUserDB(User *user, bool interactive,
      const ZvUserDB::fbs::Request *in, Zfb::Builder &fbb) {
    fbb.Finish(m_userDB->request(user, interactive, in, fbb));
    if (m_userDB->modified())
      this->run(
	  ZmFn<>{this, [](ZvCmdServer *server) { server->saveUserDB(); }},
	  ZmTimeNow(m_userDBFreq), ZmScheduler::Advance, &m_userDBTimer);
    return fbb.GetSize();
  }

  int processCmd(Link *link, User *user, bool interactive,
      const ZvCmd::fbs::Request *in, Zfb::Builder &fbb) {
    if (m_cmdPerm < 0 || !m_userDB->ok(user, interactive, m_cmdPerm)) {
      fbb.Finish(ZvCmd::fbs::CreateReqAck(fbb,
	    in->seqNo(), __LINE__,
	    Zfb::Save::str(fbb, "permission denied\n")));
      return fbb.GetSize();
    }
    thread_local ZtString out(OutBufSize);
    out.length(0);
    Context context{app(), link, user, interactive};
    auto cmd_ = in->cmd();
    ZtArray<ZtString> args;
    args.length(cmd_->size());
    Zfb::Load::all(cmd_,
	[&args](unsigned i, auto arg_) { args[i] = Zfb::Load::str(arg_); });
    int code = ZvCmdHost::processCmd(&context, args, out);
    fbb.Finish(ZvCmd::fbs::CreateReqAck(
	  fbb, in->seqNo(), code, Zfb::Save::str(fbb, out)));
    return fbb.GetSize();
  }

  int processTel(Link *link, User *user, bool interactive,
      const ZvTelemetry::fbs::Request *in, Zfb::Builder &fbb) {
    using namespace ZvTelemetry;
    if (m_telPerm < 0 || !m_userDB->ok(user, interactive, m_telPerm)) {
      using namespace Zfb::Save;
      fbb.Finish(fbs::CreateAck(fbb, false));
      return fbb.GetSize();
    }
    TelServer::process(link, in);
    using namespace Zfb::Save;
    fbb.Finish(fbs::CreateAck(fbb, true));
    return fbb.GetSize();
  }

  // default app msg handler simply disconnects
  // >=0 - message processed
  //  <0 - disconnect
  int processApp(Link *, User *, bool interactive,
      ZuArray<const uint8_t> data) { return -1; }

  void disconnected(Link *link) {
    TelServer::disconnected(link);
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
