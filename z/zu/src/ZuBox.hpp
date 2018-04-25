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

// "boxed" primitive types (concept terminology from C#)

#ifndef ZuBox_HPP
#define ZuBox_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <limits.h>
#include <math.h>
#include <float.h>

#include <ZuAssert.hpp>
#include <ZuIfT.hpp>
#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuInt.hpp>
#include <ZuConversion.hpp>
#include <ZuPrint.hpp>
#include <ZuString.hpp>

#include <ZuFP.hpp>
#include <ZuFmt.hpp>

#include <Zu_ntoa.hpp>
#include <Zu_aton.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4312 4800)
#endif

// compile-time formatting
template <typename T, class Fmt,
  bool Signed = ZuTraits<T>::IsSigned,
  bool FloatingPoint = Signed && ZuTraits<T>::IsFloatingPoint>
struct ZuBox_Print;
template <typename T, class Fmt> struct ZuBox_Print<T, Fmt, 0, 0> {
  ZuInline static unsigned length(T v)
    { return Zu_nprint<Fmt>::ulen(v); }
  ZuInline static unsigned print(T v, char *buf)
    { return Zu_nprint<Fmt>::utoa(v, buf); }
};
template <typename T, class Fmt> struct ZuBox_Print<T, Fmt, 1, 0> {
  ZuInline static unsigned length(T v)
    { return Zu_nprint<Fmt>::ilen(v); }
  ZuInline static unsigned print(T v, char *buf)
    { return Zu_nprint<Fmt>::itoa(v, buf); }
};
template <typename T, class Fmt> struct ZuBox_Print<T, Fmt, 1, 1> {
  ZuInline static unsigned length(T v)
    { return Zu_nprint<Fmt>::flen(v); }
  ZuInline static unsigned print(T v, char *buf)
    { return Zu_nprint<Fmt>::ftoa(v, buf); }
};

// variable run-time formatting
template <typename T,
  bool Signed = ZuTraits<T>::IsSigned,
  bool FloatingPoint = Signed && ZuTraits<T>::IsFloatingPoint>
struct ZuBox_VPrint;
template <typename T> struct ZuBox_VPrint<T, 0, 0> {
  ZuInline static unsigned length(const ZuVFmt &fmt, T v)
    { return Zu_vprint::ulen(fmt, v); }
  ZuInline static unsigned print(const ZuVFmt &fmt, T v, char *buf)
    { return Zu_vprint::utoa(fmt, v, buf); }
};
template <typename T> struct ZuBox_VPrint<T, 1, 0> {
  ZuInline static unsigned length(const ZuVFmt &fmt, T v)
    { return Zu_vprint::ilen(fmt, v); }
  ZuInline static unsigned print(const ZuVFmt &fmt, T v, char *buf)
    { return Zu_vprint::itoa(fmt, v, buf); }
};
template <typename T> struct ZuBox_VPrint<T, 1, 1> {
  ZuInline static unsigned length(const ZuVFmt &fmt, T v)
    { return Zu_vprint::flen(fmt, v); }
  ZuInline static unsigned print(const ZuVFmt &fmt, T v, char *buf)
    { return Zu_vprint::ftoa(fmt, v, buf); }
};

template <typename T, class Fmt,
  bool Signed = ZuTraits<T>::IsSigned,
  bool FloatingPoint = Signed && ZuTraits<T>::IsFloatingPoint>
struct ZuBox_Scan;
template <typename T_, class Fmt> struct ZuBox_Scan<T_, Fmt, 0, 0> {
  typedef uint64_t T;
  ZuInline static unsigned scan(T &v, const char *buf, unsigned n)
    { return Zu_nscan<Fmt>::atou(v, buf, n); }
};
template <typename T_, class Fmt> struct ZuBox_Scan<T_, Fmt, 1, 0> {
  typedef int64_t T;
  ZuInline static unsigned scan(T &v, const char *buf, unsigned n)
    { return Zu_nscan<Fmt>::atoi(v, buf, n); }
};
template <typename T_, class Fmt> struct ZuBox_Scan<T_, Fmt, 1, 1> {
  typedef T_ T;
  ZuInline static unsigned scan(T &v, const char *buf, unsigned n)
    { return Zu_nscan<Fmt>::atof(v, buf, n); }
};

template <typename T, class Cmp> class ZuBox;

