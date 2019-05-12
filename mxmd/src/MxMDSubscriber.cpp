//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1

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

  Mx *mx = core->mx(cf->get("mx", false, "core"));

  if (!mx) throw ZvCf::Required("subscrib:mx");

  MxEngine::init(core, this, mx, cf);

  if (ZuString ip = cf->get("interface")) m_interface = ip;
  m_filter = cf->getInt("filter", 0, 1, false, 0);
  m_maxQueueSize = cf->getInt("maxQueueSize", 1000, 1000000, false, 100000);
  m_loginTimeout = cf->getDbl("loginTimeout", 0, 3600, false, 3);
  m_timeout = cf->getDbl("timeout", 0, 3600, false, 3);
  m_reconnInterval = cf->getDbl("reconnInterval", 0, 3600, false, 10);
  m_reReqInterval = cf->getDbl("reReqInterval", 0, 3600, false, 1);
  m_reReqMaxGap = cf->getInt("reReqMaxGap", 0, 1000, false, 10);

  if (ZuString channels = cf->get("channels"))
    updateLinks(channels);

  core->addCmd(
      "subscriber.status", "",
      ZvCmdFn::Member<&MxMDSubscriber::statusCmd>::fn(this),
      "subscriber status",
      "usage: subscriber.status\n");

  core->addCmd(
      "subscriber.resend", "",
      ZvCmdFn::Member<&MxMDSubscriber::resendCmd>::fn(this),
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

void MxMDSubscriber::updateLinks(ZuString channels)
{
  MxMDChannelCSV csv;
  csv.read(channels, ZvCSVReadFn{this, [](MxMDSubscriber *sub, ZuAnyPOD *pod) {
      const MxMDChannel &channel = pod->as<MxMDChannel>();
      sub->m_channels.del(channel.id);
      sub->m_channels.add(channel);
      sub->updateLink(channel.id, nullptr);
    }});
}

ZmRef<MxAnyLink> MxMDSubscriber::createLink(MxID id)
{
  return new MxMDSubLink(id);
}

#define linkINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=, id = id()](const ZeEvent &, ZmStream &out) { out << code; })))
#define linkWARNING(code) \
    engine()->appException(ZeEVENT(Warning, \
      ([=, id = id()](const ZeEvent &, ZmStream &out) { out << code; })))

void MxMDSubLink::update(ZvCf *)
{
  engine()->channel(id(), [this](MxMDChannel *channel) {
      if (ZuUnlikely(!channel)) throw ZvCf::Required(id());
      m_channel = channel;
    });
  if (m_channel && m_channel->enabled)
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
	  s << "MxMDSubscriber{" << engineID << ':' << id << \
	  "} " << code; }))); \
    link->tcpError(tcp, io); \
  } while (0)

#define udpERROR(udp, io, code) \
  do { \
    MxMDSubLink *link = udp->link(); \
    MxMDSubscriber *engine = link->engine(); \
    engine->appException(ZeEVENT(Error, \
      ([=, engineID = engine->id(), id = link->id()]( \
	const ZeEvent &, ZmStream &s) { \
	  s << "MxMDSubscriber{" << engineID << ':' << id << \
	  "} " << code; }))); \
    link->udpError(udp, io); \
  } while (0)

void MxMDSubLink::tcpError(TCP *tcp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (tcp)
    tcp->close();

  if (!tcp)
    reconnect(false);
  else
    engine()->rxInvoke(ZmMkRef(tcp), [](ZmRef<TCP> tcp) {
      tcp->link()->tcpDisconnected(tcp);
    });
}

void MxMDSubLink::udpError(UDP *udp, ZiIOContext *io)
{
  if (io)
    io->disconnect();
  else if (udp)
    udp->close();

  if (!udp)
    reconnect(false);
  else
    engine()->rxInvoke(ZmMkRef(udp), [](ZmRef<UDP> udp) {
      udp->link()->udpDisconnected(udp);
    });
}

