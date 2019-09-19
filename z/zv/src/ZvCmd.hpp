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

#ifndef ZvCmd_HPP
#define ZvCmd_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuString.hpp>
#include <ZuByteSwap.hpp>

#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmPLock.hpp>

#include <ZtArray.hpp>
#include <ZtString.hpp>

#include <ZeLog.hpp>

#include <ZiMultiplex.hpp>
#include <ZiFile.hpp>

#include <Ztls.hpp>

#include <ZvCf.hpp>

#include <loginreq_generated.h>
#include <loginack_generated.h>
#include <userdbreq_generated.h>
#include <userdback_generated.h>
#include <zcmdreq_generated.h>
#include <zcmdack_generated.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

// cxn, args, in, out
typedef ZmFn<ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &>
  ZvCmdFn;

struct ZvCmdUsage { };

// client

template <typename App>
class ZvCmdCliLink :
    public ZmPolymorph,
    public Ztls::CliLink<App, ZvCmdCliLink>,
    public Zfb::Rx {
public:
  struct State {
    enum {
      Down = 0,
      Login,
      Up
    };
  };

  ZvCmdCliLink(App *app) : Ztls::CliLink<App, Link>(app) { }

private:
  void connected(const char *, const char *alpn) {
    if (strcmp(alpn, "zcmd")) {
      disconnect();
      return;
    }
    scheduleTimeout();
    m_state = State::Login;
    Zfb::Rx::connected();

    m_fbb.Clear();
    if (m_interactive)
      m_fbb.FinishSizePrefixed(fbs::CreateLoginReq(m_fbb,
	    LoginReqData_Login,
	    fbs::CreateLogin(m_fbb,
	      str(m_fbb, m_user), str(m_fbb, m_passwd), m_totp)));
    else
      m_fbb.FinishSizePrefixed(fbs::CreateLoginReq(m_fbb,
	    LoginReqData_Access,
	    fbs::CreateAccess(m_fbb, str(m_fbb, m_keyID),
	      bytes(m_fbb, m_token), bytes(m_fbb, m_hmac))));
    send_(m_fbb.buf());
  }

  void disconnected() {
    m_state = State::Down;
    cancelTimeout();
    Zfb::Rx::disconnected();
  }

private:
  int processLoginAck(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier(data, len);
      if (!fbs::VerifyLoginAckBuffer(verifier)) {
	m_state = State::Down;
	return -1;
      }
    }
    auto loginAck = fbs::GetLoginAck(data);
    if (!loginAck->ok) {
      m_state = State::Down;
      return -1;
    }
    m_state = State::Up;
    return len;
  }

  int processReqAck(const uint8_t *data, unsigned len) {
    // lookup seqNo in outstanding requests
    // - call corresponding completion with response/ack
    // - pending request is auto-removed for RPC calls,
    //   remains in place for pub/sub unless explicitly removed by app
    using namespace Zfb;
    using namespace Load;
    using namespace ZvCmd;
    {
      Verifier verifier(data, len);
      if (!fbs::VerifyReqAckBuffer(verifier)) {
	m_state = State::Down;
	return -1;
      }
    }
    auto reqAck = fbs::GetReqAck(data);
    auto buf = bytes(reqAck->data());
    switch ((int)reqAck->type()) {
      case MsgType_userdb: {
	{
	  Verifier verifier(buf.data(), buf.length());
	  if (!UserDB::fbs::VerifyReqAckBuffer(verifier)) {
	    m_state = State::Down;
	    return -1;
	  }
	}
	auto reqAck = UserDB::fbs::GetReqAck(buf.data());
	// FIXME
      } break;
      case MsgType_zcmd: {
      } break;
      case MsgType_app: {
      } break;
    }
    return len;
  }

public:
  // FIXME send request, register CB for response
  void request() {
    if (m_state != State::Up) {
      // FIXME
      return;
    }
    m_fbb.Clear();
    m_fbb.FinishSizePrefixed(...);
    send_(m_fbb.buf());
  }

  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    scheduleTimeout();

    return processRx(data, len,
	[this](const uint8_t *data, unsigned len) -> int {
	  if (ZuUnlikely(m_state == State::Login))
	    return processLoginAck(data, len);
	  else
	    return processReqAck(data, len);
	});
  }


protected:
  void scheduleTimeout() {
    if (this->app()->timeout())
      this->app()->mx()->add(ZmFn<>{ZmMkRef(this), [](ZvCmdLink *link) {
	link->disconnect();
      }}, ZmTimeNow(this->app()->timeout()), &m_timer);
  }
  void cancelTimeout() { this->app()->mx()->del(&m_timer); }

private:
  ZmScheduler::Timer		m_timer;
  int				m_state = State::Down;
  ZmRef<ZiIOBuf>		m_rxBuf;
  ZmRef<ZvUserDB::User>		m_user;
  bool				m_interactive = false;

  // FIXME - container of outstanding requests / subscriptions
  // keyed on seqNo (request ID), with associated completion functions
  // (callbacks), which are passed the response/reject in each case
};

