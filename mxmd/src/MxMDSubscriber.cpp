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

#include <MxMDCore.hpp>

#include <MxMDSubscriber.hpp>

void MxMDSubscriber::init(MxMDCore *core, ZvCf *cf)
{
  if (!cf->get("id")) cf->set("id", "subscrib");

  MxEngine::init(core, this, core->mx(), cf);

  if (ZuString ip = cf->get("interface")) m_interface = ip;
  m_filter = cf->getInt("filter", 0, 1, false, 0);
  m_maxQueueSize = cf->getInt("maxQueueSize", 1000, 1000000, false, 100000);
  m_loginTimeout = cf->getDbl("loginTimeout", 0, 3600, false, 10);
  m_reconnInterval = cf->getDbl("reconnInterval", 0, 3600, false, 10);
  m_reReqInterval = cf->getDbl("reReqInterval", 0, 3600, false, 1);
  m_reReqMaxGap = cf->getInt("reReqMaxGap", 0, 1000, false, 10);

  if (ZuString partitions = cf->get("partitions"))
    updateLinks(partitions);

  core->addCmd(
      "subscriber.status", "",
      MxMDCmd::Fn::Member<&MxMDSubscriber::statusCmd>::fn(this),
      "subscriber status",
      "usage: subscriber.status\n");

  core->addCmd(
      "subscriber.resend", "",
      MxMDCmd::Fn::Member<&MxMDSubscriber::resendCmd>::fn(this),
      "manually test subscriber resend",
      "usage: subscriber.resend LINK SEQNO COUNT\n"
      "    LINK: link ID (determines server IP/port)\n"
      "    SEQNO: sequence number\n"
      "    COUNT: message count\n");
}

#define engineINFO(code) \
    appException(ZeEVENT(Info, \
      ([=](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDSubscriber::final()
{
  engineINFO("MxMDSubscriber::final()");
}

void MxMDSubscriber::updateLinks(ZuString partitions)
{
  MxMDPartitionCSV csv;
  csv.read(partitions, ZvCSVReadFn{[](MxMDSubscriber *sub, ZuAnyPOD *pod) {
      const MxMDPartition &partition = pod->as<MxMDPartition>();
      sub->m_partitions.del(partition.id);
      sub->m_partitions.add(partition);
      sub->updateLink(partition.id, nullptr);
    }, this});
}

ZmRef<MxAnyLink> MxMDSubscriber::createLink(MxID id)
{
  return new MxMDSubLink(id);
}

MxEngineApp::ProcessFn MxMDSubscriber::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *app, MxAnyLink *link, MxQMsg *msg) {
	  static_cast<MxMDSubscriber *>(app)->process_(
	      static_cast<MxMDSubLink *>(link), msg);
  });
}

void MxMDSubscriber::process_(MxMDSubLink *, MxQMsg *msg)
{
  using namespace MxMDStream;
  core()->apply(msg->as<Frame>(), m_filter);
}

#define linkINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=, id = id()](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDSubLink::update(ZvCf *)
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

void MxMDSubLink::reset(MxSeqNo rxSeqNo, MxSeqNo)
{
  rxRun([rxSeqNo](Rx *rx) { rx->rxReset(rxSeqNo); });
}

#define tcpERROR(tcp, io, code) \
  do { \
    MxMDSubLink *link = tcp->link(); \
    MxMDSubscriber *engine = link->engine(); \
    engine->appException(ZeEVENT(Error, \
      ([=, engineID = engine->id(), id = link->id()]( \
	const ZeEvent &, ZmStream &s) { \
	  s << "MxMDSubscriber(" << engineID << ':' << id << \
	  ") " << code; }))); \
    link->tcpError(tcp, io); \
  } while (0)

#define udpERROR(udp, io, code) \
  do { \
    MxMDSubLink *link = udp->link(); \
    MxMDSubscriber *engine = link->engine(); \
    engine->appException(ZeEVENT(Error, \
      ([=, engineID = engine->id(), id = link->id()]( \
	const ZeEvent &, ZmStream &s) { \
	  s << "MxMDSubscriber(" << engineID << ':' << id << \
	  ") " << code; }))); \
    link->udpError(udp, io); \
  } while (0)

void MxMDSubLink::tcpError(TCP *tcp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (tcp)
    tcp->close();
  Guard connGuard(m_connLock);
  if (!tcp || tcp == m_tcp) reconnect();
}

void MxMDSubLink::udpError(UDP *udp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (udp)
    udp->close();
  Guard connGuard(m_connLock);
  if (!udp || udp == m_udp) reconnect();
}

void MxMDSubLink::connect()
{
  linkINFO("MxMDSubLink::connect(" << id << ')');

  tcpConnect();
}

void MxMDSubLink::disconnect()
{
  linkINFO("MxMDSubLink::disconnect(" << id << ')');

  ZmRef<TCP> tcp;
  ZmRef<UDP> udp;

  {
    Guard connGuard(m_connLock);
    tcp = ZuMv(m_tcp);
    udp = ZuMv(m_udp);
    m_tcp = nullptr;
    m_udp = nullptr;
  }

  if (tcp) tcp->disconnect();
  if (udp) udp->disconnect();

  disconnected();
}

// TCP connect

void MxMDSubLink::tcpConnect()
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

  linkINFO("MxMDSubLink::tcpConnect(" << id << ") " <<
      ip << ':' << ZuBoxed(port) << ')');

  mx()->connect(
      ZiConnectFn([](MxMDSubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  // link state will not be Up until TCP+UDP has connected, login ackd
	  if (link->state() != MxLinkState::Connecting)
	    return 0;
	  return (uintptr_t)(new TCP(link, ci));
	}, this),
      ZiFailFn([](MxMDSubLink *link, bool transient) {
	  if (transient)
	    link->reconnect();
	  else
	    link->disconnected();
	}, this),
      ZiIP(), 0, ip, port);
}
MxMDSubLink::TCP::TCP(MxMDSubLink *link, const ZiCxnInfo &ci) :
  ZiConnection(link->mx(), ci), m_link(link), m_state(State::Login)
{
  using namespace MxMDStream;
  m_in = new MxQMsg(new Msg());
}
void MxMDSubLink::TCP::connected(ZiIOContext &io)
{
  m_link->tcpConnected(this);
  MxMDStream::TCP::recv<TCP>(m_in, io,
      [](TCP *tcp, MxQMsg *, ZiIOContext &io) { tcp->processLoginAck(io); });
}
void MxMDSubLink::tcpConnected(MxMDSubLink::TCP *tcp)
{
  linkINFO("MxMDSubLink::tcpConnected(" << id << ") " <<
      tcp->info().remoteIP << ':' << ZuBoxed(tcp->info().remotePort));

  {
    Guard connGuard(m_connLock);
    m_tcp = tcp;
  }

  mx()->rxRun(ZmFn<>{[](MxMDSubLink *link) { link->udpConnect(); }, this});
  // TCP sendLogin() is called once UDP is receiving/queuing
}