void MxMDSubLink::connect()
{
  linkINFO("MxMDSubLink::connect(" << id << ')');

  reset(0, 0);

  tcpConnect();
}

void MxMDSubLink::disconnect()
{
  linkINFO("MxMDSubLink::disconnect(" << id << ')');

  m_reconnect = false;
  disconnect_1();
}

void MxMDSubLink::reconnect(bool immediate)
{
  linkINFO("MxMDSubLink::reconnect(" << id << ')');

  engine()->rxInvoke([this, immediate]() { reconnect_(immediate); });
}

void MxMDSubLink::reconnect_(bool immediate)
{
  m_reconnect = true;
  m_immediate = immediate;
  disconnect_1();
}

void MxMDSubLink::disconnect_1()
{
  mx()->del(&m_timer);

  m_active = false;
  m_inactive = 0;

  if (m_tcp) { m_tcp->disconnect(); m_tcp = nullptr; }
  if (m_udp) { m_udp->disconnect(); m_udp = nullptr; }

  if (m_reconnect) {
    m_reconnect = false;
    MxLink::reconnect(m_immediate);
  } else
    disconnected();
}

// TCP connect

void MxMDSubLink::tcpConnect()
{
  ZiIP ip;
  uint16_t port;
  if (!m_failover) {
    ip = m_channel->tcpIP;
    port = m_channel->tcpPort;
  } else {
    if (!(ip = m_channel->tcpIP2)) ip = m_channel->tcpIP;
    if (!(port = m_channel->tcpPort2)) port = m_channel->tcpPort;
  }

  linkINFO("MxMDSubLink::tcpConnect(" << id << ") " <<
      ip << ':' << ZuBoxed(port) << ')');

  mx()->connect(
      ZiConnectFn(this,
	[](MxMDSubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  // link state will not be Up until TCP+UDP has connected, login ackd
	  switch ((int)link->state()) {
	    case MxLinkState::Connecting:
	    case MxLinkState::Reconnecting:
	      return (uintptr_t)(new TCP(link, ci));
	    case MxLinkState::DisconnectPending:
	      link->connected();
	    default:
	      return 0;
	  }
	}),
      ZiFailFn(this, [](MxMDSubLink *link, bool transient) {
	  if (transient)
	    link->reconnect(false);
	  else
	    link->engine()->rxRun(
		ZmFn<>{link, [](MxMDSubLink *link) { link->disconnect(); }});
	}),
      ZiIP(), 0, ip, port);
}
MxMDSubLink::TCP::TCP(MxMDSubLink *link, const ZiCxnInfo &ci) :
  ZiConnection(link->mx(), ci), m_link(link), m_state(State::Login)
{
}
void MxMDSubLink::TCP::connected(ZiIOContext &io)
{
  m_link->engine()->rxRun(ZmFn<>{ZmMkRef(this), [](TCP *tcp) {
	tcp->link()->tcpConnected(tcp);
      }});
  MxMDStream::TCP::recv<TCP>(new MxQMsg(new Msg()), io,
      [](TCP *tcp, ZmRef<MxQMsg> msg, ZiIOContext &io) {
	tcp->processLoginAck(ZuMv(msg), io);
      });
}
void MxMDSubLink::tcpConnected(MxMDSubLink::TCP *tcp)
{
  linkINFO("MxMDSubLink::tcpConnected(" << id << ") " <<
      tcp->info().remoteIP << ':' << ZuBoxed(tcp->info().remotePort));

  if (ZuUnlikely(m_tcp)) m_tcp->disconnect();
  m_tcp = tcp;

  udpConnect();
  // TCP sendLogin() is called once UDP is receiving/queuing
}

// TCP disconnect

