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

// heap-allocated dynamic string class
//
// * fast, lightweight
// * short-circuits heap allocation for small strings below a built-in size
// * supports both zero-copy and deep-copy
// * thin layer on ANSI C string functions
// * no locale or character set overhead except where explicitly requested

#ifndef ZtString_HPP
#define ZtString_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <zlib/ZuIfT.hpp>
#include <zlib/ZuInt.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuStringFn.hpp>
#include <zlib/ZuUTF.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>

#include <zlib/ZmAssert.hpp>
#include <zlib/ZmStream.hpp>

#include <zlib/ZtPlatform.hpp>
#include <zlib/ZtArray.hpp>
#include <zlib/ZtIconv.hpp>

// built-in buffer size (before falling back to malloc())
// Note: must be a multiple of sizeof(uintptr_t)
#define ZtString_BuiltinSize	(3 * sizeof(uintptr_t))

// buffer size increment for vsnprintf()
#define ZtString_vsnprintf_Growth	256
#define ZtString_vsnprintf_MaxSize	(1U<<20) // 1Mb

template <typename Char> const Char *ZtString_Null();
template <> ZuInline const char *ZtString_Null() { return ""; }
template <> ZuInline const wchar_t *ZtString_Null() {
  return Zu::nullWString();
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4348)
#endif

struct ZtString___ { };
template <typename Char> struct ZtString__ : public ZtString___ { };

template <typename Char, typename Char2> class ZtString_;

template <typename Char_, typename Char2_>
class ZtString_ : public ZtString__<Char_> {
public:
  typedef Char_ Char;
  typedef Char2_ Char2;
  enum { IsWString = ZuConversion<Char, wchar_t>::Same };
  enum { BuiltinSize = (int)ZtString_BuiltinSize / sizeof(Char) };

private:
  // from same type ZtString
  template <typename U, typename R = void,
    typename V = Char, typename W = Char2,
    bool A = ZuConversion<ZtString_<V, W>, U>::Same> struct MatchZtString;
  template <typename U, typename R>
    struct MatchZtString<U, R, Char, Char2, true> { typedef R T; };

