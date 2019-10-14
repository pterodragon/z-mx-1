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

// R/W lock

#ifndef ZmRWLock_HPP
#define ZmRWLock_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuStringN.hpp>

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmLockTraits.hpp>

#if defined(ZDEBUG) && !defined(ZmRWLock_DEBUG)
#define ZmRWLock_DEBUG	// enable testing / debugging
#endif

#ifndef _WIN32
#include <ck_rwlock.h>
#include <ck_pflock.h>

class ZmRWLock {
  ZmRWLock(const ZmRWLock &);
  ZmRWLock &operator =(const ZmRWLock &);	// prevent mis-use

friend class ZmRWLockTest;

public:
  inline ZmRWLock() { memset(&m_lock, 0, sizeof(ck_rwlock_recursive_t)); }

  inline void lock() {
    ck_rwlock_recursive_write_lock(&m_lock, ZmPlatform::getTID());
  }
  inline int trylock() {
    return ck_rwlock_recursive_write_trylock(
	&m_lock, ZmPlatform::getTID()) ? 0 : -1;
  }
  inline void unlock() {
    ck_rwlock_recursive_write_unlock(&m_lock);
  }

  inline void readlock() {
    ck_rwlock_recursive_read_lock(&m_lock);
  }
  inline int readtrylock() {
    return ck_rwlock_recursive_read_trylock(&m_lock) ? 0 : -1;
  } 
  inline void readunlock() {
    ck_rwlock_recursive_read_unlock(&m_lock);
  }

  template <typename S> inline void print(S &s) const {
    s << "writer=" << ZuBoxed(m_lock.rw.writer) <<
      " n_readers=" << ZuBoxed(m_lock.rw.n_readers) <<
      " wc=" << ZuBoxed(m_lock.wc);
  }

private:
  ck_rwlock_recursive_t	m_lock;
};

template <>
struct ZmLockTraits<ZmRWLock> : public ZmGenericLockTraits<ZmRWLock> {
  enum { RWLock = 1 };
  inline static void readlock(ZmRWLock &l) { l.readlock(); }
  inline static int readtrylock(ZmRWLock &l) { return l.readtrylock(); }
  inline static void readunlock(ZmRWLock &l) { l.readunlock(); }
};

class ZmPRWLock {
  ZmPRWLock(const ZmPRWLock &);
  ZmPRWLock &operator =(const ZmPRWLock &);	// prevent mis-use

friend class ZmRWLockTest;

public:
  inline ZmPRWLock() { ck_pflock_init(&m_lock); }

  inline void lock() { ck_pflock_write_lock(&m_lock); }
  inline void unlock() { ck_pflock_write_unlock(&m_lock); }

  inline void readlock() { ck_pflock_read_lock(&m_lock); }
  inline void readunlock() { ck_pflock_read_unlock(&m_lock); }

  template <typename S> inline void print(S &s) const {
    s << "rin=" << ZuBoxed(m_lock.rin) <<
      " rout=" << ZuBoxed(m_lock.rout) <<
      " win=" << ZuBoxed(m_lock.win) <<
      " lock=" << ZuBoxed(m_lock.wout);
  }

private:
  ck_pflock_t	m_lock;
};

template <>
struct ZmLockTraits<ZmPRWLock> : public ZmGenericLockTraits<ZmPRWLock> {
  enum { CanTry = 0, Recursive = 0, RWLock = 1 };
  inline static void readlock(ZmPRWLock &l) { l.readlock(); }
  inline static void readunlock(ZmPRWLock &l) { l.readunlock(); }
private:
  inline static int trylock(ZmPRWLock &);
  inline static int readtrylock(ZmPRWLock &);
};

#else /* !_WIN32 */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include <zlib/ZmAssert.hpp>

class ZmPRWLock { // non-recursive
  ZmPRWLock(const ZmPRWLock &);
  ZmPRWLock &operator =(const ZmPRWLock &);	// prevent mis-use

public:
  inline ZmPRWLock() { InitializeSRWLock(&m_lock); }

  inline void lock() { AcquireSRWLockExclusive(&m_lock); }
  inline void unlock() { ReleaseSRWLockExclusive(&m_lock); }
  inline int trylock() { return TryAcquireSRWLockExclusive(&m_lock) ? 0 : -1; }
  inline void readlock() { AcquireSRWLockShared(&m_lock); }
  inline void readunlock() { ReleaseSRWLockShared(&m_lock); }
  inline int readtrylock() { return TryAcquireSRWLockShared(&m_lock) ? 0 : -1; }

  template <typename S> inline void print(S &s) const {
    const uintptr_t *ZuMayAlias(ptr) = (const uintptr_t *)&m_lock;
    s << ZuBoxed(*ptr);
  }

private:
  SRWLOCK			  m_lock;
};

template <>
struct ZmLockTraits<ZmPRWLock> : public ZmGenericLockTraits<ZmPRWLock> {
  enum { Recursive = 0, RWLock = 1 };
  inline static void readlock(ZmPRWLock &l) { l.readlock(); }
  inline static int readtrylock(ZmPRWLock &l) { return l.readtrylock(); }
  inline static void readunlock(ZmPRWLock &l) { l.readunlock(); }
};

// Recursive RWLock

class ZmRWLock : public ZmPRWLock {
  ZmRWLock(const ZmRWLock &);
  ZmRWLock &operator =(const ZmRWLock &);	// prevent mis-use

public:
  inline ZmRWLock() : m_tid(0), m_count(0) { }

  inline void lock() {
    if (m_tid == ZmPlatform::getTID()) {
      ++m_count;
    } else {
      ZmPRWLock::lock();
      m_tid = ZmPlatform::getTID();
      ZmAssert(!m_count);
      ++m_count;
    }
  }
  inline int trylock() {
    if (m_tid == ZmPlatform::getTID()) {
      ++m_count;
      return 0;
    } else if (ZmPRWLock::trylock() == 0) {
      m_tid = ZmPlatform::getTID();
      ZmAssert(!m_count);
      ++m_count;
      return 0;
    }
    return -1;
  }
  inline void unlock() {
    if (!--m_count) {
      m_tid = 0;
      ZmPRWLock::unlock();
    }
  }

  inline void readlock() { ZmPRWLock::readlock(); }
  inline int readtrylock() { return ZmPRWLock::readtrylock(); }
  inline void readunlock() { ZmPRWLock::readunlock(); }

  template <typename S> inline void print(S &s) const {
    ZmPRWLock::print(s);
    s << " tid=" << ZuBox<ZmPlatform::ThreadID>(m_tid) <<
      " count=" << m_count;
  }

private:
  ZmAtomic<ZmPlatform::ThreadID>  	m_tid;
  int			      		m_count;
};

template <>
struct ZmLockTraits<ZmRWLock> : public ZmGenericLockTraits<ZmRWLock> {
  enum { RWLock = 1 };
  inline static void readlock(ZmRWLock &l) { l.readlock(); }
  inline static int readtrylock(ZmRWLock &l) { return l.readtrylock(); }
  inline static void readunlock(ZmRWLock &l) { l.readunlock(); }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* !_WIN32 */

template <> struct ZuPrint<ZmRWLock> : public ZuPrintFn { };
template <> struct ZuPrint<ZmPRWLock> : public ZuPrintFn { };

#endif /* ZmRWLock_HPP */