// compile-time formatting
template <class Boxed, class Fmt> class ZuBoxFmt {
template <typename, class> friend class ZuBox;

  typedef ZuBox_Print<typename Boxed::T, Fmt> Print;

  ZuInline ZuBoxFmt(const Boxed &ref) : m_ref(ref) { }

public:
  ZuInline unsigned length() const { return Print::length(m_ref); }
  ZuInline unsigned print(char *buf) const { return Print::print(m_ref, buf); }

private:
  const Boxed	&m_ref;
};

// run-time formatting
struct ZuBoxVFmt_ { };
template <class Boxed>
class ZuBoxVFmt : public ZuBoxVFmt_, public ZuVFmt {
template <typename, class> friend class ZuBox;

  typedef ZuBox_VPrint<typename Boxed::T> Print;

public:
  ZuInline ZuBoxVFmt() : m_ptr(0) { }

  ZuInline ZuBoxVFmt &reset()
    { ZuVFmt::reset(); return *this; }
  ZuInline ZuBoxVFmt &left(unsigned width, char pad = '\0')
    { ZuVFmt::left(width, pad); return *this; }
  ZuInline ZuBoxVFmt &right(unsigned width, char pad = '0')
    { ZuVFmt::right(width, pad); return *this; }
  ZuInline ZuBoxVFmt &frac(unsigned ndp, char trim = '\0')
    { ZuVFmt::frac(ndp, trim); return *this; }
  ZuInline ZuBoxVFmt &hex(bool upper = 0)
    { ZuVFmt::hex(upper); return *this; }
  ZuInline ZuBoxVFmt &comma(char comma_ = ',')
    { ZuVFmt::comma(comma_); return *this; }
  ZuInline ZuBoxVFmt &alt()
    { ZuVFmt::alt(); return *this; }
  ZuInline ZuBoxVFmt &fp(int ndp = -ZuFmt::MaxNDP, char trim = '\0')
    { ZuVFmt::fp(ndp, trim); return *this; }

  // associate with value
  ZuInline void ptr(const Boxed *p) { m_ptr = p; }
  ZuInline const Boxed *ptr() const { return m_ptr; }

  // print
  ZuInline unsigned length() const {
    return Print::length(*this, *m_ptr);
  }
  ZuInline unsigned print(char *buf) const {
    return Print::print(*this, *m_ptr, buf);
  }

private:
  const Boxed	*m_ptr;
};

struct ZuBox_Approx_ { };
template <typename T, class Cmp>
struct ZuBox_Approx : public ZuBox<T, Cmp>, public ZuBox_Approx_ {
  inline ZuBox_Approx() { }
  inline ZuBox_Approx(const ZuBox_Approx &v) : ZuBox<T, Cmp>(v) { }
  inline ZuBox_Approx &operator =(const ZuBox_Approx &v) {
    if (this != &v) ZuBox<T, Cmp>::operator =(v);
    return *this;
  }
  template <typename P> inline ZuBox_Approx(const P &p) : ZuBox<T, Cmp>(p) { }
  template <typename P>
  inline ZuBox_Approx &operator =(const P &p) {
    ZuBox<T, Cmp>::operator =(p);
    return *this;
  }
};
template <typename T_, class Cmp, bool IsFloating> struct ZuBox_Tilde {
  typedef ZuBox_Approx<T_, Cmp> T;
  inline static T fn(const T_ &val) { return T(val); }
};
template <typename T_, class Cmp> struct ZuBox_Tilde<T_, Cmp, 0> {
  typedef T_ T;
  inline static T fn(const T_ &val) { return ~val; }
};

// SFINAE...
template <typename U, typename T, typename R = void> struct ZuBox_IsReal :
    public ZuIfT<!ZuTraits<U>::IsBoxed && !ZuTraits<U>::IsString &&
		 (ZuTraits<U>::IsReal ||
		  ZuConversion<U, T>::Exists ||
		  ZuConversion<U, int>::Exists), R> { };
#if 0
template <typename S> struct ZuBox_IsString :
    public ZuIfT<ZuTraits<S>::IsString && !ZuTraits<S>::IsWString> { };
#endif
template <typename Traits, typename T, bool IsPointer, bool IsArray>
struct ZuBox_IsCharPtr_ { };
template <typename Traits, typename T>
struct ZuBox_IsCharPtr_<Traits, T, 1, 0> :
    public ZuIfT<ZuConversion<typename Traits::Elem, char>::Same, T> { };
