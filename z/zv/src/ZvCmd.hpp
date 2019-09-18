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

#include <login_generated.h>
#include <loginack_generated.h>
#include <userdbreq_generated.h>
#include <userdback_generated.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

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
    if (m_state == State::Down) return -1; // disconnect

    scheduleTimeout();

    unsigned oldLen = m_rxBuf->length;
    unsigned newLen = oldLen + rxLen;
    memcpy(m_rxBuf->ensure(newLen) + oldLen, data, rxLen);
    m_rxBuf->length = newLen;

    while (newLen >= 4) {
      auto rxData = m_rxBuf->data();
      auto hdr = reinterpret_cast<ZuLittleEndian<uint32_t> *>(rxData);
      unsigned frameLen = *hdr + 4;

      if (newLen < frameLen) break;

      auto msgPtr = rxData + 4;
      auto msgLen = frameLen - 4;

      if (ZuUnlikely(m_state == State::Login)) {
	using namespace ZvUserDB;
	using namespace Zfb;
	using namespace Load;
	{
	  Verifier verifier(msgPtr, msgLen);
	  if (!fbs::VerifyLoginReqBuffer(verifier)) {
	    m_state = State::Down;
	    return -1; // disconnect
	  }
	}
	auto login = fbs::GetLoginReq(msgPtr);
	// FIXME - interpret LoginReq union
	// FIXME - accept both login and access, set interactive accordingly
	m_interactive = true;
	m_user = app()->login(
	    str(login->user()), str(login->passwd()), login->totp());
	if (!m_user) { // on failure, sleep then disconnect
	  // FIXME
	} else {
	  // FIXME
	  m_state = State::Up;
	}
      } else {
	// FIXME
	// up - app messages, including:
	// 1] ZvUserDB messages (individually permitted)
	// 2] ZvCmd commands (permitted as "zcmd.X" where X is command)
	// 3] App requests (forwarded to app as opaque buffer)
      }

      memmove(rxData, rxData + frameLen, newLen);
      m_rxBuf->length = (newLen -= frameLen);
    }
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
  ZmRef<ZvUserDB::User>		m_user;
  bool				m_interactive = false;
};

template <typename App>
class ZvCmdSrvLink : public ZmPolymorph, public Ztls::SrvLink<App, Link> {
public:
  using Hdr = ZvCmd_Hdr;

  struct State {
    enum {
      Down = 0,
      Login,
      Up
    };
  };

