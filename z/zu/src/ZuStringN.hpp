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

// fixed-size POD strings/buffers for use in structs and passing by value
//
// * cached length
// * always null-terminated
// * optimized for smaller sizes

#ifndef ZuStringN_HPP
#define ZuStringN_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <string.h>
#include <wchar.h>
#include <stdio.h>

#include <ZuTraits.hpp>
#include <ZuConversion.hpp>
#include <ZuIfT.hpp>
#include <ZuStringFn.hpp>
#include <ZuString.hpp>
#include <ZuPrint.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800 4996)
#endif

#pragma pack(push, 1)

struct ZuStringN__ { }; // type tag

template <typename Char_, typename StringN>
class ZuStringN_ : public ZuStringN__ {
  ZuStringN_();
  ZuStringN_(const ZuStringN_ &);
  ZuStringN_ &operator =(const ZuStringN_ &);
  bool operator *() const;

public:
  typedef Char_ Char;

private:
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Delegate> struct FromPDelegate;
  template <typename U, typename R>
  struct FromPDelegate<U, R, char, true> { typedef R T; };
  template <typename U, typename R = void, typename V = Char,
    bool B = ZuPrint<U>::Buffer> struct FromPBuffer;
  template <typename U, typename R>
  struct FromPBuffer<U, R, char, true> { typedef R T; };

  template <typename U, typename R = void, typename V = Char,
    bool S = ZuTraits<U>::IsString,
    bool W = ZuTraits<U>::IsWString> struct ToString;
  template <typename U, typename R>
  struct ToString<U, R, char, true, false> { typedef R T; };
  template <typename U, typename R>
  struct ToString<U, R, wchar_t, true, true> { typedef R T; };

protected:
  inline ZuStringN_(unsigned size) :
    m_size(size), m_length(0) { data()[0] = 0; }

  typedef enum { Nop } Nop_;
  ZuInline ZuStringN_(Nop_ _) { }

  inline ZuStringN_(unsigned size, unsigned length) :
      m_size(size), m_length(length) {
    if (m_length >= m_size) m_length = m_size - 1;
    data()[m_length] = 0;
  }

  inline void init(const Char *s) {
    if (!s) { null(); return; }
    unsigned i;
    for (i = 0; i < m_size - 1U; i++) if (!(data()[i] = *s++)) break;
    if (i == m_size - 1U) data()[i] = 0;
    m_length = i;
  }

  inline void init(const Char *s, unsigned length) {
    if (ZuUnlikely(length >= m_size)) length = m_size - 1;
    if (ZuLikely(s && length)) memcpy(data(), s, length * sizeof(Char));
    data()[m_length = length] = 0;
  }

  template <typename P> inline typename FromPDelegate<P>::T init(const P &p) {
    ZuPrint<P>::print(*static_cast<StringN *>(this), p);
  }
  template <typename P> inline typename FromPBuffer<P>::T init(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (length >= m_size)
      data()[m_length = 0] = 0;
    else
      data()[m_length = ZuPrint<P>::print(data(), length, p)] = 0;
  }

  inline void append(const Char *s, unsigned length) {
    if (m_length + length >= m_size) length = m_size - m_length - 1;
    if (s && length) memcpy(data() + m_length, s, length * sizeof(Char));
    data()[m_length += length] = 0;
  }

