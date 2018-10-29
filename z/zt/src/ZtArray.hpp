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

// heap-allocated dynamic array class

#ifndef ZtArray_HPP
#define ZtArray_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <ZtLib.hpp>
#endif

#include <initializer_list>

#include <stdlib.h>
#include <string.h>

#include <ZuNew.hpp>

#include <ZuIfT.hpp>
#include <ZuInt.hpp>
#include <ZuNull.hpp>
#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuArrayFn.hpp>
#include <ZuUTF.hpp>
#include <ZuPrint.hpp>
#include <ZuBox.hpp>

#include <ZmAssert.hpp>

#include <ZtPlatform.hpp>
#include <ZtIconv.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4348)
#endif

template <typename T, class Cmp> class ZtArray;

template <typename T> struct ZtArray_ { };

template <typename T_> struct ZtArray_Char2 { typedef ZuNull T; };
template <> struct ZtArray_Char2<char> { typedef wchar_t T; };
template <> struct ZtArray_Char2<wchar_t> { typedef char T; };

template <typename T_, class Cmp_ = ZuCmp<T_> >
class ZtArray : public ZtArray_<T_>, public ZuArrayFn<T_, Cmp_> {
public:
  typedef T_ T;
  typedef Cmp_ Cmp;
  typedef ZuArrayFn<T, Cmp> Ops;
  enum {
    IsString = ZuConversion<T, char>::Same || ZuConversion<T, wchar_t>::Same,
    IsWString = ZuConversion<T, wchar_t>::Same
  };

private:
  typedef T Char;
  typedef typename ZtArray_Char2<T>::T Char2;

  // from same type ZtArray
  template <typename U, typename R = void, typename V = T,
    bool A = ZuConversion<ZtArray_<V>, U>::Base> struct MatchZtArray;
  template <typename U, typename R>
    struct MatchZtArray<U, R, T, true> { typedef R T; };

  // from another array type with convertible element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists
    > struct MatchArray;
  template <typename U, typename R>
    struct MatchArray<U, R, T, true> { typedef R T; };

  // from another array type with same element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same
    > struct MatchSameArray;
  template <typename U, typename R>
    struct MatchSameArray<U, R, T, true> { typedef R T; };

  // from another array type with convertible element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      !ZuConversion<typename ZuTraits<U>::Elem, V>::Same &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists
    > struct MatchDiffArray;
  template <typename U, typename R>
    struct MatchDiffArray<U, R, T, true> { typedef R T; };

