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

#include <MxMDCore.hpp>

#include <MxMDPublisher.hpp>

void MxMDPublisher::init(MxMDCore *core, ZvCf *cf)
{
  if (!cf->get("id")) cf->set("id", "publisher");

  MxEngine::init(core, this, core->mx(), cf);

  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);
  if (ZuString ip = cf->get("interface")) m_interface = ip;
  m_maxQueueSize = cf->getInt("maxQueueSize", 1000, 1000000, false, 100000);
  m_loginTimeout = cf->getDbl("loginTimeout", 0, 3600, false, 10);
  m_reconnInterval = cf->getDbl("reconnInterval", 0, 3600, false, 10);
  m_reReqInterval = cf->getDbl("reReqInterval", 0, 3600, false, 1);
  m_reReqMaxGap = cf->getInt("reReqMaxGap", 0, 1000, false, 10);
  m_ttl = cf->getInt("ttl", 0, INT_MAX, false, 1);
  m_nAccepts = cf->getInt("nAccepts", 1, INT_MAX, false, 8);
  m_loopBack = cf->getInt("loopBack", 0, 1, false, 0);

  if (ZuString partitions = cf->get("partitions"))
    updateLinks(partitions);

  core->addCmd(
      "publisher.status", "",
      MxMDCmd::Fn::Member<&MxPublisher::statusCmd>::fn(this),
      "publisher status",
      "usage: publisher.status\n");
}

#define engineINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDPublisher::final()
{
  engineINFO("MxMDPublisher::final()");
}

void MxMDPublisher::updateLinks(ZuString partitions)
{
  MxMDPartitionCSV csv;
  csv.read(partitions, [](MxMDPublisher *pub, ZuAnyPOD *pod) {
      const MxMDPartition &partition = pod->as<MxMDPartition>();
      pub->m_partitions.del(partition.id);
      pub->m_partitions.add(partition);
      pub->updateLink(partition.id, nullptr);
    }, this);
}

#define linkINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=, id = id()](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDPubLink::update(ZvCf *)
{
  engine()->partition(id(), [this](MxMDPartition *partition) {
      if (ZuUnlikely(!partition)) throw ZvCf::Required(id());
      m_partition = partition;
    });
  if (m_partition && m_partition->enabled)
    up();
  else
    down();
}

void MxMDPubLink::reset(MxSeqNo, MxSeqNo txSeqNo)
{
  rxRun([rxSeqNo](Rx *rx) { rx->rxReset(rxSeqNo); });
  txRun([txSeqNo](Tx *tx) { tx->txReset(txSeqNo); });
}

#define tcpERROR(tcp, io, code) \
  do { \
    MxMDPubLink *link = tcp->link(); \
    MxMDPublisher *engine = link->engine(); \
    engine->appException(ZeEVENT(Error, \
      ([=, engineID = engine->id(), id = link->id()]( \
	const ZeEvent &, ZmStream &s) { \
	  s << "MxMDPublisher(" << engineID << ':' << id << \
	  ") " << code; }))); \
    link->tcpError(tcp, io); \
  } while (0)

#define udpERROR(udp, io, code) \
  do { \
    MxMDPubLink *link = udp->link(); \
    MxMDPublisher *engine = link->engine(); \
    engine->appException(ZeEVENT(Error, \
      ([=, engineID = engine->id(), id = link->id()]( \
	const ZeEvent &, ZmStream &s) { \
	  s << "MxMDPublisher(" << engineID << ':' << id << \
	  ") " << code; }))); \
    link->udpError(udp, io); \
  } while (0)

void MxMDPubLink::tcpError(TCP *tcp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (tcp)
    tcp->close();
}

void MxMDPubLink::udpError(UDP *udp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (udp)
    udp->close();
  Guard connGuard(m_connLock);
  if (!udp || udp == m_udp) reconnect();
}

