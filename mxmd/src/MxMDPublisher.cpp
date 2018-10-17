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

// FIXME - validate rxThread, txThread, snapThread are distinct, and
// there is at least one other worker thread

void MxMDPublisher::init(MxMDCore *core)
{
  ZmRef<ZvCf> cf = core->cf()->subset("publisher", false, true);

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
      MxMDCmd::CmdFn::Member<&MxITCHFeed::status>::fn(this),
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
  ZmGuard<ZmLock> connGuard(m_connLock);
  if (!udp || udp == m_udp) reconnect();
}

void MxMDPubLink::connect()
{
  linkINFO("MxMDPubLink::connect(" << id << ')');

  {
    ZmGuard<ZmLock> connGuard(m_connLock);
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

  // FIXME - move ring like udp, close if non-null
  // FIXME - what if broadcast hasn't been opened/attached yet?
  // (it's deferred until UDP connected)
  MxMDRing &broadcast = core()->broadcast();
  // FIXME - use shadow

  MxMDStream::detach(broadcast, broadcast.id());
  m_detachSem.wait();

  broadcast.close();

  ZiListenInfo listenInfo;
  ZmRef<TCPTbl> tcpTbl;
  ZmRef<UDP> udp;

  {
    ZmGuard<ZmLock> connGuard(m_connLock);
    listenInfo = m_listenInfo;
    m_listenInfo = ZiListenInfo();
    tcpTbl = ZuMv(m_tcpTbl);
    udp = ZuMv(m_udp);
    m_tcpTbl = nullptr;
    m_udp = nullptr;
  }

  if (listenInfo.port) mx()->stopListening(listenInfo.ip, listenInfo.port);
  if (tcpTbl) {
    auto i = tcpTbl->readIterator();
    while (TCP *tcp = i.iterateKey()) tcp->disconnect();
    tcpTbl = nullptr;
  }
  if (udp) udp->disconnect();

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
    ZmGuard<ZmLock> connGuard(m_connLock);
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
	ZmGuard<ZmLock> stateGuard(m_stateLock);
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
    ZmGuard<ZmLock> connGuard(m_connLock);
    m_tcpTbl->add(tcp);
  }
}

// TCP disconnect

void MxMDPubLink::TCP::disconnect_()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
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
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) tcpERROR(this, 0, "TCP disconnected");
}
void MxMDPubLink::tcpDisconnected(MxMDPubLink::TCP *tcp)
{
  ZmGuard<ZmLock> connGuard(m_connLock);
  m_tcpTbl->del(tcp);
}

// TCP login, recv

// FIXME - use below as template for sending messages to subscriber
/*
void MxMDPubLink::TCP::sendLogin()
{
  using namespace MxMDStream;
  ZmRef<Msg> msg = new Msg();
  new (msg->out<Login>(id(), 1, ZmTime()))
    Login{m_partition->tcpUsername, m_partition->tcpPassword};
  ZmRef<MxQMsg> qmsg = new MxQMsg(msg, MxQMsgFn(), msg->length());
  MxMDStream::TCP::send(qmsg, this); // bypass Tx queue
}
*/
void MxMDPubLink::TCP::processLogin(ZiIOContext &io)
{
  mx()->del(&m_loginTimer);
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
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
void MxMDPubLink::TCP::snap()
{
  m_snapMsg = new MsgData();
  if (!core()->snapshot(*this, broadcast.id())) disconnect();
  m_snapMsg = nullptr;
}
Frame *MxMDPubLink::TCP::push(unsigned size)
{
  if (ZuUnlikely(state() != MxLinkState::Up)) return 0;
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
  Frame *frame = m_snapMsg->frame();
  // FIXME - TCP send
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
  m_udpResendAddr = ZiSockAddr(resendIP, resendPort);
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
    ZmGuard<ZmLock> connGuard(m_connLock);
    m_udp = udp;
  }

  udp->recv(io); // begin receiving UDP packets (resend requests)

  rxInvoke([](Rx *rx) { rx->app().recv(rx)); });
  m_attachSem.wait();

  connected();
}

void MxMDPubLink::recv(Rx *rx)
{
  using namespace MxMDStream;
  MxMDRing &broadcast = core()->broadcast();
  // FIXME - connLock? or shard m_ring in rxThread
  if (!(m_ring = broadcast.shadow()) || m_ring->attach() != Zi::OK) {
    disconnect();
    m_attachSem.post();
    return;
  }
  m_attachSem.post();
  rxRun([](Rx *rx) { rx->app().recvMsg(rx); });
}