  // from string literal with same char
  template <typename U, typename V = Char> struct IsStrLiteral {
    enum { OK = ZuTraits<U>::IsCString &&
      ZuTraits<U>::IsArray && ZuTraits<U>::IsPrimitive &&
      ZuConversion<typename ZuTraits<U>::Elem, const V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchStrLiteral : public ZuIfT<IsStrLiteral<U>::OK, R> { };

  // from some other string with same char (other than a string literal)
  template <typename U, typename V = Char> struct IsString_ {
    enum { OK = !ZuConversion<ZtArray_<V>, U>::Base &&
      !IsStrLiteral<U>::OK && ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, const V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchString : public ZuIfT<IsString_<U>::OK, R> { };

  // from some other string with same char (including string literals)
  template <typename U, typename V = Char> struct IsAnyString {
    enum { OK = !ZuConversion<ZtArray_<V>, U>::Base &&
      ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, const V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchAnyString : public ZuIfT<IsAnyString<U>::OK, R> { };

  // from char2 string (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool A = !ZuConversion<ZuNull, V>::Same && ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, const V>::Same
    > struct MatchChar2String;
  template <typename U, typename R>
    struct MatchChar2String<U, R, Char2, true> { typedef R T; };

  // from individual char2 (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool B = !ZuConversion<ZuNull, V>::Same &&
      ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct MatchChar2;
  template <typename U, typename R>
  struct MatchChar2<U, R, Char2, true> { typedef R T; };

  // from printable type (if this is a char array)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::OK && !ZuPrint<U>::String> struct MatchPrint;
  template <typename U, typename R>
  struct MatchPrint<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::Delegate> struct MatchPDelegate { };
  template <typename U, typename R>
  struct MatchPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::Buffer> struct MatchPBuffer { };
  template <typename U, typename R>
  struct MatchPBuffer<U, R, char, true> { typedef R T; };

  // from real primitive types other than chars (if this is a char array)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuConversion<U, char>::Same &&
      !ZuTraits<U>::IsArray
    > struct MatchReal { };
  template <typename U, typename R>
  struct MatchReal<U, R, char, true> { typedef R T; };

  // from individual element
  template <typename U, typename R = void, typename V = T, typename W = Char2,
    bool B = ZuConversion<U, V>::Same ||
      ((!(ZuConversion<char, V>::Same || ZuConversion<wchar_t, V>::Same) ||
	(!ZuTraits<U>::IsString && // avoid MatchString ambiguity
	 !ZuConversion<U, W>::Same)) && // avoid MatchChar2 ambiguity
       (!ZuConversion<char, V>::Same ||
	((!ZuTraits<U>::IsReal ||
	  !ZuTraits<U>::IsPrimitive) && // avoid MatchReal ambiguity
	 !ZuPrint<U>::OK)) && // avoid MatchPrint ambiguity
       !ZuTraits<U>::IsArray && // avoid MatchArray ambiguity
       ZuConversion<U, V>::Exists)
    > struct MatchElem;
  template <typename U, typename R>
  struct MatchElem<U, R, T, Char2, true> { typedef R T; };

  // an unsigned|int|size_t parameter to the constructor is a buffer size
  template <typename U, typename R = void, typename V = Char,
    bool B =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same
    > struct CtorSize;
  template <typename U, typename R>
  struct CtorSize<U, R, Char, true> { typedef R T; };

  // construction from individual element
  template <typename U, typename R = void, typename V = T, typename W = Char2,
    bool B =
      !(ZuConversion<U, unsigned>::Same || // avoid CtorSize ambiguity
	ZuConversion<U, int>::Same ||
	ZuConversion<U, size_t>::Same) &&
      (ZuConversion<U, V>::Same ||
       ((!(ZuConversion<char, V>::Same || ZuConversion<wchar_t, V>::Same) ||
	 (!ZuTraits<U>::IsString && // avoid MatchString ambiguity
	  !ZuConversion<U, W>::Same)) && // avoid MatchChar2 ambiguity
       (!ZuConversion<char, V>::Same ||
	((!ZuTraits<U>::IsReal ||
	  !ZuTraits<U>::IsPrimitive) && // avoid MatchReal ambiguity
	  !ZuPrint<U>::OK)) && // avoid MatchPrint ambiguity
       !ZuTraits<U>::IsArray && // avoid MatchArray ambiguity
       ZuConversion<U, V>::Exists))
    > struct CtorElem;
  template <typename U, typename R>
  struct CtorElem<U, R, T, Char2, true> { typedef R T; };

  template <typename U, typename R = void,
    typename V = Char,
    bool S = ZuTraits<U>::IsString,
    bool W = ZuTraits<U>::IsWString> struct ToString { };
  template <typename U, typename R>
  struct ToString<U, R, char, true, false> { typedef R T; };
  template <typename U, typename R>
  struct ToString<U, R, wchar_t, true, true> { typedef R T; };

public:
  ZuInline ZtArray() { null_(); }
private:
  enum NoInit_ { NoInit };
  inline ZtArray(NoInit_ _) { }
public:
  ZuInline ZtArray(const ZtArray &a) { ctor(a); }
  ZuInline ZtArray(ZtArray &&a) noexcept {
    if (!a.owned())
      shadow_(a.m_data, a.length());
    else {
      own_(a.m_data, a.length(), a.size(), a.mallocd());
      a.owned(false);
    }
  }
  ZuInline ZtArray(std::initializer_list<T> a) {
    shadow_(a.begin(), a.size());
  }

  template <typename A> ZuInline ZtArray(A &&a) { ctor(ZuFwd<A>(a)); }

private:
  template <typename A> ZuInline typename MatchZtArray<A>::T ctor(const A &a)
    { copy_(a.m_data, a.length()); }
  template <typename A> ZuInline typename MatchArray<A>::T ctor(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    copy_(a.data(), a.length());
  }

  template <typename S> ZuInline typename MatchStrLiteral<S>::T ctor(S &&s_)
    { ZuArrayT<S> s(ZuFwd<S>(s_)); shadow_(s.data(), s.length()); }
  template <typename S> ZuInline typename MatchString<S>::T ctor(S &&s_)
    { ZuArrayT<S> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }

  template <typename S> ZuInline typename MatchChar2String<S>::T ctor(S &&s_) {
    ZuArray<const Char2> s(ZuFwd<S>(s_));
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }
  template <typename C> inline typename MatchChar2<C>::T ctor(C c) {
    ZuArray<const Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }

  template <typename P> ZuInline typename MatchPDelegate<P>::T ctor(const P &p)
    { null_(); ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename MatchPBuffer<P>::T ctor(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (!o) { null_(); return; }
    alloc_(o, 0);
    length_(ZuPrint<P>::print(m_data, o, p));
  }

  template <typename V> ZuInline typename CtorSize<V>::T ctor(V size) {
    if (!size) { null_(); return; }
    alloc_(size, 0);
  }

  template <typename R> ZuInline typename CtorElem<R>::T ctor(R &&r) {
    unsigned z = grow(0, 1);
    m_data = (T *)::malloc(z * sizeof(T));
    size_owned(z, 1);
    length_mallocd(1, 1);
    this->initItem(m_data, ZuFwd<R>(r));
  }

public:
  enum Copy_ { Copy };
  template <typename A> inline ZtArray(Copy_ _, A &&a) { copy(ZuFwd<A>(a)); }

private:
  template <typename A> inline typename MatchZtArray<A>::T copy(const A &a)
    { copy_(a.m_data, a.length()); }
  template <typename A> inline typename MatchArray<A>::T copy(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    copy_(a.data(), a.length());
  }

  template <typename S>
  ZuInline typename MatchAnyString<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename S>
  ZuInline typename MatchChar2String<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename C>
  ZuInline typename MatchChar2<C>::T copy(C c) { ctor(c); }
  template <typename R>
  ZuInline typename MatchElem<R>::T copy(R &&r) { ctor(ZuFwd<R>(r)); }

public:
  ZuInline ZtArray &operator =(const ZtArray &a)
    { assign(a); return *this; }
  ZuInline ZtArray &operator =(ZtArray &&a) noexcept {
    free_();
    new (this) ZtArray(ZuMv(a));
    return *this;
  }

  template <typename A>
  ZuInline ZtArray &operator =(const A &a) { assign(a); return *this; }

  ZuInline ZtArray &operator =(std::initializer_list<T> a) {
    shadow(a.begin(), a.size());
    return *this;
  }

private:
  template <typename A> inline typename MatchZtArray<A>::T assign(const A &a) {
    if (this == &a) return;
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(a.m_data, a.length());
    free_2(oldData, oldLength);
  }
  template <typename A> inline typename MatchArray<A>::T assign(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(a.data(), a.length());
    free_2(oldData, oldLength);
  }

  template <typename S> inline typename MatchStrLiteral<S>::T assign(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    free_();
    shadow_(s.data(), s.length());
  }
  template <typename S> inline typename MatchString<S>::T assign(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(s.data(), s.length());
    free_2(oldData, oldLength);
  }

  template <typename S> inline typename MatchChar2String<S>::T assign(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }
  template <typename C> inline typename MatchChar2<C>::T assign(C c) {
    ZuArray<Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(m_data, o), s));
  }

  template <typename P>
  ZuInline typename MatchPDelegate<P>::T assign(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  ZuInline typename MatchPBuffer<P>::T assign(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (!o) { null(); return; }
    if (!owned() || size() < o) size(o);
    length_(ZuPrint<P>::print(m_data, o, p));
  }

  template <typename R> ZuInline typename MatchReal<R>::T assign(R &&r)
    { assign(ZuBoxed(ZuFwd<R>(r))); }

  template <typename R> ZuInline typename MatchElem<R>::T assign(R &&r) {
    free_();
    ctor(ZuFwd<R>(r));
  }

public:
  template <typename A> inline ZtArray &operator -=(A &&a)
    { shadow(ZuFwd<A>(a)); return *this; }

private:
  template <typename A> inline typename MatchZtArray<A>::T shadow(const A &a) {
    if (this == &a) return;
    free_();
    shadow_(a.m_data, a.length());
  }
  template <typename A>
  inline typename MatchSameArray<A>::T shadow(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    free_();
    shadow_(a.data(), a.length());
  }

public:
  template <typename S>
  inline ZtArray(S &&s_, ZtIconv *iconv, typename ZuIsString<S>::T *_ = 0) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    convert_(s, iconv);
  }
  inline ZtArray(const Char *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char> s(data, length);
    convert_(s, iconv);
  }
  inline ZtArray(const Char2 *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char2> s(data, length);
    convert_(s, iconv);
  }

  inline ZtArray(unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size, length);
    if (initItems) this->initItems(m_data, length);
  }
  inline explicit ZtArray(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    shadow_(data, length);
  }
  inline explicit ZtArray(Copy_ _, const T *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  inline explicit ZtArray(const T *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  inline explicit ZtArray(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

  inline ~ZtArray() { free_(); }

// re-initializers

  inline void init() { free_(); init_(); }
  inline void init_() { null_(); }

  template <typename A> inline void init(A &&a) { assign(ZuFwd<A>(a)); }
  template <typename A> inline void init_(A &&a) { ctor(ZuFwd<A>(a)); }

  inline void init(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (this->size() < size || initItems) {
      free_();
      alloc_(size, length);
    } else
      length_(length);
    if (initItems) this->initItems(m_data, length);
  }
  inline void init_(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size, length);
    if (initItems) this->initItems(m_data, length);
  }
  inline void init(const T *data, unsigned length) {
    free_();
    init_(data, length);
  }
  inline void init_(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    shadow_(data, length);
  }
  inline void init(Copy_ _, const T *data, unsigned length) {
    uint32_t oldLength = 0;
    T *oldData = free_1(oldLength);
    init_(_, data, length);
    free_2(oldData, oldLength);
  }
  inline void init_(Copy_ _, const T *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  inline void init(const T *data, unsigned length, unsigned size) {
    free_();
    init_(data, length, size);
  }
  inline void init_(const T *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  inline void init(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    free_();
    init_(data, length, size, mallocd);
  }
  inline void init_(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

// internal initializers / finalizer

private:
  inline void null_() {
    m_data = 0;
    size_owned(0, 0);
    length_mallocd(0, 0);
  }

  inline void own_(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    ZmAssert(size >= length);
    if (!size) {
      if (data && mallocd) ::free((void *)data);
      null_();
      return;
    }
    m_data = (T *)data;
    size_owned(size, 1);
    length_mallocd(length, mallocd);
  }

  inline void shadow_(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data = (T *)data;
    size_owned(length, 0);
    length_mallocd(length, 0);
  }

  inline void alloc_(unsigned size, unsigned length) {
    if (!size) { null_(); return; }
    m_data = (T *)::malloc(size * sizeof(T));
    size_owned(size, 1);
    length_mallocd(length, 1);
  }

  template <typename S> inline void copy_(const S *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data = (T *)::malloc(length * sizeof(T));
    if (length) this->copyItems(m_data, data, length);
    size_owned(length, 1);
    length_mallocd(length, 1);
  }

  template <typename S> void convert_(const S &s, ZtIconv *iconv);

  inline void free_() {
    if (m_data && owned()) {
      this->destroyItems(m_data, length());
      if (mallocd()) ::free(m_data);
    }
  }
  inline T *free_1(uint32_t &length_mallocd) {
    if (!m_data || !owned()) return 0;
    length_mallocd = m_length_mallocd;
    return m_data;
  }
  inline void free_2(T *data, uint32_t length_mallocd) {
    if (data) {
      this->destroyItems(data, length_mallocd & ~(1U<<31U));
      if (length_mallocd>>31U) ::free(data);
    }
  }

public:
// truncation (to minimum size)

  inline void truncate() {
    size(length());
    unsigned n = length();
    if (!m_data || size() <= n) return;
    T *newData = (T *)::malloc(n * sizeof(T));
    this->copyItems(newData, m_data, n);
    free_();
    m_data = newData;
    mallocd(1);
    size_owned(length(), 1);
  }

// array / ptr operators

  ZuInline T &item(int i) { return m_data[i]; }
  ZuInline const T &item(int i) const { return m_data[i]; }

  ZuInline T &operator [](int i) { return m_data[i]; }
  ZuInline const T &operator [](int i) const { return m_data[i]; }

  ZuInline operator T *() { return m_data; }
  ZuInline operator const T *() const { return m_data; }

// accessors

  ZuInline T *data() { return m_data; }
  ZuInline const T *data() const { return m_data; }

  ZuInline unsigned length() const { return m_length_mallocd & ~(1U<<31U); }
  ZuInline unsigned size() const { return m_size_owned & ~(1U<<31U); }

  ZuInline bool mallocd() const { return m_length_mallocd>>31U; }
  ZuInline bool owned() const { return m_size_owned>>31U; }

private:
  ZuInline void length_(unsigned v) {
    m_length_mallocd = (m_length_mallocd & (1U<<31U)) | (uint32_t)v;
  }
  ZuInline void mallocd(bool v) {
    m_length_mallocd = (m_length_mallocd & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void length_mallocd(unsigned l, bool m) {
    m_length_mallocd = l | (((uint32_t)m)<<31U);
  }
  ZuInline void size_(unsigned v) {
    m_size_owned = (m_size_owned & (1U<<31U)) | (uint32_t)v;
  }
  ZuInline void owned(bool v) {
    m_size_owned = (m_size_owned & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void size_owned(unsigned z, bool o) {
    m_size_owned = z | (((uint32_t)o)<<31U);
  }

public:
  ZuInline T *data(bool move) {
    if (move) owned(0);
    return m_data;
  }

// reset to null array

  inline void null() {
    free_();
    null_();
  }

// set length

  ZuInline void length(unsigned length) {
    this->length(length, !ZuTraits<T>::IsPrimitive);
  }
  inline void length(unsigned length, bool initItems) {
    if (!owned() || length > size()) size(length);
    if (initItems) {
      unsigned n = this->length();
      if (length > n) {
	this->initItems(m_data + n, length - n);
      } else if (length < n) {
	this->destroyItems(m_data + length, n - length);
      }
    }
    length_(length);
  }

// set size

  inline T *size(unsigned z) {
    if (!z) { null(); return 0; }
    if (owned() && z == size()) return m_data;
    T *newData = (T *)::malloc(z * sizeof(T));
    unsigned n = z;
    if (n > length()) n = length();
    if (m_data) {
      if (n) this->copyItems(newData, m_data, n);
      free_();
    } else
      n = 0;
    m_data = newData;
    size_owned(z, 1);
    length_mallocd(n, 1);
    return newData;
  }

// hash()

  ZuInline uint32_t hash() const { return Ops::hash(m_data, length()); }

// comparison

  ZuInline bool operator !() const { return !length(); }

  template <typename A>
  ZuInline typename MatchZtArray<A, int>::T cmp(const A &a) const {
    if (this == &a) return 0;
    return cmp(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, int>::T cmp(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return cmp(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, int>::T cmp(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return cmp(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, int>::T cmp(const S &s) const {
    return cmp(ZtArray(s));
  }

  inline int cmp(const T *a, int64_t n) const {
    if (!a) return !!m_data;
    if (!m_data) return -1;
    int64_t l = length();
    if (int i = Ops::cmp(m_data, a, l < n ? l : n)) return i;
    return l - n;
  }

  template <typename A>
  ZuInline typename MatchZtArray<A, bool>::T equals(const A &a) const {
    if (this == &a) return true;
    return equals(a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A, bool>::T equals(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return equals(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, bool>::T equals(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return equals(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchChar2String<S, bool>::T equals(const S &s) const {
    return equals(ZtArray(s));
  }

  ZuInline bool equals(const T *a, unsigned n) const {
    if (!a) return !m_data;
    if (!m_data) return false;
    if (length() != n) return false;
    return Ops::equals(m_data, a, n);
  }

  template <typename A>
  ZuInline bool operator ==(const A &a) const { return equals(a); }
  template <typename A>
  ZuInline bool operator !=(const A &a) const { return !equals(a); }
  template <typename A>
  ZuInline bool operator >(const A &a) const { return cmp(a) > 0; }
  template <typename A>
  ZuInline bool operator >=(const A &a) const { return cmp(a) >= 0; }
  template <typename A>
  ZuInline bool operator <(const A &a) const { return cmp(a) < 0; }
  template <typename A>
  ZuInline bool operator <=(const A &a) const { return cmp(a) <= 0; }

// +, += operators

  template <typename A>
  ZuInline ZtArray<T, Cmp> operator +(const A &a) const { return add(a); }

private:
  template <typename A>
  ZuInline typename MatchZtArray<A, ZtArray<T, Cmp> >::T
    add(const A &a) const { return add(a.m_data, a.length()); }
  template <typename A>
  ZuInline typename MatchArray<A, ZtArray<T, Cmp> >::T add(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return add(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename MatchAnyString<S, ZtArray<T, Cmp> >::T add(S &&s_) const
    { ZuArrayT<S> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename S>
  ZuInline typename MatchChar2String<S, ZtArray<T, Cmp> >::T add(S &&s) const
    { return add(ZtArray(ZuFwd<S>(s))); }
  template <typename C>
  ZuInline typename MatchChar2<C, ZtArray<T, Cmp> >::T add(C c) const
    { return add(ZtArray(c)); }

  template <typename P>
  ZuInline typename MatchPDelegate<P, ZtArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }
  template <typename P>
  ZuInline typename MatchPBuffer<P, ZtArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }

  template <typename R>
  ZuInline typename MatchElem<R, ZtArray<T, Cmp> >::T add(R &&r) const {
    unsigned n = length();
    unsigned z = grow(n, n + 1);
    T *newData = (T *)::malloc(z * sizeof(T));
    if (n) this->copyItems(newData, m_data, n);
    this->initItem(newData + n, ZuFwd<R>(r));
    return ZtArray<T, Cmp>(newData, n + 1, z);
  }

  inline ZtArray<T, Cmp> add(const T *data, unsigned length) const {
    unsigned n = this->length();
    unsigned z = n + length;
    if (ZuUnlikely(!z)) return ZtArray<T, Cmp>();
    T *newData = (T *)::malloc(z * sizeof(T));
    if (n) this->copyItems(newData, m_data, n);
    if (length) this->copyItems(newData + n, data, length);
    return ZtArray<T, Cmp>(newData, z, z);
  }

public:
  template <typename A> ZuInline ZtArray &operator +=(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }
  template <typename A> ZuInline ZtArray &operator <<(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }

private:
  template <typename A>
  inline typename MatchZtArray<A>::T append_(const A &a) {
    if (this == &a) {
      ZtArray a_ = a;
      splice__(0, length(), 0, a_.m_data, a_.length());
    } else
      splice__(0, length(), 0, a.m_data, a.length());
  }
  template <typename A> ZuInline typename MatchArray<A>::T append_(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    splice__(0, length(), 0, a.data(), a.length());
  }

  template <typename S> ZuInline typename MatchAnyString<S>::T append_(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    splice__(0, length(), 0, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T append_(S &&s)
    { append_(ZtArray(ZuFwd<S>(s))); }
  template <typename C>
  ZuInline typename MatchChar2<C>::T append_(C c)
    { append_(ZtArray(c)); }

  template <typename P>
  ZuInline typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  inline typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned n = length();
    unsigned o = ZuPrint<P>::length(p);
    if (!owned() || size() < n + o) size(n + o);
    length(n + ZuPrint<P>::print(m_data + n, o, p));
  }

  template <typename R> ZuInline typename MatchReal<R>::T append_(R &&r)
    { append_(ZuBoxed(ZuFwd<R>(r))); }

  template <typename R> ZuInline typename MatchElem<R>::T append_(R &&r)
    { push(ZuFwd<R>(r)); }

public:
  ZuInline void append(const T *data, unsigned length) {
    if (data) splice__(0, this->length(), 0, data, length);
  }

// splice()

public:
  template <typename A>
  ZuInline void splice(
      ZtArray &removed, int offset, int length, const A &replace) {
    splice_(&removed, offset, length, replace);
  }

  template <typename A>
  ZuInline void splice(int offset, int length, const A &replace) {
    splice_(0, offset, length, replace);
  }

  ZuInline void splice(ZtArray &removed, int offset, int length) {
    splice__(&removed, offset, length, 0, 0);
  }

  ZuInline void splice(int offset, int length) {
    splice__(0, offset, length, 0, 0);
  }

private:
  template <typename A>
  inline typename MatchZtArray<A>::T splice_(
      ZtArray *removed, int offset, int length, const A &a) {
    if (this == &a) {
      ZtArray a_ = a;
      splice__(removed, offset, length, a_.m_data, a_.length());
    } else
      splice__(removed, offset, length, a.m_data, a.length());
  }
  template <typename A>
  ZuInline typename MatchArray<A>::T splice_(
      ZtArray *removed, int offset, int length, A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    splice__(removed, offset, length, a.data(), a.length());
  }

  template <typename S>
  ZuInline typename MatchAnyString<S>::T splice_(
      ZtArray *removed, int offset, int length, S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T splice_(
      ZtArray *removed, int offset, int length, const S &s) {
    splice_(removed, offset, length, ZtArray(s));
  }
  template <typename C>
  ZuInline typename MatchChar2<C>::T splice_(
      ZtArray *removed, int offset, int length, C c) {
    splice_(removed, offset, length, ZtArray(c));
  }

  template <typename R>
  ZuInline typename MatchElem<R>::T splice_(
      ZtArray *removed, int offset, int length, R &&r_) {
    T r(ZuFwd<R>(r_));
    splice__(removed, offset, length, &r, 1);
  }

public:
  ZuInline void splice(
      ZtArray &removed, int offset, int length,
      const T *replace, unsigned rlength) {
    splice__(&removed, offset, length, replace, rlength);
  }

  ZuInline void splice(
      int offset, int length, const T *replace, unsigned rlength) {
    splice__(0, offset, length, replace, rlength);
  }

  template <typename A> ZuInline typename MatchZtArray<A>::T push(A &&a)
    { splice(length(), 0, ZuFwd<A>(a)); }
  template <typename A> ZuInline typename MatchArray<A>::T push(A &&a)
    { splice(length(), 0, ZuFwd<A>(a)); }
  inline void *push() {
    unsigned n = length();
    unsigned z = size();
    if (!owned() || n + 1 > z) {
      z = grow(z, n + 1);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->copyItems(newData, m_data, n);
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(n + 1, 1);
    } else
      length_(n + 1);
    return (void *)(m_data + n);
  }
  template <typename I> ZuInline void push(I &&i) {
    this->initItem(push(), ZuFwd<I>(i));
  }
  inline T pop() {
    unsigned n = length();
    if (!n) return ZuCmp<T>::null();
    T v;
    if (ZuUnlikely(!owned())) {
      v = m_data[--n];
    } else {
      v = ZuMv(m_data[--n]);
      this->destroyItem(m_data + n);
    }
    length_(n);
    return v;
  }
  inline T shift() {
    unsigned n = length();
    if (!n) return ZuCmp<T>::null();
    T v;
    if (ZuUnlikely(!owned())) {
      v = m_data[0];
      ++m_data;
      --n;
    } else {
      v = ZuMv(m_data[0]);
      this->destroyItem(m_data);
      this->moveItems(m_data, m_data + 1, --n);
    }
    length_(n);
    return v;
  }
  template <typename A> inline typename MatchZtArray<A>::T unshift(A &&a)
    { splice(0, 0, ZuFwd<A>(a)); }
  template <typename A> inline typename MatchArray<A>::T unshift(A &&a)
    { splice(0, 0, ZuFwd<A>(a)); }
  inline void *unshift() {
    unsigned n = length();
    unsigned z = size();
    if (!owned() || n + 1 > z) {
      z = grow(z, n + 1);
      T *newData = (T *)::malloc(z * sizeof(T));
      this->copyItems(newData + 1, m_data, n);
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(n + 1, 1);
    } else {
      this->moveItems(m_data + 1, m_data, n);
      length_(n + 1);
    }
    return (void *)m_data;
  }
  template <typename I> inline void unshift(I &&i) {
    this->initItem(unshift(), ZuFwd<I>(i));
  }

private:
  inline void splice__(
      ZtArray *removed,
      int offset,
      int length,
      const T *replace,
      unsigned rlength) {
    unsigned n = this->length();
    unsigned z = size();
    if (offset < 0) { if ((offset += n) < 0) offset = 0; }
    if (length < 0) { if ((length += (n - offset)) < 0) length = 0; }

    if (offset > (int)n) {
      if (removed) removed->null();
      if (!owned() || offset + (int)rlength > (int)z) {
	z = grow(z, offset + rlength);
	size(z);
      }
      this->initItems(m_data + n, offset - n);
      if (rlength) this->copyItems(m_data + offset, replace, rlength);
      length_(offset + rlength);
      return;
    }

    if (offset + length > (int)n) length = n - offset;

    int l = n + rlength - length;

    if (l > 0 && (!owned() || l > (int)z)) {
      z = grow(z, l);
      if (removed) removed->init(Copy, m_data + offset, length);
      T *newData = (T *)::malloc(z * sizeof(T));
      if (offset) this->copyItems(newData, m_data, offset);
      if (rlength) this->copyItems(newData + offset, replace, rlength);
      if ((int)rlength != length && offset + length < (int)n)
	this->copyItems(
	    newData + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      free_();
      m_data = newData;
      size_owned(z, 1);
      length_mallocd(l, 1);
      return;
    }

    if (removed) removed->init(Copy, m_data + offset, length);
    this->destroyItems(m_data + offset, length);
    if (l > 0) {
      if ((int)rlength != length && offset + length < (int)n)
	this->moveItems(
	    m_data + offset + rlength,
	    m_data + offset + length,
	    n - (offset + length));
      if (rlength) this->copyItems(m_data + offset, replace, rlength);
    }
    length_(l);
  }

// iterate

public:
  template <typename Fn> inline void iterate(Fn fn) {
    unsigned n = length();
    for (unsigned i = 0; i < n; i++) fn(m_data[i]);
  }

// grep

  template <typename Fn> T grep(
      Fn fn, unsigned &i, unsigned end, bool del = false) {
    unsigned n = length();
    do {
      if (i >= n) i = 0;
      if (fn(m_data[i])) {
	if (del) {
	  T t = ZuMv(m_data[i]);
	  splice__(0, i, 1, 0, 0);
	  return t;
	}
	return m_data[i];
      }
    } while (++i != end);
    return ZuCmp<T>::null();
  }

// growth algorithm

  ZuInline static unsigned grow(unsigned o, unsigned n) {
    return ZtPlatform::grow(o * sizeof(T), n * sizeof(T)) / sizeof(T);
  }

// conversions
 
  template <typename S>
  ZuInline typename ToString<S, S>::T as() const {
    return ZuTraits<S>::make(m_data, length());
  }

private:
  uint32_t		m_size_owned;	// allocated size and owned flag
  uint32_t		m_length_mallocd;// initialized length and malloc'd flag
  T			*m_data;	// data buffer
};

template <typename T, class Cmp>
template <typename S>
ZuInline void ZtArray<T, Cmp>::convert_(const S &s, ZtIconv *iconv) {
  null_();
  iconv->convert(*this, s);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// traits

template <typename Elem_, class Cmp>
struct ZuTraits<ZtArray<Elem_, Cmp> > :
    public ZuGenericTraits<ZtArray<Elem_, Cmp> > {
  typedef ZtArray<Elem_, Cmp> T;
  typedef Elem_ Elem;
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

// generic printing
template <class Cmp>
struct ZuPrint<ZtArray<char, Cmp> > : public ZuPrintString { };

#endif /* ZtArray_HPP */
