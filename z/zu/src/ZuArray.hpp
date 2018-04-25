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

// ZuArray<T> is a const rvalue wrapper around a pointer+length pair
// T is the element type and the data elements are const; it performs no
// memory allocation
//
// ZuArrayT<T> is a syntactic short cut for ZuArray<typename ZuTraits<T>::Elem>
//
// ZuMkArray(x) returns a ZuArray<> for x, where x can be any Zu traited type
// or an initializer list

#ifndef ZuArray_HPP
#define ZuArray_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <initializer_list>

#include <stdlib.h>
#include <string.h>

#include <ZuTraits.hpp>
#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuConversion.hpp>
#include <ZuPrint.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// #pragma warning(disable:4800)
#endif

struct ZuArray_ { };
template <typename T_, class Cmp = ZuCmp<T_> >
class ZuArray : public ZuArray_ {
  struct Private { };

public:
  typedef T_ T;
  typedef T Elem;

  inline ZuArray() : m_data(0), m_length(0) { }
  inline ZuArray(const ZuArray &a) :
      m_data(a.m_data), m_length(a.m_length) { }
  inline ZuArray &operator =(const ZuArray &a) {
    if (this == &a) return *this;
    m_data = a.m_data;
    m_length = a.m_length;
    return *this;
  }

  inline ZuArray(std::initializer_list<T> a) :
    m_data(a.begin()), m_length(a.size()) { }
  inline ZuArray &operator =(std::initializer_list<T> a) {
    m_data = a.begin();
    m_length = a.size();
    return *this;
  }

private:
  template <typename U> struct IsStrLiteral {
    enum { OK = ZuTraits<U>::IsArray && ZuTraits<U>::IsPrimitive &&
      ZuConversion<typename ZuTraits<U>::Elem, T>::Same };
  };
  template <typename U> struct IsCString {
    typedef typename ZuTraits<U>::Elem Char;
    enum { OK = !IsStrLiteral<U>::OK &&
      ZuTraits<U>::IsPointer && ZuConversion<Char, T>::Same &&
      (ZuConversion<char, Char>::Same || ZuConversion<wchar_t, Char>::Same) };
  };
  template <typename U> struct IsOtherArray {
    enum { OK = !IsStrLiteral<U>::OK && !IsCString<U>::OK &&
      (ZuTraits<U>::IsArray || ZuTraits<U>::IsString) &&
      ZuConversion<typename ZuTraits<U>::Elem, T>::Exists };
  };

  template <typename U, typename R = void, bool OK = IsStrLiteral<U>::OK>
  struct FromStrLiteral;
  template <typename U, typename R>
  struct FromStrLiteral<U, R, 1> { typedef R T; };
  template <typename U, typename R = void, bool OK = IsCString<U>::OK>
  struct FromCString;
  template <typename U, typename R>
  struct FromCString<U, R, 1> { typedef R T; };
  template <typename U, typename R = void, bool OK = IsOtherArray<U>::OK>
  struct FromOtherArray;
  template <typename U, typename R>
  struct FromOtherArray<U, R, 1> { typedef R T; };

public:
  // compile-time length from primitive array / string literal
  template <typename A>
  ZuInline ZuArray(A &&a, typename FromStrLiteral<A, Private>::T *_ = 0) :
    m_data(&a[0]),
    m_length((ZuUnlikely(!(sizeof(a) / sizeof(a[0])) || !a[0])) ? 0U :
      (sizeof(a) / sizeof(a[0])) - 1U) { }
  template <typename A>
  ZuInline typename FromStrLiteral<A, ZuArray &>::T operator =(A &&a) {
    m_data = &a[0];
    m_length = !sizeof(a) ? 0U : !a[0] ? 0U : (sizeof(a) / sizeof(a[0])) - 1U;
    m_length = (ZuUnlikely(!(sizeof(a) / sizeof(a[0])) || !a[0])) ? 0U :
      (sizeof(a) / sizeof(a[0])) - 1U;
    return *this;
  }

  // length from deferred strlen
  template <typename A>
  ZuInline ZuArray(A &&a, typename FromCString<A, Private>::T *_ = 0) :
	m_data(a), m_length(!a ? 0 : -1) { }
  template <typename A>
  ZuInline typename FromCString<A, ZuArray &>::T operator =(A &&a) {
    m_data = a;
    m_length = !a ? 0 : -1;
    return *this;
  }

  // length from passed type
  template <typename A>
  ZuInline ZuArray(A &&a, typename FromOtherArray<A, Private>::T *_ = 0) :
	m_data(ZuTraits<A>::data(a)),
	m_length(!m_data ? 0 : (int)ZuTraits<A>::length(a)) { }
  template <typename A>
  ZuInline typename FromOtherArray<A, ZuArray &>::T operator =(A &&a) {
    m_data = ZuTraits<A>::data(a);
    m_length = !m_data ? 0 : (int)ZuTraits<A>::length(a);
    return *this;
  }

  inline ZuArray(const T *data, unsigned length) :
      m_data(data), m_length(length) { }

  inline const T *data() const { return m_data; }
  inline unsigned length() const { return length_<T>(); }

private:
  template <typename Char>
  inline typename ZuIfT<
    ZuConversion<Char, char>::Same ||
    ZuConversion<Char, wchar_t>::Same, unsigned>::T length_() const {
    if (ZuUnlikely(m_length < 0))
      return const_cast<ZuArray *>(this)->m_length =
	ZuTraits<const T *>::length(m_data);
    return m_length;
  }
  template <typename Char>
  inline typename ZuIfT<
    !ZuConversion<Char, char>::Same &&
    !ZuConversion<Char, wchar_t>::Same, unsigned>::T length_() const {
    return m_length;
  }

public:
  inline const T &operator [](int i) const { return m_data[i]; }

