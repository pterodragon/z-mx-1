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

// MxMD in-memory Rx/Tx - wrapper around ZiRing

#ifndef MxMDRing_HPP
#define MxMDRing_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZiRing.hpp>

#include <MxMDTypes.hpp>
#include <MxMDStream.hpp>

class MxMDCore;

class MxMDAPI MxMDRing {
public:
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef MxMDStream::Frame Frame;
  typedef MxMDStream::Ring Ring;

  MxMDRing(MxMDCore *core);
  ~MxMDRing();

  void init(ZvCf *cf);

  inline const ZiRingParams &config() const { return m_config; }

  bool open(); // returns true if successful, false otherwise
  void close();

private:
  void close_() {
    if (ZuLikely(m_ring)) {
      m_ring->close();
      m_ring = 0;
    }
  }

public:
  inline bool active() { return m_openCount; }

  // caller must ensure ring is open during Rx/Tx

  // Rx

  inline int attach() { return m_ring->attach(); }
  inline int detach() { return m_ring->detach(); }

  inline int id() { return m_ring->id(); }

  inline const Frame *shift() { return m_ring->shift(); }
  inline void shift2() { m_ring->shift2(); }

  inline int readStatus() { return m_ring->readStatus(); }

  // Tx

  void *push(unsigned size);
  void *out(void *ptr, unsigned length, unsigned type, ZmTime stamp);
  void push2() {
    if (ZuLikely(m_ring)) m_ring->push2();
    m_lock.unlock();
  }

  void eof();

  int writeStatus() {
    Guard guard(m_lock);
    if (ZuUnlikely(!m_ring)) return Zi::NotReady;
    return m_ring->writeStatus();
  }

  void reqDetach(int id = -1) {
    if (id < 0) {
      Guard guard(m_lock);
      if (!m_ring) return;
      id = m_ring->id();
    }
    MxMDStream::detach(*this, id);
  }

  // for snapshots
  inline MxSeqNo seqNo() const { ReadGuard guard(m_lock); return m_seqNo; }

protected:
  MxMDCore			*m_core;
  ZvRingParams			m_config;
  MxID				m_linkID;
  Lock				m_lock;
  MxSeqNo			  m_seqNo = 0;
  unsigned			  m_openCount = 0;
  ZmRef<MxMDStream::Ring>	  m_ring;
};

#endif /* MxMDRing_HPP */
