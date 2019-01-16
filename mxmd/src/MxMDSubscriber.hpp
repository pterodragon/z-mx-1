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

// MxMD TCP/UDP subscriber

#ifndef MxMDSubscriber_HPP
#define MxMDSubscriber_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZmTime.hpp>
#include <ZmPLock.hpp>
#include <ZmGuard.hpp>
#include <ZmRef.hpp>
#include <ZmSemaphore.hpp>

#include <ZiMultiplex.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMDTypes.hpp>
#include <MxMDCSV.hpp>
#include <MxMDChannel.hpp>

class MxMDCore;

class MxMDSubLink;

class MxMDAPI MxMDSubscriber : public MxEngine, public MxEngineApp {
public:
  MxMDCore *core() const;

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  ZuInline const ZiIP &interface() const { return m_interface; }
  ZuInline bool filter() const { return m_filter; }
  ZuInline unsigned maxQueueSize() const { return m_maxQueueSize; }
  ZuInline ZmTime loginTimeout() const { return ZmTime(m_loginTimeout); }
  ZuInline ZmTime timeout() const { return ZmTime(m_timeout); }
  ZuInline ZmTime reconnInterval() const { return ZmTime(m_reconnInterval); }
  ZuInline ZmTime reReqInterval() const { return ZmTime(m_reReqInterval); }
  ZuInline unsigned reReqMaxGap() const { return m_reReqMaxGap; }

  void updateLinks(ZuString channels); // update from CSV

private:
  struct ChannelIDAccessor : public ZuAccessor<MxMDChannel, MxID> {
    inline static MxID value(const MxMDChannel &channel) {
      return channel.id;
    }
  };
  typedef ZmRBTree<MxMDChannel,
	    ZmRBTreeIndex<ChannelIDAccessor,
	      ZmRBTreeLock<ZmRWLock> > > Channels;
  typedef Channels::Node Channel;

public:
  template <typename L>
  ZuInline void channel(MxID id, L &&l) const {
    if (auto node = m_channels.find(id))
      l(&(node->key()));
    else
      l(nullptr);
  }

  ZmRef<MxAnyLink> createLink(MxID id);

  // Rx (called from engine's rx thread)
  MxEngineApp::ProcessFn processFn();

  // Tx (called from engine's tx thread) (unused)
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *) { }
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return nullptr; }

  // commands
  void statusCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg);
  void resendCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg);

private:
  void process_(MxMDSubLink *link, MxQMsg *qmsg);

private:
  ZiIP			m_interface;
  bool			m_filter = false;
  unsigned		m_maxQueueSize = 0;
  double		m_loginTimeout = 0.0;
  double		m_timeout = 0.0;
  double		m_reconnInterval = 0.0;
  double		m_reReqInterval = 0.0;
  unsigned		m_reReqMaxGap = 10;

  Channels		m_channels;
};

class MxMDAPI MxMDSubLink : public MxLink<MxMDSubLink> {
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef MxMDStream::Msg Msg;

  class TCP;
friend class TCP;
  class TCP : public ZiConnection {
  public:
    struct State {
      enum _ {
	Login = 0,
	Receiving,
	Disconnect
      };
    };

    TCP(MxMDSubLink *link, const ZiCxnInfo &ci);

    ZuInline MxMDSubLink *link() const { return m_link; }

    inline unsigned state() const { return m_state.load_(); }

    void connected(ZiIOContext &);
    void close();
    void disconnect();
    void disconnected();

    void sendLogin();
    void processLoginAck(ZmRef<MxQMsg>, ZiIOContext &);
    void process(ZmRef<MxQMsg>, ZiIOContext &);

  private:
    MxMDSubLink		*m_link;

    ZmScheduler::Timer	m_loginTimer;

    ZmAtomic<unsigned>	m_state;
  };

  class UDP;
friend class UDP;
  class UDP : public ZiConnection {
  public:
    struct State {
      enum _ {
	Receiving = 0,
	Disconnect
      };
    };

    UDP(MxMDSubLink *link, const ZiCxnInfo &ci);

