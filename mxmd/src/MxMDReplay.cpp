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

void MxMDReplay::init(MxMDCore *core, ZmRef<ZvCf> cf)
{
  Mx *mx = core->mx();

  if (!cf) cf = new ZvCf();

  cf->set("id", "replay");

  MxEngine::init(core, this, mx, cf);

  if (rxThread() == mx->rxThread())
    throw ZtString() << "replay misconfigured - thread conflict -"
      " Network Rx: " << ZuBoxed(mx->rxThread()) <<
      " File Rx: " << ZuBoxed(rxThread());

  updateLink("replay", cf);

  core->addCmd(
      "replay",
      "s stop stop { type flag }",
      ZvCmdFn::Member<&MxMDReplay::replayCmd>::fn(this),
      "replay market data from file",
      "usage: replay FILE\n"
      "       replay -s\n"
      "replay market data from FILE\n\n"
      "Options:\n"
      "-s, --stop\tstop replaying\n");
}

void MxMDReplay::final() { }

bool MxMDReplay::replay(ZtString path, MxDateTime begin, bool filter)
{
  if (ZuUnlikely(!m_link)) return false;
  bool ok = m_link->replay(ZuMv(path), begin, filter);
  start();
  return ok;
}

ZtString MxMDReplay::stopReplaying()
{
  if (ZuUnlikely(!m_link)) return ZtString();
  ZtString path = m_link->stopReplaying();
  stop();
  thread_local ZmSemaphore sem;
  rxInvoke([sem = &sem]() { sem->post(); });
  sem.wait();
  return path;
}

ZmRef<MxAnyLink> MxMDReplay::createLink(MxID id)
{
  m_link = new MxMDReplayLink(id);
  return m_link;
}

MxEngineApp::ProcessFn MxMDReplay::processFn()
{
  return MxEngineApp::ProcessFn();
}

bool MxMDReplayLink::ok()
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

bool MxMDReplayLink::replay(ZtString path, MxDateTime begin, bool filter)
{
  Guard guard(m_lock);
  down();
  if (!path) return true;
  engine()->rxInvoke([this, path = ZuMv(path), begin, filter]() mutable {
	m_path = ZuMv(path);
	m_nextTime = !begin ? ZmTime() : begin.zmTime();
	m_filter = filter;
      });
  up();
  return ok();
}

ZtString MxMDReplayLink::stopReplaying()
{
  ZtString path;
  Guard guard(m_lock);
  path = ZuMv(m_path);
  down();
  return path;
}

void MxMDReplayLink::update(ZvCf *cf)
{
  if (ZtString path = cf->get("path"))
    replay(ZuMv(path),
      MxDateTime{cf->get("begin", false, "")},
      cf->getInt("filter", 0, 1, false, 0));
  else
    stopReplaying();
}

void MxMDReplayLink::reset(MxSeqNo, MxSeqNo)
{
}

#define fileERROR(path__, code) \
  engine()->appException(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  engine()->appException(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

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

  engine()->rxRun(ZmFn<>{[](MxMDReplayLink *link) { link->read(); }, this});
}

void MxMDReplayLink::disconnect()
{
  m_file.close();
  m_nextTime = ZmTime();
  m_filter = false;
  m_version = Version();
  m_msg = 0;

  if (m_path) fileINFO(m_path, "stopped replaying");

  m_path = ZtString();

  disconnected();
}

// replay

void MxMDReplayLink::read()
{
  using namespace MxMDStream;

  MxMDCore *core = this->core();

  ZeError e;

  if (!m_file) return;
// retry:
  int n = m_file.read(m_msg->ptr(), sizeof(Hdr), &e);
  if (n == Zi::IOError) {
error:
    fileERROR(m_path, e);
    return;
  }
  if (n == Zi::EndOfFile || (unsigned)n < sizeof(Hdr)) {
eof:
    fileINFO(m_path, "EOF");
    core->handler()->eof(core);
    return;
  }
  Hdr &hdr = m_msg->hdr();
  if (hdr.len > sizeof(Buf)) {
    uint64_t offset = m_file.offset();
    offset -= sizeof(Hdr);
    fileERROR(m_path,
	"message length >" << ZuBoxed(sizeof(Buf)) <<
	" at offset " << ZuBoxed(offset));
    return;
  }
  n = m_file.read(hdr.body(), hdr.len, &e);
  if (n == Zi::IOError) goto error;
  if (n == Zi::EndOfFile || (unsigned)n < hdr.len) goto eof;

  if (hdr.type == Type::HeartBeat) {
    m_lastTime = m_msg->as<HeartBeat>().stamp.zmTime();
  } else {
    if (hdr.nsec) {
      ZmTime next(ZmTime::Nano, hdr.nsec);
      next += m_lastTime;
      while (m_nextTime && next > m_nextTime) {
	MxDateTime nextTime;
	core->handler()->timer(m_nextTime, nextTime);
	m_nextTime = !nextTime ? ZmTime() : nextTime.zmTime();
      }
    }

    core->pad(hdr);
    core->apply(hdr, m_filter);
  }

  engine()->rxRun(ZmFn<>{[](MxMDReplayLink *link) { link->read(); }, this});
}

// commands

void MxMDReplay::replayCmd(ZvCmdServerCxn *,
    ZvCf *args, ZmRef<ZvCmdMsg> inMsg, ZmRef<ZvCmdMsg> &outMsg)
{
  ZuBox<int> argc = args->get("#");
  if (argc < 1 || argc > 2) throw ZvCmdUsage();
  outMsg = new ZvCmdMsg();
  auto &out = outMsg->cmd();
  if (!!args->get("stop")) {
    if (ZtString path = stopReplaying())
      out << "stopped replaying to \"" << path << "\"\n";
    return;
  }
  if (argc != 2) throw ZvCmdUsage();
  ZtString path = args->get("1");
  if (!path) ZvCmdUsage();
  if (replay(path))
    out << "started replaying from \"" << path << "\"\n";
  else
    out << "failed to replay from \"" << path << "\"\n";
}
