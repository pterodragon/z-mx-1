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
template <typename T, class Cmp> class ZtZArray;

template <typename T> struct ZtArray_ { };
template <typename T> struct ZtZArray_ { };

template <typename T_> struct ZtArray_Char2 { typedef ZuNull T; };
template <> struct ZtArray_Char2<char> { typedef wchar_t T; };
template <> struct ZtArray_Char2<wchar_t> { typedef char T; };

template <typename T_, class Cmp_ = ZuCmp<T_> >
class ZtArray : public ZtArray_<T_>, public ZuArrayFn<T_, Cmp_> {
  struct Private { };

public:
  typedef T_ T;
  typedef Cmp_ Cmp;
  typedef ZuArrayFn<T, Cmp_> Ops;
  enum {
    IsString = ZuConversion<T, char>::Same || ZuConversion<T, wchar_t>::Same,
    IsWString = ZuConversion<T, wchar_t>::Same
  };

protected:
  typedef T Char;
  typedef typename ZtArray_Char2<T>::T Char2;

  // from same type ZtZArray
  template <typename U, typename R = void, typename V = T,
    bool A = ZuConversion<ZtZArray_<V>, U>::Base> struct FromZtZArray;
  template <typename U, typename R>
    struct FromZtZArray<U, R, T, true> { typedef R T; };

  // from same type ZtArray
  template <typename U, typename R = void, typename V = T,
    bool A = ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuConversion<ZtZArray_<V>, U>::Base> struct FromZtArray;
  template <typename U, typename R>
    struct FromZtArray<U, R, T, true> { typedef R T; };

  // from another array type with convertible element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists
    > struct FromArray;
  template <typename U, typename R>
    struct FromArray<U, R, T, true> { typedef R T; };

  // from another array type with same element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same
    > struct FromSameArray;
  template <typename U, typename R>
    struct FromSameArray<U, R, T, true> { typedef R T; };

  // from another array type with convertible element type (not a string)
  template <typename U, typename R = void, typename V = T,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      !ZuTraits<U>::IsString && ZuTraits<U>::IsArray &&
      !ZuConversion<typename ZuTraits<U>::Elem, V>::Same &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Exists
    > struct FromDiffArray;
  template <typename U, typename R>
    struct FromDiffArray<U, R, T, true> { typedef R T; };

  // from some other string with same char (potentially non-null-terminated)
  template <typename U, typename R = void, typename V = Char,
    bool A = !ZuConversion<ZtArray_<V>, U>::Base &&
      ZuTraits<U>::IsString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))>
      struct FromString;
  template <typename U, typename R>
    struct FromString<U, R, Char, true> { typedef R T; };

  // from char2 string (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool A = !ZuConversion<ZuNull, V>::Same && ZuTraits<U>::IsString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))
    > struct FromChar2String;
  template <typename U, typename R>
    struct FromChar2String<U, R, Char2, true> { typedef R T; };

  // from individual char2 (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool B = !ZuConversion<ZuNull, V>::Same &&
      ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct FromChar2;
  template <typename U, typename R>
  struct FromChar2<U, R, Char2, true> { typedef R T; };

  // from printable type (if this is a char array)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::OK && !ZuPrint<U>::String> struct FromPrint;
  template <typename U, typename R>
  struct FromPrint<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::Delegate> struct FromPDelegate { };
  template <typename U, typename R>
  struct FromPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuPrint<U>::Buffer> struct FromPBuffer { };
  template <typename U, typename R>
  struct FromPBuffer<U, R, char, true> { typedef R T; };

  // from real primitive types other than chars (if this is a char array)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<char, V>::Same &&
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuConversion<U, char>::Same &&
      !ZuTraits<U>::IsArray
    > struct FromReal { };
  template <typename U, typename R>
  struct FromReal<U, R, char, true> { typedef R T; };

  // from individual element
  template <typename U, typename R = void, typename V = T, typename W = Char2,
    bool B = ZuConversion<U, V>::Same ||
      ((!(ZuConversion<char, V>::Same || ZuConversion<wchar_t, V>::Same) ||
	(!ZuTraits<U>::IsString && // avoid FromString ambiguity
	 !ZuConversion<U, W>::Same)) && // avoid FromChar2 ambiguity
       (!ZuConversion<char, V>::Same ||
	((!ZuTraits<U>::IsReal ||
	  !ZuTraits<U>::IsPrimitive) && // avoid FromReal ambiguity
	 !ZuPrint<U>::OK)) && // avoid FromPrint ambiguity
       !ZuTraits<U>::IsArray && // avoid FromArray ambiguity
       ZuConversion<U, V>::Exists)
    > struct FromElem;
  template <typename U, typename R>
  struct FromElem<U, R, T, Char2, true> { typedef R T; };

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
	 (!ZuTraits<U>::IsString && // avoid FromString ambiguity
	  !ZuConversion<U, W>::Same)) && // avoid FromChar2 ambiguity
       (!ZuConversion<char, V>::Same ||
	((!ZuTraits<U>::IsReal ||
	  !ZuTraits<U>::IsPrimitive) && // avoid FromReal ambiguity
	  !ZuPrint<U>::OK)) && // avoid FromPrint ambiguity
       !ZuTraits<U>::IsArray && // avoid FromArray ambiguity
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
protected:
  enum NoInit_ { NoInit };
  inline ZtArray(NoInit_ _) { }
