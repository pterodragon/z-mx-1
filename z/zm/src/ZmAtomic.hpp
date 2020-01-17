//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// atomic operations

#ifndef ZmAtomic_HPP
#define ZmAtomic_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#pragma warning(push)
#pragma warning(disable:4996 4311 4312)
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuAssert.hpp>

// Memory Barriers
// ---------------
// gcc (including MinGW-w64) -
//   >= 4.7 - use atomic builtins
// Visual Studio -
//   >= 2013 - use atomic intrinsics
#ifdef __GNUC__
#define ZmAtomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_RELAXED)
#define ZmAtomic_store(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_RELAXED)
#define ZmAtomic_acquire() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define ZmAtomic_release() __atomic_thread_fence(__ATOMIC_RELEASE)
#endif

#ifdef _MSC_VER
#include <atomic>
#define ZmAtomic_load(ptr) \
  (reinterpret_cast<const std::atomic<decltype(*ptr)> *>(ptr)->load( \
	std::memory_order_relaxed))
#define ZmAtomic_store(ptr, val) \
  (reinterpret_cast<std::atomic<decltype(*ptr)> *>(ptr)->store( \
	val, std::memory_order_relaxed))
#define ZmAtomic_acquire() std::atomic_thread_fence(std::memory_order_acquire)
#define ZmAtomic_release() std::atomic_thread_fence(std::memory_order_release)
#endif

// Atomic Operations (compare and exchange, etc.)
// -----------------
// non-Windows (gcc required) -
//   gcc >= 4.3 - use sync builtins
//   gcc <  4.3 - old-school inline assembler
// Windows -
//   gcc (MinGW or MinGW-w64) - old-school inline assembler
//   Visual Studio - _Interlocked intrinsics
#ifdef __GNUC__
#if ZmAtomic_GccVersion >= 40300 && !defined _WIN32
#define ZmAtomic_GccBuiltins32
#define ZmAtomic_GccBuiltins64
#if defined(__i386__) && !defined(__x86_64__)
#undef ZmAtomic_GccBuiltins64
#endif
#endif
#endif

template <typename Int, int Size> struct ZmAtomicOps;

// 32bit atomic operations
template <typename Int32> struct ZmAtomicOps<Int32, 4> {
  typedef int32_t S;
  typedef uint32_t U;

  ZuInline static Int32 load_(const Int32 *ptr) {
    return ZmAtomic_load(ptr);
  }
  ZuInline static Int32 load(const Int32 *ptr) {
    Int32 i = ZmAtomic_load(ptr);
    ZmAtomic_acquire();
    return i;
  }
  ZuInline static void store_(Int32 *ptr, Int32 value) {
    ZmAtomic_store(ptr, value);
  }
  ZuInline static void store(Int32 *ptr, Int32 value) {
    ZmAtomic_release();
    ZmAtomic_store(ptr, value);
  }

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  ZuInline static Int32 atomicXch(volatile Int32 *ptr, Int32 value) {
#ifdef ZmAtomic_GccBuiltins32
    return __sync_lock_test_and_set(ptr, value);
#else
    Int32 old;

    __asm__ __volatile__(	"xchgl %0, %1"
				: "=r" (old), "=m" (*ptr)
				: "0" (value), "m" (*ptr));
    return old;
#endif
  }

  ZuInline static Int32 atomicXchAdd(volatile Int32 *ptr, Int32 value) {
#ifdef ZmAtomic_GccBuiltins32
    return __sync_fetch_and_add(ptr, value);
#else
    Int32 old;

    __asm__ __volatile__(	"lock; xaddl %0, %1"
				: "=r" (old), "=m" (*ptr)
				: "0" (value), "m" (*ptr));
    return old;
#endif
  }

  ZuInline static Int32 atomicCmpXch(
	volatile Int32 *ptr, Int32 value, Int32 cmp) {
#ifdef ZmAtomic_GccBuiltins32
    return __sync_val_compare_and_swap(ptr, cmp, value);
#else
    Int32 old;

    __asm__ __volatile__(	"lock; cmpxchgl %2, %1"
				: "=a" (old), "=m" (*ptr)
				: "r" (value), "m" (*ptr), "0" (cmp) : "cc");
    return old;
#endif
  }
#endif /* GNUC && (i386 || x86_64) */

#if defined(_WIN32) && defined(_MSC_VER)
  ZuInline static Int32 atomicXch(volatile Int32 *ptr, Int32 value) {
    return _InterlockedExchange((volatile long *)ptr, value);
  }

  ZuInline static Int32 atomicCmpXch(
      volatile Int32 *ptr, Int32 value, Int32 cmp) {
    return _InterlockedCompareExchange((volatile long *)ptr, value, cmp);
  }

  ZuInline static Int32 atomicXchAdd(volatile Int32 *ptr, int32_t value) {
    return _InterlockedExchangeAdd((volatile long *)ptr, value);
  }
#endif /* _WIN32 && _MSC_VER */
};