void MxMDPubLink::connect()
{
  linkINFO("MxMDPubLink::connect(" << id << ')');

  {
    Guard connGuard(m_connLock);
    m_tcpTbl = new TCPTbl(ZmHashParams().bits(4).loadFactor(1.0).cBits(3).
	init(ZuStringN<ZmHeapIDSize>() <<
	  "MxMDPublisher." << id() << ".TCPTbl"));
  }

  tcpListen();
  udpConnect();
}

// - stop listening; all cxns are closed by link disconnect

void MxMDPubLink::disconnect()
{
  linkINFO("MxMDPubLink::disconnect(" << id << ')');

  ZmRef<Ring> ring;
  ZiListenInfo listenInfo;
  ZmRef<TCPTbl> tcpTbl;
  ZmRef<UDP> udp;

  wake();

  {
    Guard connGuard(m_connLock);
    ring = ZuMv(m_ring);
    listenInfo = m_listenInfo;
    tcpTbl = ZuMv(m_tcpTbl);
    udp = ZuMv(m_udp);
    m_ring = nullptr;
    m_ringID = -1;
    m_listenInfo = ZiListenInfo();
    m_tcpTbl = nullptr;
    m_udp = nullptr;
  }

  mx()->wakeFn(rxThread(), ZmFn<>());

  if (ring) {
    ring->detach();
    ring->close();
    ring = nullptr;
  }

  if (listenInfo.port) mx()->stopListening(listenInfo.ip, listenInfo.port);

  if (tcpTbl) {
    auto i = tcpTbl->readIterator();
    while (TCP *tcp = i.iterateKey()) tcp->disconnect();
    tcpTbl = nullptr;
  }

  if (udp) {
    udp->disconnect();
    udp = nullptr;
  }

  disconnected();
}

// TCP connect

void MxMDPubLink::tcpListen()
{
  ZiIP ip;
  uint16_t port;
  if (!engine()->failover()) {
    ip = m_partition->tcpIP;
    port = m_partition->tcpPort;
  } else {
    if (!(ip = m_partition->tcpIP2)) ip = m_partition->tcpIP;
    if (!(port = m_partition->tcpPort2)) port = m_partition->tcpPort;
  }

  linkINFO("MxMDPubLink::tcpListen(" << id << ") " <<
      ip << ':' << ZuBoxed(port) << ')');

  mx()->listen(
      ZiListenFn([](MxMDPubLink *link, const ZiListenInfo &info) {
	  link->tcpListening(info);
	}, this),
      ZiFailFn([](MxMDPubLink *link, bool transient) {
	  if (transient)
	    link->reconnect();
	  else
	    link->disconnected();
	}, this),
      ZiConnectFn([](MxMDPubLink *link, const ZiCxnInfo &ci) {
	  if (link->stateLocked([](MxMDPubLink *, int state) -> bool {
		return state != MxLinkState::Up; }))
	    return 0;
	  return new TCP(link, ci);
	}, this),
      ip, port, engine()->nAccepts(), ZiCxnOptions());
}
void MxMDPubLink::tcpListening(const ZiListenInfo &info)
{
  {
    Guard connGuard(m_connLock);
    m_listenInfo = info;
  }
  linkINFO("MxMDPubLink::tcpListening(" << id << ") " <<
      info.ip << ':' << ZuBoxed(info.port) << ')');
}
MxMDPubLink::TCP::TCP(MxMDPubLink *link, const ZiCxnInfo &ci) :
  ZiConnection(link->mx(), ci), m_link(link), m_state(State::Login)
{
  using namespace MxMDStream;
  m_in = new MxQMsg(new Msg());
}
void MxMDPubLink::TCP::connected(ZiIOContext &io)
{
  m_link->tcpConnected(this);
  m_in->fn = MxQMsgFn([](MxMDPubLink::TCP *tcp, MxQMsg *, ZiIOContext *io) {
      tcp->processLogin(*io); }, this);
  MxMDStream::TCP::recv(m_in, io);
  mx()->add(ZmFn<>([](MxMDPubLink::TCP *tcp) {
      {
	Guard stateGuard(m_stateLock);
	if (m_state != State::Login) return;
      }
      tcpERROR(tcp, 0, "TCP login timeout");
    }, this), ZmTimeNow(m_link->loginTimeout()), &m_loginTimer);
}
void MxMDPubLink::tcpConnected(MxMDPubLink::TCP *tcp)
{
  linkINFO("MxMDPubLink::tcpConnected(" << id << ") " <<
      tcp->info().remoteIP << ':' << ZuBoxed(tcp->info().remotePort));

  {
    Guard connGuard(m_connLock);
    m_tcpTbl->add(tcp);
  }
}

