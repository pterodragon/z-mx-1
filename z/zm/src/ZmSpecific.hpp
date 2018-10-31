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

// Phoenix TLS with sequenced cleanup and iteration over all instances

// ZmSpecific<T>::instance() returns T * pointer
//
// ZmSpecific<T>::all(ZmFn<T *> fn) calls fn for all instances of T
//
// ZmCleanup<T>::Level determines order of destruction (per ZmCleanupLevel)
//
// ZmSpecific<T, 0>::instance() can return null since T will not be
// constructed on-demand - use ZmSpecific<T, 0>::instance(new T(...))
//
// T must be ZmObject-derived

#ifndef ZmSpecific_HPP
#define ZmSpecific_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#ifndef _WIN32
#include <alloca.h>
#endif

#include <stdio.h>

#include <ZuCmp.hpp>
#include <ZuConversion.hpp>
#include <ZuCan.hpp>
#include <ZuPair.hpp>
#include <ZuIfT.hpp>

#include <ZmRef.hpp>
#include <ZmObject.hpp>
#include <ZmFn_.hpp>
#include <ZmCleanup.hpp>
#include <ZmGlobal.hpp>
#include <ZmStack_.hpp>
#include <ZmPLock.hpp>

#ifdef _WIN32
extern "C" {
  typedef void (*ZmSpecific_cleanup_Fn)(void *);
  ZmExtern void ZmSpecific_cleanup();
  ZmExtern void ZmSpecific_cleanup_add(ZmSpecific_cleanup_Fn fn, void *ptr);
  ZmExtern void ZmSpecific_cleanup_del(void *ptr);
}
#endif

// can be specialized to override initial/increment sizes for objects container
template <class T> struct ZmSpecificTraits {
  enum { Initial = 32, Increment = 32 };
};

template <typename T, bool Construct = true> struct ZmSpecificCtor {
  static T *fn() { return new T(); }
};
template <typename T> struct ZmSpecificCtor<T, false> {
  static T *fn() { return nullptr; }
};
template <class T_, bool Construct_ = true,
  auto CtorFn = ZmSpecificCtor<T_, Construct_>::fn>
class ZmSpecific : public ZmGlobal {
  ZmSpecific(const ZmSpecific &);
  ZmSpecific &operator =(const ZmSpecific &);	// prevent mis-use

public:
  typedef T_ T;

private:
  typedef ZmStack<T *, ZmStackLock<ZmPLock> > Stack;

  ZuCan(final, CanFinal);
  template <typename U> inline static typename
    ZuIfT<CanFinal<ZmCleanup<U>, void (*)(U *)>::OK, void>::T
      final(U *u) { ZmCleanup<U>::final(u); }
  template <typename U> inline static typename
    ZuIfT<!CanFinal<ZmCleanup<U>, void (*)(U *)>::OK, void>::T
      final(U *u) { }

  typedef ZmSpecificTraits<T> Traits;

public:
  inline ZmSpecific() : m_objects(ZmStackParams().
      initial(Traits::Initial).increment(Traits::Increment)) { }
  inline ~ZmSpecific() {
    while (T *ptr = m_objects.pop()) {
      final(ptr); // calls ZmCleanup<T>::final() if it exists
      ZmDEREF(ptr);
    }
  }

private:
  typedef T *Ptr;

  ZuInline static Ptr &local_() {
    thread_local Ptr ptr = 0;
    return ptr;
  }

  inline T *create_() { return add__(CtorFn()); }
  template <typename Factory>
  inline T *create_(Factory &&factory) {
    return add__(ZuFwd<Factory>(factory)());
  }

  inline T *instance_(T *ptr) {
    if (T *old = local_()) {
#ifdef _WIN32
      ZmSpecific_cleanup_del((void *)old);
#endif
      cleanup__(old);
    }
    return add__(ptr);
  }

  inline T *add__(T *ptr) {
    ZmREF(ptr);
    Ptr &local = local_();
    local = ptr;
    m_objects.push(ptr);
#ifdef _WIN32
    ZmSpecific_cleanup_add(&ZmSpecific<T>::cleanup_, (void *)ptr);
#endif
    // m_key.set((void *)ptr);
    return ptr;
  }

  inline void cleanup__(T *ptr) {
    if (ZuUnlikely(!ptr)) return;
    m_objects.del(ptr);
    final(ptr); // calls ZmCleanup<T>::final() if available
    ZmDEREF(ptr);
  }

  static void cleanup_(void *t_) { global()->cleanup__((T *)t_); }

  inline void all_(ZmFn<T *> fn) {
    all_2(&fn);
  }

  void all_2(ZmFn<T *> *fn) {
    unsigned n = m_objects.count();

    if (ZuUnlikely(!n)) return;

    T **objects = 0;
#ifdef __GNUC__
    objects = (T **)alloca(sizeof(T *) * n);
#endif
#ifdef _MSC_VER
    __try {
      objects = (T **)_alloca(sizeof(T *) * n);
    } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
      _resetstkoflw();
      objects = 0;
    }
#endif

    if (ZuUnlikely(!objects)) return;

    memset(objects, 0, sizeof(T *) * n);
    all_3(objects, n, fn);
  }

  void all_3(T **objects, unsigned n, ZmFn<T *> *fn) {
    {
      typename Stack::Iterator i(m_objects);
      for (unsigned j = 0; j < n; j++)
	if (ZuLikely(objects[j] = i.iterate()))
	  ZmREF(objects[j]);
    }

    for (unsigned j = 0; j < n; j++)
      if (ZuLikely(objects[j])) {
	(*fn)(objects[j]);
	ZmDEREF(objects[j]);
      }
  }

  Stack		m_objects;

  ZuInline static ZmSpecific *global() {
    return ZmGlobal::global<ZmSpecific>();
  }

public:
  template <bool Construct = Construct_>
  ZuInline static typename ZuIfT<Construct, T *>::T create() {
    return global()->create_();
  }
  template <bool Construct = Construct_>
  ZuInline static typename ZuIfT<!Construct, T *>::T create() { return 0; }
  ZuInline static T *instance() {
    T *ptr;
    if (ZuLikely(ptr = local_())) return ptr;
    return create<>();
  }
  inline static T *instance(T *ptr) {
    return global()->instance_(ptr);
  }

  inline static void all(ZmFn<T *> fn) { return global()->all_(fn); }
};

template <typename L> struct ZmTLS_Ctor {
  static auto fn() { return (*(L *)(void *)0)(); }
  typedef decltype(fn()) (*Fn)();
  enum { OK = ZuConversion<L, Fn>::Exists };
};
template <typename L>
ZuInline auto ZmTLS(L l, typename ZuIfT<ZmTLS_Ctor<L>::OK>::T *_ = 0) {
  typedef typename ZuDeref<decltype(*ZmTLS_Ctor<L>::fn())>::T T;
  return ZmSpecific<T, true, ZmTLS_Ctor<L>::fn>::instance();
}

#endif /* ZmSpecific_HPP */