void MxMDPubLink::recvMsg(Rx *rx)
{
  using namespace MxMDStream;
  const Frame *frame;
  // FIXME - connLock
  if (ZuUnlikely(!(frame = broadcast.shift()))) {
    if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) {
      broadcast.detach();
      broadcast.close();
      { Guard guard(m_lock); m_file.close(); }
      disconnected();
      return;
    }
    rxRun([](Rx *rx) { rx->app().recvMsg(rx); });
    return;
  }
  if (frame->len > sizeof(Buf)) {
    broadcast.shift2();
    broadcast.detach();
    broadcast.close();
    { Guard guard(m_lock); m_file.close(); }
    disconnected();
    core()->raise(ZeEVENT(Error,
	([name = ZtString(broadcast.params().name())](
	    const ZeEvent &, ZmStream &s) {
	  s << '"' << name << "\": "
	  "IPC shared memory ring buffer read error - "
	  "message too big";
	})));
    return;
  }
  // FIXME - advance both rx and tx to seqNo of this message if this is
  // the first msg
  switch ((int)frame->type) {
    case Type::EndOfSnapshot:
      {
	const EndOfSnapshot &eos = frame->as<EndOfSnapshot>();
	if (ZuUnlikely(eos.id == broadcast.id())) {
	  MxSeqNo seqNo = eos.seqNo;
	  broadcast.shift2();
	  // snapshot failure (!seqNo) is handled in snap()
	  if (seqNo) rx->stopQueuing(seqNo);
	} else
	  broadcast.shift2();
      }
      break;
    case Type::Detach:
      {
	const Detach &detach = frame->as<Detach>();
	if (ZuUnlikely(detach.id == broadcast.id())) {
	  broadcast.shift2();
	  broadcast.detach();
	  m_detachSem.post();
	  return;
	}
	broadcast.shift2();
      }
      break;
    default:
      {
	unsigned length = sizeof(Frame) + frame->len;
	ZmRef<Msg> msg = new Msg();
	Frame *msgFrame = msg->frame();
	memcpy(msgFrame, frame, length);
	broadcast.shift2();
	ZmRef<MxQMsg> qmsg = new MxQMsg(
	    msg, 0, length, MxMsgID{id(), msgFrame->seqNo});
	rx->received(qmsg);
      }
      break;
  }
  rxRun([](Rx *rx) { rx->app().recvMsg(rx); });
}

MxEngineApp::ProcessFn MxMDRecord::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *, MxAnyLink *link, MxQMsg *msg) {
    static_cast<Link *>(link)->txRun([](Tx *tx) { tx->send(msg); });
  });
}

bool MxMDPubLink::send_(MxQMsg *msg, bool more)
{
  // FIXME - low level send to UDP; fail if udp not connected
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

// UDP disconnect

void MxMDPubLink::UDP::disconnect_()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
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
    ZmGuard<ZmLock> stateGuard(m_stateLock);
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
	  udp->process(msg, io); }, this));
  MxMDStream::UDP::recv(msg, io);
}
void MxMDPubLink::UDP::process(MxQMsg *msg, ZiIOContext *io)
{
  using namespace MxMDStream;
  msg->length = io->offset + io->length;
  const Frame &frame = msg->as<Frame>();
  if (ZuUnlikely(frame.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message", msg->data(), msg->length()};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDPubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->addr = io->addr;
    m_link->udpReceived(msg);
  }
  recv(*io);
}
void MxMDPubLink::udpReceived(MxQMsg *msg)
{
  // FIXME - process incoming resend request, error if not a Resend

  Gap gap{/* FIXME */};

  txRun([gap](Tx *tx) { tx->resend(gap); });
}

// commands

void MxMDPublisher::statusCmd(const MxMDCmd::CmdArgs &args, ZtArray<char> &out)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc != 1) throw MxMDCmd::CmdUsage();
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
  out << "  TCP:    " << m_partition->tcpIP << ':' <<
    ZuBox<uint16_t>(m_partition->tcpPort) << " | " <<
    m_partition->tcpIP2 << ':' <<
    ZuBox<uint16_t>(m_partition->tcpPort2) << '\n';
  out << "  UDP:    " << m_partition->udpIP << ':' <<
    ZuBox<uint16_t>(m_partition->udpPort) << " | " <<
    m_partition->udpIP2 << ':' <<
    ZuBox<uint16_t>(m_partition->udpPort2) << '\n';
  out << "  Resend: " << m_partition->resendIP << ':' <<
    ZuBox<uint16_t>(m_partition->resendPort) << " | " <<
    m_partition->resendIP2 << ':' <<
    ZuBox<uint16_t>(m_partition->resendPort2) << '\n';
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
    rxInvoke([&sem, &out](Rx *rx) {
      const MxQueue &queue = rx->rxQueue();
      MxQueue::Gap gap = queue.gap();
      out <<
	"head: " << queue.head() << 
	"  gap: (" << gap.key() << ")," << ZuBox<unsigned>(gap.length()) <<
	"  length: " << ZuBox<unsigned>(queue.length()) << 
	"  count: " << ZuBox<unsigned>(queue.count());
      sem.post();
    });
    sem.wait();
  }
  out << '\n';
}

