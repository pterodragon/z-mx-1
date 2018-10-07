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

MxMDRecord::MxMDRecord(MxMDCore *core) : m_core(core) { }

MxMDRecord::~MxMDRecord() { m_file.close(); }

#define fileERROR(path__, code) \
  m_core->raise(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  m_core->raise(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

void MxMDRecord::init()
{
  Mx *mx = m_core->mx();
  ZmRef<ZvCf> cf = m_core->cf()->subset("record", false, true);
  MxEngine::init(m_core, this, mx, cf);
  m_snapThread = cf->getInt("snapThread", 1, mx->nThreads() + 1, true);
  m_link = MxEngine::updateLink<Link>("record", nullptr);
  m_link->init(this);
}

ZtString MxMDRecord::path() const
{
  ReadGuard guard(m_ioLock);
  return m_path;
}

bool MxMDRecord::running() const
{
  ReadGuard guard(m_stateLock);
  return m_running;
}

bool MxMDRecord::start(ZtString path)
{
  {
    Guard guard(m_stateLock);
    if (m_running) {
      // FIXME - stop first
    }
    m_running = true;
  }
  if (!fileOpen(ZuMv(path))) return false;
  if (!m_core->broadcast().open()) {
    fileClose();
    return false;
  }
  MxEngine::start();
  m_link->rxInvoke([](Rx *rx) { rx->app().recorder()->recv(rx)); });
  m_attachSem.wait();
  {
    Guard guard(m_ioLock);
    m_snapMsg = new MsgData();
  }
  mx()->run(m_snapThread, [](MxMDRecord *self) { self->snap(); }, this);
  return true;
}

ZtString MxMDRecord::stop()
{
  {
    Guard guard(m_stateLock);
    if (!m_running) return;
    m_running = false;

  {
    Guard guard(m_ioLock);
    m_snapMsg = nullptr;
  }

  using namespace MxMDStream;
  m_core->broadcast().reqDetach();
  m_detachSem.wait();
  m_running = false;

  MxEngine::stop();

  m_core->broadcast().close();
  return ZuMv(fileClose());
}

bool MxMDRecord::fileOpen(ZtString path)
{
  Guard guard(m_ioLock);
  if (!!m_file && m_path == path) return true;
  if (!!m_file) { m_file.close(); m_path.null(); }
  ZeError e;
  if (m_file.open(path,
	ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	0666, &e) != Zi::OK) {
    guard.unlock();
    fileERROR(ZuMv(path), e);
    return false;
  }
  if (!m_file.offset()) {
    using namespace MxMDStream;
    FileHdr hdr("RMD", MxMDCore::vmajor(), MxMDCore::vminor());
    if (m_file.write(&hdr, sizeof(FileHdr), &e) != Zi::OK) {
      m_file.close();;
      guard.unlock();
      fileERROR(ZuMv(path), e);
      return false;
    }
  }
  m_path = ZuMv(path);
  return true;
}

ZtString MxMDRecord::fileClose()
{
  Guard guard(m_ioLock);
  m_file.close();
  return ZuMv(m_path);
}

int MxMDRecord::write(const Frame *frame, ZeError *e)
{
  return m_file.write((const void *)frame, sizeof(Frame) + frame->len, e);
}


void MxMDRecord::recv(Rx *rx)
{
  rx->startQueuing();
  using namespace MxMDStream;
  MxMDRing &broadcast = m_core->broadcast();
  if (broadcast.attach() != Zi::OK) {
    m_attachSem.post();
    Guard guard(m_stateLock);
    m_running = false;
    return;
  }
  m_attachSem.post();
  m_link->rxRun([](Rx *rx) { rx->app().recorder()->recvMsg(rx); });
}

void MxMDRecord::recvMsg(Rx *rx)
{
  using namespace MxMDStream;
  const Frame *frame;
  MxMDRing &broadcast = m_core->broadcast();
  if (ZuUnlikely(!(frame = broadcast.shift()))) {
    if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) goto end;
    goto next;
  }
  if (ZuUnlikely(frame->type == Type::Detach)) {
    const Detach &detach = frame->as<Detach>();
    if (ZuUnlikely(detach.id == broadcast.id())) {
      broadcast.shift2();
      goto end;
    }
    broadcast.shift2();
    goto next;
  }
  {
    if (frame->len > sizeof(Buf)) {
      broadcast.shift2();
      snapStop();
      m_core->raise(ZeEVENT(Error,
	  ([name = MxTxtString(broadcast.config().name())](
	      const ZeEvent &, ZmStream &s) {
	    s << '"' << name << "\": "
	    "IPC shared memory ring buffer read error - "
	    "message too big";
	  })));
      goto end;
    }
    unsigned length = sizeof(Frame) + frame->len;
    ZmRef<Msg> msg = new Msg();
    Frame *msgFrame = msg->frame();
    memcpy(msgFrame, frame, length);
    msgFrame->linkID = m_link->id();
    ZmRef<MxQMsg> qmsg = new MxQMsg(
	msg, 0, length, MxMsgID{msgFrame->linkID, msgFrame->seqNo});
    rx->received(qmsg);
  }
  broadcast.shift2();

next:
  m_link->rxRun([](Rx *rx) { rx->app().recorder()->recvMsg(rx); });
  return;

end:
  broadcast.detach();
  m_detachSem.post();
}

MxEngineApp::ProcessFn MxMDRecord::processFn()
{
  return static_cast<MxEngineApp::ProcessFn>(
      [](MxEngineApp *app, MxAnyLink *, MxQMsg *msg) {
    static_cast<MxMDRecord *>(app)->writeMsg(msg);
  });
}

void MxMDRecord::writeMsg(MxQMsg *qmsg)
{
  ZeError e;
  Guard guard(m_ioLock);
  if (ZuUnlikely(write(qmsg->as<MsgData>().frame(), &e)) != Zi::OK) {
    ZtString path = m_path;
    m_file.close();
    guard.unlock();
    recvStop();
    fileERROR(ZuMv(path), e);
    return;
  }
}

void MxMDRecord::snap()
{
  bool failed = !m_core->snapshot(*this);
  {
    Guard guard(m_ioLock);
    if (failed) {
      m_file.close();
      guard.unlock();
      recvStop();
      return;
    }
  }
}

Frame *MxMDRecord::push(unsigned size)
{
  m_ioLock.lock();
  if (ZuUnlikely(!m_snapMsg)) {
    m_ioLock.unlock();
    return 0;
  }
  return m_snapMsg->frame();
}
void *MxMDRecord::out(void *ptr, unsigned length, unsigned type, ZmTime stamp)
{
  Frame *frame = new (ptr) Frame(
      length, type, m_link->id(), 0, stamp.sec(), stamp.nsec());
  return frame->ptr();
}
void MxMDRecord::push2()
{
  if (ZuUnlikely(!m_snapMsg)) { m_ioLock.unlock(); return; }
  Frame *frame = m_snapMsg->frame();
  ZeError e;
  if (ZuUnlikely(write(frame, &e) != Zi::OK)) {
    m_snapMsg = nullptr;
    ZtString path = m_path;
    m_ioLock.unlock();
    m_core->raise(ZeEVENT(Error, MxMDFileDiag(ZuMv(path), e)));
    return;
  }
  if (ZuUnlikely(frame->type == Type::EndOfSnapshot)) {
    m_snapshotSeqNo = frame->as<EndOfSnapshot>().seqNo;
    m_snapMsg = nullptr;
    m_ioLock.unlock();
    m_link->rxInvoke([](Rx *rx) {
	rx->stopQueuing(rx->app().recorder()->snapshotSeqNo()); });
    return;
  }
  m_ioLock.unlock();
}
MxSeqNo MxMDRecord::snapshotSeqNo()
{
  Guard guard(m_ioLock);
  return m_snapshotSeqNo;
}
