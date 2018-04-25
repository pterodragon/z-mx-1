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

// generic mixin, an implementation of Objective C's categories

// this reduces boilerplate when mixing-in class hierarchies, by
// automatically providing forwarding constructors and assignment operators;
// the mixin will also inherit type, comparison and hash traits

// WARNING: if the mixin has any virtual functions or data members, undefined
// behavior (memory corruption, run-time exceptions, crashes) will result

// class Class { ... };	// this is the class to be extended
//
// template <typename T> class Foo : public T {
//   inline void foo() { T::ptr()->bar(42); } // adds a foo() method
// };
//
// typedef ZuMixin<Class, Foo> Mixed;	// Class, with Foo mixed in
//
// Mixed m;			// calls Class::Class()
// Mixed n(42, 43, "bah")	// calls Class::Class(...)
// m = n;			// calls Class::operator =(const Class &)
// m = "baz";			// calls Class::operator =(...)
// m.foo();			// calls Class::bar(42)

#ifndef ZuMixin_HPP
#define ZuMixin_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuNull.hpp>
#include <ZuAssert.hpp>

template <class, template <class> class> class ZuMixin;

template <class T, class M> class ZuMixin_ {
template <class, template <class> class> friend class ZuMixin;
  struct Mixed : public T, public M {
    ZuInline static const T *offset(const void *ptr) {
      return
	static_cast<const T *>(
	  static_cast<const Mixed *>(
	    static_cast<const M *>(ptr)));
    }
    ZuInline static T *offset(void *ptr) {
      return
	static_cast<T *>(
	  static_cast<Mixed *>(
	    static_cast<M *>(ptr)));
    }
  };
public:
  ZuInline const T *ptr() const {
    return Mixed::offset(static_cast<const void *>(this));
  }
  ZuInline T *ptr() {
    return Mixed::offset(static_cast<void *>(this));
  }
};

template <class T_, template <class> class M_>
class ZuMixin : public T_, public M_<ZuMixin_<T_, M_<ZuNull> > > {
public:
  typedef T_ T;
  typedef M_<ZuMixin_<T_, M_<ZuNull> > > M;

  ZuInline ZuMixin() { }
  ZuInline ZuMixin(const ZuMixin &t) : T(t) { }
  ZuInline ZuMixin &operator =(const ZuMixin &t) {
    if (this != &t) T::operator =(t);
    return *this;
  }
  ZuInline ZuMixin(ZuMixin &&t) : T(static_cast<T &&>(t)) { }
  ZuInline ZuMixin &operator =(ZuMixin &&t) {
    T::operator =(static_cast<T &&>(t));
    return *this;
  }

  template <typename P> ZuInline ZuMixin(P &&p) : T(ZuFwd<P>(p)) { }
  template <typename P> ZuInline ZuMixin &operator =(P &&p) {
    T::operator =(ZuFwd<P>(p));
    return *this;
  }

  template <typename ...Args>
  ZuInline ZuMixin(Args &&... args) : T(ZuFwd<Args>(args)...) { }
};

template <typename T> struct ZuTraits;
template <class T_, template <class> class M_>
struct ZuTraits<ZuMixin<T_, M_> > : public ZuTraits<T_> {
  typedef ZuMixin<T_, M_> T;
};

template <typename T> struct ZuCmp;
template <class T_, template <class> class M_>
struct ZuCmp<ZuMixin<T_, M_> > : public ZuCmp<T_> {
  template <typename M>
  ZuInline static bool null(const M &m) { return ZuCmp<T_>::null(m); }
  ZuInline static const ZuMixin<T_, M_> &null() {
    static const ZuMixin<T_, M_> m = ZuCmp<T_>::null();
    return m;
  }
};

#define ZuMixinAlias(T, fn, oldFn) \
  template <typename ...Args> \
  ZuInline auto fn(Args &&... args) { \
    return T::ptr()->oldFn(ZuFwd<Args>(args)...); \
  } \
  template <typename ...Args> \
  ZuInline auto fn(Args &&... args) const { \
    return T::ptr()->oldFn(ZuFwd<Args>(args)...); \
  } \
  template <typename ...Args> \
  ZuInline auto fn(Args &&... args) volatile { \
    return T::ptr()->oldFn(ZuFwd<Args>(args)...); \
  } \
  template <typename ...Args> \
  ZuInline auto fn(Args &&... args) const volatile { \
    return T::ptr()->oldFn(ZuFwd<Args>(args)...); \
  }

template <typename T> struct ZuHash;
template <class T_, template <class> class M_>
struct ZuHash<ZuMixin<T_, M_> > : public ZuHash<T_> { };

template <typename T> struct ZuPrint;
template <class T_, template <class> class M_>
struct ZuPrint<ZuMixin<T_, M_> > : public ZuPrint<T_> { };

#endif /* ZuMixin_HPP */
