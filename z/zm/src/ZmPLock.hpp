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

// fast platform-native mutex
// - primitive (potentially non-recursive), cannot be used with ZmCondition

#ifndef ZmPLock_HPP
#define ZmPLock_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmLockTraits.hpp>
#include <zlib/ZmAtomic.hpp>

// Linux x86 - uses Nick Piggin's 2008 reference implementation of ticket
// spinlocks for the Linux kernel, see http://lwn.net/Articles/267968/

// Haswell Core i7 @3.6GHz results (-O6 -mtune=haswell, single NUMA package):
// ZmPLock lock+unlock is ~10.67ns uncontended
// pthread lock+unlock is ~15ns uncontended
// ck ticket lock+unlock is ~10.66ns uncontended
// ck FAS lock+unlock is ~9.5ns uncontended
// contended lock+unlock is generally 40-60ns, within which:
//   performance ranking w/ 2 threads: ck FAS, ck ticket, ZmPLock, pthread
//   performance ranking w/ 3 threads: ck FAS, pthread, ck ticket, ZmPLock
// Note: FAS is unfair, risks starvation/livelock
//
// In general, we optimize for the uncontended case, while moderating
// the penalty of the contended case; furthermore the most common use-case
// in Z is assumed to be 2 threads rather than 3+

#ifndef _WIN32
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define ZmPLock_Recursive 0
typedef uint32_t ZmPLock_;
#define ZmPLock_init(m) m = 0
#define ZmPLock_final(m) (void)0
ZuInline void ZmPLock_lock_(ZmPLock_ &m) {
  int i = 0x00010000, j;
  __asm__ __volatile__(	"lock; xaddl %0, %1\n\t"
			"movzwl %w0, %2\n\t"
			"shrl $16, %0\n"
			"1:\tcmpl %0, %2\n\t"
			"je 2f\n\t"
			"rep; nop\n\t"
			"movzwl %1, %2\n\t"
			"jmp 1b\n"
			"2:"
			: "+r" (i), "+m" (m), "=&r" (j)
			: : "memory", "cc");
}
#define ZmPLock_lock(m) ZmPLock_lock_(m)
ZuInline bool ZmPLock_trylock_(ZmPLock_ &m) {
  int i, j;
  __asm__ __volatile__(	"movl %2,%0\n\t"
			"movl %0,%1\n\t"
			"roll $16, %0\n\t"
			"cmpl %0,%1\n\t"
#ifdef __x86_64__
			"leal 0x00010000(%q0), %1\n\t"
#else
			"leal 0x00010000(%k0), %1\n\t"
#endif
			"jne 1f\n\t"
			"lock; cmpxchgl %1,%2\n"
			"1:\tsete %b1\n\t"
			"movzbl %b1,%0\n\t"
			: "=&a" (i), "=&q" (j), "+m" (m)
			: : "memory", "cc");
  return !i;
}
#define ZmPLock_trylock(m) ZmPLock_trylock_(m)
ZuInline void ZmPLock_unlock_(ZmPLock_ &m) {
  __asm__ __volatile__(	"lock; incw %0"
			: "+m" (m)
			: : "memory", "cc");
}
#define ZmPLock_unlock(m) ZmPLock_unlock_(m) 
#else
#define ZmPLock_Recursive 1
typedef pthread_mutex_t ZmPLock_;
struct ZmPLock_Attr {
  ZmPLock_Attr() {
    pthread_mutexattr_init(&m_value);
    pthread_mutexattr_settype(&m_value, PTHREAD_MUTEX_RECURSIVE);
  }
  ~ZmPLock_Attr() {
    pthread_mutexattr_destroy(&m_value);
  }
  pthread_mutexattr_t	m_value;
  static pthread_mutexattr_t *value() {
    static ZmPLock_Attr instance;
    return &instance.m_value;
  }
};
#define ZmPLock_init(m) pthread_mutex_init(&m, ZmPLock_Attr::value())
#define ZmPLock_final(m) pthread_mutex_destroy(&m)
#define ZmPLock_lock(m) pthread_mutex_lock(&m)
#define ZmPLock_trylock(m) (!pthread_mutex_trylock(&m))
#define ZmPLock_unlock(m) pthread_mutex_unlock(&m)
#endif
#else
#define ZmPLock_Recursive 1
typedef CRITICAL_SECTION ZmPLock_;
#define ZmPLock_init(m) \
  InitializeCriticalSectionAndSpinCount(&m, 0x10000)
#define ZmPLock_final(m) DeleteCriticalSection(&m)
#define ZmPLock_lock(m) EnterCriticalSection(&m)
#define ZmPLock_trylock(m) TryEnterCriticalSection(&m)
#define ZmPLock_unlock(m) LeaveCriticalSection(&m)
#endif

class ZmPLock {
  ZmPLock(const ZmPLock &);
  ZmPLock &operator =(const ZmPLock &);	// prevent mis-use

public:
  ZuInline ZmPLock() { ZmPLock_init(m_lock); }
  ZuInline ~ZmPLock() { ZmPLock_final(m_lock); }

  ZuInline void lock() { ZmPLock_lock(m_lock); }
  ZuInline int trylock() { return -!ZmPLock_trylock(m_lock); }
  ZuInline void unlock() { ZmPLock_unlock(m_lock); }

private:
  ZmPLock_		m_lock;
};

template <>
struct ZmLockTraits<ZmPLock> : public ZmGenericLockTraits<ZmPLock> {
  enum { CanTry = 1, Recursive = ZmPLock_Recursive, RWLock = 0 };
};

#endif /* ZmPLock_HPP */