template <typename Traits, typename T>
struct ZuBox_IsCharPtr_<Traits, T, 0, 1> :
    public ZuIfT<ZuConversion<typename Traits::Elem, char>::Same, T> { };
template <typename S, typename T = void> struct ZuBox_IsCharPtr :
    public ZuBox_IsCharPtr_<ZuTraits<S>, T,
			    ZuTraits<S>::IsPointer,
			    ZuTraits<S>::IsArray> { };

template <typename T_>
struct ZuBox_Unbox { typedef T_ T; };
template <typename T_, typename Cmp>
struct ZuBox_Unbox<ZuBox<T_, Cmp> > { typedef T_ T; };

template <typename T_, class Cmp_ = ZuCmp<typename ZuBox_Unbox<T_>::T> >
class ZuBox {
template <typename, class> friend class ZuBox;
template <class> friend class ZuBoxVFmt;
template <class, class> friend class ZuBoxFmt;
  struct Private { };

public:
  typedef typename ZuBox_Unbox<T_>::T T;
  typedef Cmp_ Cmp;

private:
  template <class Fmt = ZuFmt::Default> using Scan = ZuBox_Scan<T, Fmt>;
  template <class Fmt = ZuFmt::Default> using Print = ZuBox_Print<T, Fmt>;

  ZuAssert(ZuTraits<T>::IsPrimitive && ZuTraits<T>::IsReal);

public:
  ZuInline ZuBox() : m_val(Cmp::null()) { }

  ZuInline ZuBox(const ZuBox &b) : m_val(b.m_val) { }
  ZuInline ZuBox &operator =(const ZuBox &b) { m_val = b.m_val; return *this; }

  template <typename R>
  ZuInline ZuBox(R r, typename ZuBox_IsReal<R, T, Private>::T *_ = 0) :
    m_val(r) { }

  template <typename B>
  ZuInline ZuBox(B b, typename ZuIsBoxed<B, Private>::T *_ = 0) :
    m_val(!*b ? (T)Cmp::null() : (T)b.m_val) { }

  template <typename S>
  ZuInline ZuBox(S &&s_, typename ZuIsCharString<S>::T *_ = 0) :
      m_val(Cmp::null()) {
    ZuString s(ZuFwd<S>(s_));
    typename Scan<>::T val = 0;
    if (ZuLikely(s && Scan<>::scan(val, s.data(), s.length())))
      m_val = val;
  }
  template <class Fmt, typename S>
  ZuInline ZuBox(Fmt, S &&s_, typename ZuIsCharString<S, Private>::T *_ = 0) :
      m_val(Cmp::null()) {
    ZuString s(ZuFwd<S>(s_));
    typename Scan<Fmt>::T val = 0;
    if (ZuLikely(s && Scan<Fmt>::scan(val, s.data(), s.length())))
      m_val = val;
  }

  template <typename S>
  ZuInline ZuBox(S s, unsigned len,
      typename ZuBox_IsCharPtr<S, Private>::T *_ = 0) : m_val(Cmp::null()) {
    typename Scan<>::T val = 0;
    if (ZuLikely(s && Scan<>::scan(val, s, len)))
      m_val = val;
  }
  template <class Fmt, typename S>
  ZuInline ZuBox(Fmt, S s, unsigned len,
      typename ZuBox_IsCharPtr<S, Private>::T *_ = 0) : m_val(Cmp::null()) {
    typename Scan<Fmt>::T val = 0;
    if (ZuLikely(s && Scan<Fmt>::scan(val, s, len)))
      m_val = val;
  }

private:
  template <typename R>
  ZuInline typename ZuBox_IsReal<R, T>::T assign(R r) { m_val = r; }

  template <typename B>
  ZuInline typename ZuIsBoxed<B>::T assign(const B &b) {
    m_val = !*b ? (T)Cmp::null() : (T)b.m_val;
  }

  template <typename S>
  ZuInline typename ZuIsCharString<S>::T assign(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    typename Scan<>::T val = 0;
    if (ZuUnlikely(!s || !Scan<>::scan(val, s.data(), s.length())))
      m_val = Cmp::null();
    else
      m_val = val;
  }

public:
  template <typename T>
  ZuInline ZuBox &operator =(T &&t) { assign(ZuFwd<T>(t)); return *this; }

