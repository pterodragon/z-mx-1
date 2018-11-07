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

#include <ZiMultiplex.hpp>
#include <ZiFile.hpp>

#include <ZeLog.hpp>

#include <ZvCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

struct ZvCmd_Hdr {
  typedef typename ZuBigEndian<int32_t>::T Int32N;
  typedef typename ZuBigEndian<uint32_t>::T UInt32N;

  UInt32N	seqNo = 0;
  Int32N	code = 0;	// result code (0 = OK)
  UInt32N	cmdLen = 0;	// length of command/stderr portion
  UInt32N	dataLen = 0;	// length of stdin/stdout portion
};

class ZvAPI ZvCmdMsg : public ZmPolymorph {
public:
  typedef ZvCmd_Hdr Hdr;

  inline ZvCmdMsg() { }
  inline ZvCmdMsg(uint32_t seqNo) : m_hdr{seqNo} { }
  inline ZvCmdMsg(uint32_t seqNo, int32_t code) : m_hdr{seqNo, code} { }
  inline ZvCmdMsg(uint32_t seqNo, int32_t code, ZtString cmd) :
    m_hdr{seqNo, code}, m_cmd(ZuMv(cmd)) { }

  inline uint32_t seqNo() const { return m_hdr.seqNo; }
  inline int32_t code() const { return m_hdr.code; }
  inline const ZtString &cmd() const { return m_cmd; }
  inline const ZtArray<char> &data() const { return m_data; }

  inline void seqNo(uint32_t i) { m_hdr.seqNo = i; }
  inline void code(int32_t i) { m_hdr.code = i; }
  inline ZtString &cmd() { return m_cmd; }
  inline ZtArray<char> &data() { return m_data; }

  int redirect(ZmRef<ZeEvent> *e = 0);

  template <typename Cxn> void recv(ZiIOContext &io) {
    io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
	if ((io.offset += io.length) < sizeof(Hdr)) return;
	auto &hdr = msg->m_hdr;
	msg->m_cmd.length(hdr.cmdLen);
	msg->m_data.length(hdr.dataLen);
	io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
	  if ((io.offset += io.length) < msg->m_cmd.length()) return;
	  if (!msg->m_data) {
	    static_cast<Cxn *>(io.cxn)->rcvd(io.fn.mvObject<ZvCmdMsg>(), io);
	    return;
	  }
	  io.init(ZiIOFn{[](ZvCmdMsg *msg, ZiIOContext &io) {
	    if ((io.offset += io.length) < msg->m_data.length()) return;
	    static_cast<Cxn *>(io.cxn)->rcvd(io.fn.mvObject<ZvCmdMsg>(), io);
	  }, io.fn.mvObject<ZvCmdMsg>()},
	  msg->m_data.data(), msg->m_data.length(), 0);
	}, io.fn.mvObject<ZvCmdMsg>()},
	msg->m_cmd.data(), msg->m_cmd.length(), 0);
      }, ZmMkRef(this)}, &m_hdr, sizeof(Hdr), 0);
  }
  void send(ZiConnection *);

private:
  ZvCmd_Hdr		m_hdr;
  ZtString		m_cmd;
  ZtArray<char>		m_data;
};

// client

template <typename Impl, typename Mgr>
class ZvCmdCxn : public ZiConnection {
public:
  ZvCmdCxn(Mgr *mgr, const ZiConnectionInfo &info) :
    ZiConnection(mgr->mx(), info), m_mgr(mgr) { }

  ZuInline Mgr *mgr() const { return m_mgr; }

  void send(ZmRef<ZvCmdMsg> msg) {
    msg->send(this);
    static_cast<Impl *>(this)->sent();
  }

private:
  void connected(ZiIOContext &io) {
    (new ZvCmdMsg())->recv<ZvCmdCxn>(io);
    static_cast<Impl *>(this)->connected();
  }
  void disconnected() {
    static_cast<Impl *>(this)->disconnected();
  }

public:
  void rcvd(ZmRef<ZvCmdMsg> msg, ZiIOContext &io) {
    mx()->add([cxn = ZmMkRef(this), msg = ZuMv(msg)]() mutable {
      static_cast<Impl *>(cxn.ptr())->rcvd(ZuMv(msg));
    });
    (new ZvCmdMsg())->recv<ZvCmdCxn>(io);
  }

protected:
  void scheduleTimeout() {
    if (m_mgr->timeout())
      mx()->add(ZmFn<>{[](ZvCmdCxn *cxn) { cxn->disconnect(); }, ZmMkRef(this)},
	  ZmTimeNow(m_mgr->timeout()), &m_timer);
  }
  void cancelTimeout() { mx()->del(&m_timer); }

private:
  Mgr			*m_mgr = 0;
  ZmScheduler::Timer	m_timer;
};

class ZvCmdClient;

class ZvAPI ZvCmdClientCxn : public ZvCmdCxn<ZvCmdClientCxn, ZvCmdClient> {
  typedef ZvCmdCxn<ZvCmdClientCxn, ZvCmdClient> Base;

public:
  typedef ZvCmdClient Mgr;

  ZvCmdClientCxn(Mgr *mgr, const ZiConnectionInfo &info) : Base(mgr, info) { }

  void connected();
  void disconnected();
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

  unsigned		m_reconnectFreq = 0;
  ZiIP			m_localIP;
  uint16_t		m_localPort = 0;
  ZiIP			m_remoteIP;
  uint16_t		m_remotePort = 0;
  unsigned		m_timeout = 0;

  ZmScheduler::Timer	m_reconnectTimer;

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

  void connected();
  void disconnected();
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