// 64bit atomic operations
template <typename Int64> struct ZmAtomicOps<Int64, 8> {
  typedef int64_t S;
  typedef uint64_t U;

  ZuInline static Int64 load_(const Int64 *ptr) {
    return ZmAtomic_load(ptr);
  }
  ZuInline static Int64 load(const Int64 *ptr) {
    Int64 i = ZmAtomic_load(ptr);
    ZmAtomic_acquire();
    return i;
  }
  ZuInline static void store_(Int64 *ptr, Int64 value) {
    ZmAtomic_store(ptr, value);
  }
  ZuInline static void store(Int64 *ptr, Int64 value) {
    ZmAtomic_release();
    ZmAtomic_store(ptr, value);
  }

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
  ZuInline static Int64 atomicXch(volatile Int64 *ptr, Int64 value) {
#ifdef ZmAtomic_GccBuiltins64
    return __sync_lock_test_and_set(ptr, value);
#else
    Int64 old;

#ifdef __x86_64__
    __asm__ __volatile__(	"xchgq %0, %1"
				: "=q" (old), "=m" (*ptr)
				: "0" (value), "m" (*ptr));
#else
    do { old = *ptr; } while (atomicCmpXch(ptr, value, old) != old);
#endif
    return old;
#endif
  }

  ZuInline static Int64 atomicXchAdd(volatile Int64 *ptr, Int64 value) {
#ifdef ZmAtomic_GccBuiltins64
    return __sync_fetch_and_add(ptr, value);
#else
    Int64 old;

#ifdef __x86_64__
    __asm__ __volatile__(	"lock; xaddq %0, %1"
				: "=q" (old), "=m" (*ptr)
				: "0" (value), "m" (*ptr));
#else
    do { old = *ptr; } while (atomicCmpXch(ptr, old + value, old) != old);
#endif
    return old;
#endif
  }

  ZuInline static Int64 atomicCmpXch(
      volatile Int64 *ptr, Int64 value, Int64 cmp) {
#ifdef ZmAtomic_GccBuiltins64
    return __sync_val_compare_and_swap(ptr, cmp, value);
#else
    Int64 old;

#ifdef __x86_64__
    __asm__ __volatile__(	"lock; cmpxchgq %2, %1"
				: "=a" (old), "=m" (*ptr)
				: "r" (value), "m" (*ptr), "0" (cmp) : "cc");
#else
#if __PIC__
    __asm__ __volatile__(	"pushl %%ebx;"
				"movl %2, %%ebx;"
				"lock; cmpxchg8b %1;"
				"pop %%ebx"
				: "=A" (old), "=m" (*ptr)
				: "r" ((uint32_t)value),
				  "c" ((uint32_t)(value>>32)),
				  "m" (*ptr), "0" (cmp) : "cc");
#else
    __asm__ __volatile__(	"lock; cmpxchg8b %1"
				: "=A" (old), "=m" (*ptr)
				: "b" ((uint32_t)value),
				  "c" ((uint32_t)(value>>32)),
				  "m" (*ptr), "0" (cmp) : "cc");
#endif
#endif
    return old;
#endif
  }
#endif /* GNUC && (i386 || x86_64) */

#if defined(_WIN32) && defined(_MSC_VER)
#ifdef _WIN64
  ZuInline static Int64 atomicXch(volatile Int64 *ptr, Int64 value) {
    return _InterlockedExchange64((volatile long long *)ptr, value);
  }

  ZuInline static Int64 atomicXchAdd(volatile Int64 *ptr, int64_t value) {
    return _InterlockedExchangeAdd64((volatile long long *)ptr, value);
  }
#else
  ZuInline static Int64 atomicXch(volatile Int64 *ptr, Int64 value) {
    Int64 old;

    do {
      old = *ptr;
    } while (_InterlockedCompareExchange64(
	  (volatile long long *)ptr, old, old) != old);
    return old;
  }

  ZuInline static Int64 atomicXchAdd(volatile Int64 *ptr, int64_t value) {
    Int64 old;

    do {
      old = *ptr;
    } while (_InterlockedCompareExchange64(
	  (volatile long long *)ptr, old + value, old) != old);
    return old;
  }
#endif

  ZuInline static Int64 atomicCmpXch(
      volatile Int64 *ptr, Int64 value, Int64 cmp) {
    return _InterlockedCompareExchange64((volatile long long *)ptr, value, cmp);
  }
#endif /* _WIN32 && _MSC_VER */
};

