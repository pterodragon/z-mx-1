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
  Mx *mx = core->mx();

  if (!cf->get("id")) cf->set("id", "record");

  MxEngine::init(core, this, mx, cf);

  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);

  if (rxThread() == mx->rxThread() ||
      (unsigned)m_snapThread == mx->rxThread() ||
      (unsigned)m_snapThread == rxThread())
    throw ZtString() << "recorder misconfigured - thread conflict -"
      " I/O Rx: " << ZuBoxed(mx->rxThread()) <<
      " IPC Rx: " << ZuBoxed(rxThread()) <<
      " Snapshot: " << ZuBoxed(m_snapThread);

  updateLink("record", cf);

  core->addCmd(
      "record",
      "s stop stop { type flag }",
      MxMDCmd::Fn::Member<&MxMDRecord::recordCmd>::fn(this),
      "record market data to file", 
      "usage: record FILE\n"
      "       record -s\n"
      "record market data to FILE\n\n"
      "Options:\n"
      "-s, --stop\tstop recording\n");
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
  engine()->rxInvoke([this, path = ZuMv(path)]() mutable {
      m_path = ZuMv(path); });
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
  rxInvoke([rxSeqNo](Rx *rx) { rx->rxReset(rx->app().m_seqNo = rxSeqNo); });
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
      ZmFn<>{[](MxMDRecLink *link) { link->snap(); }, this});

  mx()->wakeFn(engine()->rxThread(), ZmFn<>{[](MxMDRecLink *link) {
	link->rxRun_([](Rx *rx) { rx->app().recv(rx); });
	link->wake();
      }, this});

  rxRun_([](Rx *rx) { rx->app().recv(rx); });
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
	ZmFn<>{[](MxMDRecLink *link) { link->disconnect(); }, this});
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
  Hdr *hdr = new (ptr) Hdr(
      (uint16_t)length, (uint8_t)type, (uint8_t)shardID);
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
	ZmFn<>{[](MxMDRecLink *link) { link->disconnect(); }, this});
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
      case Type::Wake:
	{
	  const Wake &wake = hdr->as<Wake>();
	  if (ZuUnlikely(wake.id == id())) {
	    broadcast.shift2();
	    return;
	  }
	  broadcast.shift2();
	}
	break;
      case Type::EndOfSnapshot:
	{
	  const EndOfSnapshot &eos = hdr->as<EndOfSnapshot>();
	  if (ZuUnlikely(eos.id == id())) {
	    MxSeqNo seqNo = eos.seqNo;
	    bool ok = eos.ok;
	    broadcast.shift2();
	    if (ok) rx->stopQueuing(seqNo);
	  } else
	    broadcast.shift2();
	}
	break;
      default:
	{
	  ZuRef<Msg> msg = new Msg();
	  unsigned msgLen = sizeof(Hdr) + hdr->len;
	  memcpy((void *)msg->ptr(), (void *)hdr, msgLen);
	  broadcast.shift2();
	  MxSeqNo seqNo = m_seqNo++;
	  Hdr &msgHdr = msg->as<Hdr>();
	  msgHdr.seqNo = seqNo;
	  rx->received(new MxQMsg(ZuMv(msg), msgLen, MxMsgID{id(), seqNo}));
	}
	break;
    }
  }
}

ZmRef<MxAnyLink> MxMDRecord::createLink(MxID id)
{
  m_link = new MxMDRecLink(id);
  return m_link;
}

MxEngineApp::ProcessFn MxMDRecord::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *, MxAnyLink *link, MxQMsg *msg) {
    static_cast<MxMDRecLink *>(link)->write(msg);
  });
}

void MxMDRecLink::wake()
{
  MxMDStream::wake(core()->broadcast(), id());
}

void MxMDRecLink::write(MxQMsg *qmsg)
{
  Guard fileGuard(m_fileLock);

  ZeError e;
  if (ZuUnlikely(write_(qmsg->ptr(), &e)) != Zi::OK) {
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

void MxMDRecord::recordCmd(const MxMDCmd::Args &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 2) throw MxMDCmd::Usage();
  if (!!args.get("stop")) {
    if (argc == 2) throw MxMDCmd::Usage();
    if (ZtString path = stopRecording())
      out << "stopped recording to \"" << path << "\"\n";
    return;
  }
  if (argc != 2) throw MxMDCmd::Usage();
  ZuString path = args.get("1");
  if (!path) MxMDCmd::Usage();
  if (record(path))
    out << "started recording to \"" << path << "\"\n";
  else
    out << "failed to record to \"" << path << "\"\n";
}
