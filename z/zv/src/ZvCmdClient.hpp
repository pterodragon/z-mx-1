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

#ifndef ZvCmdClient_HPP
#define ZvCmdClient_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuString.hpp>
#include <zlib/ZuByteSwap.hpp>
#include <zlib/ZuUnion.hpp>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmPLock.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtString.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiMultiplex.hpp>
#include <zlib/ZiFile.hpp>

#include <zlib/Zfb.hpp>
#include <zlib/Ztls.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvUserDB.hpp>
#include <zlib/ZvSeqNo.hpp>
#include <zlib/ZvTelemetry.hpp>
#include <zlib/ZvCmdMsgFn.hpp>

#include <zlib/loginreq_fbs.h>
#include <zlib/loginack_fbs.h>
#include <zlib/userdbreq_fbs.h>
#include <zlib/userdback_fbs.h>
#include <zlib/zcmdreq_fbs.h>
#include <zlib/zcmdack_fbs.h>
#include <zlib/telemetry_fbs.h>
#include <zlib/telreq_fbs.h>
#include <zlib/telack_fbs.h>

#include <zlib/ZvCmdNet.hpp>

// userDB response
using ZvCmdUserDBAckFn = ZmFn<const ZvUserDB::fbs::ReqAck *>;
// command response
using ZvCmdAckFn = ZmFn<const ZvCmd::fbs::ReqAck *>;
// telemetry response
using ZvCmdTelAckFn = ZmFn<const ZvTelemetry::fbs::ReqAck *>;

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

template <typename App, typename Link> class ZvCmdClient;