  // from string literal with same char
  template <typename U, typename V = Char> struct IsLiteral {
    enum { OK = ZuTraits<U>::IsCString &&
      ZuTraits<U>::IsArray && ZuTraits<U>::IsPrimitive &&
      ZuConversion<typename ZuTraits<U>::Elem, const V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchLiteral : public ZuIfT<IsLiteral<U>::OK, R> { };

  // from some other C string with same char (other than a string literal)
  template <typename U, typename V = Char> struct IsCString {
    enum { OK = !ZuConversion<ZtString__<V>, U>::Base &&
      !IsLiteral<U>::OK && ZuTraits<U>::IsCString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchCString : public ZuIfT<IsCString<U>::OK, R> { };

  // from some other C string with same char (including string literals)
  template <typename U, typename V = Char> struct IsAnyCString {
    enum { OK = !ZuConversion<ZtString__<V>, U>::Base &&
      ZuTraits<U>::IsCString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchAnyCString : public ZuIfT<IsAnyCString<U>::OK, R> { };

  // from some other non-C string with same char (non-null-terminated)
  template <typename U, typename V = Char> struct IsOtherString {
    enum { OK = !ZuConversion<ZtString__<V>, U>::Base &&
      ZuTraits<U>::IsString && !ZuTraits<U>::IsCString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchOtherString : public ZuIfT<IsOtherString<U>::OK, R> { };

  // from individual char (other than wchar_t, which is ambiguous)
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct MatchChar;
  template <typename U, typename R>
  struct MatchChar<U, R, Char, true> { typedef R T; };

  // from char2 string (requires conversion)
  template <typename U, typename R = void, typename V = Char2,
    bool A = ZuTraits<U>::IsString &&
      ((!ZuTraits<U>::IsWString && ZuConversion<char, V>::Same) ||
       (ZuTraits<U>::IsWString && ZuConversion<wchar_t, V>::Same))
    > struct MatchChar2String;
  template <typename U, typename R>
    struct MatchChar2String<U, R, Char2, true> { typedef R T; };

  // from individual char2 (requires conversion, char->wchar_t only)
  template <typename U, typename R = void, typename V = Char2,
    bool B = ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same> struct MatchChar2;
  template <typename U, typename R>
  struct MatchChar2<U, R, Char2, true> { typedef R T; };

  // from printable type
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct MatchPrint;
  template <typename U, typename R>
  struct MatchPrint<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Delegate> struct MatchPDelegate;
  template <typename U, typename R>
  struct MatchPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Buffer> struct MatchPBuffer;
  template <typename U, typename R>
  struct MatchPBuffer<U, R, char, true> { typedef R T; };

  // from any other real and primitive type (integers, floating point, etc.)
  template <typename U, typename R = void, typename V = Char,
    bool B =
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuConversion<U, char>::Same &&
      !ZuTraits<U>::IsArray
    > struct MatchReal { };
  template <typename U, typename R>
  struct MatchReal<U, R, char, true> { typedef R T; };

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
private:
  enum NoInit_ { NoInit };
  ZuInline ZtString_(NoInit_ _) { }
public:
  ZuInline ZtString_(const ZtString_ &s) {
    copy_(s.data_(), s.length());
  }
  ZtString_(ZtString_ &&s) noexcept {
    if (s.null__()) { null_(); return; }
    if (s.builtin()) { copy_(s.data_(), s.length()); return; }
    if (!s.owned()) { shadow_(s.data_(), s.length()); return; }
    own_(s.data_(), s.length(), s.size(), s.mallocd());
    s.owned(s.builtin());
    s.mallocd(0);
  }

  template <typename S> ZuInline ZtString_(S &&s) { ctor(ZuFwd<S>(s)); }

private:
  template <typename S> ZuInline typename MatchZtString<S>::T ctor(const S &s)
    { copy_(s.data_(), s.length()); }
  template <typename S> ZuInline typename MatchLiteral<S>::T ctor(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); shadow_(s.data(), s.length()); }
  template <typename S> ZuInline typename MatchCString<S>::T ctor(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename S> ZuInline typename MatchOtherString<S>::T ctor(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename C> ZuInline typename MatchChar<C>::T ctor(C c)
    { copy_(&c, 1); }

  template <typename S> ZuInline typename MatchChar2String<S>::T ctor(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(alloc_(o + 1, 0), o), s));
  }

  template <typename C> ZuInline typename MatchChar2<C>::T ctor(C c) {
    ZuArray<Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (!o) { null_(); return; }
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(alloc_(o + 1, 0), o), s));
  }

  template <typename P> ZuInline typename MatchPDelegate<P>::T ctor(const P &p)
    { null_(); ZuPrint<P>::print(*this, p); }
  template <typename P> ZuInline typename MatchPBuffer<P>::T ctor(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (!o) { null_(); return; }
    length_(ZuPrint<P>::print(alloc_(o + 1, 0), o, p));
  }

  template <typename V> ZuInline typename CtorSize<V>::T ctor(V size) {
    if (!size) { null_(); return; }
    alloc_(size, 0)[0] = 0;
  }

  template <typename R> ZuInline typename CtorReal<R>::T ctor(R &&r)
    { ctor(ZuBoxed(ZuFwd<R>(r))); }

public:
  enum Copy_ { Copy };
  template <typename S> ZuInline ZtString_(Copy_ _, const S &s) { copy(s); }

private:
  ZuInline void copy(const ZtString_ &s)
    { copy_(s.data_(), s.length()); }
  template <typename S> ZuInline typename MatchAnyCString<S>::T copy(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename S> ZuInline typename MatchOtherString<S>::T copy(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); copy_(s.data(), s.length()); }
  template <typename C> ZuInline typename MatchChar<C>::T copy(C c)
    { copy_(&c, 1); }

  template <typename S> ZuInline typename MatchChar2String<S>::T copy(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (ZuUnlikely(!o)) { length_(0); return; }
    unsigned z = size();
    Char *data;
    if (!owned() || o >= z)
      data = size(o + 1);
    else
      data = data_();
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(data, o), s));
  }
	  
  template <typename C> ZuInline typename MatchChar2<C>::T copy(C c) {
    ZuArray<Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    if (ZuUnlikely(!o)) { length_(0); return; }
    unsigned z = size();
    Char *data;
    if (!owned() || o >= z)
      data = size(o + 1);
    else
      data = data_();
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(data, o), s));
  }

public:
  ZuInline ZtString_ &operator =(const ZtString_ &s) {
    if (this == &s) return *this;
    Char *oldData = free_1();
    copy_(s.data_(), s.length());
    free_2(oldData);
    return *this;
  }
  ZuInline ZtString_ &operator =(ZtString_ &&s) noexcept {
    free_();
    new (this) ZtString_(ZuMv(s));
    return *this;
  }

  template <typename S>
  ZuInline ZtString_ &operator =(S &&s) { assign(ZuFwd<S>(s)); return *this; }

private:
  template <typename S> typename MatchZtString<S>::T assign(const S &s) { 
    if (this == &s) return;
    Char *oldData = free_1();
    copy_(s.data_(), s.length());
    free_2(oldData);
  }
  template <typename S> typename MatchLiteral<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    free_();
    shadow_(s.data(), s.length());
  }
  template <typename S> typename MatchCString<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    Char *oldData = free_1();
    copy_(s.data(), s.length());
    free_2(oldData);
  }
  template <typename S> typename MatchOtherString<S>::T assign(S &&s_) { 
    ZuArray<Char> s(ZuFwd<S>(s_));
    Char *oldData = free_1();
    copy_(s.data(), s.length());
    free_2(oldData);
  }
  template <typename C> typename MatchChar<C>::T assign(C c) {
    Char *oldData = free_1();
    copy_(&c, 1);
    free_2(oldData);
  }

  template <typename S> typename MatchChar2String<S>::T assign(S &&s_) {
    ZuArray<Char2> s(ZuFwd<S>(s_));
    unsigned o = ZuUTF<Char, Char2>::len(s);
    unsigned z = size();
    if (ZuUnlikely(!o)) { length_(0); return; }
    Char *data;
    if (!owned() || o >= z)
      data = size(o + 1);
    else
      data = data_();
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(data, o), s));
  }
  template <typename C> typename MatchChar2<C>::T assign(C c) {
    ZuArray<Char2> s{&c, 1};
    unsigned o = ZuUTF<Char, Char2>::len(s);
    unsigned z = size();
    if (ZuUnlikely(!o)) { length_(0); return; }
    Char *data;
    if (!owned() || o >= z)
      data = size(o + 1);
    else
      data = data_();
    length_(ZuUTF<Char, Char2>::cvt(ZuArray<Char>(data, o), s));
  }

  template <typename P> typename MatchPDelegate<P>::T assign(const P &p)
    { ZuPrint<P>::print(*this, p); }
  template <typename P> typename MatchPBuffer<P>::T assign(const P &p) {
    unsigned o = ZuPrint<P>::length(p);
    if (ZuUnlikely(!o)) { length_(0); return; }
    unsigned z = size();
    Char *data;
    if (!owned() || o >= z)
      data = size(o + 1);
    else
      data = data_();
    length_(ZuPrint<P>::print(data, o, p));
  }

  template <typename R> typename MatchReal<R>::T assign(const R &r) {
    assign(ZuBoxed(r));
  }