  // operator T *() must return 0 if the string is empty, oherwise
  // these usages stop working:
  // if (ZuString s = "") { } else { puts("ok"); }
  // if (ZuString s = 0) { } else { puts("ok"); }
  inline operator const T *() const {
    return !length_<T>() ? (const T *)0 : m_data;
  }

  inline bool operator !() const { return !length_<T>(); }

  inline void offset(unsigned n) {
    if (!n) return;
    if (n <= length_<T>()) m_data += n, m_length -= n;
  }

protected:
  inline bool same(const ZuArray &v) const { return this == &v; }
  template <typename V> inline bool same(const V &v) const { return false; }

public:
  template <typename V> inline int cmp(const V &v) const {
    if (same(v)) return 0;
    return ZuCmp<ZuArray>::cmp(*this, v);
  }
  template <typename V> inline bool equals(const V &v) const {
    if (same(v)) return true;
    return ZuCmp<ZuArray>::equals(*this, v);
  }

  template <typename V>
  inline bool operator ==(const V &v) const { return equals(v); }
  template <typename V>
  inline bool operator !=(const V &v) const { return !equals(v); }
  template <typename V>
  inline bool operator >(const V &v) const { return cmp(v) > 0; }
  template <typename V>
  inline bool operator >=(const V &v) const { return cmp(v) >= 0; }
  template <typename V>
  inline bool operator <(const V &v) const { return cmp(v) < 0; }
  template <typename V>
  inline bool operator <=(const V &v) const { return cmp(v) <= 0; }

  inline uint32_t hash() const { return ZuHash<ZuArray>::hash(*this); }

private:
  const T	*m_data;
  int		m_length;	// -ve used to defer calculation 
};

template <typename T> class ZuArray_Null {
  inline const T *data() const { return 0; }
  inline unsigned length() const { return 0; }

  inline T operator [](int i) const { return ZuCmp<T>::null(); }

  inline bool operator !() const { return true; }

  inline void offset(unsigned n) { }
};

template <typename Cmp>
class ZuArray<ZuNull, Cmp> : public ZuArray_Null<ZuNull> {
  struct Private { };

public:
  typedef ZuNull Elem;

  inline ZuArray() { }
  inline ZuArray(const ZuArray &a) { }
  inline ZuArray &operator =(const ZuArray &a) { }

  template <typename A> inline ZuArray(const A &a, typename ZuIfT<
    ZuTraits<A>::IsArray && ZuConversion<
      typename ZuTraits<A>::Elem, ZuNull>::Exists, Private>::T *_ = 0) { }
  template <typename A> inline typename ZuIfT<
    ZuTraits<A>::IsArray &&
    ZuConversion<typename ZuTraits<A>::Elem, ZuNull>::Exists, ZuArray &>::T
      operator =(const A &a) { return *this; }

  inline ZuArray(const ZuNull *data, unsigned length) { }
};

template <typename Cmp>
class ZuArray<void, Cmp> : public ZuArray_Null<void> {
  struct Private { };

public:
  typedef void Elem;

  inline ZuArray() { }
  inline ZuArray(const ZuArray &a) { }
  inline ZuArray &operator =(const ZuArray &a) { }

  template <typename A> inline ZuArray(const A &a, typename ZuIfT<
    ZuTraits<A>::IsArray && ZuConversion<
      typename ZuTraits<A>::Elem, void>::Exists, Private>::T *_ = 0) { }
  template <typename A> inline typename ZuIfT<
    ZuTraits<A>::IsArray &&
    ZuConversion<typename ZuTraits<A>::Elem, void>::Exists, ZuArray &>::T
      operator =(const A &a) { return *this; }

  inline ZuArray(const void *data, unsigned length) { }
};

template <typename Elem_>
struct ZuTraits<ZuArray<Elem_> > : public ZuGenericTraits<ZuArray<Elem_> > {
  typedef Elem_ Elem;
  typedef ZuArray<Elem_> T;
  enum {
    IsArray = 1, IsPrimitive = 0,
    IsString =
      ZuConversion<char, Elem>::Same ||
      ZuConversion<wchar_t, Elem>::Same,
    IsWString = ZuConversion<wchar_t, Elem>::Same,
    IsHashable = 1, IsComparable = 1
  };
  inline static T make(const Elem *data, unsigned length) {
    if (!data) return T();
    return T(data, length);
  }
  inline static const Elem *data(const T &a) { return a.data(); }
  inline static unsigned length(const T &a) { return a.length(); }
};

template <> struct ZuPrint<ZuArray<char> > :
  public ZuPrintString<ZuArray<char> > { };

template <typename T> using ZuArrayT = ZuArray<typename ZuTraits<T>::Elem>;

template <typename T>
inline ZuArrayT<T> ZuMkArray(T &&t) {
  return ZuArrayT<T>(ZuFwd<T>(t));
}
template <typename T>
inline ZuArray<T> ZuMkArray(std::initializer_list<T> &&t) {
  return ZuArray<T>(ZuFwd<std::initializer_list<T> >(t));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArray_HPP */
