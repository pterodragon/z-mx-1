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

#include <MxMDReplay.hpp>

#include <MxMDCore.hpp>

MxMDReplay::MxMDReplay(MxMDCore *core) : m_core(core) { }

MxMDReplay::~MxMDReplay() { }

#define fileERROR(path__, code) \
  m_core->raise(ZeEVENT(Error, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))
#define fileINFO(path__, code) \
  m_core->raise(ZeEVENT(Info, \
    ([=, path = path__](const ZeEvent &, ZmStream &s) { \
      s << "MxMD \"" << path << "\": " << code; })))

// FIXME - replay thread

void MxMDReplay::start(ZtString path, MxDateTime begin, bool filter)
{
  Guard guard(m_lock);
  m_timerNext = !begin ? ZmTime() : begin.zmTime();
  m_filter = filter;
  if (!start_(guard, path))
    fileERROR(path, "failed to start replaying");
  else
    fileINFO(path, "started replaying");
}

void MxMDReplay::stop()
{
  Guard guard(m_lock);
  if (!!m_file) {
    m_path.null();
    m_file.close();
    m_msg = 0;
    m_timerNext = ZmTime();
  }
}

bool MxMDReplay::start_(Guard &guard, ZtString path)
{
  using namespace MxMDStream;

  if (!!m_file) m_file.close();
  ZeError e;
  if (m_file.open(path, ZiFile::ReadOnly, 0, &e) != Zi::OK) {
    guard.unlock();
    fileERROR(path, e);
    return false;
  }
  m_path = ZuMv(path);
  try {
    FileHdr hdr(m_file, &e);
    m_version = ZuMkPair(hdr.vmajor, hdr.vminor);
  } catch (const FileHdr::IOError &) {
    guard.unlock();
    fileERROR(m_path, e);
    return false;
  } catch (const FileHdr::InvalidFmt &) {
    guard.unlock();
    fileERROR(m_path, "invalid format");
    return false;
  }
  if (!m_msg) m_msg = new Msg();
  m_core->mx()->add(ZmFn<>::Member<&MxMDReplay::replay>::fn(this));
  return true;
}

void MxMDReplay::replay()
{
  using namespace MxMDStream;

  ZeError e;

  {
    ZmRef<Msg> msg;

    {
      Guard guard(m_lock);
      if (!m_file) return;
      msg = m_msg;
      Frame *frame = msg->frame();
  // retry:
      int n = m_file.read(frame, sizeof(Frame), &e);
      if (n == Zi::IOError) goto error;
      if (n == Zi::EndOfFile || (unsigned)n < sizeof(Frame)) goto eof;
      if (frame->len > sizeof(Buf)) goto lenerror;
      n = m_file.read(frame->ptr(), frame->len, &e);
      if (n == Zi::IOError) goto error;
      if (n == Zi::EndOfFile || (unsigned)n < frame->len) goto eof;

      if (frame->sec) {
	ZmTime next((time_t)frame->sec, (int32_t)frame->nsec);

	while (!!m_timerNext && next > m_timerNext) {
	  MxDateTime now = m_timerNext, timerNext;
	  guard.unlock();
	  this->handler()->timer(now, timerNext);
	  guard = Guard(m_lock);
	  m_timerNext = !timerNext ? ZmTime() : timerNext.zmTime();
	}
      }
    }

    pad(msg->frame());
    apply(msg->frame());
    m_core->mx()->add(ZmFn<>::Member<&MxMDReplay::replay>::fn(this));
  }

  return;

eof:
  fileINFO(m_path, "EOF");
  m_core->handler()->eof(m_core);
  return;

error:
  fileERROR(m_path, e);
  return;

lenerror:
  {
    uint64_t offset = m_file.offset();
    offset -= sizeof(Frame);
    fileERROR(m_path,
	"message length >" << ZuBoxed(sizeof(Buf)) <<
	" at offset " << ZuBoxed(offset));
  }
}
