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

// Singleton with deterministic destruction sequencing

// ZmSingleton<T>::instance() returns T * pointer
//
// ZmCleanup<T>::Level determines order of destruction (per ZmCleanupLevel)
//
// ZmSingleton<T, 0>::instance() can return null since T will not be
// constructed on-demand - use ZmSingleton<T, 0>::instance(new T(...))
//
// T can be ZmObject-derived, but does not have to be
//
// static T v; can be replaced with:
// auto &v = *ZmSingleton<T>::instance();
// auto &v = ZmStatic([]() { return new T(); });
//
// static T v(args); can be replaced with:
// auto &v = ZmStatic([]() { return new T(args...); });

#ifndef ZmSingleton_HPP
#define ZmSingleton_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuCmp.hpp>
#include <zlib/ZuCan.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuIfT.hpp>

#include <zlib/ZmRef.hpp>
#include <zlib/ZmCleanup.hpp>
#include <zlib/ZmGlobal.hpp>

extern "C" {
  ZmExtern void ZmSingleton_ctor();
  ZmExtern void ZmSingleton_dtor();
}

template <typename T, bool IsObject = ZuIsObject_<T>::OK> struct ZmSingleton_ {
  ZuInline void ref(T *p) { ZmREF(p); }
  ZuInline void deref(T *p) { ZmDEREF(p); }
};
template <typename T> struct ZmSingleton_<T, false> {
  ZuInline static void ref(T *) { }
  ZuInline static void deref(T *p) { delete p; }
};

template <typename T, bool Construct = true> struct ZmSingletonCtor {
  static T *fn() { return new T(); }
};
template <typename T> struct ZmSingletonCtor<T, false> {
  static T *fn() { return nullptr; }
};
template <typename T_, bool Construct_ = true,
  auto CtorFn = ZmSingletonCtor<T_, Construct_>::fn>
class ZmSingleton : public ZmGlobal, public ZmSingleton_<T_> {
  ZmSingleton(const ZmSingleton &);
  ZmSingleton &operator =(const ZmSingleton &);	// prevent mis-use

public:
  typedef T_ T;

private:
  ZuCan(final, CanFinal);
  template <typename U> ZuInline static typename
    ZuIfT<CanFinal<ZmCleanup<U>, void (*)(U *)>::OK>::T
      final(U *u) { ZmCleanup<U>::final(u); }
  template <typename U> ZuInline static typename
    ZuIfT<!CanFinal<ZmCleanup<U>, void (*)(U *)>::OK>::T
      final(U *u) { }

  template <bool Construct = Construct_>
  typename ZuIfT<Construct>::T ctor() {
    T *ptr = CtorFn();
    this->ref(ptr);
    m_instance = ptr;
  }
  template <bool Construct = Construct_>
  typename ZuIfT<!Construct>::T ctor() { }

public:
  ZmSingleton() {
#ifdef ZDEBUG
    ZmSingleton_ctor();
#endif
    ctor<>();
  };

  ~ZmSingleton() {
#ifdef ZDEBUG
    ZmSingleton_dtor();
#endif
    if (T *ptr = m_instance.load_()) {
      final(ptr); // calls ZmCleanup<T>::final() if it exists
      this->deref(ptr);
    }
  }

private:
  ZmAtomic<T *>	m_instance;

  ZuInline static ZmSingleton *global() {
    return ZmGlobal::global<ZmSingleton>();
  }

  T *instance_(T *ptr) {
    this->ref(ptr);
    if (T *old = m_instance.xch(ptr)) {
      final(old);
      this->deref(old);
    }
    return ptr;
  }

public:
  static T *instance() {
    return global()->m_instance.load_();
  }
  static T *instance(T *ptr) {
    return global()->instance_(ptr);
  }
};

template <typename L> struct ZmStatic_Ctor {
  static auto fn() { return (*(L *)(void *)0)(); }
  typedef decltype(fn()) (*Fn)();
  enum { OK = ZuConversion<L, Fn>::Exists };
};
template <typename L>
ZuInline auto &ZmStatic(L l,
    typename ZuIfT<ZmStatic_Ctor<L>::OK>::T *_ = 0) {
  typedef typename ZuDeref<decltype(*ZmStatic_Ctor<L>::fn())>::T T;
  return *(ZmSingleton<T, true, ZmStatic_Ctor<L>::fn>::instance());
}

#endif /* ZmSingleton_HPP */
