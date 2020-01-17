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
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <initializer_list>

#include <stdlib.h>
#include <string.h>

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuArrayFn.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// #pragma warning(disable:4800)
#endif

struct ZuArray_ { };
template <typename T_, class Cmp_ = ZuCmp<T_> >
class ZuArray : public ZuArray_ {
public:
  typedef T_ T;
  typedef Cmp_ Cmp;
  typedef T Elem;
  typedef ZuArrayFn<T, Cmp> Ops;

  ZuArray() : m_data(0), m_length(0) { }
  ZuArray(const ZuArray &a) :
      m_data(a.m_data), m_length(a.m_length) { }
  ZuArray &operator =(const ZuArray &a) {
    if (this == &a) return *this;
    m_data = a.m_data;
    m_length = a.m_length;
    return *this;
  }

  ZuArray(std::initializer_list<T> a) :
    m_data(a.begin()), m_length(a.size()) { }
  ZuArray &operator =(std::initializer_list<T> a) {
    m_data = a.begin();
    m_length = a.size();
    return *this;
  }

private:
  template <typename U> struct IsPrimitiveArray_ {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK = ZuConversion<Elem, T>::Same &&
      ZuTraits<U>::IsArray && ZuTraits<U>::IsPrimitive };
  };
  template <typename U> struct IsCharElem_ {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK = 
      (ZuConversion<char, Elem>::Same || ZuConversion<wchar_t, Elem>::Same) };
  };
  template <typename U> struct IsStrLiteral {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK = IsPrimitiveArray_<U>::OK && IsCharElem_<U>::OK };
  };
  template <typename U> struct IsPrimitiveArray {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK = IsPrimitiveArray_<U>::OK && !IsCharElem_<U>::OK };
  };
  template <typename U> struct IsCString {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK = !IsPrimitiveArray_<U>::OK && ZuTraits<U>::IsPointer &&
      IsCharElem_<U>::OK };
  };
  template <typename U> struct IsOtherArray {
    typedef typename ZuTraits<U>::Elem Elem;
    enum { OK =
      !IsPrimitiveArray_<U>::OK && !IsCString<U>::OK &&
      (ZuTraits<U>::IsArray || ZuTraits<U>::IsString) &&
      ZuConversion<Elem, T>::Exists };
  };

  template <typename U, typename R = void, bool OK = IsStrLiteral<U>::OK>
  struct MatchStrLiteral;
  template <typename U, typename R>
  struct MatchStrLiteral<U, R, 1> { typedef R T; };
  template <typename U, typename R = void, bool OK = IsPrimitiveArray<U>::OK>
  struct MatchPrimitiveArray;
  template <typename U, typename R>
  struct MatchPrimitiveArray<U, R, 1> { typedef R T; };
  template <typename U, typename R = void, bool OK = IsCString<U>::OK>
  struct MatchCString;
  template <typename U, typename R>
  struct MatchCString<U, R, 1> { typedef R T; };
  template <typename U, typename R = void, bool OK = IsOtherArray<U>::OK>
  struct MatchOtherArray;
  template <typename U, typename R>
  struct MatchOtherArray<U, R, 1> { typedef R T; };

public:
  // compile-time length from string literal (null-terminated)
  template <typename A>
  ZuInline ZuArray(A &&a, typename MatchStrLiteral<A>::T *_ = 0) :
    m_data(&a[0]),
    m_length((ZuUnlikely(!(sizeof(a) / sizeof(a[0])) || !a[0])) ? 0U :
      (sizeof(a) / sizeof(a[0])) - 1U) { }
  template <typename A>
  ZuInline typename MatchStrLiteral<A, ZuArray &>::T operator =(A &&a) {
    m_data = &a[0];
    m_length = (ZuUnlikely(!(sizeof(a) / sizeof(a[0])) || !a[0])) ? 0U :
      (sizeof(a) / sizeof(a[0])) - 1U;
    return *this;
  }

  // compile-time length from primitive array
  template <typename A>
  ZuInline ZuArray(A &&a, typename MatchPrimitiveArray<A>::T *_ = 0) :
    m_data(&a[0]),
    m_length(sizeof(a) / sizeof(a[0])) { }
  template <typename A>
  ZuInline typename MatchPrimitiveArray<A, ZuArray &>::T operator =(A &&a) {
    m_data = &a[0];
    m_length = sizeof(a) / sizeof(a[0]);
    return *this;
  }

  // length from deferred strlen
  template <typename A>
  ZuInline ZuArray(A &&a, typename MatchCString<A>::T *_ = 0) :
	m_data(a), m_length(!a ? 0 : -1) { }
  template <typename A>
  ZuInline typename MatchCString<A, ZuArray &>::T operator =(A &&a) {
    m_data = a;
    m_length = !a ? 0 : -1;
    return *this;
  }

  // length from passed type
  template <typename A>
  ZuInline ZuArray(A &&a, typename MatchOtherArray<A>::T *_ = 0) :
	m_data(ZuTraits<A>::data(a)),
	m_length(!m_data ? 0 : (int)ZuTraits<A>::length(a)) { }
  template <typename A>
  ZuInline typename MatchOtherArray<A, ZuArray &>::T operator =(A &&a) {
    m_data = ZuTraits<A>::data(a);
    m_length = !m_data ? 0 : (int)ZuTraits<A>::length(a);
    return *this;
  }

  ZuInline ZuArray(const T *data, unsigned length) :
      m_data(data), m_length(length) { }

  ZuInline const T *data() const { return m_data; }
  ZuInline unsigned length() const { return length_<T>(); }