public:
  template <typename S> ZtString_ &operator -=(const S &s) {
    shadow(s);
    return *this;
  }

private:
  template <typename S> typename MatchZtString<S>::T shadow(const S &s) {
    if (this == &s) return;
    free_();
    shadow_(s.data_(), s.length());
  }
  template <typename S>
  typename MatchAnyCString<S>::T shadow(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); free_(); shadow_(s.data(), s.length()); }
  template <typename S>
  typename MatchOtherString<S>::T shadow(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); free_(); shadow_(s.data(), s.length()); }

public:
  template <typename S>
  ZtString_(S &&s_, ZtIconv *iconv, typename ZuIsString<S>::T *_ = 0) {
    ZuArrayT<S> s(ZuFwd<S>(s_));
    convert_(s, iconv);
  }
  ZtString_(const Char *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char> s(data, length);
    convert_(s, iconv);
  }
  ZtString_(const Char2 *data, unsigned length, ZtIconv *iconv) {
    ZuArray<Char2> s(data, length);
    convert_(s, iconv);
  }

public:
  ZtString_(unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    alloc_(size, length)[length] = 0;
  }
  explicit ZtString_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    init_(data, length);
  }
  explicit ZtString_(Copy_ _, const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  explicit ZtString_(Char *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, 1);
  }
  explicit ZtString_(
      Char *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

  ~ZtString_() { free_(); }

// re-initializers

  void init() { free_(); init_(); }
  void init_() { null_(); }

  template <typename S> void init(const S &s) { assign(s); }
  template <typename S> void init_(const S &s) { ctor(s); }

  void init(unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    unsigned z = this->size();
    if (z < size) {
      free_();
      alloc_(size, length);
    } else
      length_(length);
  }
  void init_(unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    alloc_(size, length);
  }
  void init(const Char *data, unsigned length) {
    free_();
    init_(data, length);
  }
  void init_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    if (data[length])
      copy_(data, length);
    else
      shadow_(data, length);
  }
  void init(Copy_ _, const Char *data, unsigned length) {
    Char *oldData = free_1();
    init_(_, data, length);
    free_2(oldData);
  }
  void init_(Copy_ _, const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    copy_(data, length);
  }
  void init(const Char *data, unsigned length, unsigned size) {
    free_();
    init_(data, length, size);
  }
  void init_(const Char *data, unsigned length, unsigned size) {
    if (!size) { null_(); return; }
    own_(data, length, size, true);
  }
  void init(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    free_();
    init_(data, length, size, mallocd);
  }
  void init_(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) { null_(); return; }
    own_(data, length, size, mallocd);
  }