// TCP disconnect

void MxMDPubLink::TCP::disconnect_()
{
  Guard stateGuard(m_stateLock);
  m_state = State::Disconnect;
}
void MxMDPubLink::TCP::disconnect()
{
  disconnect_();
  ZiConnection::disconnect();
}
void MxMDPubLink::TCP::close()
{
  disconnect_();
  ZiConnection::close();
}
void MxMDPubLink::TCP::disconnected()
{
  mx()->del(&m_loginTimer);
  m_link->tcpDisconnected(this);
  bool intentional;
  {
    Guard stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) tcpERROR(this, 0, "TCP disconnected");
}
void MxMDPubLink::tcpDisconnected(MxMDPubLink::TCP *tcp)
{
  Guard connGuard(m_connLock);
  m_tcpTbl->del(tcp);
}

// TCP login, recv

void MxMDPubLink::TCP::processLogin(ZiIOContext &io)
{
  mx()->del(&m_loginTimer);
  {
    Guard stateGuard(m_stateLock);
    if (m_state != State::Login) {
      stateGuard.unlock();
      tcpERROR(this, &io, "TCP unexpected message");
      return;
    }
    {
      using namespace MxMDStream;
      const Frame &frame = m_in->as<Frame>();
      if (ZuUnlikely(frame.type != Type::Login)) {
	tcpERROR(this, &io, "TCP unexpected message type");
	return;
      }
      if (!m_link->tcpLogin(frame.as<Login>())) {
	tcpERROR(this, &io, "TCP invalid login");
	return;
      }
    }
    m_state = State::Sending;
  }

  if (!m_link->tcpLogin()) { io.disconnect(); return; }

  mx()->run(m_snapThread,
      [](MxMDPubLink::TCP *tcp) { tcp->snap(); }, ZmMkRef(this));

  m_in->fn = MxQMsgFn([](MxMDPubLink::TCP *tcp, MxQMsg *, ZiIOContext *io) {
      tcp->process(*io); }, this);
}
bool MxMDPubLink::tcpLogin(const MxMDStream::Login &login)
{
  if (login.username != m_partition->tcpUsername ||
      login.password != m_partition->tcpPassword) return false;
  linkINFO("MxMDPubLink::tcpLogin(" << id << ')');
  return true;
}
void MxMDPubLink::TCP::process(ZiIOContext &io)
{
  tcpERROR(this, &io, "TCP unexpected message");
  io.disconnect();
}

// UDP connect

