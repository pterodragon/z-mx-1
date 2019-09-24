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

#ifndef ZvCmdClient_HPP
#define ZvCmdClient_HPP

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

#include <Zfb.hpp>
#include <Ztls.hpp>

#include <ZvCf.hpp>
#include <ZvUserDB.hpp>

#include <loginreq_generated.h>
#include <loginack_generated.h>
#include <userdbreq_generated.h>
#include <userdback_generated.h>
#include <zcmd_generated.h>
#include <zcmdreq_generated.h>
#include <zcmdack_generated.h>

#include <ZvCmdNet.hpp>

// userDB response
using ZvCmdUserDBAckFn = ZmFn<const ZvUserDB::fbs::ReqAck *>;
// command response
using ZvCmdAckFn = ZmFn<const ZvCmd::fbs::ReqAck *>;

struct ZvCmd_Login {
  ZtString		user;
  ZtString		passwd;
  unsigned		totp = 0;
};
struct ZvCmd_Access {
  ZtString		keyID;
  ZvUserDB::KeyData	token;
  ZvUserDB::KeyData	hmac;
};
using ZvCmd_Credentials = ZuUnion<ZvCmd_Login, ZvCmd_Access>;

template <typename App_, typename Impl_>
class ZvCmdCliLink :
    public Ztls::CliLink<App_, Impl_>,
    public ZiIORx {
public:
  using App = App_;
  using Impl = Impl_;
  using Base = Ztls::CliLink<App, Impl>;
friend Base;

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

private:
  using Credentials = ZvCmd_Credentials;
public:
  using SeqNo = ZvCmdSeqNo;
  using KeyData = ZvUserDB::KeyData;
  using Bitmap = ZvUserDB::Bitmap;

private:
  // containers of pending requests
  using UserDBReqs =
    ZmRBTree<SeqNo, ZmRBTreeVal<ZvCmdUserDBAckFn, ZmRBTreeLock<ZmPLock> > >;
  using CmdReqs =
    ZmRBTree<SeqNo, ZmRBTreeVal<ZvCmdAckFn, ZmRBTreeLock<ZmPLock> > >;

public:
  struct State {
    enum {
      Down = 0,
      Login,
      Up
    };
  };

  ZvCmdCliLink(App *app) : Base(app) { }

  void login(
      ZtString server, unsigned port,
      ZtString user, ZtString passwd, unsigned totp) {
    new (m_credentials.new1()) ZvCmd_Login{ZuMv(user), ZuMv(passwd), totp};
    this->connect(ZuMv(server), port);
  }
  void access(
      ZtString server, unsigned port,
      ZtString keyID, ZtString secret_) {
    ZtArray<uint8_t> secret;
    secret.length(Ztls::Base64::declen(secret_.length()));
    Ztls::Base64::decode(
	secret.data(), secret.length(), secret_.data(), secret_.length());
    ZvUserDB::KeyData token, hmac;
    token.length(token.size());
    hmac.length(hmac.size());
    this->app()->random(token.data(), token.length());
    {
      Ztls::HMAC hmac_(ZvUserDB::Key::keyType());
      hmac_.start(secret.data(), secret.length());
      hmac_.update(token.data(), token.length());
      hmac_.finish(hmac.data());
    }
    new (m_credentials.new2()) ZvCmd_Access{ZuMv(keyID), token, hmac};
    this->connect(ZuMv(server), port);
  }

public:
  // send userDB request
  void sendUserDB(Zfb::IOBuilder &fbb, SeqNo seqNo, ZvCmdUserDBAckFn fn) {
    using namespace ZvCmd;
    fbb.PushElement(ZvCmd_mkHdr(fbb.GetSize(), fbs::MsgType_UserDB));
    m_userDBReqs.add(seqNo, ZuMv(fn));
    this->send(fbb.buf());
  }
  // send command
  void sendCmd(Zfb::IOBuilder &fbb, SeqNo seqNo, ZvCmdAckFn fn) {
    using namespace ZvCmd;
    fbb.PushElement(ZvCmd_mkHdr(fbb.GetSize(), fbs::MsgType_Cmd));
    m_cmdReqs.add(seqNo, ZuMv(fn));
    this->send(fbb.buf());
  }
  // send app message
  void sendApp(Zfb::IOBuilder &fbb) {
    using namespace ZvCmd;
    fbb.PushElement(ZvCmd_mkHdr(fbb.GetSize(), fbs::MsgType_App));
    this->send(fbb.buf());
  }

  void loggedIn(const Bitmap &perms) { } // default

  // default app handler simply disconnects
  // >=0 - messaged consumed, continue
  //  <0 - disconnect
  int processApp(ZuArray<const uint8_t>) { return -1; } // default

  void connected(const char *, const char *alpn) {
    if (!alpn || strcmp(alpn, "zcmd")) {
      this->disconnect();
      return;
    }

    scheduleTimeout();
    m_state = State::Login;

    // initialize Rx processing
    ZiIORx::connected();

    // send login
    Zfb::IOBuilder fbb;
    if (m_credentials.type() == 1) {
      using namespace ZvUserDB;
      using namespace Zfb::Save;
      const auto &data = m_credentials.p1();
      fbb.Finish(fbs::CreateLoginReq(fbb,
	    fbs::LoginReqData_Login,
	    fbs::CreateLogin(fbb,
	      str(fbb, data.user),
	      str(fbb, data.passwd),
	      data.totp).Union()));
    } else {
      using namespace ZvUserDB;
      using namespace Zfb::Save;
      const auto &data = m_credentials.p2();
      fbb.Finish(fbs::CreateLoginReq(fbb,
	    fbs::LoginReqData_Access,
	    fbs::CreateAccess(fbb,
	      str(fbb, data.keyID),
	      bytes(fbb, data.token),
	      bytes(fbb, data.hmac)).Union()));
    }
    fbb.PushElement(ZvCmd_mkHdr(fbb.GetSize(), ZvCmd::fbs::MsgType_Login));
    this->send_(fbb.buf());
  }

  void disconnected() {
    m_userDBReqs.clean();
    m_cmdReqs.clean();

    m_state = State::Down;

    cancelTimeout();

    ZiIORx::disconnected();
  }

private:
  int processLoginAck(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier(data, len);
      if (!fbs::VerifyLoginAckBuffer(verifier)) return -1;
    }
    auto loginAck = fbs::GetLoginAck(data);
    if (!loginAck->ok()) return -1;
    m_state = State::Up;
    {
      Bitmap perms;
      all(loginAck->perms(), [&perms](unsigned i, uint64_t v) {
	if (i < Bitmap::Words) perms.data[i] = v;
      });
      impl()->loggedIn(perms);
    }
    return len;
  }

  int processMsg(int type, const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    switch (type) {
      default:
	return -1;
      case ZvCmd::fbs::MsgType_UserDB: {
	{
	  Verifier verifier(data, len);
	  if (!ZvUserDB::fbs::VerifyReqAckBuffer(verifier)) return -1;
	}
	auto reqAck = ZvUserDB::fbs::GetReqAck(data);
	if (ZvCmdUserDBAckFn fn = m_userDBReqs.delVal(reqAck->seqNo()))
	  fn(reqAck);
      } break;
      case ZvCmd::fbs::MsgType_Cmd: {
	{
	  Verifier verifier(data, len);
	  if (!ZvCmd::fbs::VerifyReqAckBuffer(verifier)) return -1;
	}
	auto cmdAck = ZvCmd::fbs::GetReqAck(data);
	if (ZvCmdAckFn fn = m_cmdReqs.delVal(cmdAck->seqNo()))
	  fn(cmdAck);
      } break;
      case ZvCmd::fbs::MsgType_App: {
	int i = impl()->processApp(ZuArray<const uint8_t>(data, len));
	if (ZuUnlikely(i < 0)) return -1;
      } break;
    }
    return len;
  }