class ZvCmdClient : public Ztls::Client<ZvCmdClient> {
  unsigned		m_reconnFreq = 0;
  ZiIP			m_localIP;
  uint16_t		m_localPort = 0;
  ZiIP			m_remoteIP;
  uint16_t		m_remotePort = 0;
  unsigned		m_timeout = 0;

  ZmScheduler::Timer	m_reconnTimer;
}

// server
template <typename App>
class ZvCmdSrvLink :
    public ZmPolymorph,
    public Ztls::SrvLink<App, ZvCmdSrvLink>,
    public Zfb::Rx {
public:
  using Hdr = ZvCmd_Hdr;

  struct State {
    enum {
      Down = 0,
      Login,
      LoginFailed,
      Up
    };
  };

  ZvCmdSrvLink(App *app) : Ztls::SrvLink<App, Link>(app) { }

private:
  void connected(const char *, const char *alpn) {
    scheduleTimeout();
    if (strcmp(alpn, "zcmd")) {
      m_state = State::LoginFailed;
      return;
    }
    m_state = State::Login;
    Zfb::Rx::connected();
  }

  void disconnected() {
    m_state = State::Down;
    cancelTimeout();
    Zfb::Rx::disconnected();
  }

private:
  int processLogin(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier(data, len);
      if (!fbs::VerifyLoginReqBuffer(verifier)) {
	m_state = State::LoginFailed;
	return 0;
      }
    }
    if (!app()->userDB()->loginReq(
	fbs::GetLoginReq(data), app()->totpRange(),
	m_user, m_interactive)) {
      m_state = State::LoginFailed;
      return 0;
    }
    m_fbb.Clear();
    m_fbb.FinishSizePrefixed(fbs::CreateLoginAck(m_fbb, 1));
    send_(m_fbb.buf());
    m_state = State::Up;
    return len;
  }

  int processReq(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvCmd;
    {
      Verifier verifier(data, len);
      if (!fbs::VerifyReqBuffer(verifier)) {
	m_state = State::Down;
	return -1;
      }
    }
    m_fbb.Clear();
    auto req = fbs::GetReq(data);
    auto buf = bytes(req->data());
    switch ((int)req->type()) {
      case MsgType_userdb: {
	{
	  Verifier verifier(buf.data(), buf.length());
	  if (!UserDB::fbs::VerifyRequestBuffer(verifier)) {
	    m_state = State::Down;
	    return -1;
	  }
	}
	m_fbb.FinishSizePrefixed(
	    userDB()->request(m_user, m_interactive,
	      m_fbb, UserDB::fbs::GetRequest(buf.data()));
      } break;
      case MsgType_zcmd: {
      } break;
      case MsgType_app: {
      } break;
    }
    send_(m_fbb.buf());
    return len;
  }

public:
  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    if (ZuUnlikely(m_state == State::LoginFailed))
      return len; // timeout then disc.

    scheduleTimeout();

    return processRx(data, len,
	[this](const uint8_t *data, unsigned len) -> int {
	  if (ZuUnlikely(m_state == State::Login))
	    return processLogin(data, len);
	  else
	    return processReq(data, len);
	});
  }

protected:
  void scheduleTimeout() {
    if (this->app()->timeout())
      this->app()->mx()->add(ZmFn<>{ZmMkRef(this), [](ZvCmdLink *link) {
	link->disconnect();
      }}, ZmTimeNow(this->app()->timeout()), &m_timer);
  }
  void cancelTimeout() { this->app()->mx()->del(&m_timer); }

private:
  ZmScheduler::Timer		m_timer;
  int				m_state = State::Down;
  Zfb::IOBuilder		m_fbb;
  ZmRef<ZvUserDB::User>		m_user;
  bool				m_interactive = false;
};

class ZvCmdServer : public Ztls::Server<ZvCmdServer> {

  using Link = ZvCmdSrvLink<ZvCmdServer>;

  inline unsigned timeout() const { return m_timeout; }

  void addCmd(ZuString name,
      ZuString syntax, ZvCmdFn fn, ZtString brief, ZtString usage);

  void start();
  void stop();

  void rcvd(ZvCmdServerCxn *cxn, ZmRef<ZvCmdMsg>);

  // FIXME - app can register handler for incoming app data

private:
  void listen();
  void listening();
  void failed(bool transient);

  void help(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &out);

  unsigned		m_rebindFreq = 0;
  ZiIP			m_ip;
  uint16_t		m_port = 0;
  unsigned		m_nAccepts = 0;
  unsigned		m_timeout = 0;

  bool			m_listening = false;

  ZmScheduler::Timer	m_rebindTimer;

  struct CmdData {
    ZvCmdFn	fn;
    ZtString	brief;
    ZtString	usage;
  };
  typedef ZmRBTree<ZtString,
	    ZmRBTreeVal<CmdData,
	      ZmRBTreeLock<ZmNoLock> > > Cmds;

  ZmRef<ZvCf>		m_syntax;
  Cmds			m_cmds;

  // FIXME - UserDB
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCmd_HPP */