template <typename App_, typename Impl_>
class ZvCmdCliLink :
    public Ztls::CliLink<App_, Impl_>,
    public ZiIORx<Ztls::IOBuf> {
public:
  using App = App_;
  using Impl = Impl_;
  using Base = Ztls::CliLink<App_, Impl_>;
friend Base;
template <typename, typename> friend class ZvCmdClient;

private:
  using IORx = ZiIORx<Ztls::IOBuf>;

public:
  const Impl *impl() const { return static_cast<const Impl *>(this); }
  Impl *impl() { return static_cast<Impl *>(this); }

private:
  using Credentials = ZvCmd_Credentials;
  using KeyData = ZvUserDB::KeyData;
  using Bitmap = ZvUserDB::Bitmap;

private:
  // containers of pending requests
  using UserDBReqs =
    ZmRBTree<ZvSeqNo,
      ZmRBTreeVal<ZvCmdUserDBAckFn,
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmPLock> > > >;
  using CmdReqs =
    ZmRBTree<ZvSeqNo,
      ZmRBTreeVal<ZvCmdAckFn,
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmPLock> > > >;
  using TelReqs =
    ZmRBTree<ZvSeqNo,
      ZmRBTreeVal<ZvCmdTelAckFn,
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmPLock> > > >;

public:
  struct State {
    enum {
      Down = 0,
      Login,
      Up
    };
  };

  ZvCmdCliLink(App *app) : Base{app} { }

  // Note: the caller must ensure that calls to login()/access()
  // are not overlapped - until loggedIn()/connectFailed()/disconnected()
  // no further calls must be made
  void login(
      ZtString server, unsigned port,
      ZtString user, ZtString passwd, unsigned totp) {
    new (m_credentials.init<ZvCmd_Login>())
      ZvCmd_Login{ZuMv(user), ZuMv(passwd), totp};
    this->connect(ZuMv(server), port);
  }
  void access(
      ZtString server, unsigned port,
      ZtString keyID, ZtString secret_) {
    ZtArray<uint8_t> secret;
    secret.length(Ztls::Base64::declen(secret_.length()));
    Ztls::Base64::decode(
	secret.data(), secret.length(), secret_.data(), secret_.length());
    secret.length(32);
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
    new (m_credentials.init<ZvCmd_Access>())
      ZvCmd_Access{ZuMv(keyID), token, hmac};
    this->connect(ZuMv(server), port);
  }

  int state() const { return m_state; }

  // available once logged in
  uint64_t userID() const { return m_userID; }
  const ZtString &userName() const { return m_userName; }
  const ZtArray<ZtString> &roles() const { return m_roles; }
  const Bitmap &perms() const { return m_perms; }
  uint8_t flags() const { return m_userFlags; }

public:
  // send userDB request
  void sendUserDB(Zfb::IOBuilder &fbb, ZvSeqNo seqNo, ZvCmdUserDBAckFn fn) {
    using namespace ZvCmd;
    ZvCmdHdr{fbb, ID::userDB()};
    m_userDBReqs.add(seqNo, ZuMv(fn));
    this->send(fbb.buf());
  }
  // send command
  void sendCmd(Zfb::IOBuilder &fbb, ZvSeqNo seqNo, ZvCmdAckFn fn) {
    using namespace ZvCmd;
    ZvCmdHdr{fbb, ID::cmd()};
    m_cmdReqs.add(seqNo, ZuMv(fn));
    this->send(fbb.buf());
  }
  // send telemetry request
  void sendTelReq(Zfb::IOBuilder &fbb, ZvSeqNo seqNo, ZvCmdTelAckFn fn) {
    using namespace ZvCmd;
    ZvCmdHdr{fbb, ID::telReq()};
    m_telReqs.add(seqNo, ZuMv(fn));
    this->send(fbb.buf());
  }

  void loggedIn() { } // default

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
    if (m_credentials.type() == Credentials::Index<ZvCmd_Login>::I) {
      using namespace ZvUserDB;
      using namespace Zfb::Save;
      const auto &data = m_credentials.v<ZvCmd_Login>();
      fbb.Finish(fbs::CreateLoginReq(fbb,
	    fbs::LoginReqData_Login,
	    fbs::CreateLogin(fbb,
	      str(fbb, data.user),
	      str(fbb, data.passwd),
	      data.totp).Union()));
    } else {
      using namespace ZvUserDB;
      using namespace Zfb::Save;
      const auto &data = m_credentials.v<ZvCmd_Access>();
      fbb.Finish(fbs::CreateLoginReq(fbb,
	    fbs::LoginReqData_Access,
	    fbs::CreateAccess(fbb,
	      str(fbb, data.keyID),
	      bytes(fbb, data.token),
	      bytes(fbb, data.hmac)).Union()));
    }
    ZvCmdHdr{fbb, ZvCmd::ID::login()};
    this->send_(fbb.buf());
  }

  void disconnected() {
    m_userDBReqs.clean();
    m_cmdReqs.clean();
    m_telReqs.clean();

    m_state = State::Down;

    cancelTimeout();

    ZiIORx::disconnected();
  }

public:
  int process(const uint8_t *data, unsigned len) {
    if (ZuUnlikely(m_state.load_() == State::Down))
      return -1; // disconnect

    ZuID id;
    int i = ZiIORx::process(data, len,
	[&id](const uint8_t *data, unsigned len) -> int {
	  if (ZuUnlikely(len < sizeof(ZvCmdHdr))) return INT_MAX;
	  auto hdr = reinterpret_cast<const ZvCmdHdr *>(data);
	  id = hdr->id;
	  return sizeof(ZvCmdHdr) + hdr->len;
	},
	[this, &id](const uint8_t *data, unsigned len) -> int {
	  data += sizeof(ZvCmdHdr), len -= sizeof(ZvCmdHdr);
	  int i;
	  if (ZuUnlikely(m_state.load_() == State::Login)) {
	    cancelTimeout();
	    if (id != ZvCmd::ID::login()) return -1;
	    i = processLoginAck(data, len);
	  } else
	    i = this->app()->dispatch(id, this, data, len);
	  if (ZuUnlikely(i <= 0)) return i;
	  return sizeof(ZvCmdHdr) + i;
	});
    if (ZuUnlikely(i < 0)) m_state = State::Down;
    return i;
  }