  ZvCmdSrvLink(App *app) : Ztls::SrvLink<App, Link>(app) { }

private:
  void connected(const char *, const char *alpn) {
    if (strcmp(alpn, "zcmd")) disconnect();
    scheduleTimeout();
    m_state = State::Login;
    m_rxBuf = new ZiIOBuf(this);
  }
  void disconnected() {
    cancelTimeout();
  }
  int process(const char *data, unsigned len) {
    if (m_state == State::Down) return -1; // disconnect

    scheduleTimeout();

    unsigned oldLen = m_rxBuf->length;
    m_rxBuf->length = oldLen + len;
    memcpy(m_rxBuf->ensure(m_rxBuf->length) + oldLen, data, len);

    while (m_rxBuf->length >= sizeof(ZvCmd_Hdr)) {
      auto hdr = reinterpret_cast<const ZvCmd_Hdr *>(m_rxBuf->data());
      unsigned frameLen = sizeof(ZvCmd_Hdr) + hdr->length;

      if (m_rxBuf->length < frameLen) break;

      auto msgPtr = m_rxBuf->data() + sizeof(ZvCmd_Hdr);
      auto msgLen = frameLen - sizeof(ZvCmd_Hdr);

      if (ZuUnlikely(m_state == State::Login)) {
	using namespace ZvUserDB;
	using namespace Zfb;
	using namespace Load;
	{
	  Verifier verifier(msgPtr, msgLen);
	  if (!fbs::VerifyLoginReqBuffer(verifier)) {
	    m_state = State::Down;
	    return -1; // disconnect
	  }
	}
	auto login = fbs::GetLoginReq(msgPtr);
	// FIXME - interpret LoginReq union
	// FIXME - accept both login and access, set interactive accordingly
	m_interactive = true;
	m_user = app()->login(
	    str(login->user()), str(login->passwd()), login->totp());
	if (!m_user) { // on failure, sleep then disconnect
	  // FIXME
	} else {
	  // FIXME
	  m_state = State::Up;
	}
      } else {
	// FIXME
	// up - app messages, including:
	// 1] ZvUserDB messages (individually permitted)
	// 2] ZvCmd commands (permitted as "zcmd.X" where X is command)
	// 3] App requests (forwarded to app as opaque buffer)
      }

      m_rxBuf->length -= frameLen;
      memmove(m_rxBuf->data(), m_rxBuf->data() + frameLen, m_rxBuf->length);
    }
    return len;
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

class ZvAPI ZvCmdClientCxn : public ZvCmdCxn<ZvCmdClientCxn, ZvCmdClient> {
  typedef ZvCmdCxn<ZvCmdClientCxn, ZvCmdClient> Base;

public:
  typedef ZvCmdClient Mgr;

  ZvCmdClientCxn(Mgr *mgr, const ZiConnectionInfo &info) : Base(mgr, info) { }

  void connected_();
  void disconnected_();
  void rcvd(ZmRef<ZvCmdMsg>);
  void sent();
};

class ZvAPI ZvCmdClient {
public:
  void init(ZiMultiplex *mx, unsigned timeout);
  void init(ZiMultiplex *mx, ZvCf *cf);
  void final();

  inline ZiMultiplex *mx() const { return m_mx; }

  inline unsigned timeout() const { return m_timeout; }

  void connect(ZiIP ip, int port);
  void connect(); // use configured endpoints
  void disconnect();

  void connected(ZvCmdClientCxn *);
  void disconnected(ZvCmdClientCxn *);

  void send(ZmRef<ZvCmdMsg>);
  void rcvd(ZmRef<ZvCmdMsg>);

  virtual void process(ZmRef<ZvCmdMsg>) = 0;

  virtual void connected() { }
  virtual void disconnected() { }

  virtual void error(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

private:
  void failed(bool transient);

  ZiMultiplex		*m_mx = 0;

  unsigned		m_reconnFreq = 0;
  ZiIP			m_localIP;
  uint16_t		m_localPort = 0;
  ZiIP			m_remoteIP;
  uint16_t		m_remotePort = 0;
  unsigned		m_timeout = 0;

  ZmScheduler::Timer	m_reconnTimer;

  typedef ZmList<ZmRef<ZvCmdMsg>, ZmListLock<ZmNoLock> > MsgList;
 
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  Lock				m_lock;
    ZmRef<ZvCmdClientCxn>	  m_cxn;
    bool			  m_busy = false;
    MsgList			  m_queue;
    uint32_t			  m_seqNo = 0;
};

// server

class ZvCmdServerCxn;

// cxn, args, in, out
typedef ZmFn<ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &>
  ZvCmdFn;

struct ZvCmdUsage { };

class ZvCmdServer;

class ZvAPI ZvCmdServerCxn : public ZvCmdCxn<ZvCmdServerCxn, ZvCmdServer> {
  typedef ZvCmdCxn<ZvCmdServerCxn, ZvCmdServer> Base;

public:
  typedef ZvCmdServer Mgr;

  ZvCmdServerCxn(Mgr *mgr, const ZiConnectionInfo &info) : Base(mgr, info) { }

  void connected_();
  void disconnected_();
  void rcvd(ZmRef<ZvCmdMsg>);
  void sent();
};

class ZvAPI ZvCmdServer {
public:
  void init(ZiMultiplex *mx, ZvCf *cf);
  void final();

  inline ZiMultiplex *mx() const { return m_mx; }

  inline unsigned timeout() const { return m_timeout; }

  void addCmd(ZuString name,
      ZuString syntax, ZvCmdFn fn, ZtString brief, ZtString usage);

  void start();
  void stop();

  void rcvd(ZvCmdServerCxn *cxn, ZmRef<ZvCmdMsg>);

  virtual void error(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

private:
  void listen();
  void listening();
  void failed(bool transient);

  void help(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &out);

  ZiMultiplex		*m_mx = 0;

  unsigned		m_rebindFreq = 0;
  ZiIP			m_ip;
  uint16_t		m_port = 0;
  unsigned		m_nAccepts = 0;
  unsigned		m_timeout = 0;

  ZmSemaphore		m_started;
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