void MxMDPubLink::udpConnect()
{
  rxInvoke([](MxMDPubLink::Rx *rx) { rx->startQueuing(); });

  ZiIP ip;
  uint16_t port;
  ZiIP resendIP;
  uint16_t resendPort;
  if (!engine()->failover()) {
    ip = m_partition->udpIP;
    port = m_partition->udpPort;
    resendIP = m_partition->resendIP;
    resendPort = m_partition->resendPort;
  } else {
    if (!(ip = m_partition->udpIP2)) ip = m_partition->udpIP;
    if (!(port = m_partition->udpPort2)) port = m_partition->udpPort;
    if (!(resendIP = m_partition->resendIP2))
      resendIP = m_partition->resendIP;
    if (!(resendPort = m_partition->resendPort2))
      resendPort = m_partition->resendPort;
  }
  m_udpAddr = ZiSockAddr(ip, port);
  ZiCxnOptions options;
  options.udp(true);
  options.multicast(true);
  options.mif(engine()->interface());
  options.ttl(engine()->ttl());
  options.loopBack(engine()->loopBack());
  mx()->udp(
      ZiConnectFn([](MxMDPubLink *link, const ZiCxnInfo &ci) {
	  if (link->stateLocked([](MxMDPubLink *, int state) -> bool {
		return state != MxLinkState::Connecting; }))
	    return 0;
	  return new UDP(link, ci);
	}, this),
      ZiFailFn([](MxMDPubLink *link, bool transient) {
	  if (transient)
	    link->reconnect();
	  else
	    link->disconnected();
	}, this),
      resendIP, resendPort, ZiIP(), 0, options);
}
MxMDPubLink::UDP::UDP(MxMDPubLink *link, const ZiCxnInfo &ci) :
    ZiConnection(link->mx(), ci), m_link(link),
    m_state(State::Receiving) { }
void MxMDPubLink::UDP::connected(ZiIOContext &io)
{
  m_link->udpConnected(this, io);
}
void MxMDPubLink::udpConnected(MxMDPubLink::UDP *udp, ZiIOContext &io)
{
  linkINFO("MxMDPubLink::udpConnected(" << id << ')');

  {
    Guard connGuard(m_connLock);
    m_udp = udp;
  }

  udp->recv(io); // begin receiving UDP packets (resend requests)

  MxMDBroadcast &broadcast = core()->broadcast();

  {
    ZmRef<ZiRing> ring;

    if (!(ring = broadcast.shadow()) || ring->attach() != Zi::OK) {
      engine()->rxInvoke(
	  ZmFn<>{[](MxMDPubLink *link) { link->disconnect(); }, this});
      return;
    }

    {
      Guard connGuard(m_connLock);
      m_ring = ring;
      m_ringID = ring->id();
    }
  }

  // begin reading broadcast

  mx()->wakeFn(engine()->rxThread(), ZmFn<>{[](MxMDPubLink *link) {
	link->rxRun_([](Rx *rx) { rx->app().recv(rx); });
	link->wake();
      }, this});

  rxRun_([](Rx *rx) { rx->app().recv(rx); });

  connected();
}

// UDP disconnect

void MxMDPubLink::UDP::disconnect_()
{
  Guard stateGuard(m_stateLock);
  m_state = State::Disconnect;
}
void MxMDPubLink::UDP::disconnect()
{
  disconnect_();
  ZiConnection::disconnect();
}
void MxMDPubLink::UDP::close()
{
  disconnect_();
  ZiConnection::close();
}
void MxMDPubLink::UDP::disconnected()
{
  bool intentional;
  {
    Guard stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) udpERROR(this, 0, "UDP disconnected");
}

// UDP recv

