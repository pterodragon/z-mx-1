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

// fixed-size POD strings/buffers for use in POD structs and passing by value
//
// * cached length
// * always null-terminated
// * optimized for smaller sizes
// * max 64k

#ifndef ZuStringN_HPP
#define ZuStringN_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <string.h>
#include <wchar.h>
#include <stdio.h>

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuIfT.hpp>
#include <zlib/ZuStringFn.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuUTF.hpp>
#include <zlib/ZuNormChar.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4996)
#endif

#pragma pack(push, 1)

struct ZuStringN__ { }; // type tag

template <typename T_> struct ZuStringN_Char2 { using T = ZuNull; };
template <> struct ZuStringN_Char2<char> { using T = wchar_t; };
template <> struct ZuStringN_Char2<wchar_t> { using T = char; };

template <typename Char_, unsigned N, typename StringN>
class ZuStringN_ : public ZuStringN__ {
  ZuConversionFriend;

  ZuStringN_(const ZuStringN_ &);
  ZuStringN_ &operator =(const ZuStringN_ &);

  ZuAssert(N >= 1);
  enum { M = N - 1 };

public:
  using Char = Char_;
  using Char2 = typename ZuStringN_Char2<Char>::T;

private:
  template <typename T> using NormChar = ZuNormChar<T>;

  // from any string with same char (including string literals)
  template <typename U, typename V = Char> struct IsString {
    using W = typename NormChar<typename ZuTraits<U>::Elem>::T;
    using X = typename NormChar<V>::T;
    enum { OK =
      (ZuTraits<U>::IsArray || ZuTraits<U>::IsString) &&
      ZuConversion<W, X>::Same };
  };
  template <typename U, typename R = void>
  struct MatchString : public ZuIfT<IsString<U>::OK, R> { };

