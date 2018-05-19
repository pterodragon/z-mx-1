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

// intrusively reference-counted smart pointer class

#ifndef ZmRef_HPP
#define ZmRef_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuObject_.hpp>
#include <ZuRef.hpp>

// rules for using ZmRef

// * always point to objects allocated using new (use ZmHeap to optimize)
// * always point to objects which derive from ZmObject_<bool Atomic>
// * be careful to maintain a positive reference count when mixing
//   with real pointers - objects will delete themselves from
//   under you if they think they are referenced by nothing!
// * pass by raw pointer and return by ZmRef value

struct ZmRef_ { }; // compile-time tag

struct ZmObject_;
template <typename> class ZmRef;

#ifdef ZmObject_DEBUG
struct ZmRef__ {
  template <typename O> ZuInline static typename ZuIs<ZmObject_, O>::T
  ZmREF_(const O *o, const void *p) { o->ref(p); }
  template <typename O> ZuInline static typename ZuIs<ZmObject_, O>::T
  ZmREF_(const ZmRef<O> &o, const void *p) { o->ref(p); }
  template <typename O> ZuInline static typename ZuIs<ZmObject_, O>::T
  ZmDEREF_(const O *o, const void *p) { if (o->deref(p)) delete o; }
  template <typename O> ZuInline static typename ZuIs<ZmObject_, O>::T
  ZmDEREF_(const ZmRef<O> &o, const void *p)
    { if (o->deref(p)) delete o.ptr(); }
  template <typename O> ZuInline static typename ZuIs<ZmObject_, O>::T
  ZmMVREF_(const O *o, const void *p, const void *n) { o->mvref(p, n); }
  template <typename O> ZuInline static void
  ZmMVREF_(const ZmRef<O> &o, const void *p, const void *n) { o->mvref(p, n); }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmREF_(const O *o, const void *) { o->ref(); }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmREF_(const ZmRef<O> &o, const void *) { o->ref(); }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmDEREF_(const O *o, const void *) { if (o->deref()) delete o; }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmDEREF_(const ZmRef<O> &o, const void *) { if (o->deref()) delete o.ptr(); }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmMVREF_(const O *, const void *, const void *) { }
  template <typename O> ZuInline static typename ZuIsNot<ZmObject_, O>::T
  ZmMVREF_(const ZmRef<O> &, const void *, const void *) { }
};
#define ZmREF(o) ZmRef__::ZmREF_((o), this)
#define ZmDEREF(o) ZmRef__::ZmDEREF_((o), this)
#define ZmMVREF(o, p, n) ZmRef__::ZmMVREF_((o), (p), (n))
#else
struct ZmRef__ {
  template <typename O> ZuInline static void
  ZmDEREF_(const O *o) { if (o->deref()) delete o; }
  template <typename O> ZuInline static void
  ZmDEREF_(const ZmRef<O> &o) { if (o->deref()) delete o.ptr(); }
};
#define ZmREF(o) ((o)->ref())
#define ZmDEREF(o) ZmRef__::ZmDEREF_(o)
#define ZmMVREF(o, p, n) ((void)0)
#endif