private:
  int processLoginAck(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyLoginAckBuffer(verifier)) return -1;
    }
    auto loginAck = fbs::GetLoginAck(data);
    if (!loginAck->ok()) return false;
    m_userID = loginAck->id();
    m_userName = str(loginAck->name());
    all(loginAck->roles(), [this](unsigned i, auto role_) {
      m_roles.push(str(role_));
    });
    all(loginAck->perms(), [this](unsigned i, uint64_t v) {
      if (i < Bitmap::Words) m_perms.data[i] = v;
    });
    m_userFlags = loginAck->flags();
    m_state = State::Up;
    impl()->loggedIn();
    return len;
  }
  int processUserDB(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvUserDB;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyReqAckBuffer(verifier)) return -1;
    }
    auto reqAck = fbs::GetReqAck(data);
    if (ZvCmdUserDBAckFn fn = m_userDBReqs.delVal(reqAck->seqNo()))
      fn(reqAck);
    return len;
  }
  int processCmd(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvCmd;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyReqAckBuffer(verifier)) return -1;
    }
    auto reqAck = fbs::GetReqAck(data);
    if (ZvCmdAckFn fn = m_cmdReqs.delVal(reqAck->seqNo()))
      fn(reqAck);
    return len;
  }
  int processTelReq(const uint8_t *data, unsigned len) {
    using namespace Zfb;
    using namespace Load;
    using namespace ZvTelemetry;
    {
      Verifier verifier{data, len};
      if (!fbs::VerifyReqAckBuffer(verifier)) return -1;
    }
    auto reqAck = fbs::GetReqAck(data);
    if (ZvCmdTelAckFn fn = m_telReqs.delVal(reqAck->seqNo()))
      fn(reqAck);
    return len;
  }
  // default telemetry handler does nothing
  int processTelemetry(const uint8_t *data, unsigned len) { return len; }

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
  ZmAtomic<int>		m_state = State::Down;
  Credentials		m_credentials;
  UserDBReqs		m_userDBReqs;
  CmdReqs		m_cmdReqs;
  TelReqs		m_telReqs;
  uint64_t		m_userID = 0;
  ZtString		m_userName;
  ZtArray<ZtString>	m_roles;
  Bitmap		m_perms;
  uint8_t		m_userFlags = 0;
};

template <typename App_, typename Link_>
class ZvCmdClient :
    public ZvCmdMsgFn,
    public Ztls::Client<App_> {
public:
  using App = App_;
  using Link = Link_;
  using MsgFn = ZvCmdMsgFn;
  using TLS = Ztls::Client<App>;
friend TLS;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  void init(ZiMultiplex *mx, const ZvCf *cf) {
    static const char *alpn[] = { "zcmd", 0 };

    MsgFn::init(); // FIXME - dispatch hash table size

    MsgFn::map(ZvCmd::ID::userDB(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processUserDB(data, len);
	});
    MsgFn::map(ZvCmd::ID::cmd(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processCmd(data, len);
	});
    MsgFn::map(ZvCmd::ID::telReq(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processTelReq(data, len);
	});
    MsgFn::map(ZvCmd::ID::telemetry(),
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<Link *>(link)->processTelemetry(data, len);
	});

    TLS::init(mx, cf->get("thread", true), cf->get("caPath", true), alpn);

    m_reconnFreq = cf->getInt("reconnFreq", 0, 3600, false, 0);
    m_timeout = cf->getInt("timeout", 0, 3600, false, 0);
  }
  void final() {
    TLS::final();

    MsgFn::final();
  }

  unsigned reconnFreq() const { return m_reconnFreq; }
  unsigned timeout() const { return m_timeout; }

private:
  unsigned		m_reconnFreq = 0;
  unsigned		m_timeout = 0;
};

#endif /* ZvCmdClient_HPP */
