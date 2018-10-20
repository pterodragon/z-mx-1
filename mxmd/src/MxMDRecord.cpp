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

#define fileERROR(path__, code) \
  core()->raise(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  core()->raise(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

void MxMDRecord::init(MxMDCore *core)
{
  ZmRef<ZvCf> cf = core->cf()->subset("record", false, false);

  Mx *mx = core->mx();

  if (!cf) {
    cf = new ZvCf();
    cf->fromString("id record");
    cf->set("snapThread", ZtString() << ZuBoxed(mx->txThread()));
  }

  MxEngine::init(core, this, mx, cf);

  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);

  updateLink("record", cf);

  core->addCmd(
      "record",
      "s stop stop { type flag }",
      CmdFn::Member<&MxMDRecord::recordCmd>::fn(this),
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
  return path;
}

bool MxMDRecLink::record(ZtString path)
{
  Guard guard(m_lock);
  down();
  if (!path) return true;
  engine()->rxInvoke(ZmFn<>{
      [this, path = ZuMv(path)]() { m_path = ZuMv(path); } });
  up();
  int state;
  thread_local ZmSemaphore sem;
  engine()->rxInvoke(ZmFn<>{
      [&state, sem = &sem](MxMDReplayLink *link) {
	  state = link->state();
	  sem->post();
	}, this});
  sem.wait();
  return state != MxLinkState::Failed;
}

ZtString MxMDRecLink::stopRecording()
{
  ZtString path;
  {
    Guard guard(m_lock);
    {
      Guard fileGuard(m_fileLock);
      path = ZuMv(m_path);
    }
    down();
  }
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
  rxInvoke([rxSeqNo](Rx *rx) { rx->rxReset(rxSeqNo); });
}

void MxMDRecLink::connect()
{
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

    MxMDRing &broadcast = core()->broadcast();

    if (!broadcast.open() || broadcast.attach() != Zi::OK) {
      m_file.close();
      fileGuard.unlock();
      disconnected();
      return;
    }

    path = m_path;
  }

  fileINFO(path, "started recording");

  rx->startQueuing();

  connected();

  mx()->run(m_snapThread, [](MxMDRecLink *link) { link->snap(); }, this);

  rxRun_([](Rx *rx) { rx->app().recv(rx); });
}

void MxMDRecLink::disconnect()
{
  MxMDRing &broadcast = core()->broadcast();

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

int MxMDRecLink::write_(const Frame *frame, ZeError *e)
{
  return m_file.write((const void *)frame, sizeof(Frame) + frame->len, e);
}

// snapshot

void MxMDRecLink::snap()
{
  m_snapMsg = new MsgData();
  if (!core()->snapshot(*this, broadcast.id()))
    engine()->rxRun(
	ZmFn<>{[](MxMDRecLink *link) { link->disconnect(); }, this});
  m_snapMsg = nullptr;
}
Frame *MxMDRecLink::push(unsigned size)
{
  if (ZuUnlikely(state() != MxLinkState::Up)) return nullptr;
  return m_snapMsg->frame();
}
void *MxMDRecLink::out(void *ptr, unsigned length, unsigned type,
    int shardID, ZmTime stamp)
{
  Frame *frame = new (ptr) Frame(
      length, type, shardID, 0, stamp.sec(), stamp.nsec());
  return frame->ptr();
}
void MxMDRecLink::push2()
{
  Guard fileGuard(m_fileLock);

  ZeError e;
  if (ZuUnlikely(write_(msg->frame(), &e) != Zi::OK)) {
    m_file.close();
    ZtString path = ZuMv(m_path);
    fileGuard.unlock();
    if (path) fileERROR(ZuMv(path), e);
    engine()->rxInvoke(
	ZmFn<>{[](MxMDRecLink *link) { link->disconnect(); }, this});
    return;
  }
}

// broadcast

void MxMDRecLink::recv(Rx *rx)
{
  using namespace MxMDStream;
  const Frame *frame;
  MxMDRing &broadcast = core()->broadcast();
  if (ZuUnlikely(!(frame = broadcast.shift()))) {
    if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) {
      broadcast.detach();
      broadcast.close();
      { FileGuard guard(m_fileLock); m_file.close(); }
      disconnected();
      return;
    }
    rxRun_([](Rx *rx) { rx->app().recv(rx); });
    return;
  }
  if (frame->len > sizeof(Buf)) {
    broadcast.shift2();
    broadcast.detach();
    broadcast.close();
    { FileGuard guard(m_fileLock); m_file.close(); }
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
    case Type::Wake:
      {
	const Wake &wake = frame->as<Wake>();
	if (ZuUnlikely(wake.id == broadcast.id())) {
	  broadcast.shift2();
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
  rxRun_([](Rx *rx) { rx->app().recv(rx); });
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
    static_cast<Link *>(link)->write(msg);
  });
}

void MxMDRecord::wake()
{
  if (ZuUnlikely(!m_link)) return;
  int id;
  if ((id = core()->broadcast().id()) >= 0) {
    m_link->wake();
    MxMDStream::wake(core()->broadcast(), id);
  }
}

void MxMDRecLink::wake()
{
  rxRun_([](Rx *rx) { rx->app().recv(rx); });
}

void MxMDRecLink::write(MxQMsg *qmsg)
{
  Guard fileGuard(m_fileLock);

  ZeError e;
  if (ZuUnlikely(write_(qmsg->as<MsgData>().frame(), &e)) != Zi::OK) {
    m_file.close();
    ZtString path = ZuMv(m_path);
    fileGuard.unlock();
    MxMDRing &broadcast = core()->broadcast();
    broadcast.detach();
    broadcast.close();
    disconnected();
    if (path) fileERROR(ZuMv(path), e);
    return;
  }
}

// commands

void MxMDRecord::recordCmd(const CmdArgs &args, ZtArray<char> &out)
{
  ZuBox<int> argc = args.get("#");
  if (argc < 1 || argc > 2) throw CmdUsage();
  if (!!args.get("stop")) {
    if (argc == 2) throw CmdUsage();
    if (ZtString path = stopRecording())
      out << "stopped recording to \"" << path << "\"\n";
    return;
  }
  if (argc != 2) throw CmdUsage();
  ZuString path = args.get("1");
  if (!path) CmdUsage();
  if (record(path))
    out << "started recording to \"" << path << "\"\n";
  else
    out << "failed to record to \"" << path << "\"\n";
}