  template <typename T>
  ZuInline static typename ZuIsFloatingPoint<T, bool>::T equals_(
      const T &t1, const T &t2) {
    if (Cmp::null(t2)) return Cmp::null(t1);
    if (Cmp::null(t1)) return false;
    return t1 == t2;
  }
  template <typename T>
  ZuInline static typename ZuNotFloatingPoint<T, bool>::T equals_(
      const T &t1, const T &t2) {
    return t1 == t2;
  }
  ZuInline bool equals(const ZuBox &b) const { return equals_(m_val, b.m_val); }

  template <typename Cmp__ = Cmp>
  ZuInline typename ZuIfT<!ZuConversion<Cmp__, ZuCmp0<T> >::Same, int>::T
  cmp_(const ZuBox &b) const {
    if (Cmp::null(b.m_val)) return Cmp::null(m_val) ? 0 : 1;
    if (Cmp::null(m_val)) return -1;
    return Cmp::cmp(m_val, b.m_val);
  }
  template <typename Cmp__ = Cmp>
  ZuInline typename ZuIfT<ZuConversion<Cmp__, ZuCmp0<T> >::Same, int>::T
  cmp_(const ZuBox &b) const {
    return Cmp::cmp(m_val, b.m_val);
  }
  ZuInline int cmp(const ZuBox &b) const { return cmp_<>(b.m_val); }

  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator ==(const R &r) const { return equals(r); }
  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator !=(const R &r) const { return !equals(r); }
  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator >(const R &r) const { return cmp(r) > 0; }
  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator >=(const R &r) const { return cmp(r) >= 0; }
  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator <(const R &r) const { return cmp(r) < 0; }
  template <typename R> ZuInline typename ZuIsNot<ZuBox_Approx_, R, bool>::T
    operator <=(const R &r) const { return cmp(r) <= 0; }

  ZuInline bool operator !() const { return !m_val; }

  ZuInline bool operator *() const { return !Cmp::null(m_val); }

  ZuInline uint32_t hash() const { return ZuHash<T>::hash(m_val); }

  ZuInline operator const T &() const { return m_val; }
  ZuInline operator T &() { return m_val; }

  // compile-time formatting
  template <class Fmt>
  ZuInline ZuBoxFmt<ZuBox, Fmt> fmt(Fmt) const { 
    return ZuBoxFmt<ZuBox, Fmt>(*this);
  }
  ZuInline ZuBoxFmt<ZuBox, ZuFmt::Hex<> > hex() const {
    return ZuBoxFmt<ZuBox, ZuFmt::Hex<> >(*this);
  }
  template <class Fmt>
  ZuInline ZuBoxFmt<ZuBox, ZuFmt::Hex<0, Fmt> > hex(Fmt) const {
    return ZuBoxFmt<ZuBox, ZuFmt::Hex<0, Fmt> >(*this);
  }
  // run-time formatting
  ZuInline ZuBoxVFmt<ZuBox> vfmt() const {
    ZuBoxVFmt<ZuBox> fmt;
    fmt.ptr(this);
    return fmt;
  }
  template <typename Fmt>
  ZuInline typename ZuIs<ZuBoxVFmt_, Fmt, const Fmt &>::T vfmt(Fmt &fmt) const {
    fmt.ptr(this);
    return fmt;
  }

  template <typename S>
  ZuInline typename ZuIsCharString<S, unsigned>::T scan(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    typename Scan<>::T val = 0;
    unsigned n = Scan<>::scan(val, s.data(), s.length());
    if (ZuUnlikely(!n)) {
      m_val = Cmp::null();
      return 0U;
    }
    m_val = val;
    return n;
  }
  template <class Fmt, typename S>
  ZuInline typename ZuIsCharString<S, unsigned>::T scan(Fmt, S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    typename Scan<Fmt>::T val = 0;
    unsigned n = Scan<Fmt>::scan(val, s.data(), s.length());
    if (ZuUnlikely(!n)) {
      m_val = Cmp::null();
      return 0U;
    }
    m_val = val;
    return n;
  }
  template <typename S>
  ZuInline typename ZuBox_IsCharPtr<S, unsigned>::T scan(S s, unsigned len) {
    typename Scan<>::T val = 0;
    unsigned n = Scan<>::scan(val, s, len);
    if (ZuUnlikely(!n)) {
      m_val = Cmp::null();
      return 0U;
    }
    m_val = val;
    return n;
  }
  template <class Fmt, typename S>
  ZuInline typename ZuBox_IsCharPtr<S, unsigned>::T
  scan(Fmt, S s, unsigned len) {
    typename Scan<Fmt>::T val = 0;
    unsigned n = Scan<Fmt>::scan(val, s, len);
    if (ZuUnlikely(!n)) {
      m_val = Cmp::null();
      return 0U;
    }
    m_val = val;
    return n;
  }

