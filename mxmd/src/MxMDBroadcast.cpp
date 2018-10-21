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

// MxMD in-memory broadcast

#include <MxMDBroadcast.hpp>

#include <MxMDCore.hpp>

MxMDBroadcast::MxMDBroadcast()
{
}

MxMDBroadcast::~MxMDBroadcast()
{
  close__();
}

void MxMDBroadcast::init(MxMDCore *core)
{
  m_core = core;

  if (ZmRef<ZvCf> cf = core->cf()->subset("broadcast", false))
    m_params.init(cf);
  else
    m_params.name("RMD").size(131072); // 131072 is ~100mics at 1Gbit/s
}

bool MxMDBroadcast::open()
{
  Guard guard(m_lock);
  return open_(guard);
}

ZmRef<MxMDBroadcast::Ring> MxMDBroadcast::shadow()
{
  Guard guard(m_lock);
  if (!open_(guard)) return nullptr;
  ZmRef<Ring> ring = new Ring(m_params);
  if (ring->shadow(*m_ring) < 0) { close_(); return nullptr; }
  return ring;
}

void MxMDBroadcast::close(ZmRef<Ring> ring)
{
  Guard guard(m_lock);
  ring->close();
  close_();
}

void MxMDBroadcast::close()
{
  Guard guard(m_lock);
  close_();
}

bool MxMDBroadcast::open_(Guard &guard)
{
  ++m_openCount;
  if (m_ring) return true;
  m_ring = new Ring(m_params);
  ZeError e;
  if (m_ring->open(Ring::Create | Ring::Read | Ring::Write, &e) < 0) {
    m_openCount = 0;
    m_ring = 0;
    guard.unlock();
    m_core->raise(ZeEVENT(Error,
      ([name = MxTxtString(m_params.name()), e](
	const ZeEvent &, ZmStream &s) {
	  s << '"' << name << "\": "
	  "failed to open IPC shared memory ring buffer: " << e;
	})));
    return false;
  }
  return true;
}

void MxMDBroadcast::close_()
{
  if (!m_openCount) return;
  if (!--m_openCount) close__();
}

void MxMDBroadcast::close__()
{
  if (ZuLikely(m_ring)) {
    m_ring->close();
    m_ring = 0;
  }
}

void MxMDBroadcast::eof()
{
  Guard guard(m_lock);
  if (ZuUnlikely(!m_ring)) return;
  m_openCount = 0;
  m_ring->eof();
  m_ring->close();
  m_ring = 0;
}

void *MxMDBroadcast::push(unsigned size)
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
      ([name = MxTxtString(m_params.name())](
	  const ZeEvent &, ZmStream &s) {
	s << '"' << name << "\": "
	"IPC shared memory ring buffer overflow";
      })));
  return 0;
}

void *MxMDBroadcast::out(void *ptr, unsigned length, unsigned type,
    int shardID, ZmTime stamp)
{
  Frame *frame = new (ptr) Frame(
      (uint16_t)length, (uint16_t)type,
      (uint64_t)shardID, ++m_seqNo,
      stamp.sec(), (uint32_t)stamp.nsec());
  return frame->ptr();
}

void MxMDBroadcast::push2()
{
  if (ZuLikely(m_ring)) m_ring->push2();
  m_lock.unlock();
}