void MxMDSubLink::TCP::disconnect()
{
  m_state = State::Disconnect;
  ZiConnection::disconnect();
}
void MxMDSubLink::TCP::close()
{
  m_state = State::Disconnect;
  ZiConnection::close();
}
void MxMDSubLink::TCP::disconnected()
{
  mx()->del(&m_loginTimer);
  if (m_state != State::Disconnect) tcpERROR(this, 0, "TCP disconnected");
}
void MxMDSubLink::tcpDisconnected(TCP *tcp)
{
  if (tcp == m_tcp) reconnect(false);
}

// TCP login, recv

ZmRef<MxQMsg> MxMDSubLink::tcpLogin()
{
  using namespace MxMDStream;
  ZuRef<Msg> msg = new Msg();
  Hdr *hdr = new (msg->ptr()) Hdr{
      (uint64_t)0, (uint32_t)0,
      (uint16_t)sizeof(Login), (uint8_t)Login::Code};
  new (hdr->body()) Login{m_channel->tcpUsername, m_channel->tcpPassword};
  unsigned msgLen = msg->length();
  return new MxQMsg(ZuMv(msg), msgLen);
}
void MxMDSubLink::TCP::sendLogin()
{
  MxMDStream::TCP::send(this, m_link->tcpLogin()); // bypass Tx queue
  mx()->rxRun(ZmFn<>{ZmMkRef(this), [](TCP *tcp) {
      if (tcp->state() != State::Login) return;
      tcpERROR(tcp, 0, "TCP login timeout");
    }}, ZmTimeNow(m_link->loginTimeout()), &m_loginTimer);
}
void MxMDSubLink::tcpLoginAck()
{
  linkINFO("MxMDSubLink::tcpLoginAck(" << id << ')');
  connected();
  hbStart();
}
void MxMDSubLink::TCP::processLoginAck(ZmRef<MxQMsg> msg, ZiIOContext &io)
{
  if (ZuUnlikely(m_state.load_() != State::Login)) {
    tcpERROR(this, &io, "TCP FSM internal error");
    return;
  }

  m_state = State::Receiving;
  mx()->del(&m_loginTimer);
  m_link->tcpLoginAck();

  if (endOfSnapshot(msg, io)) return;

  m_link->tcpProcess(msg);

  MxMDStream::TCP::recv<TCP>(ZuMv(msg), io,
      [](TCP *tcp, ZmRef<MxQMsg> msg, ZiIOContext &io) {
	tcp->process(ZuMv(msg), io);
      });
}
void MxMDSubLink::TCP::process(ZmRef<MxQMsg> msg, ZiIOContext &io)
{
  if (ZuUnlikely(m_state.load_() == State::Login)) {
    tcpERROR(this, &io, "TCP FSM internal error");
    return;
  }

  if (endOfSnapshot(msg, io)) return;

  m_link->tcpProcess(msg);

  io.fn.object(ZuMv(msg)); // recycle
}
void MxMDSubLink::tcpProcess(MxQMsg *msg)
{
  using namespace MxMDStream;
  core()->apply(msg->ptr<Msg>()->as<Hdr>(), false);
}
bool MxMDSubLink::TCP::endOfSnapshot(MxQMsg *msg, ZiIOContext &io)
{
  using namespace MxMDStream;

  const Hdr &hdr = msg->ptr<Msg>()->as<Hdr>();
  if (ZuUnlikely(hdr.type == Type::EndOfSnapshot)) {
    m_state = State::Disconnect;
    io.disconnect();
    m_link->endOfSnapshot(hdr.as<EndOfSnapshot>().seqNo);
    return true;
  }
  return false;
}
void MxMDSubLink::endOfSnapshot(MxSeqNo seqNo)
{
  rxInvoke([seqNo](MxMDSubLink::Rx *rx) { rx->stopQueuing(seqNo); });
}

// UDP connect

