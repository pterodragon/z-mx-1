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

// intrusively reference-counted smart pointer

#ifndef ZuRef_HPP
#define ZuRef_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuConversion.hpp>

// rules for using ZuRef

// * always point to objects allocated using new
// * always point to objects which derive from ZuObject
// * be careful to maintain a positive reference count when mixing
//   with real pointers - objects will delete themselves from
//   under you if they think they are referenced by nothing!
// * pass by raw pointer and return by ZuRef value

struct ZuRef_ { }; // compile-time tag

template <typename T_> class ZuRef : public ZuRef_ {
template <typename> friend class ZuRef;
  struct Private { };

public:
  typedef T_ T;

private:
  // matches ZuRef<U> where U is not T, but is in the same type hierarchy as T
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
    enum { OK = ZuConversion<ZuRef_, U>::Base };
  };
  template <typename U, typename R = void, bool OK = IsOtherRef1<U>::OK>
  struct MatchOtherRef;
  template <typename U, typename R>
  struct MatchOtherRef<U, R, true> : public MatchOtherRef2<U, R> { };

  // matches ZuRef<U> where U is either T or in the same type hierarchy as T
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
    enum { OK = ZuConversion<ZuRef_, U>::Base };
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
  inline ZuRef() : m_object(0) { }
  inline ZuRef(const ZuRef &r) : m_object(r.m_object) {
    if (T *o = m_object) o->ref();
  }
  inline ZuRef(ZuRef &&r) noexcept : m_object(r.m_object) {
    r.m_object = 0;
  }
  template <typename R>
  inline ZuRef(const R &r, typename MatchOtherRef<R, Private>::T *_ = 0) :
      m_object(static_cast<T *>(const_cast<typename R::T *>(r.m_object))) {
    if (T *o = m_object) o->ref();
  }
  inline ZuRef(T *o) : m_object(o) {
    if (o) o->ref();
  }
  template <typename O>
  inline ZuRef(O *o, typename MatchPtr<O, Private>::T *_ = 0) :
      m_object(static_cast<T *>(o)) {
    if (o) o->ref();
  }
  inline ~ZuRef() {
    if (T *o = m_object) if (o->deref()) delete o;
  }

  template <typename R> inline typename MatchRef<R>::T swap(R &r) noexcept {
    T *o = m_object;
    m_object = static_cast<T *>(r.m_object);
    r.m_object = static_cast<typename R::T *>(o);
  }

  template <typename R>
  friend inline typename MatchRef<R>::T swap(ZuRef &r1, R &r2) noexcept {
    r1.swap(r2);
  }

  inline ZuRef &operator =(ZuRef r) noexcept {
    swap(r);
    return *this;
  }
  template <typename R>
  inline typename MatchOtherRef<R, ZuRef &>::T operator =(R r) {
    swap(r);
    return *this;
  }

  template <typename O>
  inline typename MatchPtr<O, ZuRef &>::T operator =(O *n) {
    if (n) n->ref();
    T *o = m_object;
    m_object = n;
    if (o && o->deref()) delete o;
    return *this;
  }

  ZuInline operator T *() const { return m_object; }
  ZuInline T *operator ->() const { return m_object; }

  template <typename O>
  ZuInline typename MatchOtherRef<ZuRef<O>, O *>::T as() const {
    return static_cast<O *>(m_object);
  }

  ZuInline T *ptr() const { return m_object; }
  T *ptr_() const { return m_object; }

  ZuInline bool operator !() const { return !m_object; }

protected:
  T		*m_object;
};

template <typename T_>
struct ZuTraits<ZuRef<T_> > : public ZuTraits<T_ *> {
  enum { IsPrimitive = 0, IsPOD = 0 };
  typedef ZuRef<T_> T;
};

template <typename T> struct ZuCmp;
template <typename T>
struct ZuCmp<ZuRef<T> > : public ZuCmp<T *> {
  inline static bool null(const ZuRef<T> &r) {
    return !r;
  }
  inline static const ZuRef<T> &null() {
    static const ZuRef<T> r;
    return r;
  }
};

template <typename T> struct ZuHash;
template <typename T>
struct ZuHash<ZuRef<T> > : public ZuHash<T *> { };

template <typename T> ZuRef<T> ZuMkRef(T *p) { return ZuRef<T>(p); }

#endif /* ZuRef_HPP */