template <typename T> class ZmAtomic {
  ZuAssert(ZuTraits<T>::IsPrimitive && ZuTraits<T>::IsIntegral);

public:
  typedef ZmAtomicOps<T, sizeof(T)> Ops;

private:
  typedef typename Ops::S S;
  typedef typename Ops::U U;

public:
  ZuInline ZmAtomic() : m_val(0) { };

  // store/relaxed when first creating new objects
  ZuInline ZmAtomic(const ZmAtomic &a) {
    Ops::store_(&m_val, Ops::load(&a.m_val));
  };
  ZuInline ZmAtomic(T val) {
    Ops::store_(&m_val, val);
  };

  // store/release (release before store)
  ZuInline ZmAtomic &operator =(const ZmAtomic &a) {
    Ops::store(&m_val, Ops::load(&a.m_val));
    return *this;
  }
  ZuInline ZmAtomic &operator =(T val) {
    Ops::store(&m_val, val);
    return *this;
  }

  // store/relaxed
  ZuInline void store_(T val) { Ops::store_(&m_val, val); }

  // load/acquire (acquire after load)
  ZuInline operator T() const { return Ops::load(&m_val); }

  // load/relaxed
  ZuInline T load_() const { return Ops::load_(&m_val); }

  ZuInline T xch(T val) { return Ops::atomicXch(&m_val, val); }
  ZuInline T xchAdd(T val) { return Ops::atomicXchAdd(&m_val, val); }
  ZuInline T xchSub(T val) { return Ops::atomicXchAdd(&m_val, -val); }
  ZuInline T cmpXch(T val, T cmp) {
    return Ops::atomicCmpXch(&m_val, val, cmp);
  }

  ZuInline T operator ++() { return Ops::atomicXchAdd(&m_val, 1) + 1; }
  ZuInline T operator --() { return Ops::atomicXchAdd(&m_val, -1) - 1; }

  ZuInline T operator ++(int) { return Ops::atomicXchAdd(&m_val, 1); }
  ZuInline T operator --(int) { return Ops::atomicXchAdd(&m_val, -1); }

  ZuInline T operator +=(S val) {
    return Ops::atomicXchAdd(&m_val, val) + val;
  }
  ZuInline T operator -=(S val) {
    return Ops::atomicXchAdd(&m_val, -val) - val;
  }

  ZuInline T xchOr(T val) {
    T old;
    do {
      if (((old = m_val) & val) == val) return old;
    } while (Ops::atomicCmpXch(&m_val, old | val, old) != old);
    return old;
  }
  ZuInline T xchAnd(T val) {
    T old;
    do {
      if (!((old = m_val) & ~val)) return old;
    } while (Ops::atomicCmpXch(&m_val, old & val, old) != old);
    return old;
  }

  ZuInline T operator |=(T val) { return xchOr(val) | val; }
  ZuInline T operator &=(T val) { return xchAnd(val) & val; }

  ZuInline T minimum(T val) {
    T old;
    do {
      if ((old = m_val) <= val) return old;
    } while (Ops::atomicCmpXch(&m_val, val, old) != old);
    return val;
  }
  ZuInline T maximum(T val) {
    T old;
    do {
      if ((old = m_val) >= val) return old;
    } while (Ops::atomicCmpXch(&m_val, val, old) != old);
    return val;
  }

private:
  T	m_val;
};

