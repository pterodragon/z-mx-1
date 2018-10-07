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

void MxMDSubscriber::init(MxMDCore *core)
{
  ZmRef<ZvCf> cf = core->cf()->subset("subscriber", false, true);

  if (!cf->get("id")) cf->set("id", "subscriber");

  MxEngine::init(core, this, core->mx(), cf);

  if (ZuString ip = cf->get("interface")) m_interface = ip;
  m_maxQueueSize = cf->getInt("maxQueueSize", 1000, 1000000, false, 100000);
  m_loginTimeout = cf->getDbl("loginTimeout", 0, 3600, false, 10);
  m_reconnInterval = cf->getDbl("reconnInterval", 0, 3600, false, 10);
  m_reReqInterval = cf->getDbl("reReqInterval", 0, 3600, false, 1);
  m_reReqMaxGap = cf->getInt("reReqMaxGap", 0, 1000, false, 10);

  if (ZuString partitions = cf->get("partitions")) {
    MxMDPartitionCSV csv;
    csv.read(partitions, [](MxMDSubscriber *sub, ZuAnyPOD *pod) {
	sub->m_partitions.add(pod->as<MxMDPartition>());
	sub->MxEngine::updateLink<MxMDSubLink>(
	    pod->as<MxMDPartition>().id, nullptr);
      }, this);
  }

  core->addCmd(
      "subscriber.status", "",
      MxMDCmd::CmdFn::Member<&MxITCHFeed::statusCmd>::fn(this),
      "subscriber status",
      "usage: subscriber.status\n");

  core->addCmd(
      "subscriber.resend", "",
      MxMDCmd::CmdFn::Member<&MxITCHFeed::resendCmd>::fn(this),
      "manually test subscriber resend",
      "usage: subscriber.resend LINK SEQNO COUNT\n"
      "    LINK: link ID (determines server IP/port)\n"
      "    SEQNO: sequence number\n"
      "    COUNT: message count\n");
}

#define engineINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDSubscriber::final()
{
  engineINFO("MxMDSubscriber::final()");
}

void MxMDSubscriber::up()
{
  engineINFO("MxMDSubscriber::up()");
}

void MxMDSubscriber::down()
{
  engineINFO("MxMDSubscriber::down()");
}

void MxMDSubscriber::process_(MxMDSubLink *link, MxQMsg *msg)
{
  const Frame &frame = m_in->as<Frame>();
  core()->apply(&frame);
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
  ZmGuard<ZmLock> connGuard(m_connLock);
  if (io)
    io->disconnect();
  else if (tcp)
    tcp->close();
  if (!tcp || tcp == m_tcp) reconnect();
}

void MxMDSubLink::udpError(UDP *udp, ZiIOContext *io)
{
  ZmGuard<ZmLock> connGuard(m_connLock);
  if (io)
    io->disconnect();
  else if (udp)
    udp->close();
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
    ZmGuard<ZmLock> connGuard(m_connLock);
    tcp = ZuMv(m_tcp);
    udp = ZuMv(m_udp);
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
      ZiConnectFn([](MxMDSubLink *link, const ZiCxnInfo &ci) {
	  if (link->stateLocked([](MxMDSubLink *, int state) -> bool {
		return state != MxLinkState::Connecting; }))
	    return 0;
	  return new TCP(link, ci);
	}, this),
      ZiFailFn([](MxMDSubLink *link, bool transient) {
	  if (transient) link->reconnect();
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
  m_in->fn = MxQMsgFn([](MxMDSubLink::TCP *tcp, MxQMsg *, ZiIOContext *io) {
      tcp->processLoginAck(*io); }, this);
  MxMDStream::TCP::recv(m_in, io);
}
void MxMDSubLink::tcpConnected(MxMDSubLink::TCP *tcp)
{
  linkINFO("MxMDSubLink::tcpConnected(" << id << ") " <<
      tcp->info().remoteIP << ':' << ZuBoxed(tcp->info().remotePort));

  {
    ZmGuard<ZmLock> connGuard(m_connLock);
    m_tcp = tcp;
  }

  mx()->add([](MxMDSubLink *link) { link->udpConnect(); }, this);
  // TCP sendLogin() is called once UDP is receiving/queuing
}

// TCP disconnect

void MxMDSubLink::TCP::disconnect_()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);
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
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) tcpERROR(this, 0, "TCP disconnected");
}

// TCP login, recv