private:
  template <typename Char>
  ZuInline typename ZuIfT<
    ZuConversion<Char, char>::Same ||
    ZuConversion<Char, wchar_t>::Same, unsigned>::T length_() const {
    if (ZuUnlikely(m_length < 0))
      return const_cast<ZuArray *>(this)->m_length =
	ZuTraits<const T *>::length(m_data);
    return m_length;
  }
  template <typename Char>
  ZuInline typename ZuIfT<
    !ZuConversion<Char, char>::Same &&
    !ZuConversion<Char, wchar_t>::Same, unsigned>::T length_() const {
    return m_length;
  }

public:
  ZuInline const T &operator [](int i) const { return m_data[i]; }

  // operator T *() must return nullptr if the string is empty, oherwise
  // these usages stop working:
  // if (ZuString s = "") { } else { puts("ok"); }
  // if (ZuString s = 0) { } else { puts("ok"); }
  ZuInline operator const T *() const {
    return !length() ? (const T *)nullptr : m_data;
  }

  ZuInline bool operator !() const { return !length(); }

  ZuInline void offset(unsigned n) {
    if (!n) return;
    if (n <= length()) m_data += n, m_length -= n;
  }

protected:
  ZuInline bool same(const ZuArray &v) const { return this == &v; }
  template <typename V> ZuInline bool same(const V &v) const { return false; }

public:
  template <typename V> ZuInline int cmp(const V &v_) const {
    if (same(v_)) return 0;
    ZuArray v{v_};
    int64_t l = length();
    int64_t n = v.length();
    unsigned m = (l > n) ? n : l;
    if (int i = Ops::cmp(data(), v.data(), m)) return i;
    return l - n;
  }
  template <typename V> ZuInline bool equals(const V &v_) const {
    if (same(v_)) return true;
    ZuArray v{v_};
    unsigned l = length();
    unsigned n = v.length();
    if (l != n) return false;
    return Ops::equals(data(), v.data(), l);
  }

  template <typename V>
  ZuInline bool operator ==(const V &v) const { return equals(v); }
  template <typename V>
  ZuInline bool operator !=(const V &v) const { return !equals(v); }
  template <typename V>
  ZuInline bool operator >(const V &v) const { return cmp(v) > 0; }
  template <typename V>
  ZuInline bool operator >=(const V &v) const { return cmp(v) >= 0; }
  template <typename V>
  ZuInline bool operator <(const V &v) const { return cmp(v) < 0; }
  template <typename V>
  ZuInline bool operator <=(const V &v) const { return cmp(v) <= 0; }

  ZuInline uint32_t hash() const { return ZuHash<ZuArray>::hash(*this); }

private:
  const T	*m_data;
  int		m_length;	// -ve used to defer calculation 
};

template <typename T> class ZuArray_Null {
  ZuInline const T *data() const { return nullptr; }
  ZuInline unsigned length() const { return 0; }

  ZuInline T operator [](int i) const { return ZuCmp<T>::null(); }

  ZuInline bool operator !() const { return true; }

  ZuInline void offset(unsigned n) { }
};

template <typename Cmp>
class ZuArray<ZuNull, Cmp> : public ZuArray_Null<ZuNull> {
public:
  typedef ZuNull Elem;

  ZuInline ZuArray() { }
  ZuInline ZuArray(const ZuArray &a) { }
  ZuInline ZuArray &operator =(const ZuArray &a) { }

  template <typename A> ZuInline ZuArray(const A &a, typename ZuIfT<
    ZuTraits<A>::IsArray && ZuConversion<
      typename ZuTraits<A>::Elem, ZuNull>::Exists>::T *_ = 0) { }
  template <typename A> ZuInline typename ZuIfT<
    ZuTraits<A>::IsArray &&
    ZuConversion<typename ZuTraits<A>::Elem, ZuNull>::Exists, ZuArray &>::T
      operator =(const A &a) { return *this; }

  ZuInline ZuArray(const ZuNull *data, unsigned length) { }
};

template <typename Cmp>
class ZuArray<void, Cmp> : public ZuArray_Null<void> {
public:
  typedef void Elem;

  ZuInline ZuArray() { }
  ZuInline ZuArray(const ZuArray &a) { }
  ZuInline ZuArray &operator =(const ZuArray &a) { }

  template <typename A> ZuInline ZuArray(const A &a, typename ZuIfT<
    ZuTraits<A>::IsArray && ZuConversion<
      typename ZuTraits<A>::Elem, void>::Exists>::T *_ = 0) { }
  template <typename A> ZuInline typename ZuIfT<
    ZuTraits<A>::IsArray &&
    ZuConversion<typename ZuTraits<A>::Elem, void>::Exists, ZuArray &>::T
      operator =(const A &a) { return *this; }

  ZuInline ZuArray(const void *data, unsigned length) { }
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
#if 0
  static T make(const Elem *data, unsigned length) {
    if (!data) return T();
    return T(data, length);
  }
#endif
  static const Elem *data(const T &a) { return a.data(); }
  static unsigned length(const T &a) { return a.length(); }
};

template <> struct ZuPrint<ZuArray<char> > : public ZuPrintString { };
template <> struct ZuPrint<ZuArray<const char> > : public ZuPrintString { };
template <> struct ZuPrint<ZuArray<volatile char> > : public ZuPrintString { };
template <>
struct ZuPrint<ZuArray<const volatile char> > : public ZuPrintString { };

template <typename T> using ZuArrayT = ZuArray<typename ZuTraits<T>::Elem>;

template <typename T>
ZuArrayT<T> ZuMkArray(T &&t) {
  return ZuArrayT<T>(ZuFwd<T>(t));
}
template <typename T>
ZuArray<T> ZuMkArray(std::initializer_list<T> &&t) {
  return ZuArray<T>(ZuFwd<std::initializer_list<T> >(t));
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArray_HPP */