template <typename T> class ZmAtomic<T *> {
public:
  typedef ZmAtomicOps<uintptr_t, sizeof(T *)> Ops;

private:
  typedef typename Ops::S S;
  typedef typename Ops::U U;

public:
  ZuInline ZmAtomic() : m_val(0) { }
 
  // store/relaxed when first creating new objects
  ZuInline ZmAtomic(const ZmAtomic &a) {
    Ops::store_(&m_val, Ops::load(&a.m_val));
  }
  ZuInline ZmAtomic(T *val) {
    Ops::store_(&m_val, (uintptr_t)val);
  }

  // store/release (release before store)
  ZuInline ZmAtomic &operator =(const ZmAtomic &a) {
    Ops::store(&m_val, Ops::load(&a.m_val));
    return *this;
  }
  ZuInline ZmAtomic &operator =(T *val) {
    Ops::store(&m_val, (uintptr_t)val);
    return *this;
  }

  // store/relaxed
  ZuInline void store_(T *val) { Ops::store_(&m_val, (U)val); }

  // load/acquire (acquire after load)
  ZuInline operator T *() const { return (T *)Ops::load(&m_val); }
  ZuInline T *operator ->() const { return (T *)Ops::load(&m_val); }

  // load/relaxed
  ZuInline T *load_() const { return (T *)Ops::load_(&m_val); }

  ZuInline T *xch(T *val) { return (T *)Ops::atomicXch(&m_val, (U)val); }
  ZuInline T *xchAdd(S val) { return (T *)Ops::atomicXchAdd(&m_val, val); }
  ZuInline T *cmpXch(T *val, T *cmp) {
    return (T *)Ops::atomicCmpXch(&m_val, (U)val, (U)cmp);
  }

  ZuInline T *operator ++() {
    return (T *)Ops::atomicXchAdd(&m_val, sizeof(T)) + sizeof(T);
  }
  ZuInline T *operator --() {
    return (T *)Ops::atomicXchAdd(&m_val, -sizeof(T)) - sizeof(T);
  }

  ZuInline T *operator ++(int) {
    return (T *)Ops::atomicXchAdd(&m_val, sizeof(T));
  }
  ZuInline T *operator --(int) {
    return (T *)Ops::atomicXchAdd(&m_val, -sizeof(T));
  }

  ZuInline T *operator +=(S val) {
    val *= sizeof(T);
    return (T *)Ops::atomicXchAdd(&m_val, val) + val;
  }
  ZuInline T *operator -=(S val) {
    val *= sizeof(T);
    return (T *)Ops::atomicXchAdd(&m_val, -val) - val;
  }

private:
  uintptr_t	m_val;
};

template <typename T_> struct ZuTraits<ZmAtomic<T_> > : public ZuTraits<T_> {
  typedef ZmAtomic<T_> T;
  enum { IsPrimitive = 0 };
};

template <typename T>
ZuInline ZmAtomic<T> &ZmAtomize(T &v) {
  ZmAtomic<T> *ZuMayAlias(ptr) = (ZmAtomic<T> *)&v;
  return *ptr;
}
template <typename T>
ZuInline const ZmAtomic<T> &ZmAtomize(const T &v) {
  const ZmAtomic<T> *ZuMayAlias(ptr) = (const ZmAtomic<T> *)&v;
  return *ptr;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmAtomic_HPP */