// TCP disconnect

void MxMDSubLink::TCP::disconnect_()
{
  Guard stateGuard(m_stateLock);
  m_state = State::Disconnect;
}
void MxMDSubLink::TCP::disconnect()
{
  disconnect_();
  ZiConnection::disconnect();
}
void MxMDSubLink::TCP::close()
{
  disconnect_();
  ZiConnection::close();
}
void MxMDSubLink::TCP::disconnected()
{
  mx()->del(&m_loginTimer);
  bool intentional;
  {
    Guard stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) tcpERROR(this, 0, "TCP disconnected");
}

// TCP login, recv

ZmRef<MxQMsg> MxMDSubLink::tcpLogin()
{
  using namespace MxMDStream;
  ZuRef<Msg> msg = new Msg();
  Frame *frame = new (msg->ptr()) Frame(
      (uint16_t)sizeof(Login), (uint16_t)Login::Code,
      (uint32_t)0, (uint64_t)0, (uint64_t)id());
  new (frame->ptr()) Login{m_partition->tcpUsername, m_partition->tcpPassword};
  unsigned msgLen = msg->length();
  return new MxQMsg(ZuMv(msg), msgLen);
}
void MxMDSubLink::TCP::sendLogin()
{
  MxMDStream::TCP::send(this, m_link->tcpLogin()); // bypass Tx queue
  mx()->rxRun(ZmFn<>{[](MxMDSubLink::TCP *tcp) {
      {
	Guard stateGuard(tcp->m_stateLock);
	if (tcp->m_state != State::Login) return;
      }
      tcpERROR(tcp, 0, "TCP login timeout");
    }, ZmMkRef(this)}, ZmTimeNow(m_link->loginTimeout()), &m_loginTimer);
}
void MxMDSubLink::TCP::processLoginAck(ZiIOContext &io)
{
  mx()->del(&m_loginTimer);
  {
    Guard stateGuard(m_stateLock);
    if (m_state != State::Login) {
      stateGuard.unlock();
      tcpERROR(this, &io, "TCP unexpected login ack");
      return;
    }
    m_state = State::Receiving;
  }
  m_link->tcpLoginAck();
  MxMDStream::TCP::recv<TCP>(m_in, io,
      [](TCP *tcp, MxQMsg *, ZiIOContext &io) { tcp->process(io); });
  process(io);
}
void MxMDSubLink::tcpLoginAck()
{
  linkINFO("MxMDSubLink::tcpLoginAck(" << id << ')');
  connected();
}
void MxMDSubLink::TCP::process(ZiIOContext &io)
{
  using namespace MxMDStream;
  const Frame &frame = m_in->as<Frame>();
  if (ZuUnlikely(frame.type == Type::EndOfSnapshot)) {
    {
      Guard stateGuard(m_stateLock);
      m_state = State::Disconnect;
    }
    io.disconnect();
    m_link->endOfSnapshot(frame.as<EndOfSnapshot>().seqNo);
    return;
  }
  m_link->tcpProcess(m_in);
}
void MxMDSubLink::tcpProcess(MxQMsg *msg)
{
  using namespace MxMDStream;
  core()->apply(msg->as<Frame>(), false);
}
void MxMDSubLink::endOfSnapshot(MxSeqNo seqNo)
{
  snapshotSeqNo(seqNo);
  rxInvoke([](MxMDSubLink::Rx *rx) {
      rx->stopQueuing(rx->app().snapshotSeqNo()); });
}

