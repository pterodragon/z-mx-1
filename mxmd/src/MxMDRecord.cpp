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

// MxMD recorder

#include <MxMDCore.hpp>

#include <MxMDRecord.hpp>

void MxMDRecord::init(MxMDCore *core, ZvCf *cf)
{
  if (!cf->get("id")) cf->set("id", "record");

  Mx *mx = core->mx(cf->get("mx", false, "core"));

  if (!mx) throw ZvCf::Required(cf, "mx");

  MxEngine::init(core, this, mx, cf);

  m_snapThread = mx->tid(cf->get("snapThread", true));

  if (!m_snapThread ||
      rxThread() == mx->rxThread() ||
      m_snapThread == mx->rxThread() ||
      m_snapThread == rxThread())
    throw ZtString() << "recorder misconfigured - thread conflict -"
      " Network Rx: " << ZuBoxed(mx->rxThread()) <<
      " IPC Rx: " << ZuBoxed(rxThread()) <<
      " Snapshot: " << ZuBoxed(m_snapThread);

  updateLink("record", cf);

  core->addCmd(
      "record",
      "s stop stop { type flag }",
      ZvCmdFn::Member<&MxMDRecord::recordCmd>::fn(this),
      "record market data to file", 
      "usage: record FILE\n"
      "       record -s\n"
      "record market data to FILE\n\n"
      "Options:\n"
      "  -s, --stop\tstop recording\n");
}

void MxMDRecord::final() { }

bool MxMDRecord::record(ZtString path)
{
  if (ZuUnlikely(!m_link)) return false;
  bool ok = m_link->record(ZuMv(path));
  start();
  return ok;
}

ZtString MxMDRecord::stopRecording()
{
  if (ZuUnlikely(!m_link)) return ZtString();
  ZtString path = m_link->stopRecording();
  stop();
  thread_local ZmSemaphore sem;
  rxInvoke([sem = &sem]() { sem->post(); });
  sem.wait();
  return path;
}

bool MxMDRecLink::ok()
{
  int state;
  thread_local ZmSemaphore sem;
  engine()->rxInvoke([this, &state, sem = &sem]() {
    state = this->state();
    sem->post();
  });
  sem.wait();
  return state != MxLinkState::Failed;
}

bool MxMDRecLink::record(ZtString path)
{
  Guard guard(m_lock);
  down();
  if (!path) return true;
  thread_local ZmSemaphore sem;
  engine()->rxInvoke([this, path = ZuMv(path), sem = &sem]() mutable {
    m_path = ZuMv(path);
    sem->post();
  });
  sem.wait();
  up();
  return ok();
}

ZtString MxMDRecLink::stopRecording()
{
  Guard guard(m_lock);
  ZtString path;
  {
    Guard fileGuard(m_fileLock);
    path = ZuMv(m_path);
  }
  down();
  return path;
}

void MxMDRecLink::update(ZvCf *cf)
{
  if (ZtString path = cf->get("path"))
    record(ZuMv(path));
  else
    stopRecording();
}

void MxMDRecLink::reset(MxSeqNo rxSeqNo, MxSeqNo)
{
  rxInvoke([rxSeqNo](Rx *rx) { rx->rxReset(rx->impl()->m_seqNo = rxSeqNo); });
}

