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

// heap-allocated dynamic string class
//
// * fast, lightweight
// * short circuits heap allocation for small strings below a built-in size
// * supports both zero-copy and deep-copy
// * thin layer on ANSI C string functions
// * no locale or character set overhead except where explicitly requested

#ifndef ZtString_HPP
#define ZtString_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <ZtLib.hpp>
#endif

#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <ZuIfT.hpp>
#include <ZuInt.hpp>
#include <ZuNull.hpp>
#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuStringFn.hpp>
#include <ZuPrint.hpp>
#include <ZuBox.hpp>

#include <ZmAssert.hpp>
#include <ZmStream.hpp>

#include <ZtPlatform.hpp>
#include <ZtArray.hpp>
#include <ZtIconv.hpp>

// built-in buffer size (before falling back to malloc())
// Note: must be a multiple of sizeof(uintptr_t)
#define ZtString_BuiltinSize	(3 * sizeof(uintptr_t))

// buffer size increment for vsnprintf()
#define ZtString_vsnprintf_Growth	256
#define ZtString_vsnprintf_MaxSize	(1U<<20) // 1Mb

template <typename Char> const Char *ZtString_Null();
template <> inline const char *ZtString_Null() { return ""; }
template <> inline const wchar_t *ZtString_Null() {
  return Zu::nullWString();
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4348)
#endif

struct ZtString___ { };
template <typename Char> struct ZtString__ : public ZtString___ { };

template <typename Char, typename Char2> class ZtString_;
template <typename Char, typename Char2> class ZtZString_;

template <typename Char_, typename Char2_>
class ZtString_ : public ZtString__<Char_> {
  struct Private { };

public:
  typedef Char_ Char;
  typedef Char2_ Char2;
  enum { IsWString = ZuConversion<Char, wchar_t>::Same };
  enum { BuiltinSize = (int)ZtString_BuiltinSize / sizeof(Char) };

protected:
  // from same type ZtZString
  template <typename U, typename R = void,
    typename V = Char, typename W = Char2,
    bool A = ZuConversion<ZtZString_<V, W>, U>::Same> struct FromZtZString;
  template <typename U, typename R>
    struct FromZtZString<U, R, Char, Char2, true> { typedef R T; };

  // from same type ZtString
  template <typename U, typename R = void,
    typename V = Char, typename W = Char2,
    bool A = ZuConversion<ZtString_<V, W>, U>::Same> struct FromZtString;
  template <typename U, typename R>
    struct FromZtString<U, R, Char, Char2, true> { typedef R T; };

  // from some other C string with same char type
  template <typename U, typename R = void, typename V = Char,
    bool A = !ZuConversion<ZtString__<V>, U>::Base &&
      ZuTraits<U>::IsString && ZuTraits<U>::IsCString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))>
      struct FromOtherCString;
  template <typename U, typename R>
    struct FromOtherCString<U, R, Char, true> { typedef R T; };

  // from some other non-C string with same char type (non-null-terminated)
  template <typename U, typename R = void, typename V = Char,
    bool A = !ZuConversion<ZtString__<V>, U>::Base &&
      ZuTraits<U>::IsString && !ZuTraits<U>::IsCString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))>
      struct FromOtherString;
  template <typename U, typename R>
    struct FromOtherString<U, R, Char, true> { typedef R T; };

  // from individual char (other than wchar_t, which is ambiguous)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct FromChar;
  template <typename U, typename R>
  struct FromChar<U, R, Char, true> { typedef R T; };

  // from char2 string (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool A = ZuTraits<U>::IsString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))
    > struct FromChar2String;
  template <typename U, typename R>
    struct FromChar2String<U, R, Char2, true> { typedef R T; };

  // from individual char2 (requires conversion, char->wchar_t only)
  template <typename U, typename R = void, typename V = Char2,
    bool B = ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct FromChar2;
  template <typename U, typename R>
  struct FromChar2<U, R, Char2, true> { typedef R T; };

  // from printable type
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct FromPrint;
  template <typename U, typename R>
  struct FromPrint<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Delegate> struct FromPDelegate;
  template <typename U, typename R>
  struct FromPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Buffer> struct FromPBuffer;
  template <typename U, typename R>
  struct FromPBuffer<U, R, char, true> { typedef R T; };

  // from any other real and primitive type (integers, floating point, etc.)
  template <typename U, typename R = void, typename V = Char,
    bool B =
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuConversion<U, char>::Same &&
      !ZuTraits<U>::IsArray
    > struct FromReal { };
  template <typename U, typename R>
  struct FromReal<U, R, char, true> { typedef R T; };

  // an unsigned|int|size_t parameter to the constructor is a buffer size
  template <typename U, typename R = void, typename V = Char,
    bool B =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same
    > struct CtorSize;
  template <typename U, typename R>
  struct CtorSize<U, R, Char, true> { typedef R T; };

  // construction from any other real and primitive type
  template <typename U, typename R = void, typename V = Char,
    bool B =
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuConversion<U, char>::Same &&
      !ZuConversion<U, unsigned>::Same &&
      !ZuConversion<U, int>::Same &&
      !ZuConversion<U, size_t>::Same &&
      !ZuTraits<U>::IsArray && !ZuPrint<U>::OK
    > struct CtorReal { };
  template <typename U, typename R>
  struct CtorReal<U, R, char, true> { typedef R T; };

  template <typename U, typename R = void,
    typename V = Char,
    bool S = ZuTraits<U>::IsString,
    bool W = ZuTraits<U>::IsWString> struct ToString { };
  template <typename U, typename R>
  struct ToString<U, R, char, true, false> { typedef R T; };
  template <typename U, typename R>
  struct ToString<U, R, wchar_t, true, true> { typedef R T; };

public:
// constructors, assignment operators and destructor

  ZuInline ZtString_() { null_(); }
protected:
  enum NoInit_ { NoInit };
  inline ZtString_(NoInit_ _) { }
public:
  ZuInline ZtString_(const ZtString_ &s) { ctor(s); }
  inline ZtString_(ZtString_ &&s) noexcept {
    if (s.m_null) { null_(); return; }
    if (s.m_builtin) { copy_(s.data_(), s.m_length); return; }
    if (!s.m_owned) { shadow_(s.data_(), s.m_length); return; }
    own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    s.m_owned = s.m_builtin,
    s.m_mallocd = false;
  }

  template <typename S> ZuInline ZtString_(S &&s) { ctor(ZuFwd<S>(s)); }