void MxMDPubLink::UDP::recv(ZiIOContext &io)
{
  using namespace MxMDStream;
  ZmRef<MxQMsg> msg = new MxQMsg(new Msg(),
      MxQMsgFn([](MxMDPubLink::UDP *udp, MxQMsg *msg, ZiIOContext *io) {
	  udp->process(msg, *io); }, this));
  MxMDStream::UDP::recv(msg, io);
}
void MxMDPubLink::UDP::process(MxQMsg *msg, ZiIOContext &io)
{
  using namespace MxMDStream;
  msg->length = io.offset + io.length;
  const Frame &frame = msg->as<Frame>();
  if (ZuUnlikely(frame.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message", msg->data(), msg->length()};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDPubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->addr = io.addr;
    m_link->udpReceived(msg, io);
  }
  recv(io);
}
void MxMDPubLink::udpReceived(MxQMsg *msg, ZiIOContext &io)
{
  using namespace MxMDStream;
  const Frame &frame = msg->as<Frame>();
  if (ZuUnlikely(frame.type != Type::ResendReq)) {
    udpERROR(this, &io, "UDP unexpected message type");
    return;
  }
  const ResendReq &resendReq = frame.as<ResendReq>();
  Gap gap{resendReq.seqNo, resendReq.count};
  txRun([gap](Tx *tx) { tx->resend(gap); });
}

// snapshot

void MxMDPubLink::TCP::snap()
{
  if (!core()->snapshot(*this, broadcast.id()))
    m_link->engine()->rxRun(
	ZmFn<>{[](MxMDPubLink *link) { link->disconnect(); }, m_link});
  m_snapMsg = nullptr;
}
Frame *MxMDPubLink::TCP::push(unsigned size)
{
  if (ZuUnlikely(state() != MxLinkState::Up)) return 0;
  m_snapMsg = new Msg();
  return m_snapMsg->frame();
}
void *MxMDPubLink::TCP::out(void *ptr, unsigned length, unsigned type,
    int shardID, ZmTime stamp)
{
  Frame *frame = new (ptr) Frame(
      length, type, shardID, 0, stamp.sec(), stamp.nsec());
  return frame->ptr();
}
void MxMDPubLink::TCP::push2()
{
  ZmRef<MxQMsg> qmsg = new MxQMsg(ZuMv(m_snapMsg), MxQMsgFn(), msg->length());
  MxMDStream::TCP::send(qmsg, this); // bypass Tx queue
}

// broadcast

void MxMDPubLink::recv(Rx *rx)
{
  using namespace MxMDStream;
  const Frame *frame;
  ZmRef<Ring> ring;
  { Guard guard(m_connLock); ring = m_ring; }
  if (ZuUnlikely(!ring)) return;
  if (ZuUnlikely(!(frame = ring->shift()))) {
    if (ZuLikely(ring->readStatus() == Zi::EndOfFile)) {
      disconnect();
      return;
    }
    rxRun_([](Rx *rx) { rx->app().recv(rx); });
    return;
  }
  if (frame->len > sizeof(Buf)) {
    ring->shift2();
    disconnect();
    core()->raise(ZeEVENT(Error,
	([name = ZtString(ring->params().name())](
	    const ZeEvent &, ZmStream &s) {
	  s << '"' << name << "\": "
	  "IPC shared memory ring buffer read error - "
	  "message too big";
	})));
    return;
  }
  switch ((int)frame->type) {
    case Type::EndOfSnapshot:
      {
	const EndOfSnapshot &eos = frame->as<EndOfSnapshot>();
	if (ZuUnlikely(eos.id == ring->id())) {
	  MxSeqNo seqNo = eos.seqNo;
	  ring->shift2();
	  // snapshot failure (!seqNo) is handled in snap()
	  if (seqNo) rx->stopQueuing(seqNo);
	} else
	  ring->shift2();
      }
      break;
    case Type::Wake:
      {
	const Wake &wake = frame->as<Wake>();
	if (ZuUnlikely(wake.id == ring->id())) {
	  ring->shift2();
	  return;
	}
	ring->shift2();
      }
      break;
    default:
      {
	if (m_partition->shardID < 0 || frame->shardID == ~(uint64_t)0 ||
	    frame->shardID == m_partition->shardID) {
	  unsigned length = sizeof(Frame) + frame->len;
	  ZmRef<Msg> msg = new Msg();
	  Frame *msgFrame = msg->frame();
	  memcpy(msgFrame, frame, length);
	  ring->shift2();
	  ZmRef<MxQMsg> qmsg = new MxQMsg(
	      msg, 0, length, MxMsgID{id(), msgFrame->seqNo});
	  rx->received(qmsg);
	} else
	  ring->shift2();
      }
      break;
  }
  rxRun_([](Rx *rx) { rx->app().recv(rx); });
}

ZmRef<MxAnyLink> MxMDPublisher::createLink(MxID id)
{
  m_link = new MxMDPubLink(id);
  return m_link;
}

MxEngineApp::ProcessFn MxMDPublisher::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *, MxAnyLink *link, MxQMsg *msg) {
    static_cast<Link *>(link)->txRun([](Tx *tx) { tx->send(msg); });
  });
}

