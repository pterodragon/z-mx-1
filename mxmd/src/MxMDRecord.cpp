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
  ZmRef<ZvCf> cf = core->cf()->subset("record", false, true);

  Mx *mx = core->mx();

  MxEngine::init(core, this, mx, cf);

  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);
  updateLink("record", cf);
}

void MxMDRecord::final() { }

bool MxMDRecord::record(ZtString path)
{
  return m_link->record(ZuMv(path));
}

ZtString MxMDRecord::stopRecording()
{
  return m_link->stopRecording();
}

bool MxMDRecLink::record(ZtString path)
{
  down();
  { Guard guard(m_lock); if (!(m_path = ZuMv(path))) return true; }
  int state;
  thread_local ZmSemaphore sem;
  up([&state, &sem](int state_) { state = state_; sem.post(); });
  sem.wait();
  return state != MxLinkState::Failed;
}

ZtString MxMDRecLink::stopRecording()
{
  ZtString path;
  { Guard guard(m_lock); path = ZuMv(m_path); }
  down();
  return path;
}

void MxMDRecLink::update(ZvCf *cf)
{
  record(cf->get("path"));
}

void MxMDRecLink::reset(MxSeqNo rxSeqNo, MxSeqNo)
{
  rxRun([rxSeqNo](Rx *rx) { rx->rxReset(rxSeqNo); });
}

void MxMDRecLink::connect()
{
  ZtString path;

  {
    Guard guard(m_lock);

    if (!m_path) { guard.unlock(); disconnected(); return; }

    if (m_file) m_file.close();
    ZeError e;
    if (m_file.open(m_path,
	  ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	  0666, &e) != Zi::OK) {
error:
      path = m_path;
      guard.unlock();
      disconnected();
      fileERROR(ZuMv(path), e);
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

    if (!core()->broadcast().open()) {
      m_file.close();
      guard.unlock();
      disconnected();
      return;
    }

    path = m_path;
  }

  if (path) fileINFO(ZuMv(path), "started recording");

  rxInvoke([](Rx *rx) { rx->app().recv(rx)); });
  m_attachSem.wait();

  bool snap = state() == MxLinkState::Connecting;

  connected();

  if (snap)
    mx()->run(m_snapThread, [](MxMDRecLink *link) { link->snap(); }, this);
}

void MxMDRecLink::disconnect()
{
  MxMDRing &broadcast = core()->broadcast();

  MxMDStream::detach(broadcast, broadcast.id());
  m_detachSem.wait();

  broadcast.close();

  ZtString path;

  {
    Guard guard(m_lock);

    path = ZuMv(m_path);

    m_file.close();
  }

  if (path) fileINFO(ZuMv(path), "stopped recording");

  disconnected();
}

int MxMDRecLink::write(const Frame *frame, ZeError *e)
{
  return m_file.write((const void *)frame, sizeof(Frame) + frame->len, e);
}

void MxMDRecLink::recv(Rx *rx)
{
  rx->startQueuing();
  using namespace MxMDStream;
  MxMDRing &broadcast = core()->broadcast();
  if (broadcast.attach() != Zi::OK) {
    disconnect();
    m_attachSem.post();
    return;
  }
  m_attachSem.post();
  rxRun([](Rx *rx) { rx->app().recvMsg(rx); });
}

void MxMDRecLink::recvMsg(Rx *rx)
{
  using namespace MxMDStream;
  const Frame *frame;
  MxMDRing &broadcast = core()->broadcast();
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

ZmRef<MxAnyLink> MxMDRecord::createLink(MxID id)
{
  return new MxMDRecLink(id);
}

MxEngineApp::ProcessFn MxMDRecord::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *, MxAnyLink *link, MxQMsg *msg) {
    static_cast<Link *>(link)->writeMsg(msg);
  });
}

void MxMDRecLink::writeMsg(MxQMsg *qmsg)
{
  ZeError e;
  Guard guard(m_lock);
  if (ZuUnlikely(write(qmsg->as<MsgData>().frame(), &e)) != Zi::OK) {
    ZtString path = m_path;
    guard.unlock();
    disconnect();
    fileERROR(ZuMv(path), e);
    return;
  }
}

void MxMDRecLink::snap()
{
  m_snapMsg = new MsgData();
  if (!core()->snapshot(*this, broadcast.id())) disconnect();
  m_snapMsg = nullptr;
}
Frame *MxMDRecLink::push(unsigned size)
{
  m_lock.lock();
  if (ZuUnlikely(state() != MxLinkState::Up)) { m_lock.unlock(); return 0; }
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
  Frame *frame = m_snapMsg->frame();
  ZeError e;
  if (ZuUnlikely(write(frame, &e) != Zi::OK)) {
    ZtString path = m_path;
    m_lock.unlock();
    disconnect();
    fileERROR(ZuMv(path), e);
    return;
  }
  m_lock.unlock();
}