    ZuInline MxMDSubLink *link() const { return m_link; }

    inline unsigned state() const { return m_state.load_(); }

    void connected(ZiIOContext &);
    void close();
    void disconnect();
    void disconnected();

    void recv(ZiIOContext &);
    void process(ZmRef<MxQMsg>, ZiIOContext &);

  private:
    MxMDSubLink		*m_link;

    ZmAtomic<unsigned>	m_state;
  };

public:
  MxMDSubLink(MxID id) : MxLink<MxMDSubLink>(id) { }

  typedef MxMDSubscriber Engine;

  ZuInline Engine *engine() const {
    return static_cast<Engine *>(MxAnyLink::engine());
  }
  ZuInline MxMDCore *core() const {
    return engine()->core();
  }

  // MxAnyLink virtual
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);
  bool failover() const { return m_failover; }
  void failover(bool v) {
    if (v == m_failover) return;
    m_failover = v;
    reconnect(true);
  }

  void connect();			// Rx
  void disconnect();			// Rx - calls disconnect_1()

  // MxLink CTRP
  ZmTime reconnInterval(unsigned) { return engine()->reconnInterval(); }

  // MxLink Rx CRTP
  ZmTime reReqInterval() { return engine()->reReqInterval(); }
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now);
  void reRequest(const MxQueue::Gap &now);

  // MxLink Tx CRTP (unused - TCP login bypasses Tx queue)
  void loaded_(MxQMsg *) { }
  void unloaded_(MxQMsg *) { }

  bool send_(MxQMsg *msg, bool more) { return true; }
  bool resend_(MxQMsg *msg, bool more) { return true; }

  bool sendGap_(const MxQueue::Gap &gap, bool more) { return true; }
  bool resendGap_(const MxQueue::Gap &gap, bool more) { return true; }

  // command support
  void status(ZtString &out);
  ZmRef<MxQMsg> resend(MxSeqNo seqNo, unsigned count);

private:
  // connection management
  void reconnect(bool immediate);	// any thread

  void reconnect_(bool immediate);	// Rx - calls disconnect_1()
  void disconnect_1();			// Rx

  void tcpConnect();
  void tcpConnected(TCP *tcp);			// Rx
  void tcpDisconnected(TCP *tcp);		// Rx
  ZmRef<MxQMsg> tcpLogin();
  void tcpLoginAck();
  void tcpProcess(MxQMsg *);
  void endOfSnapshot(MxSeqNo seqNo);
  void udpConnect();
  void udpConnected(UDP *udp);			// Rx
  void udpDisconnected(UDP *udp);		// Rx
  void udpReceived(ZmRef<MxQMsg>);

  void tcpError(TCP *tcp, ZiIOContext *io);
  void udpError(UDP *udp, ZiIOContext *io);
  void rxQueueTooBig(uint32_t count, uint32_t max);

  // failover
  ZuInline ZmTime loginTimeout() { return engine()->loginTimeout(); }
  ZuInline ZmTime timeout() { return engine()->timeout(); }

  ZuInline void active() { m_active = true; }
  void hbStart();
  void heartbeat();

public:
  ZmTime lastTime() const { return m_lastTime; }
  void lastTime(ZmTime t) { m_lastTime = t; }

private:
  const MxMDChannel	*m_channel = 0;

  bool			m_failover = false;

  // Rx
  ZmScheduler::Timer	m_timer;
  bool			m_active = false;
  unsigned		m_inactive = 0;
  ZmTime		m_lastTime;

  ZiSockAddr		m_udpResendAddr;

  ZmRef<TCP>		m_tcp;
  ZmRef<UDP>		m_udp;
  MxSeqNo		m_snapshotSeqNo;
  bool			m_reconnect = false;
  bool			m_immediate = false;	// immediate reconnect

  ZmSemaphore		m_resendSem;	// test resend semaphore
  Lock			m_resendLock;
    MxQueue::Gap	  m_resendGap;
    ZmRef<MxQMsg>	  m_resendMsg;
};

#endif /* MxMDSubscriber_HPP */