void MxMDPubLink::wake()
{
  int id = m_ringID;
  if (id >= 0) MxMDStream::wake(core()->broadcast(), id);
}

bool MxMDPubLink::send_(MxQMsg *msg, bool)
{
  MxMDStream::UDP::send(msg, this, m_udpAddr);
}

bool MxMDPubLink::resend_(MxQMsg *msg, bool more)
{
  return send_(msg, more);
}

bool MxMDPubLink::sendGap_(const MxQueue::Gap &gap, bool more)
{
  return true;
}

bool MxMDPubLink::resendGap_(const MxQueue::Gap &gap, bool more)
{
  return true;
}

// commands

void MxMDPublisher::statusCmd(
    const MxMDCmd::Args &args, ZtArray<char> &out)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc != 1) throw MxMDCmd::Usage();
  out.size(512 * nLinks());
  out << "State: " << MxEngineState::name(state()) << '\n';
  allLinks([&out](MxAnyLink *link) {
	out << '\n';
	static_cast<MxMDPubLink *>(link)->status(out);
      });
}

void MxMDPubLink::status(ZtArray<char> &out)
{
  out << "Link " << id() << ":\n";
  out << "  TCP:    " <<
    m_partition->tcpIP << ':' << m_partition->tcpPort << " | " <<
    m_partition->tcpIP2 << ':' << m_partition->tcpPort2 << '\n';
  out << "  UDP:    " <<
    m_partition->udpIP << ':' << m_partition->udpPort << " | " <<
    m_partition->udpIP2 << ':' << m_partition->udpPort2 << '\n';
  out << "  Resend: " <<
    m_partition->resendIP << ':' << m_partition->resendPort << " | " <<
    m_partition->resendIP2 << ':' << m_partition->resendPort2 << '\n';
  out << "  TCP Username: " << m_partition->tcpUsername <<
    " Password: " << m_partition->tcpPassword << '\n';

  unsigned reconnects;
  out << "  State: " << MxLinkState::name(state(&reconnects));
  out << "  #Reconnects: " << ZuBox<unsigned>(reconnects);
  out << "  Snapshot SeqNo: " << snapshotSeqNo();
  out << "\n  TCP: ";
  if (ZmRef<TCP> tcp = m_tcp) {
    switch (tcp->state()) {
      case TCP::State::Login: out << "Login"; break;
      case TCP::State::Receiving: out << "Receiving"; break;
      case TCP::State::Disconnect: out << "Disconnect"; break;
      default: out << "Unknown"; break;
    }
  } else {
    out << "Disconnected";
  }
  out << "  UDP: ";
  if (ZmRef<UDP> udp = m_udp) {
    switch (udp->state()) {
      case UDP::State::Receiving: out << "Receiving"; break;
      case UDP::State::Disconnect: out << "Disconnect"; break;
      default: out << "Unknown"; break;
    }
  } else {
    out << "Disconnected";
  }
  out << "\n  UDP Queue: ";
  {
    thread_local ZmSemaphore sem;
    rxInvoke([&out, sem = &sem](Rx *rx) {
      const MxQueue &queue = rx->rxQueue();
      MxQueue::Gap gap = queue.gap();
      out <<
	"head: " << queue.head() << 
	"  gap: (" << gap.key() << ")," << ZuBox<unsigned>(gap.length()) <<
	"  length: " << ZuBox<unsigned>(queue.length()) << 
	"  count: " << ZuBox<unsigned>(queue.count());
      sem->post();
    });
    sem.wait();
  }
  out << '\n';
}
