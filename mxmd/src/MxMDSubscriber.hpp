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

// MxMD subscriber

#ifndef MxMDSubscriber_HPP
#define MxMDSubscriber_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZiMultiplex.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMDTypes.hpp>
#include <MxMDCSV.hpp>
#include <MxMDPartition.hpp>

class MxMDSubLink;

class MxMDAPI MxMDSubscriber : public MxEngineApp, public MxEngine {
public:
  inline MxMDSubscriber() { }
  ~MxMDSubscriber() { }

  inline MxMDCore *core() const { return static_cast<MxMDCore *>(mgr()); }

  void init(MxMultiplex *mx, ZvCf *cf);
  void final();

  void up();
  void down();

  // Rx (called from engine's rx thread)
  MxEngineApp::ProcessFn processFn() {
    return static_cast<MxEngineApp::ProcessFn>(
	[](MxEngineApp *app, MxAnyLink *link, MxQMsg *msg) {
	    static_cast<Subscriber *>(app)->process_(link, msg);
    });
  }

  // Tx (called from engine's tx thread)
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *) { }
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

private:
  inline const ZiIP &interface() const { return m_interface; }
  inline unsigned maxQueueSize() const { return m_maxQueueSize; }
  ZmTime loginTimeout() const { return ZmTime(m_loginTimeout); }
  ZmTime reconnectInterval() const { return ZmTime(m_reconnectInterval); }
  ZmTime reRequestInterval() const { return ZmTime(m_reRequestInterval); }

  void recv(MxMDSubLink *link);
  void process_(MxQMsg *qmsg);

  void tcpConnected(TCP *);

  void loginReq(MxMDStream::Login *login);

  void udpConnect();
  ZiConnection *udpConnected(const ZiCxnInfo &ci);
  void udpConnectFailed(bool transient);
  void udpConnected2(UDP *);
  void udpReceived(MxQMsg *msg);
  void udpReceived_(MxQMsg *msg);

  template <typename L>
  ZuInline void partition(MxID id, L &&l) const {
    if (auto node = m_partitions.find(id))
      l(&(node->key()));
    else
      l(nullptr);
  }

private:
  MxMDCore		*m_core;

  ZiIP			m_interface;
  unsigned		m_maxQueueSize = 0;
  double		m_loginTimeout = 0.0;
  double		m_reconnectInterval = 0.0;
  double		m_reRequestInterval = 0.0;

  struct PartitionIDAccessor : public ZuAccessor<MxMDPartition, MxID> {
    inline static MxID value(const MxMDPartition &partition) {
      return partition.id;
    }
  };
  typedef ZmRBTree<MxMDPartition,
	    ZmRBTreeIndex<PartitionIDAccessor,
	      ZmRBTreeLock<ZmRWLock> > > Partitions;
  typedef Partitions::Node Partition;

  Partitions		m_partitions;
};

class MxMDAPI MxMDSubLink : public MxLink<MxMDSubLink> {
  class TCP;
friend class TCP;
  class TCP : public ZiConnection {
  public:
    struct State {
      enum _ {
	Login = 0,
	Load,
	Disconnect
      };
    };

    TCP(MxMDSubLink *link, const ZiCxnInfo &ci);

    inline unsigned state() const {
      ZmReadGuard<ZmLock> guard(m_stateLock);
      return m_state;
    }

    ZuInline MxMDSubLink *link() const { return m_link; }

    void connected(ZiIOContext &);
    void disconnected();

    void sendLogin();
    void recvLoginAck(MxITCH::TCP::OutMsg *, ZiIOContext &);
    void loginTimeout();
    void processLoginAck(MxITCH::TCP::InMsg *, ZiIOContext &);
    void loginAckd(ZiIOContext &, bool load);

    void process(MxMDStream::TCP::InMsg *, ZiIOContext &);

    void disconnect();

  private:
    MxMDSubLink		*m_link;

    ZmScheduler::Timer	m_loginTimer;

    ZmLock		m_stateLock;
      unsigned		  m_state;

    ZmRef<MxQMsg>	m_in;
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

    inline unsigned state() const {
      ZmReadGuard<ZmLock> guard(m_stateLock);
      return m_state;
    }

    void connected(ZiIOContext &io);
    void disconnected();

    void recv(ZiIOContext &);
    void process(MxMDStream::UDP::InMsg *, ZiIOContext &);

    void disconnect();

  private:
    MxMDSubLink		*m_link;

    ZmLock		m_stateLock;
      unsigned		  m_state;

    ZmRef<MxQMsg>	m_in;
  };

  MxMDSubLink(MxID id) : MxLink<MxMDSubLink>(id) { }

  typedef MxMDSubscriber Engine;

  ZuInline Engine *engine() {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }

  ZmTime loginTimeout() { return engine()->loginTimeout(); }
  ZmTime reconnectInterval(unsigned) { return engine()->reconnectInterval(); }
  ZmTime reRequestInterval() { return engine()->reRequestInterval(); }

  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);

  void connect();
  void disconnect();

  // Rx
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now);
  void reRequest(const MxQueue::Gap &now);

  // Tx
  bool send_(MxQMsg *msg, bool more);
  bool resend_(MxQMsg *msg, bool more);

  bool sendGap_(const MxQueue::Gap &gap, bool more);
  bool resendGap_(const MxQueue::Gap &gap, bool more);

private:
  bool tcpError(TCP *tcp, ZiIOContext *io);
  bool udpError(UDP *udp, ZiIOContext *io);

private:
  const MxMDPartition	*m_partition = 0;

  ZiSockAddr		m_udpResendAddr;

  ZmLock		m_connLock;
    unsigned		  m_reconnects = 0;
    ZmRef<TCP>		  m_tcp;
    ZmRef<UDP>		  m_udp;

#if 0
  ZmSemaphore			m_resendSem;	// test resend semaphore
  ZmLock			m_resendLock;
    MxQueue::Gap		  m_resendGap;
    ZmRef<MxQMsg>		  m_resendMsg;
#endif
};

#endif /* MxMDSubscriber_HPP */
