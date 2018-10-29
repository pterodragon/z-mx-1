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

// MxMD in-memory broadcast (ZiRing wrapper)

#ifndef MxMDBroadcast_HPP
#define MxMDBroadcast_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZmPLock.hpp>
#include <ZmRef.hpp>

#include <ZiRing.hpp>

#include <ZvRingCf.hpp>

#include <MxMDTypes.hpp>
#include <MxMDStream.hpp>

class MxMDCore;

template <> struct ZiRingTraits<MxMDStream::Frame> {
  inline static unsigned size(const MxMDStream::Frame &hdr) {
    return sizeof(MxMDStream::Frame) + hdr.len;
  }
};

class MxMDAPI MxMDBroadcast {
public:
  typedef MxMDStream::Frame Frame;

  typedef ZiRing<Frame, ZiRingBase<ZmObject> > Ring;

  MxMDBroadcast();
  ~MxMDBroadcast();

  void init(MxMDCore *core);

  inline const ZiRingParams &params() const { return m_params; }

  bool open(); // returns true if successful, false otherwise
  void close();

  ZmRef<Ring> shadow();
  void close(ZmRef<Ring> ring);

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
  void *out(void *ptr, unsigned length, unsigned type,
      int shardID, ZmTime stamp);
  void push2();

  void eof();

  int writeStatus() {
    Guard guard(m_lock);
    if (ZuUnlikely(!m_ring)) return Zi::NotReady;
    return m_ring->writeStatus();
  }

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  bool open_(Guard &guard);
  void close_();
  void close__();

private:
  MxMDCore		*m_core = 0;
  ZvRingParams		m_params;
  Lock			m_lock;
    MxSession		  m_session;
    MxSeqNo		  m_seqNo = 0;
    unsigned		  m_openCount = 0;
    ZmRef<Ring>		  m_ring;
};

#endif /* MxMDBroadcast_HPP */