// internal initializers / finalizer

private:
  ZuInline void length_(unsigned n) {
    null__(0);
    length__(n);
    data_()[n] = 0;
  }

  ZuInline void null_() {
    m_data[0] = 0;
    size_owned_null(BuiltinSize, 1, 1);
    length_mallocd_builtin(0, 0, 1);
  }

  void own_(
      const Char *data, unsigned length, unsigned size, bool mallocd) {
    if (!size) {
      if (data && mallocd) ::free((void *)data);
      null_();
      return;
    }
    m_data[0] = (uintptr_t)(Char *)data;
    size_owned_null(size, 1, 0);
    length_mallocd_builtin(length, mallocd, 0);
  }

  void shadow_(const Char *data, unsigned length) {
    if (!length) { null_(); return; }
    m_data[0] = (uintptr_t)(Char *)data;
    size_owned_null(length + 1, 0, 0);
    length_mallocd_builtin(length, 0, 0);
  }

  Char *alloc_(unsigned size, unsigned length) {
    if (ZuLikely(size <= (unsigned)BuiltinSize)) {
      size_owned_null(size, 1, 0);
      length_mallocd_builtin(length, 0, 1);
      return (Char *)m_data;
    }
    Char *newData = (Char *)::malloc(size * sizeof(Char));
    if (ZuUnlikely(!newData)) { null_(); return 0; }
    m_data[0] = (uintptr_t)newData;
    size_owned_null(size, 1, 0);
    length_mallocd_builtin(length, 1, 0);
    return newData;;
  }

  void copy_(const Char *copyData, unsigned length) {
    if (!length) { null_(); return; }
    if (length < (unsigned)BuiltinSize) {
      memcpy((Char *)m_data, copyData, length * sizeof(Char));
      ((Char *)m_data)[length] = 0;
      size_owned_null(BuiltinSize, 1, 0);
      length_mallocd_builtin(length, 0, 1);
      return;
    }
    Char *newData = (Char *)::malloc((length + 1) * sizeof(Char));
    if (ZuUnlikely(!newData)) { null_(); return; }
    memcpy(newData, copyData, length * sizeof(Char));
    newData[length] = 0;
    m_data[0] = (uintptr_t)newData;
    size_owned_null(length + 1, 1, 0);
    length_mallocd_builtin(length, 1, 0);
  }

  template <typename S> void convert_(const S &s, ZtIconv *iconv);

  void free_() {
    if (mallocd()) if (Char *data = (Char *)m_data[0]) ::free(data);
  }
  Char *free_1() {
    if (!mallocd()) return 0;
    return data_();
  }
  void free_2(Char *data) {
    if (data) ::free(data);
  }

public:
// truncation (to minimum size)

  ZuInline void truncate() { size(length() + 1); }

// array / ptr operators

  ZuInline Char &operator [](int i) { return data_()[i]; }
  ZuInline Char operator [](int i) const { return data_()[i]; }

  ZuInline operator Char *() { return null__() ? 0 : data_(); }
  ZuInline operator const Char *() const { return null__() ? 0 : data_(); }

// accessors

  ZuInline Char *data() { if (null__()) return 0; return data_(); }
  ZuInline Char *data_() {
    return builtin() ? (Char *)m_data : (Char *)m_data[0];
  }
  ZuInline const Char *data() const { if (null__()) return 0; return data_(); }
  ZuInline const Char *data_() const {
    return builtin() ? (const Char *)m_data : (const Char *)m_data[0];
  }
  ZuInline const Char *ndata() const {
    if (null__()) return ZtString_Null<Char>();
    return data_();
  }

  ZuInline unsigned length() const {
    return m_length_mallocd_builtin & ~(3U<<30U);
  }
  ZuInline unsigned size() const {
    uint32_t u = m_size_owned_null;
    return u & ~(uint32_t)(((int32_t)u)>>31) & ~(3U<<30U);
  }
  ZuInline bool mallocd() const { return (m_length_mallocd_builtin>>30U) & 1U; }
  ZuInline bool builtin() const { return m_length_mallocd_builtin>>31U; }
  ZuInline bool owned() const { return (m_size_owned_null>>30U) & 1U; }