public:
  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state == State::Down))
      return -1; // disconnect

    scheduleTimeout();

    int type = -1;
    int i = ZiIORx::process(data, len,
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
	    i = processLoginAck(data, len);
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
      this->app()->mx()->add(
	  ZmFn<>{ZmMkRef(impl()), [](Impl *link) {
	    link->disconnect();
	  }}, ZmTimeNow(this->app()->timeout()), &m_timer);
  }
  void cancelTimeout() {
    this->app()->mx()->del(&m_timer);
  }

private:
  ZmScheduler::Timer	m_timer;
  int			m_state = State::Down;
  ZvCmd_Credentials	m_credentials;
  UserDBReqs		m_userDBReqs;
  CmdReqs		m_cmdReqs;
};

template <typename App_>
class ZvCmdClient : public Ztls::Client<App_> {
public:
  using App = App_;
  using Base = Ztls::Client<App>;
friend Base;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  void init(ZiMultiplex *mx, ZvCf *cf) {
    static const char *alpn[] = { "zcmd", 0 };
    Base::init(mx, cf->get("thread", true), cf->get("caPath", true), alpn);

    m_reconnFreq = cf->getInt("reconnFreq", 0, 3600, false, 0);
    m_timeout = cf->getInt("timeout", 0, 3600, false, 0);
  }
  void final() {
    Base::final();
  }

  unsigned reconnFreq() const { return m_reconnFreq; }
  unsigned timeout() const { return m_timeout; }

private:
  unsigned		m_reconnFreq = 0;
  unsigned		m_timeout = 0;
};

#endif /* ZvCmdClient_HPP */