public:
  ZuInline ZtArray(const ZtArray &a) { ctor(a); }
  ZuInline ZtArray(ZtArray &&a) noexcept {
    if (!a.m_owned)
      shadow_(a.m_data, a.m_length);
    else {
      own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
      a.m_owned = false;
    }
  }
  ZuInline ZtArray(std::initializer_list<T> a) {
    shadow_(a.begin(), a.size());
  }

  template <typename A> ZuInline ZtArray(A &&a) { ctor(ZuFwd<A>(a)); }

protected:
  template <typename A> ZuInline typename FromZtArray<A>::T ctor(const A &a)
    { copy_(a.m_data, a.m_length); }
  template <typename A> ZuInline typename FromZtZArray<A>::T ctor(const A &a) {
    if (!a.m_owned) { copy_(a.m_data, a.m_length); return; }
    own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
    const_cast<ZtZArray<T, Cmp_> &>(a).m_mallocd =
      const_cast<ZtZArray<T, Cmp_> &>(a).m_owned = false;
  }
  template <typename A> ZuInline typename FromArray<A>::T ctor(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    copy_(a.data(), a.length());
  }

  template <typename S> ZuInline typename FromString<S>::T ctor(S &&s_)
    { ZuArrayT<S> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }

  template <typename S> ZuInline typename FromChar2String<S>::T ctor(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }
  template <typename C> inline typename FromChar2<C>::T ctor(C c) {
    ZuArray<Char2> s{&c, 1};
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }

  template <typename P> ZuInline typename FromPDelegate<P>::T ctor(const P &p)
    { null_(); ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename FromPBuffer<P>::T ctor(const P &p) {
    unsigned n = ZuPrint<P>::length(p);
    if (!n) { null_(); return; }
    alloc_(n);
    m_length = ZuPrint<P>::print(m_data, n, p);
  }

  template <typename V> ZuInline typename CtorSize<V>::T ctor(V size) {
    if (!size) { null_(); return; }
    alloc_(size);
    m_length = 0;
  }

  template <typename R> ZuInline typename CtorElem<R>::T ctor(R &&r) {
    m_size = grow(0, m_length = 1);
    m_data = (T *)::malloc(m_size * sizeof(T));
    this->initItem(m_data, ZuFwd<R>(r));
    m_owned = m_mallocd = true;
  }

public:
  enum Copy_ { Copy };
  template <typename A> inline ZtArray(Copy_ _, A &&a) { copy(ZuFwd<A>(a)); }

protected:
  template <typename A> inline typename FromZtArray<A>::T copy(const A &a)
    { copy_(a.m_data, a.m_length); }
  template <typename A> inline typename FromZtZArray<A>::T copy(const A &a)
    { copy_(a.m_data, a.m_length); }
  template <typename A> inline typename FromArray<A>::T copy(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    copy_(a.data(), a.length());
  }

  template <typename S>
  ZuInline typename FromString<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename S>
  ZuInline typename FromChar2String<S>::T copy(S &&s) { ctor(ZuFwd<S>(s)); }
  template <typename C>
  ZuInline typename FromChar2<C>::T copy(C c) { ctor(c); }
  template <typename R>
  ZuInline typename FromElem<R>::T copy(R &&r) { ctor(ZuFwd<R>(r)); }

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

protected:
  template <typename A> inline typename FromZtArray<A>::T assign(const A &a) {
    if (this == &a) return;
    int oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(a.m_data, a.m_length);
    free_2(oldData, oldLength);
  }
  template <typename A> inline typename FromZtZArray<A>::T assign(const A &a) {
    if (this == (const ZtArray *)&a) return;
    int oldLength = 0;
    T *oldData = free_1(oldLength);
    if (!a.m_owned) {
      copy_(a.m_data, a.m_length);
      free_2(oldData, oldLength);
      return;
    }
    own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
    const_cast<ZtZArray<T, Cmp> &>(a).m_owned = false;
    const_cast<ZtZArray<T, Cmp> &>(a).m_mallocd = false;
    free_2(oldData, oldLength);
  }
  template <typename A> inline typename FromArray<A>::T assign(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    int oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(a.data(), a.length());
    free_2(oldData, oldLength);
  }

  template <typename S> inline typename FromString<S>::T assign(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    int oldLength = 0;
    T *oldData = free_1(oldLength);
    copy_(s.data(), s.length());
    free_2(oldData, oldLength);
  }

  template <typename S> inline typename FromChar2String<S>::T assign(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    int oldLength = 0;
    Char *oldData = free_1(oldLength);
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
    free_2(oldData, oldLength);
  }
  template <typename C> inline typename FromChar2<C>::T assign(C c) {
    ZuArray<Char2> s{&c, 1};
    int oldLength = 0;
    Char *oldData = free_1(oldLength);
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
    free_2(oldData, oldLength);
  }

  template <typename P> ZuInline typename FromPDelegate<P>::T assign(const P &p)
    { ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename FromPBuffer<P>::T assign(const P &p) {
    unsigned n = ZuPrint<P>::length(p);
    if (m_size < n) size(n);
    m_length = ZuPrint<P>::print(m_data, n, p);
  }

  template <typename R> ZuInline typename FromReal<R>::T assign(R &&r)
    { assign(ZuBoxed(ZuFwd<R>(r))); }

  template <typename R> ZuInline typename FromElem<R>::T assign(R &&r) {
    free_();
    ctor(ZuFwd<R>(r));
  }

public:
  template <typename A> inline ZtArray &operator -=(A &&a)
    { shadow(ZuFwd<A>(a)); return *this; }

protected:
  template <typename A> inline typename FromZtArray<A>::T shadow(const A &a) {
    if (this == &a) return;
    free_();
    shadow_(a.m_data, a.m_length);
  }
  template <typename A> inline typename FromZtZArray<A>::T shadow(const A &a) {
    if (this == (const ZtArray *)&a) return;
    free_();
    shadow_(a.m_data, a.m_length);
  }
  template <typename A>
  inline typename FromSameArray<A>::T shadow(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    free_();
    shadow_(a.data(), a.length());
  }

public:
  template <typename S>
  inline ZtArray(S &&s_, ZtIconv *iconv,
      typename ZuIsString<S, Private>::T *_ = 0) {
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

  inline ZtArray(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size);
    m_length = length;
    if (initItems) this->initItems(m_data, m_length);
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
    if (m_size < size || initItems) {
      free_();
      alloc_(size);
    }
    m_length = length;
    if (initItems) this->initItems(m_data, m_length);
  }
  inline void init_(
      unsigned length, unsigned size,
      bool initItems = !ZuTraits<T>::IsPrimitive) {
    if (!size) { null_(); return; }
    alloc_(size);
    m_length = length;
    if (initItems) this->initItems(m_data, m_length);
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
    int oldLength = 0;
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

protected:
  inline void null_() {
    m_data = 0;
    m_length = m_size = 0;
    m_owned = m_mallocd = false;
  }

  inline void own_(
      const T *data, unsigned length, unsigned size, bool mallocd) {
    ZmAssert(size >= length);
    if (!size) {
      if (data && mallocd) ::free((void *)data);
      null_();
      return;
    }
    m_length = length;
    m_size = size;
    m_data = (T *)data;
    m_owned = true, m_mallocd = mallocd;
  }

  inline void shadow_(const T *data, unsigned length) {
    if (!length) { null_(); return; }
    m_length = m_size = length;
    m_data = (T *)data;
    m_owned = m_mallocd = false;
  }

  inline void alloc_(unsigned size) {
    if (!size) { null_(); return; }
    m_size = size;
    m_data = (T *)::malloc(m_size * sizeof(T));
    m_owned = m_mallocd = true;
  }

  template <typename S> inline void copy_(const S *data, unsigned length) {
    if (!data || !length) { null_(); return; }
    m_length = m_size = length;
    m_data = (T *)::malloc(m_size * sizeof(T));
    if (length) this->copyItems(m_data, data, length);
    m_owned = m_mallocd = true;
  }

  template <typename S> void convert_(const S &s, ZtIconv *iconv);

  inline void free_() {
    if (m_data && m_owned) {
      this->destroyItems(m_data, m_length);
      if (m_mallocd) ::free(m_data);
    }
  }
  inline T *free_1(int &length) {
    if (!m_data || !m_owned) return 0;
    length = m_mallocd ? (int)m_length : -(int)m_length;
    return m_data;
  }
  inline void free_2(T *data, int length) {
    if (data) {
      this->destroyItems(data, length > 0 ? length : -length);
      if (length > 0) ::free(data);
    }
  }

// duplication

public:
  inline ZtZArray<T, Cmp> dup() const {
    return ZtZArray<T, Cmp>(ZtZArray<T, Cmp>::Copy, m_data, m_length);
  }

  inline ZtZArray<T, Cmp> move() {
    bool mallocd = m_mallocd;
    m_owned = m_mallocd = false;
    return ZtZArray<T, Cmp>(m_data, m_length, m_size, mallocd);
  }

// truncation (to minimum size)

  inline void truncate() {
    if (!m_data || m_size <= m_length) return;
    T *newData = (T *)::malloc((m_size = m_length) * sizeof(T));
    this->copyItems(newData, m_data, m_size);
    free_();
    m_data = newData;
    m_owned = true;
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
  ZuInline unsigned length() const { return m_length; }
  ZuInline unsigned size() const { return m_size; }

  ZuInline bool owned() const { return m_owned; }
  ZuInline bool mallocd() const { return m_mallocd; }

  ZuInline T *data(bool move) {
    if (move) m_owned = false;
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
    if (!m_owned || length > m_size) size(length);
    if (initItems) {
      if (length > m_length) {
	this->initItems(m_data + m_length, length - m_length);
      } else if (length < m_length) {
	this->destroyItems(m_data + length, m_length - length);
      }
    }
    m_length = length;
  }

// set size

  inline void size(unsigned size) {
    if (!size) { null(); return; }
    if (!m_owned || size != m_size) {
      T *newData = (T *)::malloc(size * sizeof(T));
      if (m_data) {
	if (m_length) {
	  int n = size;
	  if (n > (int)m_length) n = m_length;
	  this->copyItems(newData, m_data, n);
	}
	free_();
      }
      m_size = size;
      m_data = newData;
      m_owned = m_mallocd = true;
      if ((int)m_length > size) m_length = size;
    }
  }

// hash()

  ZuInline uint32_t hash() const { return Ops::hash(m_data, m_length); }

// comparison

  ZuInline bool operator !() const { return !m_data || !m_length; }

  template <typename A>
  ZuInline typename FromZtArray<A, int>::T cmp(const A &a) const {
    if (this == &a) return 0;
    return cmp(a.m_data, a.m_length);
  }
  template <typename A>
  ZuInline typename FromZtZArray<A, int>::T cmp(const A &a) const {
    if (this == (const ZtArray *)&a) return 0;
    return cmp(a.m_data, a.m_length);
  }
  template <typename A>
  ZuInline typename FromArray<A, int>::T cmp(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return cmp(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename FromString<S, int>::T cmp(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return cmp(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromChar2String<S, int>::T cmp(const S &s) const {
    return cmp(ZtArray(s));
  }

  inline int cmp(const T *a, unsigned n) const {
    if (!a) return !!m_data;
    if (!m_data) return -1;

    unsigned l = m_length <= n ? m_length : n;
    int i;

    if (i = Ops::cmp(m_data, a, l)) return i;
    return m_length - n;
  }

  template <typename A>
  ZuInline typename FromZtArray<A, bool>::T equals(const A &a) const {
    if (this == &a) return true;
    return equals(a.m_data, a.m_length);
  }
  template <typename A>
  ZuInline typename FromZtZArray<A, bool>::T equals(const A &a) const {
    if (this == (const ZtArray *)&a) return true;
    return equals(a.m_data, a.m_length);
  }
  template <typename A>
  ZuInline typename FromArray<A, bool>::T equals(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return equals(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename FromString<S, bool>::T equals(S &&s_) const {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    return equals(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromChar2String<S, bool>::T equals(const S &s) const {
    return equals(ZtArray(s));
  }

  ZuInline bool equals(const T *a, unsigned n) const {
    if (!a) return !m_data;
    if (!m_data) return false;
    if (m_length != n) return false;
    return Ops::equals(m_data, a, m_length);
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
  ZuInline ZtZArray<T, Cmp> operator +(const A &a) const { return add(a); }

protected:
  template <typename A>
  ZuInline typename FromZtArray<A, ZtZArray<T, Cmp> >::T
    add(const A &a) const { return add(a.m_data, a.m_length); }
  template <typename A>
  ZuInline typename FromZtZArray<A, ZtZArray<T, Cmp> >::T
    add(const A &a) const { return add(a.m_data, a.m_length); }
  template <typename A>
  ZuInline typename FromArray<A, ZtZArray<T, Cmp> >::T add(A &&a_) const {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    return add(a.data(), a.length());
  }
  template <typename S>
  ZuInline typename FromString<S, ZtZArray<T, Cmp> >::T add(S &&s_) const
    { ZuArrayT<S> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename S>
  ZuInline typename FromChar2String<S, ZtZArray<T, Cmp> >::T add(S &&s) const
    { return add(ZtArray(ZuFwd<S>(s))); }
  template <typename C>
  ZuInline typename FromChar2<C, ZtZArray<T, Cmp> >::T add(C c) const
    { return add(ZtArray(c)); }

  template <typename P>
  ZuInline typename FromPDelegate<P, ZtZArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }
  template <typename P>
  ZuInline typename FromPBuffer<P, ZtZArray<T, Cmp> >::T add(P &&p) const
    { return add(ZtArray(ZuFwd<P>(p))); }

  template <typename R>
  ZuInline typename FromElem<R, ZtZArray<T, Cmp> >::T add(R &&r) const {
    unsigned newSize = grow(m_length, m_length + 1);
    T *newData = (T *)::malloc(newSize * sizeof(T));
    if (m_length) this->copyItems(newData, m_data, m_length);
    this->initItem(newData + m_length, ZuFwd<R>(r));
    return ZtZArray<T, Cmp>(newData, m_length + 1, newSize);
  }

  inline ZtZArray<T, Cmp> add(const T *data, unsigned length) const {
    unsigned newLength = m_length + length;

    if (ZuUnlikely(!newLength)) return ZtZArray<T, Cmp>();

    T *newData = (T *)::malloc(newLength * sizeof(T));

    if (m_length) this->copyItems(newData, m_data, m_length);
    if (length) this->copyItems(newData + m_length, data, length);

    return ZtZArray<T, Cmp>(newData, newLength, newLength);
  }

public:
  template <typename A> ZuInline ZtArray &operator +=(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }
  template <typename A> ZuInline ZtArray &operator <<(A &&a)
    { append_(ZuFwd<A>(a)); return *this; }

protected:
  template <typename A>
  inline typename FromZtArray<A>::T append_(const A &a) {
    if (this == &a) {
      ZtArray a_ = a;
      splice__(0, m_length, 0, a_.m_data, a_.m_length);
    } else
      splice__(0, m_length, 0, a.m_data, a.m_length);
  }
  template <typename A>
  inline typename FromZtZArray<A>::T append_(const A &a) {
    if (this == (const ZtArray *)&a) {
      ZtArray a_ = a;
      splice__(0, m_length, 0, a_.m_data, a_.m_length);
    } else
      splice__(0, m_length, 0, a.m_data, a.m_length);
  }
  template <typename A> ZuInline typename FromArray<A>::T append_(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    splice__(0, m_length, 0, a.data(), a.length());
  }

  template <typename S> ZuInline typename FromString<S>::T append_(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    splice__(0, m_length, 0, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename FromChar2String<S>::T append_(S &&s)
    { append_(ZtArray(ZuFwd<S>(s))); }
  template <typename C>
  ZuInline typename FromChar2<C>::T append_(C c)
    { append_(ZtArray(c)); }

  template <typename P>
  ZuInline typename FromPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  inline typename FromPBuffer<P>::T append_(const P &p) {
    int n = ZuPrint<P>::length(p);
    if (!m_owned || (int)m_size < (int)m_length + n) size(m_length + n);
    m_length += ZuPrint<P>::print(m_data + m_length, n, p);
  }

  template <typename R> ZuInline typename FromReal<R>::T append_(R &&r)
    { append_(ZuBoxed(ZuFwd<R>(r))); }

  template <typename R> ZuInline typename FromElem<R>::T append_(R &&r)
    { push(ZuFwd<R>(r)); }

public:
  ZuInline void append(const T *data, unsigned length) {
    if (data) splice__(0, m_length, 0, data, length);
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

protected:
  template <typename A>
  inline typename FromZtArray<A>::T splice_(
      ZtArray *removed, int offset, int length, const A &a) {
    if (this == &a) {
      ZtArray a_ = a;
      splice__(removed, offset, length, a_.m_data, a_.m_length);
    } else
      splice__(removed, offset, length, a.m_data, a.m_length);
  }
  template <typename A>
  inline typename FromZtZArray<A>::T splice_(
      ZtArray *removed, int offset, int length, const A &a) {
    if (this == (const ZtArray *)&a) {
      ZtArray a_ = a;
      splice__(removed, offset, length, a_.m_data, a_.m_length);
    } else
      splice__(removed, offset, length, a.m_data, a.m_length);
  }
  template <typename A>
  ZuInline typename FromArray<A>::T splice_(
      ZtArray *removed, int offset, int length, A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    splice__(removed, offset, length, a.data(), a.length());
  }

  template <typename S>
  ZuInline typename FromString<S>::T splice_(
      ZtArray *removed, int offset, int length, S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }

  template <typename S>
  ZuInline typename FromChar2String<S>::T splice_(
      ZtArray *removed, int offset, int length, const S &s) {
    splice_(removed, offset, length, ZtArray(s));
  }
  template <typename C>
  ZuInline typename FromChar2<C>::T splice_(
      ZtArray *removed, int offset, int length, C c) {
    splice_(removed, offset, length, ZtArray(c));
  }

  template <typename R>
  ZuInline typename FromElem<R>::T splice_(
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

  template <typename A> ZuInline typename FromZtArray<A>::T push(A &&a)
    { splice(m_length, 0, ZuFwd<A>(a)); }
  template <typename A> ZuInline typename FromZtZArray<A>::T push(A &&a)
    { splice(m_length, 0, ZuFwd<A>(a)); }
  template <typename A> ZuInline typename FromArray<A>::T push(A &&a)
    { splice(m_length, 0, ZuFwd<A>(a)); }
  inline void *push() {
    if (!m_owned || m_length + 1 > m_size) {
      m_size = grow(m_size, m_length + 1);
      T *newData = (T *)::malloc(m_size * sizeof(T));
      this->copyItems(newData, m_data, m_length);
      free_();
      m_data = newData;
      m_owned = m_mallocd = true;
    }
    return (void *)(m_data + m_length++);
  }
  template <typename I> ZuInline void push(I &&i) {
    this->initItem(push(), ZuFwd<I>(i));
  }
  inline T pop() {
    if ((int)m_length <= 0) return ZuCmp<T>::null();
    T t = ZuMv(m_data[--m_length]);
    this->destroyItem(m_data + m_length);
    return t;
  }
  inline T shift() {
    if ((int)m_length <= 0) return ZuCmp<T>::null();
    T t = ZuMv(m_data[0]);
    this->destroyItem(m_data);
    this->moveItems(m_data, m_data + 1, --m_length);
    return t;
  }
  template <typename A> inline typename FromZtArray<A>::T unshift(A &&a)
    { splice(0, 0, ZuFwd<A>(a)); }
  template <typename A> inline typename FromZtZArray<A>::T unshift(A &&a)
    { splice(0, 0, ZuFwd<A>(a)); }
  template <typename A> inline typename FromArray<A>::T unshift(A &&a)
    { splice(0, 0, ZuFwd<A>(a)); }
  inline void *unshift() {
    if (!m_owned || m_length + 1 > m_size) {
      m_size = grow(m_size, m_length + 1);
      T *newData = (T *)::malloc(m_size * sizeof(T));
      this->copyItems(newData + 1, m_data, m_length);
      free_();
      m_data = newData;
      m_owned = m_mallocd = true;
    } else {
      this->moveItems(m_data + 1, m_data, m_length);
    }
    ++m_length;
    return (void *)m_data;
  }
  template <typename I> inline void unshift(I &&i) {
    this->initItem(unshift(), ZuFwd<I>(i));
  }

protected:
  inline void splice__(
      ZtArray *removed,
      int offset,
      int length,
      const T *replace,
      unsigned rlength) {
    if (offset < 0) { if ((offset += m_length) < 0) offset = 0; }
    if (length < 0) { if ((length += (m_length - offset)) < 0) length = 0; }

    if (offset > (int)m_length) {
      if (!m_owned || offset + rlength > (int)m_size) {
	m_size = grow(m_size, offset + rlength);
	T *newData = (T *)::malloc(m_size * sizeof(T));
	this->copyItems(newData, m_data, m_length);
	this->initItems(newData + m_length, offset - m_length);
	if (replace) this->copyItems(newData + offset, replace, rlength);
	if (removed) removed->null();
	free_();
	m_data = newData;
	m_length = offset + rlength;
	m_owned = m_mallocd = true;
	return;
      }
      if (removed) removed->null();
      this->initItems(m_data + m_length, offset - m_length);
      if (replace) this->copyItems(m_data + offset, replace, rlength);
      m_length = offset + rlength;
      return;
    }

    if (offset + length > (int)m_length) length = m_length - offset;

    unsigned int newLength = m_length + rlength - length;

    if (!m_owned || newLength > m_size) {
      m_size = grow(m_size, newLength);
      T *newData = (T *)::malloc(m_size * sizeof(T));
      this->copyItems(newData, m_data, offset);
      if (replace) this->copyItems(newData + offset, replace, rlength);
      this->copyItems(newData + offset + rlength,
		      m_data + offset + length,
		      m_length - (offset + length));
      if (removed) removed->init(Copy, m_data + offset, length);
      free_();
      m_data = newData;
      m_length = newLength;
      m_owned = m_mallocd = true;
      return;
    }

    if (removed) removed->init(Copy, m_data + offset, length);
    this->destroyItems(m_data + offset, length);
    if ((int)rlength != length)
      this->moveItems(
	  m_data + offset + rlength,
	  m_data + offset + length,
	  m_length - (offset + length));
    if (replace) this->copyItems(m_data + offset, replace, rlength);
    m_length = newLength;
  }

// iterate

public:
  template <typename Fn> inline void iterate(Fn fn) {
    for (unsigned i = 0; i < m_length; i++) fn(m_data[i]);
  }

// grep

  template <typename Fn> T grep(
      Fn fn, unsigned &i, unsigned end, bool del = false) {
    do {
      if (i >= m_length) i = 0;
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
    return ZuTraits<S>::make(m_data, m_length);
  }

protected:

  unsigned		m_size:31,	// allocated size of buffer
			m_owned:1;	// owned
  unsigned		m_length:31,	// initialized length
			m_mallocd:1;	// malloc'd (implies owned)
  T			*m_data;	// data buffer
};

template <typename T_, class Cmp_ = ZuCmp<T_> >
class ZtZArray : public ZtZArray_<T_>, public ZtArray<T_, Cmp_> {
  struct Private { };

public:
  typedef T_ T;
  typedef Cmp_ Cmp;

private:
  typedef ZtArray<T_, Cmp_> Base;
  typedef typename Base::Char Char;
  typedef typename Base::Char2 Char2;

public:
  ZuInline ZtZArray() { this->null_(); }

  ZuInline ZtZArray(const ZtZArray &a) : Base(Base::NoInit) { ctor(a); }
  inline ZtZArray(ZtZArray &&a) noexcept : Base(Base::NoInit) {
    if (!a.m_owned) { this->shadow_(a.m_data, a.m_length); return; }
    this->own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
    a.m_owned = false;
  }
  ZuInline ZtZArray(std::initializer_list<T> a) {
    this->shadow_(a.begin(), a.size());
  }

  template <typename A>
  ZuInline ZtZArray(A &&a) : Base(Base::NoInit) { ctor(ZuFwd<A>(a)); }

private:
  template <typename A>
  ZuInline typename Base::template FromZtArray<A>::T ctor(const A &a)
    { this->shadow_(a.data(), a.length()); }
  template <typename A>
  ZuInline typename Base::template FromZtZArray<A>::T ctor(const A &a) {
    if (!a.m_owned) { this->shadow_(a.m_data, a.m_length); return; }
    this->own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
    const_cast<ZtZArray &>(a).m_owned = false;
  }
  template <typename A>
  ZuInline typename Base::template FromSameArray<A>::T ctor(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->shadow_(a.data(), a.length());
  }
  template <typename A>
  ZuInline typename Base::template FromDiffArray<A>::T ctor(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->copy_(a.data(), a.length());
  }

  template <typename S>
  ZuInline typename Base::template FromString<S>::T ctor(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    this->shadow_(s.data(), s.length());
  }

  template <typename S> ZuInline typename Base::template FromChar2String<S>::T
    ctor(const S &s) { Base::ctor(s); }
  template <typename C> ZuInline typename Base::template FromChar2<C>::T
    ctor(C c) { Base::ctor(c); }

  template <typename P> ZuInline typename Base::template FromPDelegate<P>::T
    ctor(const P &p) { Base::ctor(p); }
  template <typename P> ZuInline typename Base::template FromPBuffer<P>::T
    ctor(const P &p) { Base::ctor(p); }

  template <typename V> ZuInline typename Base::template CtorSize<V>::T
    ctor(V size) { Base::ctor(size); }

  template <typename R> ZuInline typename Base::template CtorElem<R>::T
    ctor(R &&r) { Base::ctor(ZuFwd<R>(r)); }

public:
  enum Copy_ { Copy };
  template <typename A>
  ZuInline ZtZArray(Copy_ _, const A &a) { this->copy(a); }

  ZuInline ZtZArray &operator =(const ZtZArray &a)
    { assign(a); return *this; }
  ZuInline ZtZArray &operator =(ZtZArray &&a) noexcept {
    this->free_();
    new (this) ZtZArray(ZuMv(a));
    return *this;
  }

  template <typename A> ZuInline ZtZArray &operator =(const A &a)
    { assign(a); return *this; }

private:
  template <typename A>
  inline typename Base::template FromZtArray<A>::T assign(const A &a) {
    if (this == &a) return;
    this->free_();
    this->shadow_(a.data(), a.length());
  }
  template <typename A>
  inline typename Base::template FromZtZArray<A>::T assign(const A &a) {
    if ((Base *)this == (const Base *)&a) return;
    this->free_();
    if (!a.m_owned)
      this->shadow_(a.m_data, a.m_length);
    else {
      this->own_(a.m_data, a.m_length, a.m_size, a.m_mallocd);
      const_cast<ZtZArray &>(a).m_owned = false;
    }
  }
  template <typename A>
  inline typename Base::template FromArray<A>::T assign(A &&a_) {
    ZuArrayT<A> a(ZuFwd<A>(a_));
    this->free_();
    this->shadow_(a.data(), a.length());
  }

  template <typename S>
  inline typename Base::template FromString<S>::T assign(S &&s_) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    this->free_();
    this->shadow_(s.data(), s.length());
  }

  template <typename S> ZuInline typename Base::template FromChar2String<S>::T
    assign(const S &s) { Base::assign(s); }
  template <typename C> ZuInline typename Base::template FromChar2<C>::T
    assign(C c) { Base::assign(c); }

  template <typename P> ZuInline typename Base::template FromPDelegate<P>::T
    assign(const P &p) { Base::assign(p); }
  template <typename P> ZuInline typename Base::template FromPBuffer<P>::T
    assign(const P &p) { Base::assign(p); }

  template <typename R> ZuInline typename Base::template FromReal<R>::T
    assign(R &&r) { Base::assign(ZuFwd<R>(r)); }

  template <typename R> ZuInline typename Base::template FromElem<R>::T
    assign(R &&r) { Base::assign(ZuFwd<R>(r)); }

public:
  template <typename A> ZuInline ZtZArray &operator -=(const A &a)
    { *(Base *)this -= a; return *this; }

  template <typename S>
  inline ZtZArray(S &&s_, ZtIconv *iconv,
      typename ZuIsString<S, Private>::T *_ = 0) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    this->convert_(s, iconv);
  }
  inline ZtZArray(const Char *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char> s(data, length);
    this->convert_(s, iconv);
  }
  inline ZtZArray(const Char2 *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char2> s(data, length);
    this->convert_(s, iconv);
  }

public:
  ZuInline ZtZArray(
	unsigned length, unsigned size,
	bool initItems = !ZuTraits<T>::IsPrimitive) :
      Base(length, size, initItems) { }
  ZuInline explicit ZtZArray(const T *data, unsigned length) :
      Base(data, length) { }
  ZuInline explicit ZtZArray(Copy_ _, const T *data, unsigned length) :
      Base(Base::Copy, data, length) { }
  ZuInline explicit ZtZArray(const T *data, unsigned length, unsigned size) :
      Base(data, length, size) { }
  ZuInline explicit ZtZArray(
	const T *data, unsigned length, unsigned size, bool mallocd) :
      Base(data, length, size, mallocd) { }

  template <typename A> ZuInline ZtZArray &operator +=(const A &a)
    { *(Base *)this += a; return *this; }
  template <typename A> ZuInline ZtZArray &operator <<(const A &a)
    { *(Base *)this << a; return *this; }
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
template <typename T, class Cmp>
struct ZuTraits<ZtZArray<T, Cmp> > : public ZuTraits<ZtArray<T, Cmp> > {
  inline static ZtZArray<T, Cmp> make(const char *data, unsigned length)
    { return ZtZArray<T, Cmp>(data, length); }
};

// generic printing
template <class Cmp> struct ZuPrint<ZtArray<char, Cmp> > :
  public ZuPrintString<ZtArray<char, Cmp> > { };
template <class Cmp> struct ZuPrint<ZtZArray<char, Cmp> > :
  public ZuPrintString<ZtZArray<char, Cmp> > { };

#endif /* ZtArray_HPP */
