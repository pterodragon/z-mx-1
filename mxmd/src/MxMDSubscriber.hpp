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
#include <MxMDPartition.hpp>

class MxMDCore;

class MxMDSubLink;

class MxMDAPI MxMDSubscriber : public MxEngine, public MxEngineApp {
public:
  MxMDSubscriber() { }
  ~MxMDSubscriber() { }

  MxMDCore *core() const;

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  ZuInline const ZiIP &interface() const { return m_interface; }
  ZuInline bool filter() const { return m_filter; }
  ZuInline unsigned maxQueueSize() const { return m_maxQueueSize; }
  ZuInline ZmTime loginTimeout() const { return ZmTime(m_loginTimeout); }
  ZuInline ZmTime reconnInterval() const { return ZmTime(m_reconnInterval); }
  ZuInline ZmTime reReqInterval() const { return ZmTime(m_reReqInterval); }
  ZuInline unsigned reReqMaxGap() const { return m_reReqMaxGap; }

  bool failover() const { return m_failover; }
  void failover(bool v) { m_failover = v; }

  void updateLinks(ZuString partitions); // update from CSV

private:
  struct PartitionIDAccessor : public ZuAccessor<MxMDPartition, MxID> {
    inline static MxID value(const MxMDPartition &partition) {
      return partition.id;
    }
  };
  typedef ZmRBTree<MxMDPartition,
	    ZmRBTreeIndex<PartitionIDAccessor,
	      ZmRBTreeLock<ZmRWLock> > > Partitions;
  typedef Partitions::Node Partition;

public:
  template <typename L>
  ZuInline void partition(MxID id, L &&l) const {
    if (auto node = m_partitions.find(id))
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
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

  // commands
  void statusCmd(const MxMDCmd::Args &args, ZtArray<char> &out);
  void resendCmd(const MxMDCmd::Args &args, ZtArray<char> &out);

private:
  void process_(MxMDSubLink *link, MxQMsg *qmsg);

private:
  ZiIP			m_interface;
  bool			m_filter = false;
  unsigned		m_maxQueueSize = 0;
  double		m_loginTimeout = 0.0;
  double		m_reconnInterval = 0.0;
  double		m_reReqInterval = 0.0;
  unsigned		m_reReqMaxGap = 10;

  Partitions		m_partitions;

  bool			m_failover = false;
};

class MxMDAPI MxMDSubLink : public MxLink<MxMDSubLink> {
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

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

    inline unsigned state() const {
      ReadGuard guard(m_stateLock);
      return m_state;
    }

    ZuInline MxMDSubLink *link() const { return m_link; }

    void connected(ZiIOContext &);
    void disconnect();
    void disconnected();

    void sendLogin();
    void processLoginAck(ZiIOContext &);
    void process(ZiIOContext &);

  private:
    MxMDSubLink		*m_link;

    ZmScheduler::Timer	m_loginTimer;

    Lock		m_stateLock;
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
      ReadGuard guard(m_stateLock);
      return m_state;
    }

    void connected(ZiIOContext &io);
    void disconnect();
    void disconnected();

    void recv(ZiIOContext &);
    static void process(MxQMsg *, ZiIOContext &);

  private:
    MxMDSubLink		*m_link;

    Lock		m_stateLock;
      unsigned		  m_state;

    ZmRef<MxQMsg>	m_in;
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

  ZmTime loginTimeout() { return engine()->loginTimeout(); }

  // MxAnyLink virtual
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);

  void connect();
  void disconnect();

  // MxLink CTRP
  ZmTime reconnInterval(unsigned) { return engine()->reconnInterval(); }

  // MxLink Rx CRTP
  ZmTime reReqInterval() { return engine()->reReqInterval(); }
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now);
  void reRequest(const MxQueue::Gap &now);

  // MxLink Tx CRTP (unused - TCP login bypasses Tx queue)
  bool send_(MxQMsg *msg, bool more) { return true; }
  bool resend_(MxQMsg *msg, bool more) { return true; }

  bool sendGap_(const MxQueue::Gap &gap, bool more) { return true; }
  bool resendGap_(const MxQueue::Gap &gap, bool more) { return true; }

  // command support
  void status(ZtArray<char> &out);
  ZmRef<MxQMsg> resend(MxSeqNo seqNo, unsigned count);

  // connection management
  void tcpConnect();
  void tcpConnected(TCP *tcp);
  void tcpLoginAck();
  void udpConnect();
  void udpConnected(UDP *udp, ZiIOContext &io);
  void udpReceived(MxQMsg *);

  void tcpError(TCP *tcp, ZiIOContext *io);
  void udpError(UDP *udp, ZiIOContext *io);

  inline void snapshotSeqNo(MxSeqNo seqNo) {
    Guard connGuard(m_connLock);
    m_snapshotSeqNo = seqNo;
  }
  inline MxSeqNo snapshotSeqNo() {
    Guard connGuard(m_connLock);
    return m_snapshotSeqNo;
  }

private:
  const MxMDPartition	*m_partition = 0;

  ZiSockAddr		m_udpResendAddr;

  Lock			m_connLock;
    MxSeqNo		  m_snapshotSeqNo;
    ZmRef<TCP>		  m_tcp;
    ZmRef<UDP>		  m_udp;

  ZmSemaphore		m_resendSem;	// test resend semaphore
  Lock			m_resendLock;
    MxQueue::Gap	  m_resendGap;
    ZmRef<MxQMsg>	  m_resendMsg;
};

#endif /* MxMDSubscriber_HPP */
