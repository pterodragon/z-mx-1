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

// MxMD TCP/UDP publisher

#ifndef MxMDPublisher_HPP
#define MxMDPublisher_HPP

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

class MxMDCore;
class MxMDPubLink;

class MxMDAPI MxMDPublisher :
  public ZuObject, public MxEngineApp, public MxEngine {
public:
  inline MxMDPublisher() { }
  ~MxMDPublisher() { }

  ZuInline MxMDCore *core() const { return static_cast<MxMDCore *>(mgr()); }

  void init(MxMDCore *core);
  void final();

  void updateLinks(ZuString partitions); // update from CSV

  // Rx
  MxEngineApp::ProcessFn processFn();

  // Tx (called from engine's tx thread)
  void sent(MxAnyLink *, MxQMsg *);
  void aborted(MxAnyLink *, MxQMsg *);
  void archive(MxAnyLink *, MxQMsg *);
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo);

  // commands
  void statusCmd(const MxMDCmd::CmdArgs &args, ZtArray<char> &out);

private:
  ZuInline const ZiIP &interface() const { return m_interface; }
  ZuInline unsigned maxQueueSize() const { return m_maxQueueSize; }
  ZuInline ZmTime loginTimeout() const { return ZmTime(m_loginTimeout); }
  ZuInline unsigned reReqMaxGap() const { return m_reReqMaxGap; }
  ZuInline unsigned nAccepts() const { return m_nAccepts; }
  ZuInline unsigned ttl() const { return m_ttl; }
  ZuInline bool loopBack() const { return m_loopBack; }

  template <typename L>
  ZuInline void partition(MxID id, L &&l) const {
    if (auto node = m_partitions.find(id))
      l(&(node->key()));
    else
      l(nullptr);
  }

  bool failover() const { return m_failover; }
  void failover(bool v) { m_failover = v; }

private:
  ZiIP			m_interface;
  unsigned		m_maxQueueSize = 0;
  double		m_loginTimeout = 0.0;
  double		m_reconnInterval = 0.0;
  double		m_reReqInterval = 0.0;
  unsigned		m_reReqMaxGap = 10;
  unsigned		m_nAccepts = 8;
  unsigned		m_ttl = 1;
  bool			m_loopBack = false;

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

  bool			m_failover = false;
};

class MxMDAPI MxMDPubLink : public MxLink<MxMDPubLink> {
  class TCP;
friend class TCP;
  class TCP : public ZiConnection {
  public:
    struct SocketAccessor : public ZuAccessor<TCP *, ZiPlatform::Socket> {
      inline static ZiPlatform::Socket value(const TCP *tcp) {
	return tcp->info().socket;
      }
    };

    struct State {
      enum _ {
	Login = 0,
	Sending,
	Disconnect
      };
    };

    TCP(MxMDPubLink *link, const ZiCxnInfo &ci);

    inline unsigned state() const {
      ZmReadGuard<ZmLock> guard(m_stateLock);
      return m_state;
    }

    ZuInline MxMDPubLink *link() const { return m_link; }

    void connected(ZiIOContext &);
    void disconnect();
    void disconnected();

    void processLogin(ZiIOContext &);
    void process(ZiIOContext &);

    void snap();
    Frame *push(unsigned size);
    void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
    void push2();

  private:
    typedef MxMDStream::Msg Msg;

    MxMDPubLink		*m_link;

    ZmScheduler::Timer	m_loginTimer;

    ZmLock		m_stateLock;
      unsigned		  m_state;

    ZuRef<Msg>		m_snapMsg;
  };

  struct TCP_HeapID {
    ZuInline static const char *id() { return "MxMDPublisher.TCP"; }
  };
  typedef ZmHash<TCP *,
	    ZmHashIndex<TCP::SocketAccessor,
	      ZmHashLock<ZmNoLock,
		ZmHashObject<ZuObject,
		  ZmHashHeapID<TCP_HeapID,
		    ZmHashBase<ZmObject> > > > > > TCPTbl;

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

    UDP(MxMDPubLink *link, const ZiCxnInfo &ci);

    ZuInline MxMDPubLink *link() const { return m_link; }

    inline unsigned state() const {
      ZmReadGuard<ZmLock> guard(m_stateLock);
      return m_state;
    }

    void connected(ZiIOContext &io);
    void disconnect();
    void disconnected();

    void recv(ZiIOContext &);
    static void process(MxQMsg *, ZiIOContext &);

  private:
    MxMDPubLink		*m_link;

    ZmLock		m_stateLock;
      unsigned		  m_state;

    ZmRef<MxQMsg>	m_in;
  };

public:
  MxMDPubLink(MxID id) : MxLink<MxMDPubLink>(id) { }

  typedef MxMDPublisher Engine;

  ZuInline Engine *engine() const {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }
  ZuInline MxMDCore *core() const {
    return static_cast<MxMDCore *>(engine()->mgr());
  }

  bool publish();
  void stopPublishing();

  ZmTime loginTimeout() { return engine()->loginTimeout(); }

  // MxAnyLink virtual
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);

  void connect();
  void disconnect();

  // MxLink CRTP (unused)
  ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

  // MxLink Rx CRTP (unused)
  ZmTime reReqInterval() { return ZmTime{1}; }
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
  void reRequest(const MxQueue::Gap &now) { }

  // MxLink Tx CRTP
  bool send_(MxQMsg *msg, bool more);
  bool resend_(MxQMsg *msg, bool more);

  bool sendGap_(const MxQueue::Gap &gap, bool more);
  bool resendGap_(const MxQueue::Gap &gap, bool more);

  // connection management
  void tcpListen();
  void tcpListening(const ZiListenInfo &);
  void tcpConnected(TCP *tcp);
  void tcpDisconnected(TCP *tcp);
  bool tcpLogin(MxMDStream::Login &);
  void udpConnect();
  void udpConnected(UDP *udp, ZiIOContext &io)
  void udpReceived(MxQMsg *);

  void tcpError(TCP *tcp, ZiIOContext *io);
  void udpError(UDP *udp, ZiIOContext *io);

  // command support
  void status(ZtArray<char> &out);

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  typedef MxQueueRx<MxMDRecLink> Rx;
  typedef MxMDStream::MsgData MsgData;

  typedef MxMDBroadcast::Ring Ring;

  int write_(const Frame *frame, ZeError *e);

  // Rx thread
  void wake();
  void recv(Rx *rx);

  // snap thread
  void snap();
  Frame *push(unsigned size);
  void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
  void push2();

private:
  const MxMDPartition	*m_partition = 0;

  ZiSockAddr		m_udpAddr;

  ZmAtomic<int>		m_ringID = -1;

  Lock			m_connLock;
    ZmRef<Ring>		  m_ring;
    ZiListenInfo	  m_listenInfo;
    ZmRef<TCPTbl>	  m_tcpTbl;
    ZmRef<UDP>		  m_udp;
};

#endif /* MxMDPublisher_HPP */