  template <typename P> inline typename FromPDelegate<P>::T append(const P &p) {
    ZuPrint<P>::print(*static_cast<StringN *>(this), p);
  }
  template <typename P> inline typename FromPBuffer<P>::T append(const P &p) {
    unsigned length = ZuPrint<P>::length(p);
    if (m_length + length >= m_size) return;
    data()[m_length += ZuPrint<P>::print(data() + m_length, length, p)] = 0;
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
  ZuInline unsigned size() const { return m_size; }

// chomp(), trim(), strip()

private:
  // match whitespace
  inline auto matchS() {
    return [](int c) {
      return c == ' ' || c == '\t' || c == '\n' || c == '\t';
    };
  }
public:
  // remove trailing characters
  template <typename Match>
  inline void chomp(Match &&match) noexcept {
    int o = m_length;
    if (!o) return;
    while (--o >= 0 && match(data()[0]));
    data()[m_length = o + 1] = 0;
  }
  ZuInline void chomp() noexcept { return chomp(matchS()); }

  // remove leading characters
  template <typename Match>
  inline void trim(Match &&match) noexcept {
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
  inline void strip(Match &&match) noexcept {
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

  inline StringN &sprintf(const Char *format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(format, args);
    va_end(args);
    return *static_cast<StringN *>(this);
  }
  inline StringN &vsprintf(const Char *format, va_list args) {
    vsnprintf(format, args);
    return *static_cast<StringN *>(this);
  }

private:
  void vsnprintf(const Char *format, va_list args) {
    if (m_size <= m_length + 2) return;
    int n = Zu::vsnprintf(
	data() + m_length, m_size - m_length, format, args);
    if (n < 0) {
      calcLength();
      return;
    }
    n += m_length;
    if (n == (int)m_size || n == (int)m_size - 1) {
      data()[m_length = m_size - 1] = 0;
      return;
    }
    m_length = n;
  }

public:
// reset to null string

  ZuInline void null() { data()[m_length = 0] = 0; }

// set length

  inline void length(unsigned length) {
    if (length >= m_size) length = m_size - 1;
    data()[m_length = length] = 0;
  }

  inline void calcLength() {
    data()[m_size - 1] = 0;
    m_length = Zu::strlen_(data());
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
  ZuInline bool equals(const S &s) const {
    if (same(s)) return true;
    return ZuCmp<StringN>::equals(*static_cast<const StringN *>(this), s);
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

  ZuInline uint32_t hash() const {
    return ZuHash<StringN>::hash(*static_cast<const StringN *>(this));
  }

// conversions
 
  template <typename S>
  ZuInline typename ToString<S, S>::T as() const {
    return ZuTraits<S>::make(data(), m_length);
  }

protected:
  uint32_t	m_size;
  uint32_t	m_length;
};

template <typename T_>
struct ZuStringN_Traits : public ZuGenericTraits<T_> {
  typedef T_ T;
  typedef typename T::Char Elem;
  enum {
    IsPOD = 1, IsCString = 1, IsString = 1, IsStream = 1,
    IsWString = ZuConversion<Elem, wchar_t>::Same,
    IsHashable = 1, IsComparable = 1
  };
  inline static T make(const Elem *data, unsigned length) {
    if (ZuUnlikely(!data)) return T();
    return T(data, length);
  }
  inline static T &append(T &s, const Elem *data, unsigned length) {
    if (ZuLikely(data)) s.append(data, length);
    return s;
  }
  inline static const Elem *data(const T &s) { return s.data(); }
  inline static unsigned length(const T &s) { return s.length(); }
};

// ZuStringN<N> can be cast and used as ZuStringN<>

template <unsigned N_ = 1>
class ZuStringN : public ZuStringN_<char, ZuStringN<N_> > {
  ZuAssert(N_ < (1U<<16) - 1U);
  struct Private { };

public:
  typedef char Char;
  typedef ZuStringN_<char, ZuStringN<N_> > Base;
  enum { N = N_ };

private:
  template <typename U, typename R = void,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct FromPrint;
  template <typename U, typename R>
  struct FromPrint<U, R, true> { typedef R T; };
  template <typename U, typename R = void,
    bool B = ZuTraits<U>::IsBoxed> struct FromBoxed;
  template <typename U, typename R>
  struct FromBoxed<U, R, true> { typedef R T; };

  template <typename U, typename R = void,
    bool S = ZuTraits<U>::IsString &&
	     !ZuTraits<U>::IsWString &&
	     !ZuTraits<U>::IsPrimitive
    > struct FromString;
  template <typename U, typename R>
  struct FromString<U, R, true> { typedef R T; };

  template <typename U, typename R = Private,
    bool E = ZuConversion<U, unsigned>::Same ||
	     ZuConversion<U, int>::Same ||
	     ZuConversion<U, size_t>::Same
    > struct CtorLength;
  template <typename U, typename R>
  struct CtorLength<U, R, true> { typedef R T; };

public:
  ZuInline ZuStringN() : Base(N) { }

  ZuInline ZuStringN(const ZuStringN &s) : Base(Base::Nop) {
    this->m_size = N;
    this->init(s.data(), s.length());
  }
  ZuInline ZuStringN &operator =(const ZuStringN &s) {
    if (this != &s) this->init(s.data(), s.length());
    return *this;
  }

  // string types
  template <typename S>
  ZuInline ZuStringN(S &&s_, typename FromString<S, Private>::T *_ = 0) :
      Base(Base::Nop) {
    ZuString s(ZuFwd<S>(s_));
    this->m_size = N;
    this->init(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromString<S, ZuStringN &>::T operator =(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->init(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuStringN &>::T operator +=(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuStringN &>::T operator <<(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    this->append(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuStringN &>::T update(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    if (s.length()) this->init(s.data(), s.length());
    return *this;
  }

  // C string types
  ZuInline ZuStringN(const char *s) : Base(Base::Nop) {
    this->m_size = N;
    this->init(s);
  }
  ZuInline ZuStringN(const char *s, unsigned length) : Base(Base::Nop) {
    this->m_size = N;
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

  // boxed types
  template <typename P>
  ZuInline ZuStringN(const P &p, typename FromPrint<P, Private>::T *_ = 0) :
      Base(Base::Nop) {
    this->m_size = N;
    this->init(p);
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuStringN &>::T operator =(const P &p) {
    this->init(p);
    return *this;
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuStringN &>::T operator +=(const P &p) {
    this->append(p);
    return *this;
  }
  template <typename P>
  ZuInline typename FromPrint<P, ZuStringN &>::T operator <<(const P &p) {
    this->append(p);
    return *this;
  }
  template <typename B>
  ZuInline typename FromBoxed<B, ZuStringN &>::T
      print(const B &b, int precision = -1, int comma = 0, int pad = -1) {
    unsigned length = b.length(precision, comma);
    if (this->m_length + length >= this->m_size) return;
    this->data()[this->m_length +=
      b.print(this->data() + this->m_length,
	  length, precision, comma, pad)] = 0;
    return *this;
  }

  // length
  template <typename L>
  ZuInline ZuStringN(L l, typename CtorLength<L>::T *_ = 0) : Base(N, l) { }

  // element types 
  ZuInline ZuStringN &operator =(char c) {
    this->init(&c, 1);
    return *this;
  }
  ZuInline ZuStringN &operator +=(char c) {
    this->append(&c, 1);
    return *this;
  }
  ZuInline ZuStringN &operator <<(char c) {
    this->append(&c, 1);
    return *this;
  }

private:
  char		m_data[N];
};

template <unsigned N> struct ZuTraits<ZuStringN<N> > :
    public ZuStringN_Traits<ZuStringN<N> > { };

// generic printing
template <unsigned N> struct ZuPrint<ZuStringN<N> > :
  public ZuPrintString<ZuStringN<N> > { };

// ZuWStringN<N> can be cast and used as ZuWStringN<>

template <unsigned N_ = 1>
class ZuWStringN : public ZuStringN_<wchar_t, ZuWStringN<N_> > {
  ZuAssert(N_ < (1U<<15) - 1U);
  struct Private { };

public:
  typedef wchar_t Char;
  typedef ZuStringN_<wchar_t, ZuWStringN<N_> > Base;
  enum { N = N_ };

private:
  template <typename U, typename R = void,
    bool S = ZuTraits<U>::IsString &&
	     ZuTraits<U>::IsWString &&
	     !ZuTraits<U>::IsPrimitive
    > struct FromString;
  template <typename U, typename R>
  struct FromString<U, R, true> { typedef R T; };

  template <typename U, typename R = Private,
    bool E = ZuConversion<U, unsigned>::Same ||
	     ZuConversion<U, int>::Same ||
	     ZuConversion<U, size_t>::Same
    > struct CtorLength { };
  template <typename U, typename R>
  struct CtorLength<U, R, true> { typedef R T; };

public:
  ZuInline ZuWStringN() : Base(N) { }

  ZuInline ZuWStringN(const ZuWStringN &s) : Base(Base::Nop) {
    this->m_size = N;
    this->init(s.data(), s.length());
  }
  ZuInline ZuWStringN &operator =(const ZuWStringN &s) {
    if (this != &s) init(s.data(), s.length());
    return *this;
  }

  // string types
  template <typename S>
  ZuInline ZuWStringN(S &&s_, typename FromString<S, Private>::T *_ = 0) :
      Base(Base::Nop) {
    ZuWString s(ZuFwd<S>(s_));
    this->m_size = N;
    this->init(s.data(), s.length());
  }
  template <typename S>
  ZuInline typename FromString<S, ZuWStringN &>::T operator =(S &&s_) {
    ZuWString s(ZuFwd<S>(s_));
    this->init(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuWStringN &>::T operator +=(S &&s_) {
    ZuWString s(ZuFwd<S>(s_));
    this->append(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuWStringN &>::T operator <<(S &&s_) {
    ZuWString s(ZuFwd<S>(s_));
    this->append(s.data(), s.length());
    return *this;
  }
  template <typename S>
  ZuInline typename FromString<S, ZuWStringN &>::T update(S &&s_) {
    ZuWString s(ZuFwd<S>(s_));
    if (s.length()) this->init(s.data(), s.length());
    return *this;
  }

  // C string types
  ZuInline ZuWStringN(const wchar_t *s) : Base(Base::Nop) {
    this->m_size = N;
    this->init(s);
  }
  ZuInline ZuWStringN(const wchar_t *s, unsigned length) : Base(Base::Nop) {
    this->m_size = N;
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

  // length
  template <typename L>
  ZuInline ZuWStringN(L l, typename CtorLength<L>::T *_ = 0) : Base(N, l) { }

  // wchar_t is dangerously ambiguous
#if 0
  // element types
  inline ZuWStringN &operator =(const wchar_t &c) {
    this->init(&c, 1);
    return *this;
  }
  inline ZuWStringN &operator +=(const wchar_t &c) {
    this->append(&c, 1);
    return *this;
  }
  inline ZuWStringN &operator <<(const wchar_t &c) {
    this->append(&c, 1);
    return *this;
  }
#endif

private:
  wchar_t	m_data[N];
};

template <unsigned N> struct ZuTraits<ZuWStringN<N> > :
    public ZuStringN_Traits<ZuWStringN<N> > { };

#pragma pack(pop)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuStringN_HPP */