protected:
  template <typename S> ZuInline typename FromZtString<S>::T ctor(const S &s)
    { copy_(s.data_(), s.m_length); }
  template <typename S> ZuInline typename FromZtZString<S>::T ctor(const S &s) {
    if (s.m_null) { null_(); return; }
    if (!s.m_owned || s.m_builtin) { copy_(s.data_(), s.m_length); return; }
    own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    const_cast<ZtZString_<Char, Char2> &>(s).m_owned = s.m_builtin,
    const_cast<ZtZString_<Char, Char2> &>(s).m_mallocd = false;
  }
  template <typename S> ZuInline typename FromOtherCString<S>::T ctor(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename S> ZuInline typename FromOtherString<S>::T ctor(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename C> ZuInline typename FromChar<C>::T ctor(C c)
    { copy_(&c, 1); }

  template <typename S> ZuInline typename FromChar2String<S>::T ctor(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }

  template <typename C> ZuInline typename FromChar2<C>::T ctor(C c) {
    ZuArray<Char2> s{&c, 1};
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }

  template <typename P> ZuInline typename FromPDelegate<P>::T ctor(const P &p)
    { null_(); ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename FromPBuffer<P>::T ctor(const P &p) {
    unsigned n = ZuPrint<P>::length(p);
    if (!n) { null_(); return; }
    Char *newData = alloc_(n + 1);
    newData[m_length = ZuPrint<P>::print(newData, n, p)] = 0;
  }

  template <typename V> ZuInline typename CtorSize<V>::T ctor(V size) {
    if (!size) { null_(); return; }
    alloc_(size)[m_length = 0] = 0;
  }

  template <typename R> ZuInline typename CtorReal<R>::T ctor(R &&r)
    { ctor(ZuBoxed(ZuFwd<R>(r))); }

public:
  enum Copy_ { Copy };
  template <typename S> ZuInline ZtString_(Copy_ _, const S &s) { copy(s); }

protected:
  template <typename S> ZuInline typename FromZtString<S>::T copy(const S &s)
    { copy_(s.data_(), s.m_length); }
  template <typename S> ZuInline typename FromZtZString<S>::T copy(const S &s)
    { copy_(s.data_(), s.m_length); }
  template <typename S> ZuInline typename FromOtherCString<S>::T copy(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename S> ZuInline typename FromOtherString<S>::T copy(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename C> ZuInline typename FromChar<C>::T copy(C c)
    { copy_(&c, 1); }

  template <typename S> ZuInline typename FromChar2String<S>::T copy(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }
	  
  template <typename C> ZuInline typename FromChar2<C>::T copy(C c) {
    ZuArray<Char2> s{&c, 1};
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
  }

public:
  ZuInline ZtString_ &operator =(const ZtString_ &s)
    { assign(s); return *this; }
  ZuInline ZtString_ &operator =(ZtString_ &&s) noexcept {
    free_();
    new (this) ZtString_(ZuMv(s));
    return *this;
  }

  template <typename S> ZuInline ZtString_ &operator =(const S &s)
    { assign(s); return *this; }

protected:
  template <typename S> inline typename FromZtString<S>::T assign(const S &s) {
    if (this == &s) return;
    Char *oldData = free_1();
    copy_(s.data_(), s.m_length);
    free_2(oldData);
  }
  template <typename S> inline typename FromZtZString<S>::T assign(const S &s) {
    if (this == (const ZtString_ *)&s) return;
    Char *oldData = free_1();
    if (s.m_null) { null_(); free_2(oldData); return; }
    if (!s.m_owned || s.m_builtin) {
      copy_(s.data_(), s.m_length);
      free_2(oldData);
      return;
    }
    own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    const_cast<ZtZString_<Char, Char2> &>(s).m_owned = s.m_builtin,
    const_cast<ZtZString_<Char, Char2> &>(s).m_mallocd = false;
    free_2(oldData);
  }
  template <typename S>
  inline typename FromOtherCString<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    Char *oldData = free_1();
    copy_(s.data(), s.length());
    free_2(oldData);
  }
  template <typename S>
  inline typename FromOtherString<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    Char *oldData = free_1();
    copy_(s.data(), s.length());
    free_2(oldData);
  }
  template <typename C> inline typename FromChar<C>::T assign(C c) {
    Char *oldData = free_1();
    copy_(&c, 1);
    free_2(oldData);
  }

  template <typename S>
  inline typename FromChar2String<S>::T assign(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    Char *oldData = free_1();
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
    free_2(oldData);
  }
  template <typename C> inline typename FromChar2<C>::T assign(C c) {
    ZuArray<Char2> s{&c, 1};
    Char *oldData = free_1();
    convert_(s, ZtIconvDefault<Char, Char2>::instance());
    free_2(oldData);
  }

  template <typename P> inline typename FromPDelegate<P>::T assign(const P &p) {
    if (m_owned && m_size > 0)
      data_()[m_length = 0] = 0;
    else
      null();
    ZuPrint<P>::print(*this, p);
  }
  template <typename P> inline typename FromPBuffer<P>::T assign(const P &p) {
    unsigned n = ZuPrint<P>::length(p);
    if (!n) { free_(); null_(); return; }
    Char *newData = m_size <= n ? size(n + 1) : (m_null = false, data_());
    newData[m_length = ZuPrint<P>::print(newData, n, p)] = 0;
  }

  template <typename R> inline typename FromReal<R>::T assign(const R &r)
    { assign(ZuBoxed(r)); }

public:
  template <typename S> inline ZtString_ &operator -=(const S &s)
    { shadow(s); return *this; }

protected:
  template <typename S> inline typename FromZtString<S>::T shadow(const S &s) {
    if (this == &s) return;
    free_();
    shadow_(s.data_(), s.m_length);
  }
  template <typename S> inline typename FromZtZString<S>::T shadow(const S &s) {
    if (this == (const ZtString_ *)&s) return;
    free_();
    shadow_(s.data_(), s.m_length);
  }
  template <typename S>
  inline typename FromOtherCString<S>::T shadow(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); free_(); shadow_(s.data(), s.length()); }
  template <typename S>
  inline typename FromOtherString<S>::T shadow(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); free_(); shadow_(s.data(), s.length()); }

public:
  template <typename S>
  inline ZtString_(S &&s_, ZtIconv *iconv,
      typename ZuIsString<S, Private>::T *_ = 0) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    convert_(s, iconv);
  }
  inline ZtString_(const Char *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char> s(data, length);
    convert_(s, iconv);
  }
  inline ZtString_(const Char2 *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char2> s(data, length);
    convert_(s, iconv);
  }

