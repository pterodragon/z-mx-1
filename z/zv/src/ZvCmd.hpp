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
class ZvCmdCliLink : public ZmPolymorph, public Ztls::CliLink<App, Link> {
public:
  using Hdr = ZvCmd_Hdr;

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
    if (strcmp(alpn, "zcmd")) disconnect();
    scheduleTimeout();
    m_state = State::Login;
    m_rxBuf = new ZiIOBuf(this);
    ZmRef<IOBuf> buf;
    {
      using namespace Zfb;
      using namespace Save;
      IOBuilder b;
      b.FinishSizePrefixed(fbs::CreateLoginReq(b,
	  fbs::CreateLogin(b, str(b, m_user), str(b, m_passwd), m_totp)));
      buf = b.buf();
    }
    send_(buf);
  }
  void disconnected() {
    cancelTimeout();
  }
  int process(const char *data, unsigned rxLen) {
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
class ZvCmdSrvLink : public ZmPolymorph, public Ztls::SrvLink<App, Link> {
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
    m_rxBuf = new ZiIOBuf(this);
  }

  void disconnected() {
    cancelTimeout();
  }

  int process(const char *data, unsigned rxLen) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    if (ZuUnlikely(m_state == State::LoginFailed))
      return rxLen; // timeout then disc.

    scheduleTimeout();

    unsigned oldLen = m_rxBuf->length;
    unsigned len = oldLen + rxLen;
    auto rxData = m_rxBuf->ensure(len);
    memcpy(rxData + oldLen, data, rxLen);
    m_rxBuf->length = len;

    auto rxPtr = rxData;
    while (len >= 4) {
      auto hdr = reinterpret_cast<ZuLittleEndian<uint32_t> *>(rxPtr);
      unsigned frameLen = *hdr + 4;

      if (len < frameLen) break;

      auto msgPtr = rxPtr + 4;
      auto msgLen = frameLen - 4;

      using namespace Zfb;
      using namespace Load;

      if (ZuUnlikely(m_state == State::Login)) {
	using namespace ZvUserDB;
	{
	  Verifier verifier(msgPtr, msgLen);
	  if (!fbs::VerifyLoginReqBuffer(verifier)) {
	    m_state = State::LoginFailed;
	    return rxLen;
	  }
	}
	bool ok = app()->userDB()->loginReq(
	    fbs::GetLoginReq(msgPtr), app()->totpRange(),
	    m_user, m_interactive);
	if (!ok) {
	  m_state = State::LoginFailed;
	  return rxLen;
	}
	{
	  m_fbb.Clear();
	  m_fbb.Finish(fbs::CreateLoginAck(m_fbb, 1));
	  send_(m_fbb.buf());
	}
	m_state = State::Up;
      } else {
	using namespace ZvCmd;
	{
	  Verifier verifier(msgPtr, msgLen);
	  if (!fbs::VerifyReqBuffer(verifier)) {
	    m_state = State::Down;
	    return -1;
	  }
	}
	m_fbb.Clear();
	auto req = fbs::GetReq(msgPtr);
	switch ((int)req->type()) {
	  case MsgType_userdb: {
	    auto buf = Load::bytes(req->data());
	    {
	      Verifier verifier(buf.data(), buf.length());
	      if (!UserDB::fbs::VerifyRequestBuffer(verifier)) {
		m_state = State::Down;
		return -1;
	      }
	    }
	    m_fbb.Finish(userDB()->request(m_user, m_interactive,
		  m_fbb, UserDB::fbs::GetRequest(buf.data()));
	  } break;
	  case MsgType_zcmd:
	  case MsgType_app:
	}
	send_(m_fbb.buf());
      }

      rxPtr += frameLen;
      len -= frameLen;
    }
    if (len && (rxPtr - rxData)) {
      memmove(rxData, rxPtr - rxData, len);
      m_rxBuf->length = len;
    } else
      m_rxBuf->length = 0;
    return rxLen;
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
  Zfb::IOBuilder		m_fbb;
  ZmRef<ZvUserDB::User>		m_user;
  bool				m_interactive = false;
};

class ZvCmdServer : public Ztls::Server<ZvCmdServer> {

  // FIXME
  inline unsigned timeout() const { return m_timeout; }

  void addCmd(ZuString name,
      ZuString syntax, ZvCmdFn fn, ZtString brief, ZtString usage);

  void start();
  void stop();

  void rcvd(ZvCmdServerCxn *cxn, ZmRef<ZvCmdMsg>);

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
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCmd_HPP */