void MxMDSubLink::udpConnect()
{
  rxInvoke([](Rx *rx) { rx->startQueuing(); });

  ZiIP ip;
  uint16_t port;
  ZiIP resendIP;
  uint16_t resendPort;
  if (!m_failover) {
    ip = m_channel->udpIP;
    port = m_channel->udpPort;
    resendIP = m_channel->resendIP;
    resendPort = m_channel->resendPort;
  } else {
    if (!(ip = m_channel->udpIP2)) ip = m_channel->udpIP;
    if (!(port = m_channel->udpPort2)) port = m_channel->udpPort;
    if (!(resendIP = m_channel->resendIP2))
      resendIP = m_channel->resendIP;
    if (!(resendPort = m_channel->resendPort2))
      resendPort = m_channel->resendPort;
  }
  m_udpResendAddr = ZiSockAddr(resendIP, resendPort);
  ZiCxnOptions options;
  options.udp(true);
  if (ip.multicast()) {
    options.multicast(true);
    options.mreq(ZiMReq(ip, engine()->interface_()));
  }
  mx()->udp(
      ZiConnectFn(this,
	[](MxMDSubLink *link, const ZiCxnInfo &ci) -> uintptr_t {
	  // link state will not be Up until TCP+UDP has connected, login ackd
	  switch ((int)link->state()) {
	    case MxLinkState::Connecting:
	    case MxLinkState::Reconnecting:
	      return (uintptr_t)(new UDP(link, ci));
	    case MxLinkState::DisconnectPending:
	      link->connected();
	    default:
	      return 0;
	  }
	}),
      ZiFailFn(this, [](MxMDSubLink *link, bool transient) {
	  if (transient)
	    link->reconnect(false);
	  else
	    link->engine()->rxRun(
		ZmFn<>{link, [](MxMDSubLink *link) { link->disconnect(); }});
	}),
      ZiIP(), port, ZiIP(), 0, options);
}
MxMDSubLink::UDP::UDP(MxMDSubLink *link, const ZiCxnInfo &ci) :
    ZiConnection(link->mx(), ci), m_link(link),
    m_state(State::Receiving) { }
void MxMDSubLink::UDP::connected(ZiIOContext &io)
{
  m_link->engine()->rxRun(ZmFn<>{ZmMkRef(this), [](UDP *udp) {
    udp->link()->udpConnected(udp);
  }});
  recv(io); // begin receiving UDP packets
}
void MxMDSubLink::udpConnected(MxMDSubLink::UDP *udp)
{
  linkINFO("MxMDSubLink::udpConnected(" << id << ')');

  if (ZuUnlikely(!m_tcp)) { udp->disconnect(); return; }

  if (ZuUnlikely(m_udp)) m_udp->disconnect();
  m_udp = udp;

  linkINFO("MxMDSubLink::udpConnected(" << id << ") TCP sendLogin");

  m_tcp->sendLogin(); // login to TCP
}

// UDP disconnect

void MxMDSubLink::UDP::disconnect()
{
  m_state = State::Disconnect;
  ZiConnection::disconnect();
}
void MxMDSubLink::UDP::close()
{
  m_state = State::Disconnect;
  ZiConnection::close();
}
void MxMDSubLink::UDP::disconnected()
{
  if (m_state != State::Disconnect) udpERROR(this, 0, "UDP disconnected");
}
void MxMDSubLink::udpDisconnected(UDP *udp)
{
  if (udp == m_udp) reconnect(false);
}

// UDP recv