// UDP connect

void MxMDSubLink::udpConnect()
{
  rxInvoke([](MxMDSubLink::Rx *rx) { rx->startQueuing(); });

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
  if (ip.multicast()) {
    options.multicast(true);
    options.mreq(ZiMReq(ip, engine()->interface()));
  }
  mx()->udp(
      ZiConnectFn([](MxMDSubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  // link state will not be Up until TCP+UDP has connected, login ackd
	  if (link->state() != MxLinkState::Connecting)
	    return 0;
	  return (uintptr_t)(new UDP(link, ci));
	}, this),
      ZiFailFn([](MxMDSubLink *link, bool transient) {
	  if (transient)
	    link->reconnect();
	  else
	    link->disconnected();
	}, this),
      ZiIP(), port, ZiIP(), 0, options);
}
MxMDSubLink::UDP::UDP(MxMDSubLink *link, const ZiCxnInfo &ci) :
    ZiConnection(link->mx(), ci), m_link(link),
    m_state(State::Receiving) { }
void MxMDSubLink::UDP::connected(ZiIOContext &io)
{
  m_link->udpConnected(this, io);
}
void MxMDSubLink::udpConnected(MxMDSubLink::UDP *udp, ZiIOContext &io)
{
  linkINFO("MxMDSubLink::udpConnected(" << id << ')');

  ZmRef<TCP> tcp;

  {
    Guard connGuard(m_connLock);
    if (!(tcp = m_tcp)) { // paranoia
      connGuard.unlock();
      udp->disconnect();
      return;
    }
    m_udp = udp;
  }

  udp->recv(io); // begin receiving UDP packets

  linkINFO("MxMDSubLink::udpConnected(" << id << ") TCP sendLogin");

  tcp->sendLogin(); // login to TCP
}

// UDP disconnect

void MxMDSubLink::UDP::disconnect_()
{
  Guard stateGuard(m_stateLock);
  m_state = State::Disconnect;
}
void MxMDSubLink::UDP::disconnect()
{
  disconnect_();
  ZiConnection::disconnect();
}
void MxMDSubLink::UDP::close()
{
  disconnect_();
  ZiConnection::close();
}
void MxMDSubLink::UDP::disconnected()
{
  bool intentional;
  {
    Guard stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) udpERROR(this, 0, "UDP disconnected");
}

// UDP recv

