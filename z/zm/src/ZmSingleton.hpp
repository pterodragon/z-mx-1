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

// Phoenix Singleton

// ZmSingleton<T>::instance() returns T * pointer
//
// ZmCleanup<T>::Level determines order of destruction (per ZmCleanupLevel)
//
// ZmSingleton<T, 0>::instance() can return null since T will not be
// constructed on-demand - use ZmSingleton<T, 0>::instance(new T(...))
//
// T can be ZmObject-derived, but does not have to be

#ifndef ZmSingleton_HPP
#define ZmSingleton_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuCmp.hpp>
#include <ZuCan.hpp>

#include <ZmRef.hpp>
#include <ZmCleanup.hpp>
#include <ZmGlobal.hpp>

extern "C" {
  ZmExtern void ZmSingleton_ctor();
  ZmExtern void ZmSingleton_dtor();
}

template <typename T, bool IsObject = ZuIsObject_<T>::OK> struct ZmSingleton_ {
  ZuInline void ref(T *p) { ZmREF(p); }
  ZuInline void deref(T *p) { ZmDEREF(p); }
};
template <typename T> struct ZmSingleton_<T, 0> {
  ZuInline static void ref(T *) { }
  ZuInline static void deref(T *p) { delete p; }
};

template <typename T_, bool Construct_ = 1>
class ZmSingleton : public ZmGlobal, public ZmSingleton_<T_> {
  ZmSingleton(const ZmSingleton &);
  ZmSingleton &operator =(const ZmSingleton &);	// prevent mis-use

public:
  typedef T_ T;

private:
  ZuCan(final, CanFinal);
  template <typename U> inline static typename
    ZuIfT<CanFinal<ZmCleanup<U>, void (*)(U *)>::OK>::T
      final(U *u) { ZmCleanup<U>::final(u); }
  template <typename U> inline static typename
    ZuIfT<!CanFinal<ZmCleanup<U>, void (*)(U *)>::OK>::T
      final(U *u) { }

  template <bool Construct = Construct_>
  inline typename ZuIfT<Construct>::T ctor() {
    T *ptr = new T();
    this->ref(ptr);
    m_instance = ptr;
  }
  template <bool Construct = Construct_>
  inline typename ZuIfT<!Construct>::T ctor() { }

public:
  inline ZmSingleton() {
#ifdef ZDEBUG
    ZmSingleton_ctor();
#endif
    ctor<>();
  };

  inline ~ZmSingleton() {
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

  inline T *instance_(T *ptr) {
    this->ref(ptr);
    if (T *old = m_instance.xch(ptr)) {
      final(old);
      this->deref(old);
    }
    return ptr;
  }

public:
  inline static T *instance() {
    return global()->m_instance.load_();
  }
  inline static T *instance(T *ptr) {
    return global()->instance_(ptr);
  }
};

#endif /* ZmSingleton_HPP */