void MxMDSubLink::TCP::sendLogin()
{
  using namespace MxMDStream;
  ZmRef<Msg> msg = new Msg();
  new (msg->out<Login>(id(), 1, ZmTime()))
    Login{m_partition->tcpUsername, m_partition->tcpPassword};
  ZmRef<MxQMsg> qmsg = new MxQMsg(msg, MxQMsgFn(), msg->length());
  MxMDStream::TCP::send(qmsg, this); // bypass Tx queue
  mx()->add(ZmFn<>([](MxMDSubLink::TCP *tcp) {
      {
	ZmGuard<ZmLock> stateGuard(m_stateLock);
	if (m_state != State::Login) return;
      }
      tcpERROR(tcp, 0, "TCP login timeout");
    }, this), ZmTimeNow(m_link->loginTimeout()), &m_loginTimer);
}
void MxMDSubLink::TCP::processLoginAck(ZiIOContext &io)
{
  mx()->del(&m_loginTimer);
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    if (m_state != State::Login) {
      stateGuard.unlock();
      tcpERROR(this, &io, "TCP unexpected login ack");
      return;
    }
    m_state = State::Receiving;
  }
  m_link->tcpLoginAck();
  m_in->fn = MxQMsgFn([](MxMDSubLink::TCP *tcp, MxQMsg *, ZiIOContext *io) {
      tcp->process(*io); }, this);
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
      ZmGuard<ZmLock> stateGuard(m_stateLock);
      m_state = State::Disconnect;
    }
    io.disconnect();
    m_link->endOfSnapshot(frame.as<EndOfSnapshot>().seqNo);
    return;
  }
  engine()->process_(this, m_in);
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
  options.multicast(true);
  options.mreq(ZiMReq(ip, engine()->interface()));
  mx()->udp(
      ZiConnectFn([](MxMDSubLink *link, const ZiCxnInfo &ci) {
	  if (link->stateLocked([](MxMDSubLink *, int state) -> bool {
		return state != MxLinkState::Connecting; }))
	    return 0;
	  return new UDP(link, ci);
	}, this),
      ZiFailFn([](MxMDSubLink *link, bool transient) {
	  if (transient) link->reconnect();
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
    ZmGuard<ZmLock> connGuard(m_connLock);
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
  ZmGuard<ZmLock> stateGuard(m_stateLock);
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
    ZmGuard<ZmLock> stateGuard(m_stateLock);
    intentional = m_state == State::Disconnect;
  }
  if (!intentional) udpERROR(this, 0, "UDP disconnected");
}

// UDP recv

void MxMDSubLink::UDP::recv(ZiIOContext &io)
{
  using namespace MxMDStream;
  ZmRef<MxQMsg> msg = new MxQMsg(new Msg(),
      MxQMsgFn([](MxMDSubLink::UDP *udp, MxQMsg *msg, ZiIOContext *io) {
	  udp->process(msg, io); }, this));
  MxMDStream::UDP::recv(msg, io);
}
void MxMDSubLink::UDP::process(MxQMsg *msg, ZiIOContext *io)
{
  using namespace MxMDStream;
  msg->length = io->offset + io->length;
  const Frame &frame = msg->as<Frame>();
  if (ZuUnlikely(frame.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message", msg->data(), msg->length()};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDSubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->addr = io->addr;
    m_link->udpReceived(msg);
  }
  recv(*io);
}
void MxMDSubLink::udpReceived(MxQMsg *msg)
{
  if (ZuUnlikely(
	msg->addr.ip() == m_partition->resendIP ||
	msg->addr.ip() == m_partition->resendIP2)) {
    ZmGuard<ZmLock> guard(m_resendLock);
    if (ZuUnlikely(*m_resendSeqNo)) {
      uint64_t seqNo = msg->seqNo();
      if (seqNo >= m_resendSeqNo && seqNo < m_resendSeqNo + m_resendCount) {
	m_resendMsg = msg;
	guard.unlock();
	m_resendSem.post();
	return;
      }
    }
  }
  msg->fn = MxQMsgFn([](MxMDSubLink *link, MxQMsg *msg, ZiIOContext *) {
	link->rx()->received(msg); }, this);
  engine()->rxRun([](MxQMsg *msg) { msg->fn(msg, nullptr); }, ZmMkRef(msg));
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
  ZmRef<Msg> msg = new Msg();
  new (msg->out<ResendReq>(id(), 0, ZmTime()))
    ResendReq{now.key(), now.length()};
  msg->addr = m_udpResendAddr;
  ZmRef<MxQMsg> qmsg = new MxQMsg(msg, MxQMsgFn(), msg->length());
  ZmRef<UDP> udp;
  {
    ZmGuard<ZmLock> connGuard(m_connLock);
    udp = m_udp;
  }
  if (ZuLikely(udp)) MxMDStream::UDP::send(qmsg, udp);
}

// commands

void MxMDSubscriber::statusCmd(const MxMDCmd::CmdArgs &args, ZtArray<char> &out)
{
  int argc = ZuBox<int>(args.get("#"));
  if (argc != 1) throw MxMDCmd::CmdUsage();
  out.size(512 * nLinks());
  out << "State: " << MxEngineState::name(state()) << '\n';
  allLinks([&out](MxAnyLink *link) {
	out << '\n';
	static_cast<MxMDSubLink *>(link)->status(out);
      });
}

void MxMDSubLink::status(ZtArray<char> &out)
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

void MxMDSubscriber::resendCmd(const MxMDCmd::CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc(args.get("#"));
  if (argc != 4) throw MxMDCmd::CmdUsage();
  auto id = args.get("1");
  ZmRef<MxAnyLink> link_ = link(id);
  if (!link_) throw ZtString() << id << " - unknown link";
  auto link = static_cast<MxMDSubLink *>(link_.ptr());
  ZuBox<uint64_t> seqNo(args.get("2"));
  ZuBox<uint16_t> count(args.get("3"));
  if (!*seqNo || !*count) throw MxMDCmd::CmdUsage();
  ZmRef<MxQMsg> msg = link->resend(seqNo, count);
  if (!msg) throw ZtString("timed out");
  seqNo = msg->seqNo();
  out << "seqNo: " << msg->id.seqNo << '\n';
  const Frame &frame = msg->as<Frame>();
  out << ZtHexDump(
      ZtString() << "type: " << Type::name(frame.type),
      msg->ptr(), msg->length) << '\n';
}

ZmRef<MxQMsg> MxSubLink::resend(MxSeqNo seqNo, unsigned count);
{
  using namespace MxMDStream;
  MxQueue::Gap gap{seqNo, count};
  {
    ZmGuard<ZmLock> guard(m_resendLock);
    m_resendGap = gap;
  }
  reRequest(gap);
  if (m_resendSem.timedwait(ZmTimeNow() + engine()->ReRequestInterval()))
    return 0;
  ZmRef<MxQMsg> msg;
  {
    ZmGuard<ZmLock> guard(m_resendLock);
    m_resendGap = MxQueue::Gap();
    msg = ZuMv(m_resendMsg);
  }
  return msg;
}