void MxMDSubLink::UDP::recv(ZiIOContext &io)
{
  using namespace MxMDStream;
  ZmRef<MxQMsg> msg = new MxQMsg(new Msg());
  MxMDStream::UDP::recv<UDP>(ZuMv(msg), io,
      [](UDP *udp, ZmRef<MxQMsg> msg, ZiIOContext &io) {
	udp->process(ZuMv(msg), io);
      });
}
void MxMDSubLink::UDP::process(MxQMsg *msg, ZiIOContext &io)
{
  using namespace MxMDStream;
  msg->length = io.offset + io.length;
  const Frame &frame = msg->as<Frame>();
  if (ZuUnlikely(frame.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message", msg->ptr(), msg->length};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDSubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->addr = io.addr;
    m_link->udpReceived(msg);
  }
  recv(io);
}
void MxMDSubLink::udpReceived(MxQMsg *msg)
{
  if (ZuUnlikely(
	msg->addr.ip() == m_partition->resendIP ||
	msg->addr.ip() == m_partition->resendIP2)) {
    Guard guard(m_resendLock);
    unsigned gapLength = m_resendGap.length();
    if (ZuUnlikely(gapLength)) {
      uint64_t seqNo = msg->as<MxMDFrame>().seqNo;
      uint64_t gapSeqNo = m_resendGap.key();
      if (seqNo >= gapSeqNo && seqNo < gapSeqNo + gapLength) {
	m_resendMsg = msg;
	guard.unlock();
	m_resendSem.post();
	return;
      }
    }
  }
  rxInvoke([msg = ZmMkRef(msg)](Rx *rx) mutable { rx->received(ZuMv(msg)); });
}
void MxMDSubLink::request(const MxQueue::Gap &, const MxQueue::Gap &now)
{
  reRequest(now);
}
void MxMDSubLink::reRequest(const MxQueue::Gap &now)
{
  if (now.length() > engine()->reReqMaxGap()) {
    reconnect();
    return;
  }
  using namespace MxMDStream;
  ZuRef<Msg> msg = new Msg();
  Frame *frame = new (msg->ptr()) Frame(
      (uint16_t)sizeof(ResendReq), (uint16_t)ResendReq::Code,
      (uint32_t)0, (uint64_t)0, (uint64_t)id());
  new (frame->ptr()) ResendReq{now.key(), now.length()};
  unsigned msgLen = msg->length();
  ZmRef<MxQMsg> qmsg = new MxQMsg(ZuMv(msg), msgLen);
  ZmRef<UDP> udp;
  {
    Guard connGuard(m_connLock);
    udp = m_udp;
  }
  if (ZuLikely(udp))
    MxMDStream::UDP::send(udp.ptr(), ZuMv(qmsg), m_udpResendAddr);
}

// commands

void MxMDSubscriber::statusCmd(
    const MxMDCmd::Args &args, ZtArray<char> &out)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc != 1) throw MxMDCmd::Usage();
  out.size(512 * nLinks());
  out << "State: " << MxEngineState::name(state()) << '\n';
  allLinks<MxMDSubLink>([&out](MxMDSubLink *link) -> uintptr_t {
	out << '\n'; link->status(out); return 0; });
}

void MxMDSubLink::status(ZtArray<char> &out)
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

void MxMDSubscriber::resendCmd(
    const MxMDCmd::Args &args, ZtArray<char> &out)
{
  using namespace MxMDStream;
  ZuBox<int> argc(args.get("#"));
  if (argc != 4) throw MxMDCmd::Usage();
  auto id = args.get("1");
  ZmRef<MxAnyLink> link_ = link(id);
  if (!link_) throw ZtString() << id << " - unknown link";
  auto link = static_cast<MxMDSubLink *>(link_.ptr());
  ZuBox<uint64_t> seqNo(args.get("2"));
  ZuBox<uint16_t> count(args.get("3"));
  if (!*seqNo || !*count) throw MxMDCmd::Usage();
  ZmRef<MxQMsg> msg = link->resend(seqNo, count);
  if (!msg) throw ZtString("timed out");
  seqNo = msg->as<MxMDFrame>().seqNo;
  out << "seqNo: " << seqNo << '\n';
  const Frame &frame = msg->as<Frame>();
  out << ZtHexDump(
      ZtString() << "type: " << Type::name(frame.type),
      msg->ptr(), msg->length) << '\n';
}

ZmRef<MxQMsg> MxMDSubLink::resend(MxSeqNo seqNo, unsigned count)
{
  using namespace MxMDStream;
  MxQueue::Gap gap{seqNo, count};
  {
    Guard guard(m_resendLock);
    m_resendGap = gap;
  }
  reRequest(gap);
  if (m_resendSem.timedwait(ZmTimeNow() + engine()->reReqInterval()))
    return 0;
  ZmRef<MxQMsg> msg;
  {
    Guard guard(m_resendLock);
    m_resendGap = MxQueue::Gap();
    msg = ZuMv(m_resendMsg);
  }
  return msg;
}
