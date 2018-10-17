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

// MxMD replay

#include <MxMDCore.hpp>

#include <MxMDReplay.hpp>

#define fileERROR(path__, code) \
  core()->raise(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  core()->raise(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

void MxMDReplay::init(MxMDCore *core)
{
  ZmRef<ZvCf> cf = core->cf()->subset("replay", false, true);

  Mx *mx = core->mx();

  MxEngine::init(core, this, mx, cf);

  updateLink("replay", cf);
}

void MxMDReplay::final() { }

ZmRef<MxAnyLink> MxMDReplay::createLink(MxID id)
{
  return new MxMDReplayLink(id);
}

bool MxMDReplay::replay(ZtString path)
{
  return m_link->replay(ZuMv(path));
}

ZtString MxMDReplay::stopReplaying()
{
  return m_link->stopReplaying();
}

bool MxMDReplayLink::replay(ZtString path, MxDateTime begin, bool filter)
{
  if (engine()->isRx()) return false;
  Guard guard(m_lock);
  down();
  if (!path) return true;
  engine()->rxRun(ZmFn{
      [path = ZuMv(path), begin, filter](MxMDReplayLink *link) {
	link->m_path = ZuMv(path);
	link->m_replayNext = !begin ? ZmTime() : begin.zmTime();
	link->m_filter = filter;
      }, this});
  up();
  int state;
  thread_local ZmSemaphore sem;
  engine()->rxRun(ZmFn{
      [&state, &sem](MxMDReplayLink *link) {
	  state = link->state();
	  sem.post();
	}, this});
  sem.wait();
  return state != MxLinkState::Failed;
}

ZtString MxMDReplayLink::stopReplaying()
{
  ZtString path;
  if (engine()->isRx()) return path;
  { Guard guard(m_lock); path = ZuMv(m_path); }
  down();
  return path;
}

void MxMDReplayLink::update(ZvCf *cf)
{
  replay(cf->get("path"),
      MxDateTime{cf->get("begin", false, "")},
      cf->getInt("filter", 0, 1, false, 0));
}

void MxMDReplayLink::reset(MxSeqNo, MxSeqNo)
{
}

void MxMDReplayLink::connect()
{
  using namespace MxMDStream;

  if (!m_path) { disconnected(); return; }

  if (m_file) m_file.close();
  ZeError e;
  if (m_file.open(m_path, ZiFile::ReadOnly, 0, &e) != Zi::OK) {
    fileERROR(m_path, e);
    disconnected();
    return;
  }
  try {
    FileHdr hdr(m_file, &e);
    m_version = ZuMkPair(hdr.vmajor, hdr.vminor);
  } catch (const FileHdr::IOError &) {
    fileERROR(m_path, e);
    disconnected();
    return;
  } catch (const FileHdr::InvalidFmt &) {
    fileERROR(m_path, "invalid format");
    disconnected();
    return;
  }

  if (!m_msg) m_msg = new Msg();

  fileINFO(m_path, "started replaying");

  connected();

  engine()->rxRun(ZmFn{[](MxMDReplayLink *link) { link->read(); }, this});
}

void MxMDReplayLink::disconnect()
{
  m_file.close();
  m_replayNext = ZmTime();
  m_filter = false;
  m_version = Version();
  m_msg = 0;

  if (m_path) fileINFO(m_path, "stopped replaying");

  m_path = ZtString();

  disconnected();
}

void MxMDReplayLink::read()
{
  using namespace MxMDStream;

  MxMDCore *core = this->core();
  Frame *frame = m_msg->frame();

  ZeError e;

  if (!m_file) return;
// retry:
  int n = m_file.read(frame, sizeof(Frame), &e);
  if (n == Zi::IOError) {
error:
    ZtString path = m_path;
    guard.unlock();
    fileERROR(ZuMv(path), e);
    return;
  }
  if (n == Zi::EndOfFile || (unsigned)n < sizeof(Frame)) {
eof:
    fileINFO(m_path, "EOF");
    core->handler()->eof(core);
    return;
  }
  if (frame->len > sizeof(Buf)) {
    uint64_t offset = m_file.offset();
    offset -= sizeof(Frame);
    fileERROR(m_path,
	"message length >" << ZuBoxed(sizeof(Buf)) <<
	" at offset " << ZuBoxed(offset));
    return;
  }
  n = m_file.read(frame->ptr(), frame->len, &e);
  if (n == Zi::IOError) goto error;
  if (n == Zi::EndOfFile || (unsigned)n < frame->len) goto eof;

  if (frame->sec) {
    ZmTime next((time_t)frame->sec, (int32_t)frame->nsec);

    while (m_replayNext && next > m_replayNext) {
      MxDateTime replayNext;
      core->handler()->timer(m_replayNext, replayNext);
      m_replayNext = !replayNext ? ZmTime() : replayNext.zmTime();
    }
  }

  core->pad(frame);
  core->apply(frame, m_filter);

  engine()->rxRun(ZmFn{[](MxMDReplayLink *link) { link->read(); }, this});
}