  ZuInline unsigned length() const {
    return Print<>::length(m_val);
  }
  ZuInline unsigned print(char *buf) const {
    return Print<>::print(m_val, buf);
  }

  // Note: the caller is required to check for null, BY DESIGN

  ZuInline ZuBox operator -() { return -m_val; }

  template <typename R>
  ZuInline ZuBox operator +(const R &r) const { return m_val + r; }
  template <typename R>
  ZuInline ZuBox operator -(const R &r) const { return m_val - r; }
  template <typename R>
  ZuInline ZuBox operator *(const R &r) const { return m_val * r; }
  template <typename R>
  ZuInline ZuBox operator /(const R &r) const { return m_val / r; }
  template <typename R>
  ZuInline ZuBox operator %(const R &r) const { return m_val % r; }
  template <typename R>
  ZuInline ZuBox operator |(const R &r) const { return m_val | r; }
  template <typename R>
  ZuInline ZuBox operator &(const R &r) const { return m_val & r; }
  template <typename R>
  ZuInline ZuBox operator ^(const R &r) const { return m_val ^ r; }

  ZuInline ZuBox operator ++(int) { return m_val++; }
  ZuInline ZuBox &operator ++() { ++m_val; return *this; }
  ZuInline ZuBox operator --(int) { return m_val--; }
  ZuInline ZuBox &operator --() { --m_val; return *this; }