void MxMDSubLink::UDP::recv(ZiIOContext &io)
{
  using namespace MxMDStream;
  ZmRef<MxQMsg> msg = new MxQMsg(new Msg());
  MxMDStream::UDP::recv<UDP>(ZuMv(msg), io,
      [](UDP *udp, ZmRef<MxQMsg> msg, ZiIOContext &io) mutable {
	udp->process(ZuMv(msg), io);
      });
}
void MxMDSubLink::UDP::process(ZmRef<MxQMsg> msg, ZiIOContext &io)
{
  using namespace MxMDStream;
  const Hdr &hdr = msg->ptr<Msg>()->as<Hdr>();
  if (ZuUnlikely(hdr.scan(msg->length))) {
    ZtHexDump msg_{"truncated UDP message",
      msg->ptr<Msg>()->ptr(), msg->length};
    m_link->engine()->appException(ZeEVENT(Warning,
      ([=, id = m_link->id(), msg_ = ZuMv(msg_)](
	const ZeEvent &, ZmStream &out) {
	  out << "MxMDSubLink::UDP::process() link " << id << ' ' << msg_;
	})));
  } else {
    msg->id.linkID = m_link->id();
    msg->id.seqNo = hdr.seqNo;
    m_link->udpReceived(ZuMv(msg));
  }
  recv(io);
}
void MxMDSubLink::udpReceived(ZmRef<MxQMsg> msg)
{
  using namespace MxMDStream;
  {
    ZiIP ip = msg->ptr<Msg>()->addr.ip();
    if (ZuUnlikely(ip == m_channel->resendIP || ip == m_channel->resendIP2)) {
      Guard guard(m_resendLock);
      unsigned gapLength = m_resendGap.length();
      if (ZuUnlikely(gapLength)) {
	uint64_t seqNo = msg->ptr<Msg>()->as<Hdr>().seqNo;
	uint64_t gapSeqNo = m_resendGap.key();
	if (seqNo >= gapSeqNo && seqNo < gapSeqNo + gapLength) {
	  m_resendMsg = msg;
	  guard.unlock();
	  m_resendSem.post();
	  return;
	}
      }
    }
  }
  received(ZuMv(msg), [](Rx *rx) {
    auto link = rx->impl();
    link->active();
    if (ZuUnlikely(rx->rxQueue()->count() > link->engine()->maxQueueSize())) {
      link->rxQueueTooBig(
	  rx->rxQueue()->count(),
	  link->engine()->maxQueueSize());
      link->reconnect(true);
    }
  });
}
void MxMDSubLink::rxQueueTooBig(uint32_t count, uint32_t max)
{
  linkWARNING("MxMDSubLink::udpReceived(" << id <<
      "): Rx queue too large (" <<
      ZuBoxed(count) << " > " << ZuBoxed(max) << ')');
}
void MxMDSubLink::request(const MxQueue::Gap &, const MxQueue::Gap &now)
{
  reRequest(now);
}
void MxMDSubLink::reRequest(const MxQueue::Gap &now)
{
  if (now.length() > engine()->reReqMaxGap()) {
    uint32_t len = now.length();
    uint32_t max = engine()->reReqMaxGap();
    linkWARNING("MxMDSubLink::reRequest(" << id <<
	"): too many missing messages (" <<
	ZuBoxed(len) << " > " << ZuBoxed(max) << ')');
    reconnect(true);
    return;
  }
  using namespace MxMDStream;
  ZuRef<Msg> msg = new Msg();
  Hdr *hdr = new (msg->ptr()) Hdr{
      (uint64_t)0, (uint32_t)0,
      (uint16_t)sizeof(ResendReq), (uint8_t)ResendReq::Code};
  new (hdr->body()) ResendReq{now.key(), now.length()};
  unsigned msgLen = msg->length();
  ZmRef<MxQMsg> qmsg = new MxQMsg(ZuMv(msg), msgLen);
  if (ZuLikely(m_udp))
    MxMDStream::UDP::send(m_udp.ptr(), ZuMv(qmsg), m_udpResendAddr);
}

// Rx

void MxMDSubLink::process(MxQMsg *msg)
{
  using namespace MxMDStream;
  const Hdr &hdr = msg->ptr<Msg>()->as<Hdr>();
  if (hdr.type == Type::HeartBeat) {
    lastTime(hdr.as<HeartBeat>().stamp.zmTime());
    return;
  }
#if 0
  ZmTime latency = ZmTimeNow() - (lastTime() + ZmTime(ZmTim::Nano, hdr.nsec));
#endif
  engine()->process(msg);
}