// FIXME from here
//
void MxMDPublisher::TCP::send(const Frame *frame)
{
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:	send_<AddTickSizeTbl>(frame); break;
    case Type::ResetTickSizeTbl: send_<ResetTickSizeTbl>(frame); break;
    case Type::AddTickSize:	send_<AddTickSize>(frame); break;
    case Type::AddSecurity:	send_<AddSecurity>(frame); break;
    case Type::UpdateSecurity:	send_<UpdateSecurity>(frame); break;
    case Type::AddOrderBook:	send_<AddOrderBook>(frame); break;
    case Type::DelOrderBook:	send_<DelOrderBook>(frame); break;
    case Type::AddCombination:	send_<AddCombination>(frame); break;
    case Type::DelCombination:	send_<DelCombination>(frame); break;
    case Type::UpdateOrderBook:	send_<UpdateOrderBook>(frame); break;
    case Type::TradingSession:	send_<TradingSession>(frame); break;
    case Type::L1:		send_<L1>(frame); break;
    case Type::PxLevel:		send_<PxLevel>(frame); break;
    case Type::L2:		send_<L2>(frame); break;
    case Type::AddOrder:	send_<AddOrder>(frame); break;
    case Type::ModifyOrder:	send_<ModifyOrder>(frame); break;
    case Type::CancelOrder:	send_<CancelOrder>(frame); break;
    case Type::ResetOB:		send_<ResetOB>(frame); break;
    case Type::AddTrade:	send_<AddTrade>(frame); break;
    case Type::CorrectTrade:	send_<CorrectTrade>(frame); break;
    case Type::CancelTrade:	send_<CancelTrade>(frame); break;
    case Type::RefDataLoaded:	send_<RefDataLoaded>(frame); break;
    
    case Type::EndOfSnapshot:
      send_<EndOfSnapshot>(frame);
      std::cout << "Publisher TCP send EndOfSnapshot +++++++" << std::endl;
      break;
    
    default:
      m_publisher->core()->raise(ZeEVENT(Info, "Publisher TCP send default"));
    
      break;
  }
}

void MxMDPublisher::UDP::send(const Frame *frame)
{
  using namespace MxMDStream;

  switch ((int)frame->type) {
    case Type::AddTickSizeTbl:	send_<AddTickSizeTbl>(frame); break;
    case Type::ResetTickSizeTbl: send_<ResetTickSizeTbl>(frame); break;
    case Type::AddTickSize:	send_<AddTickSize>(frame); break;
    case Type::AddSecurity:	send_<AddSecurity>(frame); break;
    case Type::UpdateSecurity:	send_<UpdateSecurity>(frame); break;
    case Type::AddOrderBook:	send_<AddOrderBook>(frame); break;
    case Type::DelOrderBook:	send_<DelOrderBook>(frame); break;
    case Type::AddCombination:	send_<AddCombination>(frame); break;
    case Type::DelCombination:	send_<DelCombination>(frame); break;
    case Type::UpdateOrderBook:	send_<UpdateOrderBook>(frame); break;
    case Type::TradingSession:	send_<TradingSession>(frame); break;
    case Type::L1:		send_<L1>(frame); break;
    case Type::PxLevel:		send_<PxLevel>(frame); break;
    case Type::L2:		send_<L2>(frame); break;
    case Type::AddOrder:	send_<AddOrder>(frame); break;
    case Type::ModifyOrder:	send_<ModifyOrder>(frame); break;
    case Type::CancelOrder:	send_<CancelOrder>(frame); break;
    case Type::ResetOB:		send_<ResetOB>(frame); break;
    case Type::AddTrade:	send_<AddTrade>(frame); break;
    case Type::CorrectTrade:	send_<CorrectTrade>(frame); break;
    case Type::CancelTrade:	send_<CancelTrade>(frame); break;
    case Type::RefDataLoaded:	send_<RefDataLoaded>(frame); break;
    default: 
      m_publisher->core()->raise(ZeEVENT(Info, "Publisher UDP send default"));
      break;
  }
}
