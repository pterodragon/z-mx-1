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

// fast platform-native recursive mutex

#ifndef ZmLock_HPP
#define ZmLock_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmPLock.hpp>

#if defined(ZDEBUG) && !defined(ZmLock_DEBUG)
#define ZmLock_DEBUG	// enable testing / debugging
#endif

#ifdef ZmLock_DEBUG
#include <zlib/ZmBackTracer.hpp>
#endif

template <class Lock> class ZmCondition;

#ifdef ZmLock_DEBUG
template <unsigned> class ZmBackTracer;
class ZmAPI ZmLock_Debug {
friend class ZmLock;
  static void enable();
  static void disable();
  static void capture(unsigned skip = 1);
  static ZmBackTracer<64> *tracer();
};
#endif /* ZmLock_DEBUG */

#ifdef _MSC_VER
#if ZmPLock_Recursive == 0 || defined(ZmLock_DEBUG)
template class ZmAPI ZmAtomic<uint32_t>;
template class ZmAPI ZmAtomic<ZmPlatform::ThreadID>;
#endif
#endif

class ZmLock {
  ZmLock(const ZmLock &);
  ZmLock &operator =(const ZmLock &);	// prevent mis-use

friend class ZmCondition<ZmLock>;

public:
  inline ZmLock()
#if ZmPLock_Recursive == 0 || defined(ZmLock_DEBUG)
    : m_count(0), m_tid(0)
#ifdef ZmLock_DEBUG
    , m_prevTID(0)
#endif
#endif
    { ZmPLock_init(m_lock); }
  inline ~ZmLock() { ZmPLock_final(m_lock); }

  inline void lock() {
#if ZmPLock_Recursive == 1 && !defined(ZmLock_DEBUG)
    ZmPLock_lock(m_lock);
#else
    ZmPlatform::ThreadID tid = ZmPlatform::getTID();
    if (m_tid == tid) { m_count++; return; }
#ifdef ZmLock_DEBUG
    if (m_prevTID && m_prevTID != tid)
      ZmLock_Debug::capture();
    m_prevTID = tid;
#endif
    ZmPLock_lock(m_lock);
    m_tid = tid;
    m_count = 1;
#endif
  }
  inline int trylock() {
#if ZmPLock_Recursive == 1 && !defined(ZmLock_DEBUG)
    return ZmPLock_trylock(m_lock);
#else
    ZmPlatform::ThreadID tid = ZmPlatform::getTID();
    if (m_tid == tid) { m_count++; return 0; }
    if (ZmPLock_trylock(m_lock)) return -1;
    m_tid = tid;				// remember we locked it
    m_count = 1;
    return 0;
#endif
  }
  inline void unlock() {
#if ZmPLock_Recursive == 1 && !defined(ZmLock_DEBUG)
    ZmPLock_unlock(m_lock);
#else
    if (!m_count) return;
    ZmPlatform::ThreadID tid = ZmPlatform::getTID();
    if (m_tid != tid) return;
    if (!--m_count) {
      m_tid = 0;
      ZmPLock_unlock(m_lock);
    }
#endif
  }

#ifdef ZmLock_DEBUG
  inline static void traceEnable() { ZmLock_Debug::enable(); }
  inline static void traceDisable() { ZmLock_Debug::disable(); }
  inline static ZmBackTracer<64> *tracer() { return ZmLock_Debug::tracer(); }
#endif

private:
#if ZmPLock_Recursive == 1 && !defined(ZmLock_DEBUG)
  class Wait { };

  inline Wait wait() { return Wait(); }
#else
  class Wait;
friend class Wait;
  class Wait {
  friend class ZmLock;
  private:
    inline Wait(ZmLock &lock) :
	m_lock(lock), m_count(lock.m_count), m_tid(lock.m_tid) {
      m_lock.m_count = 0;
      m_lock.m_tid = 0;
    }
  public:
    inline ~Wait() {
      m_lock.m_count = m_count;
      m_lock.m_tid = m_tid;
    }
  private:
    ZmLock			&m_lock;
    uint32_t			m_count;
    ZmPlatform::ThreadID	m_tid;
  };

  inline Wait wait() { return Wait(*this); }
#endif

  ZmPLock_				m_lock;
#if ZmPLock_Recursive == 0 || defined(ZmLock_DEBUG)
  ZmAtomic<uint32_t>			m_count;
  ZmAtomic<ZmPlatform::ThreadID>	m_tid;
#ifdef ZmLock_DEBUG
  ZmPlatform::ThreadID			m_prevTID;
#endif
#endif
};

#endif /* ZmLock_HPP */
