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
  if (!cf->get("id")) cf->set("id", "publish");

  MxMultiplex *mx = core->mx();

  MxEngine::init(core, this, mx, cf);

  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);

  if (rxThread() == mx->rxThread() ||
      m_snapThread == mx->rxThread() ||
      m_snapThread == rxThread())
    throw ZtString() << "publisher misconfigured - thread conflict -"
      " I/O Rx: " << ZuBoxed(mx->rxThread()) <<
      " IPC Rx: " << ZuBoxed(rxThread()) <<
      " Snapshot: " << ZuBoxed(m_snapThread);

  if (ZuString ip = cf->get("interface")) m_interface = ip;
  m_maxQueueSize = cf->getInt("maxQueueSize", 1000, 1000000, false, 100000);
  m_loginTimeout = cf->getDbl("loginTimeout", 0, 3600, false, 3);
  m_ackInterval = cf->getDbl("ackInterval", 0, 3600, false, 10);
  m_reReqMaxGap = cf->getInt("reReqMaxGap", 0, 1000, false, 10);
  m_ttl = cf->getInt("ttl", 0, INT_MAX, false, 1);
  m_nAccepts = cf->getInt("nAccepts", 1, INT_MAX, false, 8);
  m_loopBack = cf->getInt("loopBack", 0, 1, false, 0);

  if (ZuString partitions = cf->get("partitions"))
    updateLinks(partitions);

  core->addCmd(
      "publisher.status", "",
      MxMDCmd::Fn::Member<&MxMDPublisher::statusCmd>::fn(this),
      "publisher status",
      "usage: publisher.status\n");
}

