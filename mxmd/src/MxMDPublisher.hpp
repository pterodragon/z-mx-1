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

#include <ZmTime.hpp>
#include <ZmPLock.hpp>
#include <ZmGuard.hpp>
#include <ZmRef.hpp>

#include <ZiMultiplex.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMDTypes.hpp>
#include <MxMDCSV.hpp>
#include <MxMDChannel.hpp>
#include <MxMDBroadcast.hpp>

class MxMDCore;

class MxMDPubLink;

class MxMDAPI MxMDPublisher : public MxEngine, public MxEngineApp {
public:
  MxMDPublisher() { }
  ~MxMDPublisher() { }

  MxMDCore *core() const;

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  ZuInline const ZiIP &interface() const { return m_interface; }
  ZuInline unsigned maxQueueSize() const { return m_maxQueueSize; }
  ZuInline ZmTime loginTimeout() const { return ZmTime(m_loginTimeout); }
  ZuInline ZmTime ackInterval() const { return ZmTime(m_ackInterval); }
  ZuInline unsigned reReqMaxGap() const { return m_reReqMaxGap; }
  ZuInline unsigned nAccepts() const { return m_nAccepts; }
  ZuInline unsigned ttl() const { return m_ttl; }
  ZuInline bool loopBack() const { return m_loopBack; }

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

  // Rx (unused)
  MxEngineApp::ProcessFn processFn();

  // Tx (called from engine's tx thread)
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *);
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return nullptr; }

  // broadcast
  bool attach();
  void detach();
  void wake();
  void recv();
  void ack();

  // snapshot
  ZuInline unsigned snapThread() const { return m_snapThread; }

  // commands
  void statusCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg);

private:
  unsigned		m_snapThread = 0;
  ZiIP			m_interface;
  unsigned		m_maxQueueSize = 0;
  double		m_loginTimeout = 0.0;
  unsigned		m_reReqMaxGap = 10;
  double		m_ackInterval = 0;
  unsigned		m_nAccepts = 8;
  unsigned		m_ttl = 1;
  bool			m_loopBack = false;

  Channels		m_channels;

  typedef MxMDBroadcast::Ring Ring;

  // Rx exclusive
  unsigned		m_attached = 0;
  ZmRef<Ring>		m_ring;

  // Tx queue GC
  ZmScheduler::Timer	m_ackTimer;
};

class MxMDAPI MxMDPubLink : public MxLink<MxMDPubLink> {
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef MxMDStream::Msg Msg;

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

    ZuInline MxMDPubLink *link() const { return m_link; }

    inline unsigned state() const { return m_state.load_(); }

    void connected(ZiIOContext &);
    void close();
    void disconnect();
    void disconnected();

    void process(ZmRef<MxQMsg>, ZiIOContext &);

    void snap(MxSeqNo seqNo);
    void *push(unsigned size);
    void *out(void *ptr, unsigned length, unsigned type, int shardID);
    void push2();

  private:
    MxMDPubLink		*m_link;

    ZmScheduler::Timer	m_loginTimer;

    ZmAtomic<unsigned>	m_state;

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
	Sending = 0,
	Disconnect
      };
    };

    UDP(MxMDPubLink *link, const ZiCxnInfo &ci);

    ZuInline MxMDPubLink *link() const { return m_link; }

    inline unsigned state() const { return m_state.load_(); }

    void connected(ZiIOContext &);
    void close();
    void disconnect();
    void disconnected();

    void recv(ZiIOContext &);
    void process(ZmRef<MxQMsg>, ZiIOContext &);

  private:
    MxMDPubLink		*m_link;

    ZmAtomic<unsigned>	m_state;
  };

public:
  typedef MxMDStream::Hdr Hdr;

  MxMDPubLink(MxID id) : MxLink<MxMDPubLink>(id) { }

  typedef MxMDPublisher Engine;

  ZuInline Engine *engine() const {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }
  ZuInline MxMDCore *core() const {
    return engine()->core();
  }

  ZuInline ZmTime loginTimeout() const { return engine()->loginTimeout(); }

  // MxAnyLink virtual
  void update(ZvCf *);
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo);
  bool failover() const { return m_failover; }
  void failover(bool v) {
    if (v == m_failover) return;
    m_failover = v;
    reconnect(true);
  }

  void connect();
  void disconnect();
private:
  void reconnect(bool immediate);
  void disconnect_();
public:

  // MxLink CRTP (unused)
  ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

  // MxLink Rx CRTP (unused)
  ZmTime reReqInterval() { return ZmTime{1}; }
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
  void reRequest(const MxQueue::Gap &now) { }

  // MxLink Tx CRTP
  void loaded_(MxQMsg *msg);
  void unloaded_(MxQMsg *msg);

  bool send_(MxQMsg *msg, bool more);
  bool resend_(MxQMsg *msg, bool more);

  bool sendGap_(const MxQueue::Gap &gap, bool more);
  bool resendGap_(const MxQueue::Gap &gap, bool more);

  // broadcast
  void sendMsg(const Hdr *hdr);
  void ack();

  // command support
  void status(ZtString &out);

private:
  // connection management
  void tcpListen();
  void tcpListening(const ZiListenInfo &);
  void tcpConnected(TCP *tcp);
  void tcpDisconnected(TCP *tcp);
  bool tcpLogin(const MxMDStream::Login &);
  void udpConnect();
  void udpConnected(UDP *udp, ZiIOContext &io);
  void udpReceived(const MxMDStream::ResendReq &);

  void tcpError(TCP *tcp, ZiIOContext *io);
  void udpError(UDP *udp, ZiIOContext *io);

  // snapshot
  void snap(ZmRef<TCP> tcp);

private:
  typedef MxQueueRx<MxMDPubLink> Rx;
  typedef MxQueueTx<MxMDPubLink> Tx;

  const MxMDChannel	*m_channel = 0;

  bool			m_failover = false;

  ZiSockAddr		m_udpAddr;

  Lock			m_connLock;
    ZiListenInfo	  m_listenInfo;
    ZmRef<TCPTbl>	  m_tcpTbl;
    ZmRef<UDP>		  m_udp;
};

#endif /* MxMDPublisher_HPP */