void MxMDSubscriber::process(MxQMsg *msg)
{
  using namespace MxMDStream;
  core()->apply(msg->ptr<Msg>()->as<Hdr>(), m_filter);
}

// failover

void MxMDSubLink::hbStart()
{
  m_active = false;
  m_inactive = 0;
  engine()->rxRun(ZmFn<>{this, [](MxMDSubLink *link) { link->heartbeat(); }},
      ZmTimeNow(1), &m_timer);
}

void MxMDSubLink::heartbeat()
{
  if (!m_active) {
    if (++m_inactive >= (unsigned)timeout()) {
      m_inactive = 0;
      linkWARNING("MxMDSubLink::heartbeat(" << id << "): inactivity timeout");
      reconnect(true);
      return;
    }
  } else {
    m_active = false;
    m_inactive = 0;
  }
  engine()->rxRun(ZmFn<>{this, [](MxMDSubLink *link) { link->heartbeat(); }},
      ZmTimeNow(1), &m_timer);
}

// commands

void MxMDSubscriber::statusCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  int argc = ZuBox<int>(args->get("#"));
  if (argc != 1) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  out.size(512 * nLinks());
  out << "State: " << MxEngineState::name(state()) << '\n';
  allLinks<MxMDSubLink>([&out](MxMDSubLink *link) -> uintptr_t {
	out << '\n'; link->status(out); return 0; });
}

void MxMDSubLink::status(ZtString &out)
{
  out << "Link " << id() << ":\n";
  out << "  TCP:    " <<
    m_channel->tcpIP << ':' << m_channel->tcpPort << " | " <<
    m_channel->tcpIP2 << ':' << m_channel->tcpPort2 << '\n';
  out << "  UDP:    " <<
    m_channel->udpIP << ':' << m_channel->udpPort << " | " <<
    m_channel->udpIP2 << ':' << m_channel->udpPort2 << '\n';
  out << "  Resend: " <<
    m_channel->resendIP << ':' << m_channel->resendPort << " | " <<
    m_channel->resendIP2 << ':' << m_channel->resendPort2 << '\n';
  out << "  TCP Username: " << m_channel->tcpUsername <<
    " Password: " << m_channel->tcpPassword << '\n';

  {
    MxAnyLink::Telemetry data;
    telemetry(data);
    out
      << "  State: " << MxLinkState::name(data.state)
      << "  #Reconnects: " << ZuBoxed(data.reconnects)
      << "  RxSeqNo: " << ZuBoxed(data.rxSeqNo)
      << "  TxSeqNo: " << ZuBoxed(data.txSeqNo);
  }
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
      const MxQueue *queue = rx->rxQueue();
      MxQueue::Gap gap = queue->gap();
      out <<
	"head: " << queue->head() << 
	"  gap: (" << gap.key() << ")," << gap.length() <<
	"  length: " << queue->length() << 
	"  count: " << queue->count();
      sem->post();
    });
    sem.wait();
  }
  out << '\n';
}

void MxMDSubscriber::resendCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  using namespace MxMDStream;
  ZuBox<int> argc(args->get("#"));
  if (argc != 4) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  auto id = args->get("1");
  ZmRef<MxAnyLink> link_ = link(id);
  if (!link_) throw ZtString() << id << " - unknown link";
  auto link = static_cast<MxMDSubLink *>(link_.ptr());
  ZuBox<uint64_t> seqNo(args->get("2"));
  ZuBox0(uint16_t) count(args->get("3"));
  if (!*seqNo || !count) throw ZvCmdUsage();
  ZmRef<MxQMsg> msg = link->resend(seqNo, count);
  if (!msg) throw ZtString("timed out");
  const auto &hdr = msg->ptr<Msg>()->as<Hdr>();
  seqNo = hdr.seqNo;
  out << "seqNo: " << seqNo << '\n';
  out << ZtHexDump{
      ZtString() << "type: " << Type::name(hdr.type),
      msg->ptr<Msg>()->ptr(), msg->length} << '\n';
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