template <typename T_> class ZmRef : public ZuRef_, public ZmRef_ {
template <typename> friend class ZmRef;
  struct Private { };

public:
  typedef T_ T;

private:
  // matches ZmRef<U> where U is not T, but is in the same type hierarchy as T
  template <typename U> struct IsOtherRef2 {
    enum { OK =
      (ZuConversion<T, typename U::T>::Base ||
       ZuConversion<typename U::T, T>::Base) };
  };
  template <typename U, typename R = void, bool OK = IsOtherRef2<U>::OK>
  struct MatchOtherRef2 { };
  template <typename U, typename R>
  struct MatchOtherRef2<U, R, true> { typedef R T; };
  template <typename U> struct IsOtherRef1 {
    enum { OK = ZuConversion<ZmRef_, U>::Base };
  };
  template <typename U, typename R = void, bool OK = IsOtherRef1<U>::OK>
  struct MatchOtherRef;
  template <typename U, typename R>
  struct MatchOtherRef<U, R, true> : public MatchOtherRef2<U, R> { };

  // matches ZmRef<U> where U is either T or in the same type hierarchy as T
  template <typename U> struct IsRef2 {
    enum { OK =
      (ZuConversion<T, typename U::T>::Is ||
       ZuConversion<typename U::T, T>::Is) };
  };
  template <typename U, typename R = void, bool OK = IsRef2<U>::OK>
  struct MatchRef2 { };
  template <typename U, typename R>
  struct MatchRef2<U, R, true> { typedef R T; };
  template <typename U> struct IsRef1 {
    enum { OK = ZuConversion<ZmRef_, U>::Base };
  };
  template <typename U, typename R = void, bool OK = IsRef1<U>::OK>
  struct MatchRef;
  template <typename U, typename R>
  struct MatchRef<U, R, true> : public MatchRef2<U, R> { };

  // matches U * where U is either T or in the same type hierarchy as T
  template <typename U> struct IsPtr {
    enum { OK = (ZuConversion<T, U>::Is || ZuConversion<U, T>::Is) };
  };
  template <typename U, typename R = void, bool OK = IsPtr<U>::OK>
  struct MatchPtr;
  template <typename U, typename R>
  struct MatchPtr<U, R, true> { typedef R T; };

public:
  inline ZmRef() : m_object(0) { }
  inline ZmRef(const ZmRef &r) : m_object(r.m_object) {
    if (T *o = m_object) ZmREF(o);
  }
  inline ZmRef(ZmRef &&r) noexcept : m_object(r.m_object) {
    r.m_object = 0;
#ifdef ZmObject_DEBUG
    if (T *o = m_object) ZmMVREF(o, &r, this);
#endif
  }
  template <typename R>
  inline ZmRef(const R &r, typename MatchOtherRef<R, Private>::T *_ = 0) :
      m_object(static_cast<T *>(const_cast<typename R::T *>(r.m_object))) {
    if (T *o = m_object) ZmREF(o);
  }
  inline ZmRef(T *o) : m_object(o) {
    if (o) ZmREF(o);
  }
  template <typename O>
  inline ZmRef(O *o, typename MatchPtr<O, Private>::T *_ = 0) :
      m_object(static_cast<T *>(o)) {
    if (o) ZmREF(o);
  }
  inline ~ZmRef() {
    if (T *o = m_object) ZmDEREF(o);
  }

  template <typename R> inline typename MatchRef<R>::T swap(R &r) noexcept {
    T *o = m_object;
    m_object = static_cast<T *>(r.m_object);
    r.m_object = static_cast<typename R::T *>(o);
#ifdef ZmObject_DEBUG
    if (o) ZmMVREF(o, this, &r);
    if (m_object) ZmMVREF(m_object, &r, this);
#endif
  }

  template <typename R>
  friend inline typename MatchRef<R>::T swap(ZmRef &r1, R &r2) noexcept {
    r1.swap(r2);
  }

  inline ZmRef &operator =(ZmRef r) noexcept {
    swap(r);
    return *this;
  }
  template <typename R>
  inline typename MatchOtherRef<R, ZmRef &>::T operator =(R r) {
    swap(r);
    return *this;
  }

  template <typename O>
  inline typename MatchPtr<O, ZmRef &>::T operator =(O *n) {
    if (n) ZmREF(n);
    T *o = m_object;
    m_object = n;
    if (o) ZmDEREF(o);
    return *this;
  }

  inline operator T *() const { return m_object; }
  inline T *operator ->() const { return m_object; }

  template <typename O>
  inline typename MatchOtherRef<ZmRef<O>, O *>::T as() const {
    return static_cast<O *>(m_object);
  }

  inline T *ptr() const { return m_object; }
  T *ptr_() const { return m_object; }

  inline bool operator !() const { return !m_object; }

protected:
  T		*m_object;
};

template <typename T_>
struct ZuTraits<ZmRef<T_> > : public ZuTraits<T_ *> {
  enum { IsPrimitive = 0, IsPOD = 0 };
  typedef ZmRef<T_> T;
};

template <typename T> struct ZuCmp;
template <typename T>
struct ZuCmp<ZmRef<T> > : public ZuCmp<T *> {
  inline static bool null(const ZmRef<T> &r) {
    return !r;
  }
  inline static const ZmRef<T> &null() {
    static const ZmRef<T> r;
    return r;
  }
};

template <typename T> struct ZuHash;
template <typename T>
struct ZuHash<ZmRef<T> > : public ZuHash<T *> { };

template <typename T> ZmRef<T> ZmMkRef(T *p) { return ZmRef<T>(p); }

#endif /* ZmRef_HPP */