  // from individual char
  template <typename U, typename V = Char> struct IsChar {
    enum { OK = ZuConversion<U, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar : public ZuIfT<IsChar<U>::OK, R> { };

  // from char2 string (requires conversion)
  template <typename U, typename V = Char2> struct IsChar2String {
    enum { OK = ZuTraits<U>::IsString &&
      ZuConversion<typename ZuTraits<U>::Elem, V>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2String : public ZuIfT<IsChar2String<U>::OK, R> { };

  // from individual char2 (requires conversion, char->wchar_t only)
  template <typename U, typename V = Char2> struct IsChar2 {
    enum { OK = ZuConversion<U, V>::Same &&
      !ZuConversion<U, wchar_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchChar2 : public ZuIfT<IsChar2<U>::OK, R> { };

  // from printable type (if this is a char array)
  template <typename U, typename V = Char> struct IsPDelegate {
    enum { OK = ZuConversion<char, V>::Same && ZuPrint<U>::Delegate };
  };
  template <typename U, typename R = void>
  struct MatchPDelegate : public ZuIfT<IsPDelegate<U>::OK, R> { };
  template <typename U, typename V = Char> struct IsPBuffer {
    enum { OK = ZuConversion<char, V>::Same && ZuPrint<U>::Buffer };
  };
  template <typename U, typename R = void>
  struct MatchPBuffer : public ZuIfT<IsPBuffer<U>::OK, R> { };

  // from real primitive types other than chars (if this is a char string)
  template <typename U, typename V = Char> struct IsReal {
    enum { OK = ZuConversion<char, V>::Same &&
      !ZuConversion<U, V>::Same &&
      ZuTraits<U>::IsReal && ZuTraits<U>::IsPrimitive &&
      !ZuTraits<U>::IsArray };
  };
  template <typename U, typename R = void>
  struct MatchReal : public ZuIfT<IsReal<U>::OK, R> { };

protected:
  ZuStringN_() : m_length(0) { data()[0] = 0; }

  using Nop_ = enum { Nop };
  ZuInline ZuStringN_(Nop_ _) { }

  ZuStringN_(unsigned length) : m_length(length) {
    if (m_length >= N) m_length = M;
    data()[m_length] = 0;
  }

  void init(const Char *s) {
    if (!s) { null(); return; }
    unsigned i;
    for (i = 0; i < M; i++) if (!(data()[i] = *s++)) break;
    if (i == M) data()[i] = 0;
    m_length = i;
  }

  void init(const Char *s, unsigned length) {
    if (ZuUnlikely(length >= N)) length = M;
    if (ZuLikely(s && length)) memcpy(data(), s, length * sizeof(Char));
    memset(data() + (m_length = length), 0, (N - length) * sizeof(Char));
  }

  template <typename S> ZuInline typename MatchString<S>::T init(S &&s_) {
    ZuArray<typename ZuConst<Char>::T> s(s_);
    init(s.data(), s.length());
  }

  template <typename C> ZuInline typename MatchChar<C>::T init(C c) {
    auto data = this->data();
    data[0] = c;
    data[m_length = 1] = 0;
  }

  template <typename S> ZuInline typename MatchChar2String<S>::T init(S &&s) {
    data()[m_length = ZuUTF<Char, Char2>::cvt({data(), M}, s)] = 0;
  }
  template <typename C> ZuInline typename MatchChar2<C>::T init(C c) {
    data()[m_length = ZuUTF<Char, Char2>::cvt({data(), M}, {&c, 1})] = 0;
  }

  template <typename P> typename MatchPDelegate<P>::T init(const P &p) {
    m_length = 0;
    ZuPrint<P>::print(*static_cast<StringN *>(this), p);
  }
  template <typename P> typename MatchPBuffer<P>::T init(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (length >= N)
      data()[m_length = 0] = 0;
    else
      data()[m_length = ZuPrint<P>::print(data(), length, p)] = 0;
  }

  template <typename V> ZuInline typename MatchReal<V>::T init(V v) {
    init(ZuBoxed(v));
  }

  void append(const Char *s, unsigned length) {
    if (m_length + length >= N) length = M - m_length;
    if (s && length) memcpy(data() + m_length, s, length * sizeof(Char));
    data()[m_length += length] = 0;
  }

  template <typename S> ZuInline typename MatchString<S>::T append_(S &&s_) {
    ZuArray<const Char> s(s_);
    append(s.data(), s.length());
  }

  template <typename C> ZuInline typename MatchChar<C>::T append_(C c) {
    if (m_length < M) {
      auto data = this->data();
      data[m_length++] = c;
      data[m_length] = 0;
    }
  }

  template <typename S>
  ZuInline typename MatchChar2String<S>::T append_(S &&s) {
    if (m_length < M)
      data()[m_length = ZuUTF<Char, Char2>::cvt(
	  {data() + m_length, M - m_length}, s)] = 0;
  }

  template <typename C> ZuInline typename MatchChar2<C>::T append_(C c) {
    if (m_length < M)
      data()[m_length += ZuUTF<Char, Char2>::cvt(
	  {data() + m_length, M - m_length}, {&c, 1})] = 0;
  }

  template <typename P>
  typename MatchPDelegate<P>::T append_(const P &p) {
    ZuPrint<P>::print(*static_cast<StringN *>(this), p);
  }
  template <typename P>
  typename MatchPBuffer<P>::T append_(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (m_length + length >= N) return;
    data()[m_length += ZuPrint<P>::print(data() + m_length, length, p)] = 0;
  }

  template <typename V> ZuInline typename MatchReal<V>::T append_(V v) {
    append_(ZuBoxed(v));
  }

public:
// array/ptr operators

  ZuInline Char &operator [](int i) { return data()[i]; }
  ZuInline const Char &operator [](int i) const { return data()[i]; }

  // operator Char *() must return 0 if the string is empty, oherwise
  // these usages stop working:
  // if (ZuStringN<2> s = "") { } else { puts("ok"); }
  // if (ZuStringN<2> s = 0) { } else { puts("ok"); }
  ZuInline operator Char *() {
    return !m_length ? (Char *)0 : data();
  }
  ZuInline operator const Char *() const {
    return !m_length ? (const Char *)0 : data();
  }

// accessors

  ZuInline Char *data() { return (Char *)(&this[1]); }
  ZuInline const Char *data() const { return (const Char *)(&this[1]); }
  ZuInline unsigned length() const { return m_length; }
  ZuInline static constexpr unsigned size() { return N; }

// chomp(), trim(), strip()

private:
  // match whitespace
  auto matchS() {
    return [](int c) {
      return c == ' ' || c == '\t' || c == '\n' || c == '\t';
    };
  }
public:
  // remove trailing characters
  template <typename Match>
  void chomp(Match match) noexcept {
    int o = m_length;
    if (!o) return;
    while (--o >= 0 && match(data()[0]));
    data()[m_length = o + 1] = 0;
  }
  ZuInline void chomp() noexcept { return chomp(matchS()); }

  // remove leading characters
  template <typename Match>
  void trim(Match match) noexcept {
    int o;
    for (o = 0; o < (int)m_length && match(data()[0]); o++);
    if (!o) return;
    if (!(m_length -= o)) { null(); return; }
    memmove(data(), data() + o, m_length * sizeof(Char));
    data()[m_length] = 0;
  }
  ZuInline void trim() noexcept { return trim(matchS()); }

  // remove leading & trailing characters
  template <typename Match>
  void strip(Match match) noexcept {
    int o = m_length;
    if (!o) return;
    while (--o >= 0 && match(data()[o]));
    if (o < 0) { null(); return; }
    m_length = o + 1;
    for (o = 0; o < (int)m_length && match(data()[0]); o++);
    if (!o) { data()[m_length] = 0; return; }
    if (!(m_length -= o)) { null(); return; }
    memmove(data(), data() + o, m_length * sizeof(Char));
    data()[m_length] = 0;
  }
  ZuInline void strip() noexcept { return strip(matchS()); }

// sprintf

  StringN &sprintf(const Char *format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(format, args);
    va_end(args);
    return *static_cast<StringN *>(this);
  }
  StringN &vsprintf(const Char *format, va_list args) {
    vsnprintf(format, args);
    return *static_cast<StringN *>(this);
  }

private:
  void vsnprintf(const Char *format, va_list args) {
    if (N <= m_length + 2) return;
    int n = Zu::vsnprintf(
	data() + m_length, N - m_length, format, args);
    if (n < 0) {
      calcLength();
      return;
    }
    n += m_length;
    if (n == (int)N || n == (int)M) {
      data()[m_length = M] = 0;
      return;
    }
    m_length = n;
  }

public:
// reset to null string

  ZuInline void clear() { null(); }
  ZuInline void null() { data()[m_length = 0] = 0; }

// set length

  void length(unsigned length) {
    if (length >= N) length = M;
    data()[m_length = length] = 0;
  }

  void calcLength() {
    data()[M] = 0;
    m_length = Zu::strlen_(data());
  }

// buffer access

  ZuInline auto buf() {
    return ZuArray<Char>{data(), M};
  }
  ZuInline auto cbuf() const {
    return ZuArray<typename ZuConst<Char>::T>{data(), m_length};
  }

// comparison

  ZuInline bool operator !() const { return !m_length; }

protected:
  ZuInline bool same(const StringN &s) const {
    return static_cast<const StringN *>(this) == &s;
  }
  template <typename S> ZuInline bool same(const S &s) const { return false; }

public:
  template <typename S>
  ZuInline int cmp(const S &s) const {
    if (same(s)) return 0;
    return ZuCmp<StringN>::cmp(*static_cast<const StringN *>(this), s);
  }
  template <typename S>
  ZuInline bool less(const S &s) const {
    return !same(s) &&
      ZuCmp<StringN>::less(*static_cast<const StringN *>(this), s);
  }
  template <typename S>
  ZuInline bool greater(const S &s) const {
    return !same(s) &&
      ZuCmp<StringN>::less(s, *static_cast<const StringN *>(this));
  }
  template <typename S>
  ZuInline bool equals(const S &s) const {
    return same(s) ||
      ZuCmp<StringN>::equals(*static_cast<const StringN *>(this), s);
  }

  template <typename S>
  ZuInline bool operator ==(const S &s) const { return equals(s); }
  template <typename S>
  ZuInline bool operator !=(const S &s) const { return !equals(s); }
  template <typename S>
  ZuInline bool operator >(const S &s) const { return greater(s); }
  template <typename S>
  ZuInline bool operator >=(const S &s) const { return !less(s); }
  template <typename S>
  ZuInline bool operator <(const S &s) const { return less(s); }
  template <typename S>
  ZuInline bool operator <=(const S &s) const { return !greater(s); }

  ZuInline uint32_t hash() const {
    return ZuHash<StringN>::hash(*static_cast<const StringN *>(this));
  }

protected:
  uint16_t	m_length;
};

template <typename T_>
struct ZuStringN_Traits : public ZuGenericTraits<T_> {
  using T = T_;
  using Elem = typename T::Char;
  enum {
    IsPOD = 1, IsCString = 1, IsString = 1, IsStream = 1,
    IsWString = ZuConversion<Elem, wchar_t>::Same,
    IsComparable = 1, IsHashable = 1
  };
  ZuInline static const Elem *data(const T &s) { return s.data(); }
  ZuInline static unsigned length(const T &s) { return s.length(); }
};

template <unsigned N_>
class ZuStringN : public ZuStringN_<char, N_, ZuStringN<N_>> {
  ZuAssert(N_ > 1);
  ZuAssert(N_ < (1U<<16) - 1U);

public:
  using Char = char;
  using Char2 = wchar_t;
  using Base = ZuStringN_<char, N_, ZuStringN<N_>>;
  enum { N = N_ };

private:
  // an unsigned|int|size_t parameter to the constructor is a buffer length
  template <typename U> struct IsCtorLength {
    enum { OK =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchCtorLength : public ZuIfT<IsCtorLength<U>::OK, R> { };

  // constructor arg
  template <typename U> struct IsCtorArg {
    enum { OK =
      !IsCtorLength<U>::OK &&
      !ZuConversion<Base, U>::Base };
  };
  template <typename U, typename R = void>
  struct MatchCtorArg : public ZuIfT<IsCtorArg<U>::OK, R> { };

public:
  ZuInline ZuStringN() { }

  ZuInline ZuStringN(const ZuStringN &s) : Base(Base::Nop) {
    this->init(s.data(), s.length());
  }
  ZuInline ZuStringN &operator =(const ZuStringN &s) {
    if (ZuLikely(this != &s)) this->init(s.data(), s.length());
    return *this;
  }

  // update()
  template <typename S>
  ZuInline ZuStringN &update(S &&s_) {
    ZuString s(s_);
    if (s.length()) this->init(s.data(), s.length());
    return *this;
  }

  // C string types
  ZuInline ZuStringN(const char *s) : Base(Base::Nop) {
    this->init(s);
  }
  ZuInline ZuStringN(const char *s, unsigned length) : Base(Base::Nop) {
    this->init(s, length);
  }
  ZuInline ZuStringN &operator =(const char *s) {
    this->init(s);
    return *this;
  }
  ZuInline ZuStringN &operator +=(const char *s_) {
    ZuString s(s_);
    this->append(s.data(), s.length());
    return *this;
  }
  ZuInline ZuStringN &operator <<(const char *s_) {
    ZuString s(s_);
    this->append(s.data(), s.length());
    return *this;
  }
  ZuInline ZuStringN &update(const char *s) {
    if (s) this->init(s);
    return *this;
  }

  // miscellaneous types handled by base class
  template <typename S>
  ZuInline ZuStringN(S &&s,
      typename MatchCtorArg<S>::T *_ = 0) : Base(Base::Nop) {
    this->init(ZuFwd<S>(s));
  }
  template <typename S>
  ZuInline ZuStringN &operator =(S &&s) {
    this->init(ZuFwd<S>(s));
    return *this;
  }
  template <typename S>
  ZuInline ZuStringN &operator +=(S &&s) {
    this->append_(ZuFwd<S>(s));
    return *this;
  }
  template <typename S>
  ZuInline ZuStringN &operator <<(S &&s) {
    this->append_(ZuFwd<S>(s));
    return *this;
  }

  // length
  template <typename L>
  ZuInline ZuStringN(L l, typename MatchCtorLength<L>::T *_ = 0) : Base(l) { }

private:
  char		m_data[N];
};

template <unsigned N>
struct ZuTraits<ZuStringN<N> > : public ZuStringN_Traits<ZuStringN<N> > { };

// generic printing
template <unsigned N>
struct ZuPrint<ZuStringN<N> > : public ZuPrintString { };

template <unsigned N_>
class ZuWStringN : public ZuStringN_<wchar_t, N_, ZuWStringN<N_>> {
  ZuAssert(N_ > 1);
  ZuAssert(N_ < (1U<<15) - 1U);

public:
  using Char = wchar_t;
  using Char2 = char;
  using Base = ZuStringN_<wchar_t, N_, ZuWStringN<N_>>;
  enum { N = N_ };

private:
  // an unsigned|int|size_t parameter to the constructor is a buffer length
  template <typename U> struct IsCtorLength {
    enum { OK =
      ZuConversion<U, unsigned>::Same ||
      ZuConversion<U, int>::Same ||
      ZuConversion<U, size_t>::Same };
  };
  template <typename U, typename R = void>
  struct MatchCtorLength : public ZuIfT<IsCtorLength<U>::OK, R> { };

  // constructor arg
  template <typename U> struct IsCtorArg {
    enum { OK =
      !IsCtorLength<U>::OK &&
      !ZuConversion<Base, U>::Base };
  };
  template <typename U, typename R = void>
  struct MatchCtorArg : public ZuIfT<IsCtorArg<U>::OK, R> { };

public:
  ZuInline ZuWStringN() { }

  ZuInline ZuWStringN(const ZuWStringN &s) : Base(Base::Nop) {
    this->init(s.data(), s.length());
  }
  ZuInline ZuWStringN &operator =(const ZuWStringN &s) {
    if (ZuLikely(this != &s)) init(s.data(), s.length());
    return *this;
  }

  // update()
  template <typename S> ZuInline ZuWStringN &update(S &&s_) {
    ZuWString s(s_);
    if (s.length()) this->init(s.data(), s.length());
    return *this;
  }

  // C string types
  ZuInline ZuWStringN(const wchar_t *s) : Base(Base::Nop) {
    this->init(s);
  }
  ZuInline ZuWStringN(const wchar_t *s, unsigned length) : Base(Base::Nop) {
    this->init(s, length);
  }
  ZuInline ZuWStringN &operator =(const wchar_t *s) {
    this->init(s);
    return *this;
  }
  ZuInline ZuWStringN &operator +=(const wchar_t *s_) { // unoptimized
    ZuWString s(s_);
    this->append(s.data(), s.length());
    return *this;
  }
  ZuInline ZuWStringN &operator <<(const wchar_t *s_) { // unoptimized
    ZuWString s(s_);
    this->append(s.data(), s.length());
    return *this;
  }
  ZuInline ZuWStringN &update(const wchar_t *s) {
    if (s) this->init(s);
    return *this;
  }

  // miscellaneous types handled by base class
  template <typename S>
  ZuInline ZuWStringN(S &&s,
      typename MatchCtorArg<S>::T *_ = 0) : Base(Base::Nop) {
    this->init(ZuFwd<S>(s));
  }
  template <typename S>
  ZuInline ZuWStringN &operator =(S &&s) {
    this->init(ZuFwd<S>(s));
    return *this;
  }
  template <typename S>
  ZuInline ZuWStringN &operator +=(S &&s) {
    this->append_(ZuFwd<S>(s));
    return *this;
  }
  template <typename S>
  ZuInline ZuWStringN &operator <<(S &&s) {
    this->append_(ZuFwd<S>(s));
    return *this;
  }

  // length
  template <typename L>
  ZuInline ZuWStringN(L l, typename MatchCtorLength<L>::T *_ = 0) : Base(l) { }

private:
  wchar_t	m_data[N];
};

template <unsigned N>
struct ZuTraits<ZuWStringN<N> > : public ZuStringN_Traits<ZuWStringN<N> > { };

#pragma pack(pop)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuStringN_HPP */