#define engineINFO(code) \
    appException(ZeEVENT(Info, \
      ([=](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDPublisher::final()
{
  engineINFO("MxMDPublisher::final()");
}

void MxMDPublisher::updateLinks(ZuString partitions)
{
  MxMDPartitionCSV csv;
  csv.read(partitions, ZvCSVReadFn{[](MxMDPublisher *pub, ZuAnyPOD *pod) {
      const MxMDPartition &partition = pod->as<MxMDPartition>();
      pub->m_partitions.del(partition.id);
      pub->m_partitions.add(partition);
      pub->updateLink(partition.id, nullptr);
    }, this});
}

ZmRef<MxAnyLink> MxMDPublisher::createLink(MxID id)
{
  return new MxMDPubLink(id);
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
  if (!udp || udp == m_udp) { connGuard.unlock(); reconnect(false); }
}

void MxMDPubLink::connect()
{
  linkINFO("MxMDPubLink::connect(" << id << ')');

  reset(0, 0);

  {
    Guard connGuard(m_connLock);
    m_tcpTbl = new TCPTbl(ZmHashParams().bits(4).loadFactor(1.0).cBits(3).
	init(ZuStringN<ZmHeapIDSize>() <<
	  "MxMDPublisher." << id() << ".TCPTbl"));
  }

  txInvoke([](Tx *tx) { tx->start(); }); // start sending

  tcpListen();
  udpConnect();
}

// - stop listening; all cxns are closed by link disconnect

void MxMDPubLink::disconnect()
{
  linkINFO("MxMDPubLink::disconnect(" << id << ')');

  disconnect_();

  disconnected();
}

void MxMDPubLink::reconnect(bool immediate)
{
  linkINFO("MxMDPubLink::reconnect(" << id << ')');

  disconnect_();
  
  MxLink::reconnect(immediate);
}

void MxMDPubLink::disconnect_()
{
  ZiListenInfo listenInfo;
  ZmRef<TCPTbl> tcpTbl;
  ZmRef<UDP> udp;

  {
    Guard connGuard(m_connLock);
    listenInfo = m_listenInfo;
    tcpTbl = ZuMv(m_tcpTbl);
    udp = ZuMv(m_udp);
    m_listenInfo = ZiListenInfo();
    m_tcpTbl = nullptr;
    m_udp = nullptr;
  }

  txInvoke([](Tx *tx) { tx->stop(); }); // stop sending

  engine()->detach();

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
}

// TCP connect

void MxMDPubLink::tcpListen()
{
  ZiIP ip;
  uint16_t port;
  if (!m_failover) {
    ip = m_partition->tcpIP;
    port = m_partition->tcpPort;
  } else {
    if (!(ip = m_partition->tcpIP2)) ip = m_partition->tcpIP;
    if (!(port = m_partition->tcpPort2)) port = m_partition->tcpPort;
  }

  linkINFO("MxMDPubLink::tcpListen(" << id << ") " <<
      ip << ':' << ZuBoxed(port));

  mx()->listen(
      ZiListenFn([](MxMDPubLink *link, const ZiListenInfo &info) {
	  link->tcpListening(info);
	}, this),
      ZiFailFn([](MxMDPubLink *link, bool transient) {
	  if (transient)
	    link->reconnect(false);
	  else
	    link->engine()->rxRun(ZmFn<>{
		[](MxMDPubLink *link) { link->disconnect(); }, link});
	}, this),
      ZiConnectFn([](MxMDPubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  if (link->state() != MxLinkState::Up)
	    return 0;
	  return (uintptr_t)(new TCP(link, ci));
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
      info.ip << ':' << ZuBoxed(info.port));
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
  MxMDStream::TCP::recv<TCP>(m_in, io,
      [](TCP *tcp, MxQMsg *, ZiIOContext &io) { tcp->process(io); });
  mx()->rxRun(ZmFn<>{[](MxMDPubLink::TCP *tcp) {
      if (tcp->m_state != State::Login) return;
      tcpERROR(tcp, 0, "TCP login timeout");
    }, ZmMkRef(this)}, ZmTimeNow(m_link->loginTimeout()), &m_loginTimer);
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

void MxMDPubLink::TCP::disconnect()
{
  m_state = State::Disconnect;
  ZiConnection::disconnect();
}
void MxMDPubLink::TCP::close()
{
  m_state = State::Disconnect;
  ZiConnection::close();
}
void MxMDPubLink::TCP::disconnected()
{
  if (m_state == State::Login) mx()->del(&m_loginTimer);
  m_link->tcpDisconnected(this);
  // if (m_state != State::Disconnect) tcpERROR(this, 0, "TCP disconnected");
}
void MxMDPubLink::tcpDisconnected(MxMDPubLink::TCP *tcp)
{
  Guard connGuard(m_connLock);
  m_tcpTbl->del(tcp->info().socket);
}

// TCP login, recv

void MxMDPubLink::TCP::process(ZiIOContext &io)
{
  if (ZuUnlikely(m_state.load_() != State::Login)) {
    tcpERROR(this, &io, "TCP unexpected message");
    return;
  }

  mx()->del(&m_loginTimer);

  {
    using namespace MxMDStream;
    const Hdr &hdr = m_in->as<Hdr>();
    if (ZuUnlikely(hdr.type != Type::Login)) {
      tcpERROR(this, &io, "TCP unexpected message type");
      return;
    }
    if (!m_link->tcpLogin(hdr.as<Login>())) {
      tcpERROR(this, &io, "TCP invalid login");
      return;
    }
  }

  m_state = State::Sending;

  m_link->engine()->rxRun(
      ZmFn<>{[tcp = ZmMkRef(this)](MxMDPubLink *link) mutable {
	link->snap(ZuMv(tcp));
      }, m_link});
}
bool MxMDPubLink::tcpLogin(const MxMDStream::Login &login)
{
  if (login.username != m_partition->tcpUsername ||
      login.password != m_partition->tcpPassword) return false;
  linkINFO("MxMDPubLink::tcpLogin(" << id << ')');
  return true;
}

// UDP connect

void MxMDPubLink::udpConnect()
{
  ZiIP ip;
  uint16_t port;
  ZiIP resendIP;
  uint16_t resendPort;
  if (!m_failover) {
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
  if (ip.multicast()) {
    options.multicast(true);
    options.mif(engine()->interface());
    options.ttl(engine()->ttl());
    options.loopBack(engine()->loopBack());
  }
  mx()->udp(
      ZiConnectFn([](MxMDPubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  // link state will not be Up until UDP has connected
	  switch ((int)link->state()) {
	    case MxLinkState::Connecting:
	    case MxLinkState::Reconnecting:
	      return (uintptr_t)(new UDP(link, ci));
	    case MxLinkState::DisconnectPending:
	      link->connected();
	    default:
	      return 0;
	  }
	}, this),
      ZiFailFn([](MxMDPubLink *link, bool transient) {
	  if (transient)
	    link->reconnect(false);
	  else
	    link->engine()->rxRun(ZmFn<>{
		[](MxMDPubLink *link) { link->disconnect(); }, link});
	}, this),
      ZiIP(), resendPort, ZiIP(), 0, options);
}
MxMDPubLink::UDP::UDP(MxMDPubLink *link, const ZiCxnInfo &ci) :
    ZiConnection(link->mx(), ci), m_link(link),
    m_state(State::Sending) { }
void MxMDPubLink::UDP::connected(ZiIOContext &io)
{
  m_link->udpConnected(this, io);
}
void MxMDPubLink::udpConnected(MxMDPubLink::UDP *udp, ZiIOContext &io)
{
  linkINFO("MxMDPubLink::udpConnected(" << id << ')');

  ZmRef<UDP> old;
  {
    Guard connGuard(m_connLock);
    old = m_udp;
    m_udp = udp;
  }
  if (ZuUnlikely(old)) { old->disconnect(); old = nullptr; } // paranoia

  udp->recv(io); // begin receiving UDP packets (resend requests)

  if (!engine()->attach()) return;

  connected();
}

// UDP disconnect

void MxMDPubLink::UDP::disconnect()
{
  m_state = State::Disconnect;
  ZiConnection::disconnect();
}
void MxMDPubLink::UDP::close()
{
  m_state = State::Disconnect;
  ZiConnection::close();
}
void MxMDPubLink::UDP::disconnected()
{
  if (m_state != State::Disconnect) udpERROR(this, 0, "UDP disconnected");
}

// UDP recv

void MxMDPubLink::UDP::recv(ZiIOContext &io)
{
  using namespace MxMDStream;
  ZmRef<MxQMsg> msg = new MxQMsg(new Msg());
  MxMDStream::UDP::recv<UDP>(ZuMv(msg), io,
      [](UDP *udp, ZmRef<MxQMsg> msg, ZiIOContext &io) {
	udp->process(ZuMv(msg), io);
      });
}
void MxMDPubLink::UDP::process(MxQMsg *msg, ZiIOContext &io)
{
  using namespace MxMDStream;
  msg->length = io.offset + io.length;
  const Hdr &hdr = msg->as<Hdr>();
  if (ZuUnlikely(hdr.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message", msg->ptr(), msg->length};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDPubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->addr = io.addr;
    const Hdr &hdr = msg->as<Hdr>();
    if (ZuUnlikely(hdr.type != Type::ResendReq)) {
      // ignore to prevent (probably accidental) DOS
      // udpERROR(this, &io, "UDP unexpected message type");
      return;
    }
    m_link->udpReceived(hdr.as<ResendReq>());
  }
  recv(io);
}
void MxMDPubLink::udpReceived(const MxMDStream::ResendReq &resendReq)
{
  using namespace MxMDStream;
  MxQueue::Gap gap{resendReq.seqNo, resendReq.count};
  txRun([gap](Tx *tx) {
	if (gap.key() < tx->txQueue().head()) return;
	if (gap.length() > tx->app().engine()->reReqMaxGap()) return;
	tx->resend(gap);
      });
}

// snapshot

void MxMDPubLink::snap(ZmRef<TCP> tcp)
{
  txRun([tcp = ZuMv(tcp)](Tx *tx) mutable {
      tx->app().mx()->run(tx->app().engine()->snapThread(),
	  ZmFn<>{[seqNo = tx->txQueue().tail()](MxMDPubLink::TCP *tcp) {
	    tcp->snap(seqNo); }, ZuMv(tcp)});
      });
}
void MxMDPubLink::TCP::snap(MxSeqNo seqNo)
{
  if (!m_link->core()->snapshot(*this, m_link->id(), seqNo))
    m_link->engine()->rxRun(
	ZmFn<>{[](MxMDPubLink *link) { link->disconnect(); }, m_link});
  else
    disconnect();
  m_snapMsg = nullptr;
}
void *MxMDPubLink::TCP::push(unsigned size)
{
  if (ZuUnlikely(m_link->state() != MxLinkState::Up)) return 0;
  m_snapMsg = new Msg();
  return m_snapMsg->ptr();
}
void *MxMDPubLink::TCP::out(
    void *ptr, unsigned length, unsigned type, int shardID)
{
  Hdr *hdr = new (ptr) Hdr(
      (uint16_t)length, (uint8_t)type, (uint8_t)shardID);
  return hdr->body();
}
void MxMDPubLink::TCP::push2()
{
  unsigned msgLen = m_snapMsg->length();
  ZmRef<MxQMsg> qmsg = new MxQMsg(ZuMv(m_snapMsg), msgLen);
  MxMDStream::TCP::send(this, ZuMv(qmsg));
}

// broadcast

bool MxMDPublisher::attach()
{
  if (m_attached++) return true;

  MxMDBroadcast &broadcast = core()->broadcast();

  if (!(m_ring = broadcast.shadow()) || m_ring->attach() != Zi::OK) {
    m_ring = nullptr;
    m_attached = 0;
    return false;
  }

  mx()->wakeFn(rxThread(), ZmFn<>{[](MxMDPublisher *pub) {
	pub->rxRun_(ZmFn<>{[](MxMDPublisher *pub) { pub->recv(); }, pub});
	pub->wake();
      }, this});

  rxRun_(ZmFn<>{[](MxMDPublisher *pub) { pub->recv(); }, this});

  mx()->run(txThread(), ZmFn<>{[](MxMDPublisher *pub) {
	pub->ack();
      }, this}, ZmTimeNow(ackInterval()), &m_ackTimer);

  return true;
}

void MxMDPublisher::detach()
{
  if (!m_attached || --m_attached) return; 

  mx()->del(&m_ackTimer);

  if (!m_ring) return; // paranoia

  m_ring->detach();
  m_ring->close();
  m_ring = nullptr;
}

void MxMDPublisher::wake()
{
  MxMDStream::wake(core()->broadcast(), id());
}

void MxMDPublisher::recv()
{
  using namespace MxMDStream;
  if (ZuUnlikely(!m_ring)) return;
  switch ((int)state()) {
    case MxEngineState::Starting:
    case MxEngineState::Running:
      break;
    default:
      mx()->wakeFn(rxThread(), ZmFn<>());
      return;
  }
  const Hdr *hdr;
  for (;;) {
    if (ZuUnlikely(!(hdr = m_ring->shift()))) {
      if (ZuLikely(m_ring->readStatus() == Zi::EndOfFile)) {
	allLinks<MxMDPubLink>([](MxMDPubLink *link) -> uintptr_t {
	    link->disconnect(); return 0; });
	return;
      }
      continue;
    }
    if (hdr->len > sizeof(Buf)) {
      m_ring->shift2();
      allLinks<MxMDPubLink>([](MxMDPubLink *link) -> uintptr_t {
	  link->disconnect(); return 0; });
      core()->raise(ZeEVENT(Error,
	  ([name = ZtString(m_ring->params().name())](
	      const ZeEvent &, ZmStream &s) {
	    s << '"' << name << "\": "
	    "IPC shared memory ring buffer read error - "
	    "message too big / corrupt";
	  })));
      return;
    }
    switch ((int)hdr->type) {
      case Type::Wake:
	{
	  const Wake &wake = hdr->as<Wake>();
	  if (ZuUnlikely(wake.id == id())) {
	    m_ring->shift2();
	    return;
	  }
	  m_ring->shift2();
	}
	break;
      case Type::EndOfSnapshot: // ignored
	m_ring->shift2();
	break;
      default:
	allLinks<MxMDPubLink>([hdr](MxMDPubLink *link) -> uintptr_t {
	    link->sendMsg(hdr); return 0; });
	m_ring->shift2();
	break;
    }
  }
}

MxEngineApp::ProcessFn MxMDPublisher::processFn()
{
  return nullptr;
}

void MxMDPubLink::sendMsg(const Hdr *hdr)
{
  if (m_partition->shardID >= 0 && hdr->shard != 0xff &&
      hdr->shard != m_partition->shardID) return;
  using namespace MxMDStream;
  ZuRef<Msg> msg = new Msg();
  unsigned msgLen = sizeof(Hdr) + hdr->len;
  memcpy((void *)msg->ptr(), (void *)hdr, msgLen);
  ZmRef<MxQMsg> qmsg = new MxQMsg(msg, msgLen);
  switch ((int)hdr->type) {
    case Type::Wake:
    case Type::EndOfSnapshot:
    case Type::Login:
    case Type::ResendReq:
      return;
  }
  txInvoke([qmsg = ZuMv(qmsg)](Tx *tx) { tx->send(ZuMv(qmsg)); });
}

void MxMDPubLink::loaded_(MxQMsg *msg)
{
  using namespace MxMDStream;
  Hdr &hdr = msg->as<Hdr>();
  hdr.seqNo = msg->id.seqNo;
}

void MxMDPubLink::unloaded_(MxQMsg *msg) { } // unused

bool MxMDPubLink::send_(MxQMsg *msg, bool more)
{
  ZmRef<UDP> udp;
  {
    Guard connGuard(m_connLock);
    udp = m_udp;
  }
  if (ZuUnlikely(!udp)) return false;
  MxMDStream::UDP::send(udp.ptr(), msg, m_udpAddr);
  return true;
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

void MxMDPublisher::ack()
{
  if (state() != MxEngineState::Running) return;
  allLinks<MxMDPubLink>(
      [maxQueueSize = maxQueueSize()](MxMDPubLink *link) -> uintptr_t {
	link->ack(); return 0; });
  mx()->run(txThread(), ZmFn<>{[](MxMDPublisher *pub) {
	pub->ack();
      }, this}, ZmTimeNow(ackInterval()), &m_ackTimer);
}

void MxMDPubLink::ack()
{
  txInvoke([maxQueueSize = engine()->maxQueueSize()](MxMDPubLink::Tx *tx) {
	MxSeqNo seqNo = tx->txQueue().tail();
	if (seqNo < maxQueueSize) return;
	tx->ackd(seqNo - maxQueueSize);
      });
}

void MxMDPublisher::archive(MxAnyLink *link, MxQMsg *msg)
{
  link->archived(msg->id.seqNo);
}

// commands

void MxMDPublisher::statusCmd(
    const MxMDCmd::Args &args, ZtArray<char> &out)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc != 1) throw MxMDCmd::Usage();
  out.size(512 * nLinks());
  out << "State: " << MxEngineState::name(state()) << '\n';
  allLinks<MxMDPubLink>([&out](MxMDPubLink *link) -> uintptr_t {
	out << '\n'; link->status(out); return 0; });
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

  out << "  State: " << MxLinkState::name(state());
  out << '\n';
  if (ZmRef<TCPTbl> tcpTbl = m_tcpTbl) {
    auto i = tcpTbl->readIterator();
    while (TCP *tcp = i.iterateKey()) {
      const ZiCxnInfo &info = tcp->info();
      out << "  TCP " <<
	info.remoteIP << ':' << ZuBoxed(info.remotePort) << ' ';
      switch (tcp->state()) {
	case TCP::State::Login: out << "Login"; break;
	case TCP::State::Sending: out << "Sending"; break;
	case TCP::State::Disconnect: out << "Disconnect"; break;
	default: out << "Unknown"; break;
      }
      out << '\n';
    }
  } else {
    out << "  TCP Disconnected\n";
  }
  out << "  UDP ";
  if (ZmRef<UDP> udp = m_udp) {
    switch (udp->state()) {
      case UDP::State::Sending: out << "Sending"; break;
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
