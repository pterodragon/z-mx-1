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

// MxMD in-memory Rx/Tx

#include <MxMDRing.hpp>

#include <MxMDCore.hpp>

MxMDRing(MxMDCore *core) : m_core(core), m_linkID("RMD"), m_seqNo(0)
{
  m_config.name("RMD").size(131072); // 131072 is ~100mics at 1Gbit/s
}

~MxMDRing()
{
  close_();
}

void init(ZvCf *cf)
{
  // FIXME - linkID
  m_config.init(cf);
}

bool open()
{
  Guard guard(m_lock);
  ++m_openCount;
  if (m_ring) return true;
  m_ring = new Ring(m_config);
  ZeError e;
  if (m_ring->open(Ring::Create | Ring::Read | Ring::Write, &e) < 0) {
    m_openCount = 0;
    m_ring = 0;
    guard.unlock();
    m_core->raise(ZeEVENT(Error,
      ([name = MxTxtString(m_config.name()), e](
	const ZeEvent &, ZmStream &s) {
	  s << '"' << name << "\": "
	  "failed to open IPC shared memory ring buffer: " << e;
	})));
    return false;
  }
  return true;
}

void close()
{
  Guard guard(m_lock);
  if (!m_openCount) return;
  if (!--m_openCount) close_();
}

inline void eof()
{
  Guard guard(m_lock);
  if (ZuUnlikely(!m_ring)) return;
  m_openCount = 0;
  m_ring->eof();
  m_ring->close();
  m_ring = 0;
}

void *push(unsigned size)
{
  m_lock.lock();
  if (ZuUnlikely(!m_ring)) { m_lock.unlock(); return 0; }
retry:
  if (void *ptr = m_ring->push(size)) return ptr;
  int i = m_ring->writeStatus();
  if (ZuUnlikely(i == Zi::EndOfFile)) { // should never happen
    m_ring->eof(false);
    goto retry;
  }
  m_openCount = 0;
  m_ring->eof();
  m_ring->close();
  m_ring = 0;
  m_lock.unlock();
  if (i != Zi::NotReady)
    m_core->raise(ZeEVENT(Error,
      ([name = MxTxtString(m_config.name())](
	  const ZeEvent &, ZmStream &s) {
	s << '"' << name << "\": "
	"IPC shared memory ring buffer overflow";
      })));
  return 0;
}

void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp)
{
  Frame *frame = new (ptr) Frame(
      length, type, m_linkID, ++m_seqNo, stamp.sec(), stamp.nsec());
  return frame->ptr();
}