public:
  inline ZtString_(unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    alloc_(size)[m_length = length] = 0;
  }
  inline explicit ZtString_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    init_(data, length);
  }
  inline explicit ZtString_(Copy_ _, const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  inline explicit ZtString_(const Char *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  inline explicit ZtString_(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

  inline ~ZtString_() { free_(); }

// re-initializers

  inline void init() { free_(); init_(); }
  inline void init_() { null_(); }

  template <typename S> inline void init(const S &s) { assign(s); }
  template <typename S> inline void init_(const S &s) { ctor(s); }

  inline void init(unsigned length, unsigned size) {
    if (m_size < size) {
      free_();
      alloc_(size);
    }
    data_()[m_length = length] = 0, m_null = false;
  }
  inline void init_(unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    alloc_(size)[m_length = length] = 0;
  }
  inline void init(const Char *data, unsigned length) {
    free_();
    init_(data, length);
  }
  inline void init_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    if (data[length])
      copy_(data, length);
    else
      shadow_(data, length);
  }
  inline void init(Copy_ _, const Char *data, unsigned length) {
    Char *oldData = free_1();
    init_(_, data, length);
    free_2(oldData);
  }
  inline void init_(Copy_ _, const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  inline void init(const Char *data, unsigned length, unsigned size) {
    free_();
    init_(data, length, size);
  }
  inline void init_(const Char *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  inline void init(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    free_();
    init_(data, length, size, mallocd);
  }
  inline void init_(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

// internal initializers / finalizer

protected:
  inline void null_() {
    m_length = 0;
    m_data[0] = 0;
    m_owned = true, m_mallocd = false, m_builtin = true, m_null = true;
    m_size = BuiltinSize;
  }

  inline void own_(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    ZmAssert(size > length);
    if (!size) {
      if (data && mallocd) ::free((void *)data);
      null_();
      return;
    }
    m_length = length;
    m_data[0] = (uintptr_t)(Char *)data;
    m_owned = true, m_mallocd = mallocd, m_builtin = false, m_null = false;
    m_size = size;
  }

  inline void shadow_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    m_length = length;
    m_data[0] = (uintptr_t)(Char *)data;
    m_owned = m_mallocd = m_builtin = false, m_null = false;
    m_size = length + 1;
  }

  inline Char *alloc_(unsigned size) {
    if (!size) { null_(); return 0; }
    if (size <= (unsigned)BuiltinSize) {
      m_owned = true, m_mallocd = false, m_builtin = true, m_null = false;
      m_size = BuiltinSize;
      return (Char *)m_data;
    } else {
      m_data[0] = (uintptr_t)::malloc(size * sizeof(Char));
      m_owned = m_mallocd = true, m_builtin = false, m_null = false;
      m_size = size;
      return (Char *)m_data[0];
    }
  }

  inline void copy_(const Char *copyData, unsigned length) {
    if (!copyData || !length) { null_(); return; }
    Char *newData;
    if ((m_length = length) < (unsigned)BuiltinSize) {
      newData = (Char *)m_data;
      m_owned = true, m_mallocd = false, m_builtin = true, m_null = false;
      m_size = (unsigned)BuiltinSize;
    } else {
      m_data[0] =
	(uintptr_t)(newData = (Char *)::malloc((length + 1) * sizeof(Char)));
      m_owned = m_mallocd = true, m_builtin = false, m_null = false;
      m_size = length + 1;
    }
    if (newData) memcpy(newData, copyData, length * sizeof(Char));
    newData[length] = 0;
  }

  template <typename S> void convert_(const S &s, ZtIconv *iconv);

  inline void free_() {
    if (m_mallocd) if (Char *data = (Char *)m_data[0]) ::free(data);
  }
  inline Char *free_1() {
    if (!m_mallocd) return 0;
    return data_();
  }
  inline void free_2(Char *data) {
    if (data) ::free(data);
  }

// duplication

public:
  inline ZtZString_<Char, Char2> dup() const {
    return ZtZString_<Char, Char2>(
	ZtZString_<Char, Char2>::Copy, data_(), m_length);
  }

  inline ZtZString_<Char, Char2> move() {
    if (m_null) return ZtZString_<Char, Char2>();
    if (m_builtin)
      return ZtZString_<Char, Char2>(
	  ZtZString_<Char, Char2>::Copy, (Char *)m_data, m_length);
    bool mallocd = m_mallocd;
    m_owned = m_mallocd = false;
    return ZtZString_<Char, Char2>(
	(Char *)m_data[0], m_length, m_size, mallocd);
  }

// conversion

  template <typename S>
  ZuInline typename ToString<S, S>::T as() const {
    return ZuTraits<S>::make(data(), m_length);
  }

// truncation (to minimum size)

  ZuInline void truncate() { size(m_length + 1); }

// array / ptr operators

  ZuInline Char &operator [](int i) { return data_()[i]; }
  ZuInline Char operator [](int i) const { return data_()[i]; }

  ZuInline operator Char *() { return m_null ? 0 : data_(); }
  ZuInline operator const Char *() const { return m_null ? 0 : data_(); }

// accessors

  ZuInline Char *data() { if (m_null) return 0; return data_(); }
  ZuInline Char *data_() {
    return m_builtin ? (Char *)m_data : (Char *)m_data[0];
  }
  ZuInline const Char *data() const { if (m_null) return 0; return data_(); }
  ZuInline const Char *data_() const {
    return m_builtin ? (const Char *)m_data : (const Char *)m_data[0];
  }
  ZuInline const Char *ndata() const {
    if (m_null) return ZtString_Null<Char>();
    return data_();
  }
  ZuInline unsigned length() const { return m_length; }
  ZuInline unsigned size() const { return m_null ? 0 : m_size; }

  ZuInline bool owned() const { return m_owned; }
  ZuInline bool mallocd() const { return m_mallocd; }
  ZuInline bool builtin() const { return m_builtin; }

  inline Char *data(bool move) {
    if (!move) return data_();
    if (m_null) return 0;
    if (m_builtin) {
      Char *newData = (Char *)::malloc(BuiltinSize * sizeof(Char));
      memcpy(newData, m_data, (m_length + 1) * sizeof(Char));
      return newData;
    } else {
      m_owned = false, m_mallocd = false;
      return (Char *)m_data[0];
    }
  }

// reset to null string

  ZuInline void null() {
    free_();
    null_();
  }

// set length

  inline void length(unsigned length) {
    if (!m_owned || length >= m_size)
      size(length + 1)[length] = 0;
    else
      data_()[length] = 0, m_null = false;
    m_length = length;
  }
  inline void calcLength() {
    if (m_null)
      m_length = 0;
    else
      m_length = Zu::strlen_(data_());
  }

// set size

  inline Char *size(unsigned size) {
    if (!size) { null(); return 0; }
    if (m_owned && size == m_size) return data_();
    Char *oldData;
    if (m_builtin)
      oldData = (Char *)m_data;
    else
      oldData = (Char *)m_data[0];
    Char *newData;
    if (size <= (unsigned)BuiltinSize)
      newData = (Char *)m_data;
    else
      newData = (Char *)::malloc(size * sizeof(Char));
    if (oldData != newData) {
      if (m_length) {
	unsigned n = size;
	if (n > m_length + 1U) n = m_length + 1U;
	memcpy(newData, oldData, n * sizeof(Char));
      }
      if (m_mallocd) ::free(oldData);
    }
    if (m_length >= size) newData[m_length = size - 1] = 0;
    if (size <= (unsigned)BuiltinSize) {
      m_owned = true, m_mallocd = false, m_builtin = true, m_null = false;
      m_size = BuiltinSize;
    } else {
      m_data[0] = (uintptr_t)newData;
      m_owned = m_mallocd = true, m_builtin = false, m_null = false;
      m_size = size;
    }
    return newData;
  }

// common prefix

  template <typename S>
  inline typename FromZtString<S, ZtZArray<Char> >::T prefix(const S &s) {
    if (this == &s) return ZtZArray<Char>(data_(), m_length + 1);
    return prefix(s.data_(), s.m_length);
  }
  template <typename S>
  inline typename FromZtZString<S, ZtZArray<Char> >::T prefix(const S &s) {
    if (this == (const ZtString_ *)&s)
      return ZtZArray<Char>(data_(), m_length + 1);
    return prefix(s.data_(), s.m_length);
  }
  template <typename S>
  inline typename FromOtherCString<S, ZtZArray<Char> >::T prefix(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); return prefix(s.data(), s.length()); }
  template <typename S>
  inline typename FromOtherString<S, ZtZArray<Char> >::T prefix(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); return prefix(s.data(), s.length()); }

  inline ZtZArray<Char> prefix(const Char *pfxData, unsigned length) const {
    if (m_null) return ZtZArray<Char>();
    const Char *p1 = data_();
    if (!pfxData) return ZtZArray<Char>(p1, 1);
    const Char *p2 = pfxData;
    unsigned n, l = m_length > length ? length : m_length;
    for (n = 0; ++n <= l && *p1++ == *p2++; );
    return ZtZArray<Char>(data_(), n);
  }

public:
  ZuInline uint32_t hash() const { return ZuHash<ZtString_>::hash(*this); }

// comparison

  ZuInline bool operator !() const { return !m_length; }

  template <typename S>
  ZuInline int cmp(const S &s) const {
    return ZuCmp<ZtString_>::cmp(*this, s);
  }
  template <typename S>
  ZuInline bool equals(const S &s) const {
    return ZuCmp<ZtString_>::equals(*this, s);
  }

  template <typename S>
  ZuInline bool operator ==(const S &s) const { return equals(s); }
  template <typename S>
  ZuInline bool operator !=(const S &s) const { return !equals(s); }
  template <typename S>
  ZuInline bool operator >(const S &s) const { return cmp(s) > 0; }
  template <typename S>
  ZuInline bool operator >=(const S &s) const { return cmp(s) >= 0; }
  template <typename S>
  ZuInline bool operator <(const S &s) const { return cmp(s) < 0; }
  template <typename S>
  ZuInline bool operator <=(const S &s) const { return cmp(s) <= 0; }

  inline int cmp(const Char *s, unsigned n) const {
    if (m_null) return s ? -1 : 0;
    if (!s) return 1;
    return Zu::strcmp_(data_(), s, n);
  }
  inline bool equals(const Char *s, unsigned n) const {
    if (m_null) return !s;
    if (!s) return false;
    return !Zu::strcmp_(data_(), s, n);
  }
  inline int icmp(const Char *s, unsigned n) const {
    if (m_null) return s ? -1 : 0;
    if (!s) return 1;
    return Zu::stricmp_(data_(), s, n);
  }

// +, += operators
  template <typename S>
  inline ZtZString_<Char, Char2> operator +(const S &s) const { return add(s); }

protected:
  template <typename S>
  ZuInline typename FromZtString<S, ZtZString_<Char, Char2> >::T
    add(const S &s) const { return add(s.data_(), s.m_length); }
  template <typename S>
  ZuInline typename FromZtZString<S, ZtZString_<Char, Char2> >::T
    add(const S &s) const { return add(s.data_(), s.m_length); }
  template <typename S>
  ZuInline typename FromOtherCString<S, ZtZString_<Char, Char2> >::T
    add(S &&s_) const
      { ZuArray<Char> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename S>
  ZuInline typename FromOtherString<S, ZtZString_<Char, Char2> >::T
    add(S &&s_) const
      { ZuArray<Char> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename C>
  ZuInline typename FromChar<C, ZtZString_<Char, Char2> >::T
    add(C c) const { return add(&c, 1); }

  template <typename S>
  ZuInline typename FromChar2String<S, ZtZString_<Char, Char2> >::T
    add(const S &s) const { return add(ZtString_(s)); }
  template <typename C>
  ZuInline typename FromChar2<C, ZtZString_<Char, Char2> >::T
    add(C c) const { return add(ZtString_(c)); }

  template <typename P>
  ZuInline typename FromPDelegate<P, ZtZString_<Char, Char2> >::T
      add(const P &p) const { return add(ZtString_(p)); }
  template <typename P>
  ZuInline typename FromPBuffer<P, ZtZString_<Char, Char2> >::T
      add(const P &p) const { return add(ZtString_(p)); }

  inline ZtZString_<Char, Char2> add(
      const Char *addData, unsigned length) const {
    Char *newData = (Char *)::malloc((m_length + length + 1) * sizeof(Char));
    if (m_length)
      memcpy(newData, data_(), m_length * sizeof(Char));
    if (addData) memcpy(newData + m_length, addData, length * sizeof(Char));
    newData[m_length + length] = 0;
    return ZtZString_<Char, Char2>(
	newData, m_length + length, m_length + length + 1);
  }

public:
  template <typename S> inline ZtString_ &operator +=(const S &s)
    { append_(s); return *this; }
  template <typename S> inline ZtString_ &operator <<(const S &s)
    { append_(s); return *this; }

protected:
  template <typename S>
  inline typename FromZtString<S>::T append_(const S &s) {
    if (this == &s) {
      ZtString_ s_ = s;
      splice__(0, m_length, 0, s_.data_(), s_.m_length);
    } else
      splice__(0, m_length, 0, s.data_(), s.m_length);
  }
  template <typename S>
  inline typename FromZtZString<S>::T append_(const S &s) {
    if (this == (const ZtString_ *)&s) {
      ZtString_ s_ = s;
      splice__(0, m_length, 0, s_.data_(), s_.m_length);
    } else
      splice__(0, m_length, 0, s.data_(), s.m_length);
  }
  template <typename S>
  ZuInline typename FromOtherCString<S>::T append_(S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(0, m_length, 0, s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromOtherString<S>::T append_(S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(0, m_length, 0, s.data(), s.length());
  }
  template <typename C>
  inline typename FromChar<C>::T append_(C c) {
    Char *newData;
    if (!m_owned || m_size < m_length + 2)
      newData = size(grow(m_size, m_length + 2));
    else
      newData = data_(), m_null = false;
    newData[m_length++] = c;
    newData[m_length] = 0;
  }

  template <typename S>
  ZuInline typename FromChar2String<S>::T append_(const S &s)
    { append_(ZtString_(s)); }
  template <typename C>
  ZuInline typename FromChar2<C>::T append_(C c)
    { append_(ZtString_(c)); }

  template <typename P>
  inline typename FromPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P> inline typename FromPBuffer<P>::T append_(const P &p) {
    int n = ZuPrint<P>::length(p);
    Char *newData;
    if (!m_owned || m_size <= m_length + n)
      newData = size(grow(m_size, m_length + n + 1));
    else
      newData = data_(), m_null = false;
    newData[m_length += ZuPrint<P>::print(newData + m_length, n, p)] = 0;
  }

  template <typename R> ZuInline typename FromReal<R>::T append_(R &&r)
    { append_(ZuBoxed(ZuFwd<R>(r))); }

public:
  ZuInline void append(const Char *data, unsigned length) {
    if (data) splice__(0, m_length, 0, data, length);
  }

// splice()

  ZuInline void splice(
      ZtString_ &removed, int offset, int length, const ZtString_ &replace) {
    splice_(&removed, offset, length, replace);
  }

  ZuInline void splice(int offset, int length, const ZtString_ &replace) {
    splice_(0, offset, length, replace);
  }

  ZuInline void splice(ZtString_ &removed, int offset, int length) {
    splice__(&removed, offset, length, 0, 0);
  }

  ZuInline void splice(int offset) {
    splice__(0, offset, INT_MAX, 0, 0);
  }

  ZuInline void splice(int offset, int length) {
    splice__(0, offset, length, 0, 0);
  }

  template <typename S>
  ZuInline void splice(
      ZtString_ &removed, int offset, int length, const S &replace) {
    splice_(&removed, offset, length, replace);
  }

  template <typename S>
  ZuInline void splice(int offset, int length, const S &replace) {
    splice_(0, offset, length, replace);
  }

protected:
  template <typename S>
  inline typename FromZtString<S>::T splice_(
      ZtString_ *removed, int offset, int length, const S &s) {
    if (this == &s) {
      ZtString_ s_ = s;
      splice__(removed, offset, length, s_.data_(), s_.m_length);
    } else
      splice__(removed, offset, length, s.data_(), s.m_length);
  }
  template <typename S>
  inline typename FromZtZString<S>::T splice_(
      ZtString_ *removed, int offset, int length, const S &s) {
    if (this == (const ZtString_ *)&s) {
      ZtString_ s_ = s;
      splice__(removed, offset, length, s_.data_(), s_.m_length);
    } else
      splice__(removed, offset, length, s.data_(), s.m_length);
  }
  template <typename S>
  ZuInline typename FromOtherCString<S>::T splice_(
      ZtString_ *removed, int offset, int length, S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromOtherString<S>::T splice_(
      ZtString_ *removed, int offset, int length, S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }
  template <typename C>
  ZuInline typename FromChar<C>::T splice_(
      ZtString_ *removed, int offset, int length, C c) {
    splice__(removed, offset, length, &c, 1);
  }
  template <typename S>
  ZuInline typename FromChar2String<S>::T splice_(
      ZtString_ *removed, int offset, int length, const S &s) {
    splice_(removed, offset, length, ZtString_(s));
  }
  template <typename C>
  ZuInline typename FromChar2<C>::T splice_(
      ZtString_ *removed, int offset, int length, C c) {
    splice_(removed, offset, length, ZtString_(c));
  }

public:
  ZuInline void splice(
      ZtString_ &removed, int offset, int length,
      const Char *replace, unsigned rlength) {
    splice__(&removed, offset, length, replace, rlength);
  }

  ZuInline void splice(
      int offset, int length, const Char *replace, unsigned rlength) {
    splice__(0, offset, length, replace, rlength);
  }

  // simple read-only cases

  inline ZtZString_<Char, Char2> splice(int offset) const {
    if (offset < 0) {
      if ((offset += m_length) < 0) offset = 0;
    } else {
      if (offset > (int)m_length) offset = m_length;
    }
    return ZtZString_<Char, Char2>(data_() + offset, m_length - offset);
  }

  inline ZtZArray<Char> splice(int offset, int length) const {
    if (offset < 0) {
      if ((offset += m_length) < 0) offset = 0;
    } else {
      if (offset > (int)m_length) offset = m_length;
    }
    if (length < 0) {
      if ((length += m_length - offset) < 0) length = 0;
    } else {
      if (offset + length > (int)m_length) length = m_length - offset;
    }
    return ZtZArray<Char>(data_() + offset, length);
  }

protected:
  inline void splice__(
      ZtString_ *removed,
      int offset,
      int length,
      const Char *replace,
      unsigned rlength) {
    if (offset < 0) { if ((offset += m_length) < 0) offset = 0; }
    if (length < 0) { if ((length += (m_length - offset)) < 0) length = 0; }

    if (offset > (int)m_length) {
      Char *newData;
      if (!m_owned || (int)m_size < offset + rlength + 1)
	newData = size(grow(m_size, offset + rlength + 1));
      else
	newData = data_(), m_null = false;
      if (removed) removed->null();
      Zu::strpad(newData + m_length, offset - m_length);
      if (replace && rlength)
	memcpy(newData + offset, replace, rlength * sizeof(Char));
      newData[m_length = offset + rlength] = 0;
      return;
    }

    if (length == INT_MAX || offset + length > (int)m_length)
      length = m_length - offset;

    int newLength = m_length + rlength - length;

    if (newLength > 0 && (!m_owned || (int)m_size < newLength + 1)) {
      unsigned size = grow(m_size, newLength + 1);
      Char *oldData;
      if (m_builtin)
	oldData = (Char *)m_data;
      else
	oldData = (Char *)m_data[0];
      Char *newData;
      if (size <= (unsigned)BuiltinSize)
	newData = (Char *)m_data;
      else
	newData = (Char *)::malloc(size * sizeof(Char));
      if (removed) removed->init(Copy, oldData + offset, length);
      if (oldData != newData && offset)
	memcpy(newData, oldData, offset * sizeof(Char));
      if (replace && rlength)
	memcpy(newData + offset, replace, rlength * sizeof(Char));
      if (oldData != newData) {
	if (length != (int)rlength)
	  memmove(newData + offset + rlength,
		  oldData + offset + length,
		  (m_length - (offset + length)) * sizeof(Char));
	if (m_mallocd) ::free(oldData);
      }
      newData[m_length = newLength] = 0;
      if (size <= (unsigned)BuiltinSize) {
	m_owned = true, m_mallocd = false, m_builtin = true, m_null = false;
	m_size = BuiltinSize;
      } else {
	m_data[0] = (uintptr_t)newData;
	m_owned = m_mallocd = true, m_builtin = false, m_null = false;
	m_size = size;
      }
      return;
    }

    Char *newData = data_();
    m_null = false;
    if (removed && length) removed->init(Copy, newData + offset, length);
    if (newLength) {
      if ((int)rlength != length && offset + length < (int)m_length)
	memmove(newData + offset + rlength,
		newData + offset + length,
		(m_length - (offset + length)) * sizeof(Char));
      if (replace && rlength)
	memcpy(newData + offset, replace, rlength * sizeof(Char));
    }
    if (newData) newData[m_length = newLength] = 0;
  }

// chomp(), trim(), strip()

private:
  // match whitespace
  ZuInline auto matchS() noexcept {
    return [](int c) {
      return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    };
  }
public:
  // remove trailing characters
  template <typename Match>
  inline void chomp(Match &&match) noexcept {
    if (!m_owned) truncate();
    int o = m_length;
    if (!o) return;
    Char *newData = data_();
    while (--o >= 0 && match(newData[o]));
    newData[m_length = o + 1] = 0;
  }
  ZuInline void chomp() noexcept { return chomp(matchS()); }

  // remove leading characters
  template <typename Match>
  inline void trim(Match &&match) noexcept {
    if (!m_owned) truncate();
    int o;
    Char *newData = data_();
    for (o = 0; o < (int)m_length && match(newData[o]); o++);
    if (!o) return;
    if (!(m_length -= o)) { null(); return; }
    memmove(newData, newData + o, m_length * sizeof(Char));
    newData[m_length] = 0;
  }
  ZuInline void trim() noexcept { return trim(matchS()); }

  // remove leading & trailing characters
  template <typename Match>
  inline void strip(Match &&match) noexcept {
    if (!m_owned) truncate();
    int o = m_length;
    if (!o) return;
    Char *newData = data_();
    while (--o >= 0 && match(newData[o]));
    if (o < 0) { null(); return; }
    m_length = o + 1;
    for (o = 0; o < (int)m_length && match(newData[o]); o++);
    if (!o) { newData[m_length] = 0; return; }
    if (!(m_length -= o)) { null(); return; }
    memmove(newData, newData + o, m_length * sizeof(Char));
    newData[m_length] = 0;
  }
  ZuInline void strip() noexcept { return strip(matchS()); }
 
// sprintf(), vsprintf()

  inline ZtString_ &sprintf(const Char *format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(format, args);
    va_end(args);
    return *this;
  }
  inline ZtString_ &vsprintf(const Char *format, va_list args) {
    vsnprintf(format, args);
    return *this;
  }

  inline static unsigned grow(unsigned o, unsigned n) {
    if (n <= (unsigned)BuiltinSize) return BuiltinSize;
    return
      ZtPlatform::grow(o * sizeof(Char), n * sizeof(Char)) / sizeof(Char);
  }

protected:
  inline void vsnprintf_grow() {
    size(grow(m_size, m_size + ZtString_vsnprintf_Growth));
  }

  void vsnprintf(const Char *format, va_list args) {
    if (!m_owned || m_size <= m_length + 2) vsnprintf_grow();

retry:
    int n = Zu::vsnprintf(
	data_() + m_length, m_size - m_length, format, args);

    if (n < 0) {
      if (m_size >= ZtString_vsnprintf_MaxSize) goto truncate;
      vsnprintf_grow();
      goto retry;
    }

    n += m_length;

    if (n == (int)m_size || n == (int)m_size - 1) {
      if (m_size >= ZtString_vsnprintf_MaxSize) goto truncate;
      vsnprintf_grow();
      goto retry;
    }

    if (n > (int)m_size) {
      if (m_size >= ZtString_vsnprintf_MaxSize) goto truncate;
      size(grow(m_size, n + 2));
      goto retry;
    }

    length(n);
    return;

truncate:
    length(m_size - 1);
  }

  unsigned		m_size:30,	// allocated size of buffer
			m_owned:1,	// owned
			m_mallocd:1;	// malloc'd (implies owned)
  unsigned		m_length:30,	// length
			m_builtin:1,	// built-in (implies !mallocd)
			m_null:1;	// null (implies builtin)
  uintptr_t		m_data[ZtString_BuiltinSize / sizeof(uintptr_t)];
};

template <typename Char_, typename Char2_>
class ZtZString_ : public ZtString_<Char_, Char2_> {
  struct Private { };

public:
  typedef Char_ Char;
  typedef Char2_ Char2;

private:
  typedef ZtString_<Char, Char2> Base;

public:
  ZuInline ZtZString_() { this->null_(); }

  ZuInline ZtZString_(const ZtZString_ &s) :
    Base(Base::NoInit) { Base::ctor(s); }
  inline ZtZString_(ZtZString_ &&s) noexcept {
    if (s.m_null) { this->null_(); return; }
    if (s.m_builtin) { this->copy_(s.data_(), s.m_length); return; }
    if (!s.m_owned) { this->shadow_(s.data_(), s.m_length); return; }
    this->own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    s.m_owned = s.m_builtin,
    s.m_mallocd = false;
  }

  template <typename S>
  ZuInline ZtZString_(S &&s) : Base(Base::NoInit) { Base::ctor(ZuFwd<S>(s)); }

private:
  template <typename S>
  inline typename Base::template FromZtString<S>::T ctor(const S &s)
    { this->shadow_(s.data(), s.length()); }
  template <typename S>
  inline typename Base::template FromZtZString<S>::T ctor(const S &s) {
    if (s.m_null) { this->null_(); return; }
    if (s.m_builtin) { this->copy_(s.data_(), s.m_length); return; }
    if (!s.m_owned) { this->shadow_(s.data_(), s.m_length); return; }
    this->own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    const_cast<ZtZString_ &>(s).m_owned = s.m_builtin,
    const_cast<ZtZString_ &>(s).m_mallocd = false;
  }
  template <typename S>
  inline typename Base::template FromOtherCString<S>::T ctor(S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    const Char *data = s.data();
    uint32_t length = s.length();
    if (!length) { this->null_(); return; }
    this->shadow_(data, length);
  }

  template <typename S> ZuInline typename Base::template FromOtherString<S>::T
    ctor(const S &s) { Base::ctor(s); }
  template <typename C> ZuInline typename Base::template FromChar<C>::T
    ctor(C c) { Base::ctor(c); }

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

  template <typename R> ZuInline typename Base::template CtorReal<R>::T
    ctor(R &&r) { Base::ctor(ZuFwd<R>(r)); }

public:
  enum Copy_ { Copy };
  template <typename S>
  inline ZtZString_(Copy_ _, const S &s) { this->copy(s); }

  inline ZtZString_ &operator =(const ZtZString_ &s)
    { assign(s); return *this; }
  inline ZtZString_ &operator =(ZtZString_ &&s) {
    this->free_();
    new (this) ZtZString_(ZuMv(s));
    return *this;
  }

  template <typename S> inline ZtZString_ &operator =(const S &s)
    { assign(s); return *this; }

private:
  template <typename S>
  inline typename Base::template FromZtString<S>::T assign(const S &s) {
    if ((Base *)this == &s) return;
    this->free_();
    this->shadow_(s.data(), s.length());
  }
  template <typename S>
  inline typename Base::template FromZtZString<S>::T assign(const S &s) {
    if (this == &s) return;
    this->free_();
    if (s.m_null) { this->null_(); return; }
    if (s.m_builtin) { this->copy_(s.data_(), s.m_length); return; }
    if (!s.m_owned) { this->shadow_(s.data_(), s.m_length); return; }
    this->own_(s.data_(), s.m_length, s.m_size, s.m_mallocd);
    const_cast<ZtZString_ &>(s).m_owned = s.m_builtin,
    const_cast<ZtZString_ &>(s).m_mallocd = false;
  }
  template <typename S>
  inline typename Base::template FromOtherCString<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    this->free_();
    const Char *data = s.data();
    uint32_t length = s.length();
    if (!length) { this->null_(); return; }
    this->shadow_(data, length);
  }
  template <typename S> ZuInline typename Base::template FromOtherString<S>::T
    assign(const S &s) { Base::assign(s); }
  template <typename C> ZuInline typename Base::template FromChar<C>::T
    assign(C c) { Base::assign(c); }

  template <typename S> ZuInline typename Base::template FromChar2String<S>::T
    assign(const S &s) { Base::assign(s); }
  template <typename C> ZuInline typename Base::template FromChar2<C>::T
    assign(C c) { Base::assign(c); }

  template <typename P> ZuInline typename Base::template FromPDelegate<P>::T
    assign(const P &p) { Base::assign(p); }
  template <typename P> ZuInline typename Base::template FromPBuffer<P>::T
    assign(const P &p) { Base::assign(p); }

  template <typename R> ZuInline typename Base::template FromReal<R>::T
    assign(const R &r) { Base::assign(r); }

public:
  template <typename S> ZuInline ZtZString_ &operator -=(const S &s)
    { *(Base *)this -= s; return *this; }

  template <typename S>
  inline ZtZString_(S &&s_, ZtIconv *iconv,
      typename ZuIsString<S, Private>::T *_ = 0) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    this->convert_(s, iconv);
  }
  inline ZtZString_(const Char *data, int length, ZtIconv *iconv) {
    ZuArray<Char> s(data, length);
    this->convert_(s, iconv);
  }
  inline ZtZString_(const Char2 *data, int length, ZtIconv *iconv) {
    ZuArray<Char2> s(data, length);
    this->convert_(s, iconv);
  }

  ZuInline ZtZString_(unsigned length, unsigned size) :
      Base(length, size) { }
  ZuInline explicit ZtZString_(const Char *data, unsigned length) :
      Base(data, length) { }
  ZuInline explicit ZtZString_(Copy_ _, const Char *data, unsigned length) :
      Base(Base::Copy, data, length) { }
  ZuInline explicit ZtZString_(
	const Char *data, unsigned length, unsigned size) :
      Base(data, length, size) { }
  ZuInline explicit ZtZString_(
	const Char *data, unsigned length, unsigned size, bool malloc) :
      Base(data, length, size, malloc) { }

  template <typename S> ZuInline ZtZString_ &operator +=(const S &s)
    { *(Base *)this += s; return *this; }
  template <typename S> ZuInline ZtZString_ &operator <<(const S &s)
    { *(Base *)this << s; return *this; }

  inline ZtZString_ &sprintf(const Char *format, ...) {
    va_list args;

    va_start(args, format);
    Base::vsnprintf(format, args);
    va_end(args);
    return *this;
  }
  inline ZtZString_ &vsprintf(const Char *format, va_list args) {
    Base::vsnprintf(format, args);
    return *this;
  }
};

template <typename Char, typename Char2>
template <typename S>
ZuInline void ZtString_<Char, Char2>::convert_(const S &s, ZtIconv *iconv)
{
  null_();
  iconv->convert(*this, s);
}

#ifdef _MSC_VER
ZtExplicit template class ZtAPI ZtString_<char, wchar_t>;
ZtExplicit template class ZtAPI ZtString_<wchar_t, char>;
ZtExplicit template class ZtAPI ZtZString_<char, wchar_t>;
ZtExplicit template class ZtAPI ZtZString_<wchar_t, char>;
#endif

typedef ZtString_<char, wchar_t> ZtString;
typedef ZtString_<wchar_t, char> ZtWString;
typedef ZtZString_<char, wchar_t> ZtZString;
typedef ZtZString_<wchar_t, char> ZtZWString;

// traits

template <typename T> struct ZtStringTraits : public ZuGenericTraits<T> {
  typedef typename T::Char Elem;
  enum {
    IsCString = 1, IsString = 1,
    IsWString = ZuConversion<Elem, wchar_t>::Same,
    IsHashable = 1, IsComparable = 1
  };
  inline static T make(const Elem *data, unsigned length) {
    return T(data, length);
  }
  inline static const Elem *data(const T &s) { return s.data(); }
  inline static unsigned length(const T &s) { return s.length(); }
};
template <> struct ZuTraits<ZtString> : public ZtStringTraits<ZtString> { };
template <> struct ZuTraits<ZtZString> : public ZtStringTraits<ZtString> {
  typedef ZtZString T;
  inline static ZtZString make(const char *data, int length)
    { return ZtZString(data, length); }
};
template <> struct ZuTraits<ZtWString> : public ZtStringTraits<ZtWString> { };
template <> struct ZuTraits<ZtZWString> : public ZtStringTraits<ZtWString> {
  typedef ZtZWString T;
  inline static ZtZWString make(const wchar_t *data, int length)
    { return ZtZWString(data, length); }
};

// RVO shortcuts for ZtZString().sprintf() etc.

#ifdef __GNUC__
ZtZString ZtSprintf(const char *, ...) __attribute__((format(printf, 1, 2)));
#endif
inline ZtZString ZtSprintf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  ZtZString s;
  s.vsprintf(format, args);
  va_end(args);
  return s;
}
inline ZtZWString ZtWSprintf(const wchar_t *format, ...)
{
  va_list args;

  va_start(args, format);
  ZtZWString s;
  s.vsprintf(format, args);
  va_end(args);
  return s;
}

// join

template <
  typename DChar, typename EChar, typename VChar, bool =
    ZuConversion<VChar, DChar>::Same &&
    ZuConversion<VChar, EChar>::Same>
struct ZtJoinable_3 { enum { OK = 0 }; };
template <
  typename DChar, typename EChar, typename VChar>
struct ZtJoinable_3<DChar, EChar, VChar, true> { enum { OK = 1 }; };

template <
  typename DChar, typename E, typename VChar, bool =
    ZuTraits<E>::IsString> struct ZtJoinable_2 { enum { OK = 0 }; };
template <typename DChar, typename E, typename VChar>
struct ZtJoinable_2<DChar, E, VChar, true> :
  public ZtJoinable_3<
    typename ZuTraits<DChar>::T,
    typename ZuTraits<typename ZuTraits<E>::Elem>::T,
    typename ZuTraits<VChar>::T> { };

template <
  typename D, typename A, typename V, bool =
    ZuTraits<D>::IsString &&
    ZuTraits<A>::IsArray &&
    ZuConversion<ZtString___, V>::Base> struct ZtJoinable_ { enum { OK = 0 }; };
template <typename D, typename A, typename V>
struct ZtJoinable_<D, A, V, true> :
  public ZtJoinable_2<
    typename ZuTraits<D>::Elem,
    typename ZuTraits<A>::Elem,
    typename V::Char> { };

template <typename D, typename A, typename O>
inline typename ZuIfT<ZtJoinable_<D, A, O>::OK>::T
ZtJoin(D &&d_, A &&a_, O &o) {
  ZuArrayT<D> d(ZuFwd<D>(d_));
  ZuArrayT<A> a(ZuFwd<A>(a_));
  unsigned n = a.length();
  unsigned l = 0;

  for (unsigned i = 0; i < n; i++) {
    if (i) l += d.length();
    l += ZuTraits<typename ZuTraits<A>::Elem>::length(a[i]);
  }

  o.size(l + 1);

  for (unsigned i = 0; i < n; i++) {
    if (ZuLikely(i)) o << d;
    o << a[i];
  }
}
template <typename D, typename A>
inline typename ZuIfT<ZtJoinable_<D, A, ZtZString>::OK, ZtZString>::T
ZtJoin(D &&d, A &&a) {
  ZtZString o;
  ZtJoin(ZuFwd<D>(d), ZuFwd<A>(a), o);
  return o;
}
template <typename D, typename E>
inline typename ZuIfT<ZtJoinable_<D, ZuArray<E>, ZtZString>::OK, ZtZString>::T
ZtJoin(D &&d, std::initializer_list<E> &&a) {
  ZtZString o;
  ZtJoin(ZuFwd<D>(d), ZuArray<E>(ZuFwd<std::initializer_list<E> >(a)), o);
  return o;
}
template <typename D, typename A>
inline typename ZuIfT<ZtJoinable_<D, A, ZtZWString>::OK, ZtZWString>::T
ZtJoin(D &&d, A &&a) {
  ZtZWString o;
  ZtJoin(ZuFwd<D>(d), ZuFwd<A>(a), o);
  return o;
}
template <typename D, typename E>
inline typename ZuIfT<ZtJoinable_<D, ZuArray<E>, ZtZWString>::OK, ZtZWString>::T
ZtJoin(D &&d, std::initializer_list<E> &&a) {
  ZtZString o;
  ZtJoin(ZuFwd<D>(d), ZuArray<E>(ZuFwd<std::initializer_list<E> >(a)), o);
  return o;
}

// hex dump

// copies of the prefix and the data are taken since this is used mainly for
// troubleshooting / logging; ZeLog printing is deferred to a later time by
// the logger, which runs in a different thread and stack, and both the
// prefix and the data are possibly/probably transient and subsequently
// overwritten/freed by the caller in the interim
struct ZtAPI ZtHexDump {
  ZuInline ZtHexDump(ZuString prefix_, const void *data_, unsigned length_) :
      prefix(prefix_), data(0), length(length_) {
    if (ZuLikely(data = (uint8_t *)::malloc(length)))
      memcpy(data, data_, length);
  }
  ZuInline ~ZtHexDump() {
    if (ZuLikely(data)) ::free(data);
  }
  ZuInline ZtHexDump(ZtHexDump &&d) :
    prefix(ZuMv(d.prefix)), data(d.data), length(d.length) {
    d.data = 0;
  }
  ZuInline ZtHexDump &operator =(ZtHexDump &&d) {
    prefix = ZuMv(d.prefix);
    data = d.data;
    length = d.length;
    d.data = 0;
    return *this;
  }
  ZtHexDump(const ZtHexDump &) = delete;
  ZtHexDump &operator =(const ZtHexDump &) = delete;

  void print(ZmStream &s) const;

private:
  ZtString	prefix;
  uint8_t	*data;
  unsigned	length;
};
template <> struct ZuPrint<ZtHexDump> : public ZuPrintDelegate<ZtHexDump> {
  ZuInline static void print(ZmStream &s, const ZtHexDump &d) {
    d.print(s);
  }
  template <typename S>
  ZuInline static void print(S &s_, const ZtHexDump &d) {
    ZmStream s(s_);
    d.print(s);
  }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// generic printing
template <> struct ZuPrint<ZtString> : public ZuPrintString<ZtString> { };
template <> struct ZuPrint<ZtZString> : public ZuPrintString<ZtString> { };

#endif /* ZtString_HPP */