  template <typename R>
  ZuInline ZuBox &operator +=(const R &r) { m_val += r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator -=(const R &r) { m_val -= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator *=(const R &r) { m_val *= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator /=(const R &r) { m_val /= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator %=(const R &r) { m_val %= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator |=(const R &r) { m_val |= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator &=(const R &r) { m_val &= r; return *this; }
  template <typename R>
  ZuInline ZuBox &operator ^=(const R &r) { m_val ^= r; return *this; }

  // apply update (leaves existing value in place if r is null)
  ZuInline ZuBox &update(const ZuBox &r) {
    if (!Cmp::null(r)) m_val = r.m_val;
    return *this;
  }
  // apply update, with additional sentinel value signifying "reset to null"
  ZuInline ZuBox &update(const ZuBox &r, const ZuBox &reset) {
    if (!Cmp::null(r)) {
      if (r == reset)
	m_val = Cmp::null();
      else
	m_val = r;
    }
    return *this;
  }

  ZuInline static ZuBox inf() { return ZuBox(Cmp::inf()); }
  ZuInline ZuBox epsilon() const { return ZuBox(Cmp::epsilon(m_val)); }
  ZuInline bool feq(const T &r) const {
    if (Cmp::null(m_val)) return Cmp::null(r);
    if (Cmp::null(r)) return false;
    if (m_val == r) return true;
    if (m_val > r) return m_val - r <= Cmp::epsilon(m_val);
    return r - m_val <= Cmp::epsilon(r);
  }
  ZuInline bool fne(const T &r) const { return !feq(r); }
  ZuInline bool fge(const T &r) const {
    if (Cmp::null(m_val)) return Cmp::null(r);
    if (Cmp::null(r)) return false;
    if (m_val >= r) return true;
    return r - m_val <= Cmp::epsilon(r);
  }
  ZuInline bool fle(const T &r) const {
    if (Cmp::null(m_val)) return Cmp::null(r);
    if (Cmp::null(r)) return false;
    if (m_val <= r) return true;
    return m_val - r <= Cmp::epsilon(m_val);
  }
  ZuInline bool fgt(const T &r) const { return !fle(r); }
  ZuInline bool flt(const T &r) const { return !fge(r); }

  ZuInline int fcmp(const T &r) const {
    if (Cmp::null(r)) return Cmp::null(m_val) ? 0 : 1;
    if (Cmp::null(m_val)) return -1;
    if (m_val == r ||
	(m_val > r && m_val - r <= Cmp::epsilon(m_val)) ||
	(m_val < r && r - m_val <= Cmp::epsilon(r)))
      return 0;
    return m_val > r ? 1 : -1;
  }

  struct FCmp {
    ZuInline static int cmp(const ZuBox &f1, const ZuBox &f2)
      { return f1.fcmp(f2); }
    ZuInline static bool equals(const ZuBox &f1, const ZuBox &f2)
      { return f1.feq(f2); }
    ZuInline static bool null(const ZuBox &t) { return !*t; }
    ZuInline static const ZuBox &null() { static const ZuBox t; return t; }
  };

  typedef ZuBox_Tilde<T, Cmp, ZuTraits<T>::IsFloatingPoint> Tilde;
  ZuInline typename Tilde::T operator ~() const { return Tilde::fn(*this); }

  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator ==(const R &r) const { return feq(r); }
  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator !=(const R &r) const { return !feq(r); }
  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator >(const R &r) const { return !fle(r); }
  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator >=(const R &r) const { return fge(r); }
  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator <(const R &r) const { return !fge(r); }
  template <typename R> ZuInline typename ZuIs<ZuBox_Approx_, R, bool>::T
    operator <=(const R &r) const { return fle(r); }

private:
  T	m_val;
};

#define ZuBox0(T) ZuBox<T, ZuCmp0<T> >
#define ZuBox_1(T) ZuBox<T, ZuCmp_1<T> >
#define ZuBoxN(T, N) ZuBox<T, ZuCmpN<T, N> >

template <typename T_, typename Cmp>
struct ZuTraits<ZuBox<T_, Cmp> > : public ZuTraits<T_> {
  typedef ZuBox<T_, Cmp> T;
  enum { IsPrimitive = 0, IsComparable = 1, IsHashable = 1, IsBoxed = 1 };
};

// ZuCmp has to be specialized since null() is otherwise !t (instead of !*t)
template <typename T_, typename Cmp>
struct ZuCmp<ZuBox<T_, Cmp> > {
  typedef ZuBox<T_, Cmp> T;
  ZuInline static int cmp(const T &t1, const T &t2) { return t1.cmp(t2); }
  ZuInline static bool equals(const T &t1, const T &t2) { return t1 == t2; }
  ZuInline static bool null(const T &t) { return !*t; }
  ZuInline static const T &null() { static const T t; return t; }
};

// generic printing
template <typename B> struct ZuBoxPrint : public ZuPrintBuffer<B> {
  ZuInline static unsigned length(const B &b) {
    return b.length();
  }
  ZuInline static unsigned print(char *buf, unsigned, const B &b) {
    return b.print(buf);
  }
};
template <typename T, typename Cmp>
struct ZuPrint<ZuBox<T, Cmp> > : public ZuBoxPrint<ZuBox<T, Cmp> > { };
template <typename T, typename Fmt>
struct ZuPrint<ZuBoxFmt<T, Fmt> > : public ZuBoxPrint<ZuBoxFmt<T, Fmt> > { };
template <typename T>
struct ZuPrint<ZuBoxVFmt<T> > : public ZuBoxPrint<ZuBoxVFmt<T> > { };

// ZuBoxed(t) is a convenience function to cast primitives to boxed
template <typename T>
inline const typename ZuIsBoxed<T, T>::T &ZuBoxed(const T &t) { return t; }
template <typename T>
inline typename ZuIsBoxed<T, T>::T &ZuBoxed(T &t) { return t; }

// ZuBoxPtr(t) - convenience function to box pointers as uintptr_t
#define ZuBoxPtr(x) (ZuBox<uintptr_t, ZuCmp0<uintptr_t> >{(uintptr_t)(x)})

template <typename T>
inline const typename ZuNotBoxed<T, ZuBox<T> >::T &ZuBoxed(const T &t) {
  const ZuBox<T> *ZuMayAlias(t_) = (const ZuBox<T> *)&t;
  return *t_;
}
template <typename T>
inline typename ZuNotBoxed<T, ZuBox<T> >::T &ZuBoxed(T &t) {
  ZuBox<T> *ZuMayAlias(t_) = (ZuBox<T> *)&t;
  return *t_;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuBox_HPP */