private:
  ZuInline void length__(unsigned v) {
    m_length_mallocd_builtin = (m_length_mallocd_builtin & (3U<<30U)) | v;
  }
  ZuInline void mallocd(bool v) {
    m_length_mallocd_builtin =
      (m_length_mallocd_builtin & ~(1U<<30U)) | (((uint32_t)v)<<30U);
  }
  ZuInline void builtin(bool v) {
    m_length_mallocd_builtin =
      (m_length_mallocd_builtin & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void length_mallocd_builtin(unsigned l, bool m, bool b) {
    m_length_mallocd_builtin = l | (((uint32_t)m)<<30U) | (((uint32_t)b)<<31U);
  }
  ZuInline unsigned size_() const {
    return m_size_owned_null & ~(3U<<30U);
  }
  ZuInline void size_(unsigned v) {
    m_size_owned_null = (m_size_owned_null & (3U<<30U)) | v;
  }
  ZuInline void owned(bool v) {
    m_size_owned_null =
      (m_size_owned_null & ~(1U<<30U)) | (((uint32_t)v)<<30U);
  }
  ZuInline bool null__() const { return m_size_owned_null>>31U; }
  ZuInline void null__(bool v) {
    m_size_owned_null =
      (m_size_owned_null & ~(1U<<31U)) | (((uint32_t)v)<<31U);
  }
  ZuInline void size_owned_null(unsigned z, bool o, bool n) {
    m_size_owned_null = z | (((uint32_t)o)<<30U) | (((uint32_t)n)<<31U);
  }

public:
  Char *data(bool move) {
    if (!move) return data_();
    if (null__()) return 0;
    if (builtin()) {
      Char *newData = (Char *)::malloc(BuiltinSize * sizeof(Char));
      memcpy(newData, m_data, (length() + 1) * sizeof(Char));
      return newData;
    } else {
      owned(0);
      mallocd(0);
      return (Char *)m_data[0];
    }
  }

// reset to null string

  ZuInline void null() {
    free_();
    null_();
  }

// set length

  void length(unsigned n) {
    if (!owned() || n >= size_()) size(n + 1);
    length_(n);
  }
  void calcLength() {
    if (null__())
      length__(0);
    else
      length__(Zu::strlen_(data_()));
  }

// set size

  Char *ensure(unsigned z) {
    if (ZuLikely(owned() && z <= size_())) return data_();
    return size(z);
  }
  Char *size(unsigned z) {
    if (ZuUnlikely(!z)) { null(); return 0; }
    if (owned() && z == size_()) return data_();
    Char *oldData = data_();
    Char *newData;
    if (z <= (unsigned)BuiltinSize)
      newData = (Char *)m_data;
    else
      newData = (Char *)::malloc(z * sizeof(Char));
    unsigned n = z - 1U;
    if (n > length()) n = length();
    if (oldData != newData) {
      memcpy(newData, oldData, (n + 1) * sizeof(Char));
      if (mallocd()) ::free(oldData);
    }
    if (z <= (unsigned)BuiltinSize) {
      size_owned_null(z, 1, 0);
      length_mallocd_builtin(n, 0, 1);
      return newData;
    }
    m_data[0] = (uintptr_t)newData;
    size_owned_null(z, 1, 0);
    length_mallocd_builtin(n, 1, 0);
    return newData;
  }

// common prefix

  template <typename S>
  typename MatchZtString<S, ZuArray<Char> >::T prefix(const S &s) {
    if (this == &s) return ZuArray<Char>(data_(), length() + 1);
    return prefix(s.data_(), s.length());
  }
  template <typename S>
  typename MatchAnyCString<S, ZuArray<Char> >::T prefix(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); return prefix(s.data(), s.length()); }
  template <typename S>
  typename MatchOtherString<S, ZuArray<Char> >::T prefix(S &&s_)
    { ZuArray<Char> s(ZuFwd<S>(s_)); return prefix(s.data(), s.length()); }

  ZuArray<Char> prefix(const Char *pfxData, unsigned length) const {
    if (null__()) return ZuArray<Char>();
    const Char *p1 = data_();
    if (!pfxData) return ZuArray<Char>(p1, 1);
    const Char *p2 = pfxData;
    unsigned i, n = this->length();
    n = n > length ? length : n;
    for (i = 0; i < n && p1[i] == p2[i]; ++i);
    return ZuArray<Char>(data_(), i);
  }

public:
  ZuInline uint32_t hash() const { return ZuHash<ZtString_>::hash(*this); }

// comparison

  ZuInline bool operator !() const { return !length(); }

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

  int cmp(const Char *s, unsigned n) const {
    if (null__()) return s ? -1 : 0;
    if (!s) return 1;
    return Zu::strcmp_(data_(), s, n);
  }
  bool equals(const Char *s, unsigned n) const {
    if (null__()) return !s;
    if (!s) return false;
    return !Zu::strcmp_(data_(), s, n);
  }
  int icmp(const Char *s, unsigned n) const {
    if (null__()) return s ? -1 : 0;
    if (!s) return 1;
    return Zu::stricmp_(data_(), s, n);
  }

// +, += operators
  template <typename S>
  ZtString_<Char, Char2> operator +(const S &s) const { return add(s); }

private:
  template <typename S>
  ZuInline typename MatchZtString<S, ZtString_<Char, Char2> >::T
    add(const S &s) const { return add(s.data_(), s.length()); }
  template <typename S>
  ZuInline typename MatchAnyCString<S, ZtString_<Char, Char2> >::T
    add(S &&s_) const
      { ZuArray<Char> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename S>
  ZuInline typename MatchOtherString<S, ZtString_<Char, Char2> >::T
    add(S &&s_) const
      { ZuArray<Char> s(ZuFwd<S>(s_)); return add(s.data(), s.length()); }
  template <typename C>
  ZuInline typename MatchChar<C, ZtString_<Char, Char2> >::T
    add(C c) const { return add(&c, 1); }

  template <typename S>
  ZuInline typename MatchChar2String<S, ZtString_<Char, Char2> >::T
    add(const S &s) const { return add(ZtString_(s)); }
  template <typename C>
  ZuInline typename MatchChar2<C, ZtString_<Char, Char2> >::T
    add(C c) const { return add(ZtString_(c)); }

  template <typename P>
  ZuInline typename MatchPDelegate<P, ZtString_<Char, Char2> >::T
      add(const P &p) const { return add(ZtString_(p)); }
  template <typename P>
  ZuInline typename MatchPBuffer<P, ZtString_<Char, Char2> >::T
      add(const P &p) const { return add(ZtString_(p)); }

  ZtString_<Char, Char2> add(
      const Char *data, unsigned length) const {
    unsigned n = this->length();
    unsigned o = n + length;
    if (ZuUnlikely(!o)) return ZtString_<Char, Char2>();
    Char *newData = (Char *)::malloc((o + 1) * sizeof(Char));
    if (n) memcpy(newData, data_(), n * sizeof(Char));
    if (length) memcpy(newData + n, data, length * sizeof(Char));
    newData[o] = 0;
    return ZtString_<Char, Char2>(newData, o, o + 1);
  }

public:
  template <typename S> ZtString_ &operator +=(const S &s)
    { append_(s); return *this; }
  template <typename S> ZtString_ &operator <<(const S &s)
    { append_(s); return *this; }

private:
  template <typename S>
  typename MatchZtString<S>::T append_(const S &s) {
    if (this == &s) {
      ZtString_ s_ = s;
      splice__(0, length(), 0, s_.data_(), s_.length());
    } else
      splice__(0, length(), 0, s.data_(), s.length());
  }
  template <typename S>
  ZuInline typename MatchAnyCString<S>::T append_(S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(0, length(), 0, s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchOtherString<S>::T append_(S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(0, length(), 0, s.data(), s.length());
  }
  template <typename C>
  typename MatchChar<C>::T append_(C c) {
    unsigned n = length();
    unsigned z = size_();
    Char *data;
    if (!owned() || n + 2 >= z)
      data = size(grow(z, n + 2));
    else
      data = data_();
    data[n++] = c;
    length_(n);
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T append_(const S &s)
    { append_(ZtString_(s)); }
  template <typename C>
  ZuInline typename MatchChar2<C>::T append_(C c)
    { append_(ZtString_(c)); }

  template <typename P>
  typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P> typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned n = length();
    unsigned z = size_();
    unsigned o = ZuPrint<P>::length(p);
    Char *data;
    if (!owned() || z <= n + o)
      data = size(grow(z, n + o + 1));
    else
      data = data_();
    length_(n + ZuPrint<P>::print(data + n, o, p));
  }

  template <typename R> ZuInline typename MatchReal<R>::T append_(R &&r)
    { append_(ZuBoxed(ZuFwd<R>(r))); }

public:
  ZuInline void append(const Char *data, unsigned length) {
    if (data) splice__(0, this->length(), 0, data, length);
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

private:
  template <typename S>
  typename MatchZtString<S>::T splice_(
      ZtString_ *removed, int offset, int length, const S &s) {
    if (this == &s) {
      ZtString_ s_ = s;
      splice__(removed, offset, length, s_.data_(), s_.length());
    } else
      splice__(removed, offset, length, s.data_(), s.length());
  }
  template <typename S>
  ZuInline typename MatchAnyCString<S>::T splice_(
      ZtString_ *removed, int offset, int length, S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }
  template <typename S>
  ZuInline typename MatchOtherString<S>::T splice_(
      ZtString_ *removed, int offset, int length, S &&s_) {
    ZuArray<Char> s(ZuFwd<S>(s_));
    splice__(removed, offset, length, s.data(), s.length());
  }
  template <typename C>
  ZuInline typename MatchChar<C>::T splice_(
      ZtString_ *removed, int offset, int length, C c) {
    splice__(removed, offset, length, &c, 1);
  }
  template <typename S>
  ZuInline typename MatchChar2String<S>::T splice_(
      ZtString_ *removed, int offset, int length, const S &s) {
    splice_(removed, offset, length, ZtString_(s));
  }
  template <typename C>
  ZuInline typename MatchChar2<C>::T splice_(
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

  ZuArray<Char> splice(int offset) const {
    unsigned n = length();
    if (offset < 0) {
      if ((offset += n) < 0) offset = 0;
    } else {
      if (offset > (int)n) offset = n;
    }
    return ZuArray<Char>(data_() + offset, n - offset);
  }

  ZuArray<Char> splice(int offset, int length) const {
    unsigned n = this->length();
    if (offset < 0) {
      if ((offset += n) < 0) offset = 0;
    } else {
      if (offset > (int)n) offset = n;
    }
    if (length < 0) {
      if ((length += n - offset) < 0) length = 0;
    } else {
      if (offset + length > (int)n) length = n - offset;
    }
    return ZuArray<Char>(data_() + offset, length);
  }

private:
  void splice__(
      ZtString_ *removed,
      int offset,
      int length,
      const Char *replace,
      unsigned rlength) {
    unsigned n = this->length();
    unsigned z = size_();
    if (offset < 0) { if ((offset += n) < 0) offset = 0; }
    if (length < 0) { if ((length += (n - offset)) < 0) length = 0; }

    if (offset > (int)n) {
      if (removed) removed->null();
      Char *data;
      if (!owned() || offset + (int)rlength >= (int)z) {
	z = grow(z, offset + rlength + 1);
	data = size(z);
      } else
	data = data_();
      Zu::strpad(data + n, offset - n);
      if (rlength) memcpy(data + offset, replace, rlength * sizeof(Char));
      length_(offset + rlength);
      return;
    }

    if (length == INT_MAX || offset + length > (int)n)
      length = n - offset;

    int l = n + rlength - length;

    if (l > 0 && (!owned() || l >= (int)z)) {
      z = grow(z, l + 1);
      Char *oldData = data_();
      if (removed) removed->init(Copy, oldData + offset, length);
      Char *newData;
      if (z <= (unsigned)BuiltinSize)
	newData = (Char *)m_data;
      else
	newData = (Char *)::malloc(z * sizeof(Char));
      if (oldData != newData && offset)
	memcpy(newData, oldData, offset * sizeof(Char));
      if (rlength)
	memcpy(newData + offset, replace, rlength * sizeof(Char));
      if (oldData != newData) {
	if ((int)rlength != length && offset + length < (int)n)
	  memmove(newData + offset + rlength,
		  oldData + offset + length,
		  (n - (offset + length)) * sizeof(Char));
	if (mallocd()) ::free(oldData);
      }
      newData[l] = 0;
      if (z <= (unsigned)BuiltinSize) {
	size_owned_null(z, 1, 0);
	length_mallocd_builtin(l, 0, 1);
	return;
      }
      m_data[0] = (uintptr_t)newData;
      size_owned_null(z, 1, 0);
      length_mallocd_builtin(l, 1, 0);
      return;
    }

    Char *data = data_();
    if (removed) removed->init(Copy, data + offset, length);
    if (l > 0) {
      if ((int)rlength != length && offset + length < (int)n)
	memmove(data + offset + rlength,
		data + offset + length,
		(n - (offset + length)) * sizeof(Char));
      if (rlength) memcpy(data + offset, replace, rlength * sizeof(Char));
    }
    length_(l);
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
  void chomp(Match match) noexcept {
    if (!owned()) truncate();
    int o = length();
    if (!o) return;
    Char *data = data_();
    while (--o >= 0 && match(data[o]));
    length_(o + 1);
  }
  ZuInline void chomp() noexcept { return chomp(matchS()); }

  // remove leading characters
  template <typename Match>
  void trim(Match match) noexcept {
    if (!owned()) truncate();
    unsigned n = length();
    unsigned o;
    Char *data = data_();
    for (o = 0; o < n && match(data[o]); o++);
    if (!o) return;
    if (!(n -= o)) { null(); return; }
    memmove(data, data + o, n * sizeof(Char));
    length_(n);
  }
  ZuInline void trim() noexcept { return trim(matchS()); }

  // remove leading & trailing characters
  template <typename Match>
  void strip(Match match) noexcept {
    if (!owned()) truncate();
    int o = length();
    if (!o) return;
    Char *data = data_();
    while (--o >= 0 && match(data[o]));
    if (o < 0) { null(); return; }
    length_(o + 1);
    unsigned n = o + 1;
    for (o = 0; o < (int)n && match(data[o]); o++);
    if (!o) { length_(n); return; }
    if (!(n -= o)) { null(); return; }
    memmove(data, data + o, n * sizeof(Char));
    length_(n);
  }
  ZuInline void strip() noexcept { return strip(matchS()); }
 
// sprintf(), vsprintf()

  ZtString_ &sprintf(const Char *format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(format, args);
    va_end(args);
    return *this;
  }
  ZtString_ &vsprintf(const Char *format, va_list args) {
    vsnprintf(format, args);
    return *this;
  }

  static unsigned grow(unsigned o, unsigned n) {
    if (n <= (unsigned)BuiltinSize) return BuiltinSize;
    return
      ZtPlatform::grow(o * sizeof(Char), n * sizeof(Char)) / sizeof(Char);
  }

private:
  unsigned vsnprintf_grow(unsigned z) {
    z = grow(z, z + ZtString_vsnprintf_Growth);
    size(z);
    return z;
  }

  void vsnprintf(const Char *format, va_list args) {
    unsigned n = length();
    unsigned z = size_();

    if (!owned() || n + 2 >= z)
      z = vsnprintf_grow(z);

retry:
    Char *data = data_();

    int r = Zu::vsnprintf(data + n, z - n, format, args);

    if (r < 0 || (n += r) == z || n == z - 1) {
      if (z >= ZtString_vsnprintf_MaxSize) goto truncate;
      z = vsnprintf_grow(z);
      n = length();
      goto retry;
    }

    if (n > z) {
      if (z >= ZtString_vsnprintf_MaxSize) goto truncate;
      size(z = grow(z, n + 2));
      n = length();
      goto retry;
    }

    length_(n);
    return;

truncate:
    length_(z - 1);
  }

  uint32_t		m_size_owned_null;
  uint32_t		m_length_mallocd_builtin;
  uintptr_t		m_data[ZtString_BuiltinSize / sizeof(uintptr_t)];
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
#endif

typedef ZtString_<char, wchar_t> ZtString;
typedef ZtString_<wchar_t, char> ZtWString;

// traits

template <typename T> struct ZtStringTraits : public ZuGenericTraits<T> {
  typedef typename T::Char Elem;
  enum {
    IsCString = 1, IsString = 1,
    IsWString = ZuConversion<Elem, wchar_t>::Same,
    IsHashable = 1, IsComparable = 1
  };
  static const Elem *data(const T &s) { return s.data(); }
  static unsigned length(const T &s) { return s.length(); }
};
template <> struct ZuTraits<ZtString> : public ZtStringTraits<ZtString> { };
template <> struct ZuTraits<ZtWString> : public ZtStringTraits<ZtWString> { };

// RVO shortcuts

#ifdef __GNUC__
ZtString ZtSprintf(const char *, ...) __attribute__((format(printf, 1, 2)));
#endif
inline ZtString ZtSprintf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  ZtString s;
  s.vsprintf(format, args);
  va_end(args);
  return s;
}
inline ZtWString ZtWSprintf(const wchar_t *format, ...)
{
  va_list args;

  va_start(args, format);
  ZtWString s;
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
inline typename ZuIfT<ZtJoinable_<D, A, ZtString>::OK, ZtString>::T
ZtJoin(D &&d, A &&a) {
  ZtString o;
  ZtJoin(ZuFwd<D>(d), ZuFwd<A>(a), o);
  return o;
}
template <typename D, typename E>
inline typename ZuIfT<ZtJoinable_<D, ZuArray<E>, ZtString>::OK, ZtString>::T
ZtJoin(D &&d, std::initializer_list<E> &&a) {
  ZtString o;
  ZtJoin(ZuFwd<D>(d), ZuArray<E>(ZuFwd<std::initializer_list<E> >(a)), o);
  return o;
}
template <typename D, typename A>
inline typename ZuIfT<ZtJoinable_<D, A, ZtWString>::OK, ZtWString>::T
ZtJoin(D &&d, A &&a) {
  ZtWString o;
  ZtJoin(ZuFwd<D>(d), ZuFwd<A>(a), o);
  return o;
}
template <typename D, typename E>
inline typename ZuIfT<ZtJoinable_<D, ZuArray<E>, ZtWString>::OK, ZtWString>::T
ZtJoin(D &&d, std::initializer_list<E> &&a) {
  ZtWString o;
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
template <> struct ZuPrint<ZtHexDump> : public ZuPrintDelegate {
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
template <> struct ZuPrint<ZtString> : public ZuPrintString { };

#endif /* ZtString_HPP */