#define fileERROR(path__, code) \
  engine()->appException(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  engine()->appException(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

void MxMDRecLink::connect()
{
  reset(0, m_seqNo = 0);

  ZtString path;

  {
    Guard fileGuard(m_fileLock);

    if (!m_path) { disconnected(); return; }

    if (m_file) m_file.close();

    ZeError e;
    if (m_file.open(m_path,
	  ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	  0666, &e) != Zi::OK) {
  error:
      path = ZuMv(m_path);
      fileGuard.unlock();
      if (path) fileERROR(ZuMv(path), e);
      disconnected();
      return;
    }

    if (!m_file.offset()) {
      using namespace MxMDStream;
      FileHdr hdr("RMD", MxMDCore::vmajor(), MxMDCore::vminor());
      if (m_file.write(&hdr, sizeof(FileHdr), &e) != Zi::OK) {
	m_file.close();
	goto error;
      }
    }

    MxMDBroadcast &broadcast = core()->broadcast();

    if (!broadcast.open() || broadcast.attach() != Zi::OK) {
      m_file.close();
      fileGuard.unlock();
      disconnected();
      return;
    }

    path = m_path;
  }

  fileINFO(path, "started recording");

  rxInvoke([](Rx *rx) { rx->startQueuing(); });

  connected();

  m_seqNo = 0;

  mx()->run(engine()->snapThread(),
      ZmFn<>{this, [](MxMDRecLink *link) { link->snap(); }});

  mx()->wakeFn(engine()->rxThread(),
      ZmFn<>{this, [](MxMDRecLink *link) {
	link->rxRun_([](Rx *rx) { rx->impl()->recv(rx); });
	link->wake();
      }});

  rxRun_([](Rx *rx) { rx->impl()->recv(rx); });
}

void MxMDRecLink::disconnect()
{
  MxMDBroadcast &broadcast = core()->broadcast();

  broadcast.detach();
  broadcast.close();

  ZtString path;

  {
    Guard fileGuard(m_fileLock);
    m_file.close();
    path = ZuMv(m_path);
  }

  if (path) fileINFO(ZuMv(path), "stopped recording");

  disconnected();
}

int MxMDRecLink::write_(const void *ptr, ZeError *e)
{
  using namespace MxMDStream;
  return m_file.write(ptr, sizeof(Hdr) + ((const Hdr *)ptr)->len, e);
}

// snapshot

void MxMDRecLink::snap()
{
  m_snapMsg = new Msg();
  if (!core()->snapshot(*this, id(), 0))
    engine()->rxRun(
	ZmFn<>{this, [](MxMDRecLink *link) { link->disconnect(); }});
  m_snapMsg = nullptr;
}
void *MxMDRecLink::push(unsigned size)
{
  if (ZuUnlikely(state() != MxLinkState::Up)) return nullptr;
  return m_snapMsg->ptr();
}
void *MxMDRecLink::out(void *ptr, unsigned length, unsigned type, int shardID)
{
  using namespace MxMDStream;
  Hdr *hdr = new (ptr) Hdr{
      (uint64_t)0, (uint32_t)0,
      (uint16_t)length, (uint8_t)type, (uint8_t)shardID};
  return hdr->body();
}
void MxMDRecLink::push2()
{
  Guard fileGuard(m_fileLock);

  ZeError e;
  if (ZuUnlikely(write_(m_snapMsg->ptr(), &e) != Zi::OK)) {
    m_file.close();
    ZtString path = ZuMv(m_path);
    fileGuard.unlock();
    if (path) fileERROR(ZuMv(path), e);
    engine()->rxRun(
	ZmFn<>{this, [](MxMDRecLink *link) { link->disconnect(); }});
    return;
  }
}

// broadcast

void MxMDRecLink::recv(Rx *rx)
{
  using namespace MxMDStream;
  if (ZuUnlikely(state() != MxLinkState::Up)) {
    mx()->wakeFn(engine()->rxThread(), ZmFn<>());
    return;
  }
  const Hdr *hdr;
  MxMDBroadcast &broadcast = core()->broadcast();
  for (;;) {
    if (ZuUnlikely(!(hdr = broadcast.shift()))) {
      if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) {
	broadcast.detach();
	broadcast.close();
	{ Guard guard(m_fileLock); m_file.close(); }
	disconnected();
	return;
      }
      continue;
    }
    if (hdr->len > sizeof(Buf)) {
      broadcast.shift2();
      broadcast.detach();
      broadcast.close();
      { Guard guard(m_fileLock); m_file.close(); }
      disconnected();
      core()->raise(ZeEVENT(Error,
	  ([name = ZtString(broadcast.params().name())](
	      const ZeEvent &, ZmStream &s) {
	    s << '"' << name << "\": "
	    "IPC shared memory ring buffer read error - "
	    "message too big / corrupt";
	  })));
      return;
    }
    switch ((int)hdr->type) {
      case Type::Wake: {
	const Wake &wake = hdr->as<Wake>();
	if (ZuUnlikely(wake.id == id())) {
	  broadcast.shift2();
	  return;
	}
	broadcast.shift2();
      } break;
      case Type::EndOfSnapshot: {
	const EndOfSnapshot &eos = hdr->as<EndOfSnapshot>();
	if (ZuUnlikely(eos.id == id())) {
	  MxSeqNo seqNo = eos.seqNo;
	  bool ok = eos.ok;
	  broadcast.shift2();
	  if (ok) rx->stopQueuing(seqNo);
	} else
	  broadcast.shift2();
      } break;
      default: {
	ZuRef<Msg> msg = new Msg();
	unsigned len = sizeof(Hdr) + hdr->len;
	memcpy((void *)msg->ptr(), (void *)hdr, len);
	broadcast.shift2();
	MxSeqNo seqNo = m_seqNo++;
	Hdr &msgHdr = msg->as<Hdr>();
	msgHdr.seqNo = seqNo;
	rx->received(new MxQMsg(ZuMv(msg), len, MxMsgID{id(), seqNo}));
      } break;
    }
  }
}

ZmRef<MxAnyLink> MxMDRecord::createLink(MxID id)
{
  m_link = new MxMDRecLink(id);
  return m_link;
}

void MxMDRecLink::wake()
{
  MxMDStream::wake(core()->broadcast(), id());
}

void MxMDRecLink::process(MxQMsg *qmsg)
{
  Guard fileGuard(m_fileLock);

  ZeError e;
  if (ZuUnlikely(write_(qmsg->ptr<Msg>()->ptr(), &e)) != Zi::OK) {
    m_file.close();
    ZtString path = ZuMv(m_path);
    fileGuard.unlock();
    MxMDBroadcast &broadcast = core()->broadcast();
    broadcast.detach();
    broadcast.close();
    disconnected();
    if (path) fileERROR(ZuMv(path), e);
    return;
  }
}

// commands

void MxMDRecord::recordCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc < 1 || argc > 2) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  if (!!args->get("stop")) {
    if (argc == 2) throw ZvCmdUsage();
    if (ZtString path = stopRecording())
      out << "stopped recording to \"" << path << "\"\n";
    return;
  }
  if (argc != 2) throw ZvCmdUsage();
  ZtString path = args->get("1");
  if (!path) ZvCmdUsage();
  if (record(path))
    out << "started recording to \"" << path << "\"\n";
  else {
    outMsg->code(1);
    out << "failed to record to \"" << path << "\"\n";
  }
}
