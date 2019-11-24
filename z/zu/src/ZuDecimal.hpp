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

// 128bit decimal fixed point with 36 digits and constant 10^18 scaling, i.e.
// 18 integer digits and 18 fractional digits (18 decimal places)

#ifndef ZuDecimal_HPP
#define ZuDecimal_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuDecimalFn.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuFmt.hpp>
#include <zlib/ZuBox.hpp>

template <typename Fmt> struct ZuDecimalFmt;	// internal
template <bool Ref = 0> struct ZuDecimalVFmt;	// internal

struct ZuDecimal {
  template <unsigned N> using Pow10 = ZuDecimalFn::Pow10<N>;

  static constexpr const int128_t minimum() {
    return -Pow10<36U>::pow10() + 1;
  }
  static constexpr const int128_t maximum() {
    return Pow10<36U>::pow10() - 1;
  }
  static constexpr const int128_t reset() {
    return -Pow10<36U>::pow10();
  }
  static constexpr const int128_t null() {
    return ((int128_t)1)<<127;
  }
  static constexpr const uint64_t scale() { // 10^18
    return Pow10<18U>::pow10();
  }
  static constexpr const long double scale_fp() { // 10^18
    return 1000000000000000000.0L;
  }

  int128_t	value;

  ZuInline ZuDecimal() : value{null()} { }
  ZuInline ZuDecimal(const ZuDecimal &v) : value(v.value) { }
  ZuInline ZuDecimal &operator =(const ZuDecimal &v) {
    value = v.value;
    return *this;
  }
  ZuInline ZuDecimal(ZuDecimal &&v) : value(v.value) { }
  ZuInline ZuDecimal &operator =(ZuDecimal &&v) {
    value = v.value;
    return *this;
  }
  ZuInline ~ZuDecimal() { }

  enum NoInit_ { NoInit };
  ZuInline ZuDecimal(NoInit_ _) { }

  enum Unscaled_ { Unscaled };
  ZuInline ZuDecimal(Unscaled_ _, int128_t v) : value{v} { }

  template <typename V>
  ZuInline ZuDecimal(V v, typename ZuIsIntegral<V>::T *_ = 0) :
      value(((int128_t)v) * scale()) { }

  template <typename V>
  ZuInline ZuDecimal(V v, typename ZuIsFloatingPoint<V>::T *_ = 0) :
      value((long double)v * scale_fp()) { }

  template <typename V>
  ZuInline ZuDecimal(V v, unsigned ndp, typename ZuIsIntegral<V>::T *_ = 0) :
      value(((int128_t)v) * ZuDecimalFn::pow10_64(18 - ndp)) { }

  ZuInline int128_t adjust(unsigned ndp) const {
    if (ZuUnlikely(ndp == 18)) return value;
    return value / ZuDecimalFn::pow10_64(18 - ndp);
  }

  ZuInline ZuDecimal operator -() { return ZuDecimal{Unscaled, -value}; }

  ZuInline ZuDecimal operator +(const ZuDecimal &v) const {
    return ZuDecimal{Unscaled, value + v.value};
  }
  ZuInline ZuDecimal &operator +=(const ZuDecimal &v) {
    value += v.value;
    return *this;
  }

  ZuInline ZuDecimal operator -(const ZuDecimal &v) const {
    return ZuDecimal{Unscaled, value - v.value};
  }
  ZuInline ZuDecimal &operator -=(const ZuDecimal &v) {
    value -= v.value;
    return *this;
  }

  // mul and div functions based on reference code (BSD licensed) at:
  // https://www.codeproject.com/Tips/618570/UInt-Multiplication-Squaring 

  // h:l = u * v
  static void mul128by128(
      uint128_t u, uint128_t v,
      uint128_t &h, uint128_t &l) {
    uint128_t u1 = ((uint64_t)u);
    uint128_t v1 = ((uint64_t)v);
    uint128_t t = (u1 * v1);
    uint128_t w3 = ((uint64_t)t);
    uint128_t k = (t>>64);

    u >>= 64;
    t = (u * v1) + k;
    k = ((uint64_t)t);
    uint128_t w1 = (t>>64);

    v >>= 64;
    t = (u1 * v) + k;
    k = (t>>64);

    h = (u * v) + w1 + k;
    l = (t<<64) + w3;
  }

  // same as mul128by128, but constant multiplier 10^18
  // h:l = u * 10^18;
  static void mul128scale(
      uint128_t u,
      uint128_t &h, uint128_t &l) {
    uint128_t u1 = ((uint64_t)u);
    const uint128_t v1 = scale();
    uint128_t t = (u1 * v1);
    uint128_t w3 = ((uint64_t)t);
    uint128_t k = (t>>64);

    u >>= 64;

    t = u * v1 + k;
    k = ((uint64_t)t);

    h = (t>>64) + (k>>64);
    l = (k<<64) + w3;
  }

  // q = u1:u0 / v
  static void div256by128(
      const uint128_t u1, const uint128_t u0, uint128_t v,
      uint128_t &q) {// , uint128_t &r)
    const uint128_t b = ((uint128_t)1)<<64;
    uint128_t un1, un0, vn1, vn0, q1, q0, un128, un21, un10, rhat, left, right;
    size_t s;

    if (!v)
      s = 0;
    else if (v < b)
      s = __builtin_clzll((uint64_t)v) + 64;
    else
      s = __builtin_clzll((uint64_t)(v>>64));
    v <<= s;
    vn0 = (uint64_t)v;
    vn1 = v>>64;

    if (s > 0) {
      un128 = (u1<<s) | (u0>>(128 - s));
      un10 = u0<<s;
    } else {
      un128 = u1;
      un10 = u0;
    }

    un1 = un10>>64;
    un0 = (uint64_t)un10;

    q1 = un128 / vn1;
    rhat = un128 % vn1;

    left = q1 * vn0;
    right = (rhat<<64) + un1;

  loop1:
    if ((q1 >= b) || (left > right)) {
      --q1;
      rhat += vn1;
      if (rhat < b) {
	left -= vn0;
	right = (rhat<<64) | un1;
	goto loop1;
      }
    }

    un21 = (un128<<64) + (un1 - (q1 * v));

    q0 = un21 / vn1;
    rhat = un21 % vn1;

    left = q0 * vn0;
    right = (rhat<<64) | un0;

  loop2:
    if ((q0 >= b) || (left > right)) {
      --q0;
      rhat += vn1;
      if (rhat < b) {
	left -= vn0;
	right = (rhat<<64) | un0;
	goto loop2;
      }
    }

    // r = ((un21<<64) + (un0 - (q0 * v)))>>s;
    q = (q1<<64) | q0;
  }

  // same as div256by128, but constant divisor 10^18, no remainder
  // q = u1:u0 * 10^-18
  static uint128_t div256scale(const uint128_t u1, const uint128_t u0) {
    const uint128_t b = ((uint128_t)1)<<64;
    const uint128_t v = ((uint128_t)scale())<<68;

    uint128_t un1, un0, vn1, vn0, q1, q0, un128, un21, un10, rhat, left, right;

    vn0 = (uint64_t)v;
    vn1 = v>>64;

    un128 = (u1<<68) | (u0>>60);
    un10 = u0<<68;

    un1 = un10>>64;
    un0 = (uint64_t)un10;

    q1 = un128 / vn1;
    rhat = un128 % vn1;

    left = q1 * vn0;
    right = (rhat<<64) + un1;

  loop1:
    if ((q1 >= b) || (left > right)) {
      --q1;
      rhat += vn1;
      if (rhat < b) {
	left -= vn0;
	right = (rhat<<64) | un1;
	goto loop1;
      }
    }

    un21 = (un128<<64) + (un1 - (q1 * v));

    q0 = un21 / vn1;
    rhat = un21 % vn1;

    left = q0 * vn0;
    right = (rhat<<64) | un0;

  loop2:
    if ((q0 >= b) || (left > right)) {
      --q0;
      rhat += vn1;
      if (rhat < b) {
	left -= vn0;
	right = (rhat<<64) | un0;
	goto loop2;
      }
    }

    // r = ((un21<<64) + (un0 - (q0 * v)))>>s;
    return (q1<<64) | q0;
  }

  static int128_t mul(const int128_t u_, const int128_t v_) {
    uint128_t u, v;
    bool negative;

    if (u_ < 0) {
      u = -u_;
      if (v_ < 0) {
	v = -v_;
	negative = false;
      } else {
	v = v_;
	negative = true;
      }
    } else {
      u = u_;
      if (v_ < 0) {
	v = -v_;
	negative = true;
      } else {
	v = v_;
	negative = false;
      }
    }

    uint128_t h, l;

    mul128by128(u, v, h, l);

    if (h >= scale()) return null(); // overflow

    u = div256scale(h, l);

    if (negative) return -((int128_t)u);
    return u;
  }

  static int128_t div(const int128_t u_, const int128_t v_) {
    uint128_t u, v;
    bool negative;

    if (u_ < 0) {
      u = -u_;
      if (v_ < 0) {
	v = -v_;
	negative = false;
      } else {
	v = v_;
	negative = true;
      }
    } else {
      u = u_;
      if (v_ < 0) {
	v = -v_;
	negative = true;
      } else {
	v = v_;
	negative = false;
      }
    }

    uint128_t h, l;

    mul128scale(u, h, l);

    div256by128(h, l, v, u);

    if (negative) return -((int128_t)u);
    return u;
  }

public:
  ZuInline ZuDecimal operator *(const ZuDecimal &v) const {
    return ZuDecimal{Unscaled, mul(value, v.value)};
  }

  ZuInline ZuDecimal &operator *=(const ZuDecimal &v) {
    value = mul(value, v.value);
    return *this;
  }

  ZuInline ZuDecimal operator /(const ZuDecimal &v) const {
    return ZuDecimal{Unscaled, div(value, v.value)};
  }
  ZuInline ZuDecimal &operator /=(const ZuDecimal &v) {
    value = div(value, v.value);
    return *this;
  }

  // scan from string
  template <typename S>
  inline ZuDecimal(const S &s_, typename ZuIsString<S>::T *_ = 0) {
    ZuString s(s_);
    if (ZuUnlikely(!s)) goto null;
    {
      bool negative = s[0] == '-';
      if (ZuUnlikely(negative)) {
	s.offset(1);
	if (ZuUnlikely(!s)) goto null;
      }
      while (s[0] == '0') s.offset(1);
      if (!s) { value = 0; return; }
      uint64_t iv = 0, fv = 0;
      unsigned n = s.length();
      if (ZuUnlikely(s[0] == '.')) {
	if (ZuUnlikely(n == 1)) { value = 0; return; }
	goto frac;
      }
      n = Zu_atou(iv, s.data(), n);
      if (ZuUnlikely(!n)) goto null;
      if (ZuUnlikely(n > 18)) goto null; // overflow
      s.offset(n);
      if ((n = s.length()) > 1 && s[0] == '.') {
  frac:
	if (--n > 18) n = 18;
	n = Zu_atou(fv, &s[1], n);
	if (fv && n < 18)
	  fv *= ZuDecimalFn::pow10_64(18 - n);
      }
      value = ((uint128_t)iv) * scale() + fv;
      if (ZuUnlikely(negative)) value = -value;
    }
    return;
  null:
    value = null();
    return;
  }

  // convert to floating point
  ZuInline long double fp() {
    if (ZuUnlikely(value == null())) return ZuFP<long double>::nan();
    return (long double)value / scale_fp();
  }

  // comparisons
  ZuInline bool equals(const ZuDecimal &v) const { return value == v.value; }
  ZuInline int cmp(const ZuDecimal &v) const {
    return value > v.value ? 1 : value < v.value ? -1 : 0;
  }
  ZuInline bool operator ==(const ZuDecimal &v) const { return value == v.value; }
  ZuInline bool operator !=(const ZuDecimal &v) const { return value != v.value; }
  ZuInline bool operator >(const ZuDecimal &v) const { return value > v.value; }
  ZuInline bool operator >=(const ZuDecimal &v) const { return value >= v.value; }
  ZuInline bool operator <(const ZuDecimal &v) const { return value < v.value; }
  ZuInline bool operator <=(const ZuDecimal &v) const { return value <= v.value; }

  // ! is zero, unary * is !null
  ZuInline bool operator !() const { return !value; }
  ZuOpBool

  ZuInline bool operator *() const {
    // return value != null(); // disabled due to compiler bug
    int128_t v = value - null();
    return (bool)((uint64_t)(v>>64) | (uint64_t)(v));
  }

  template <typename S> void print(S &s) const;

  template <typename Fmt> ZuDecimalFmt<Fmt> fmt(Fmt) const;
  ZuDecimalVFmt<> vfmt() const;
  ZuDecimalVFmt<1> vfmt(ZuVFmt &) const;
};
template <typename Fmt> struct ZuDecimalFmt {
  const ZuDecimal	&fixed;

  template <typename S> inline void print(S &s) const {
    if (ZuUnlikely(!*fixed)) return;
    uint128_t iv, fv;
    if (ZuUnlikely(fixed.value < 0)) {
      s << '-';
      iv = -fixed.value;
    } else
      iv = fixed.value;
    fv = iv % ZuDecimal::scale();
    iv /= ZuDecimal::scale();
    s << ZuBoxed(iv).fmt(Fmt());
    if (fv) s << '.' << ZuBoxed(fv).fmt(ZuFmt::Frac<18>());
  }
};
template <typename Fmt>
struct ZuPrint<ZuDecimalFmt<Fmt> > : public ZuPrintFn { };
template <class Fmt>
ZuInline ZuDecimalFmt<Fmt> ZuDecimal::fmt(Fmt) const
{
  return ZuDecimalFmt<Fmt>{*this};
}
template <typename S> inline void ZuDecimal::print(S &s) const
{
  s << ZuDecimalFmt<ZuFmt::Default>{*this};
}
template <> struct ZuPrint<ZuDecimal> : public ZuPrintFn { };
template <bool Ref>
struct ZuDecimalVFmt : public ZuVFmtWrapper<ZuDecimalVFmt<Ref>, Ref> {
  const ZuDecimal	&fixed;

  ZuInline ZuDecimalVFmt(const ZuDecimal &fixed_) :
    fixed{fixed_} { }
  ZuInline ZuDecimalVFmt(const ZuDecimal &fixed_, ZuVFmt &fmt_) :
    ZuVFmtWrapper<ZuDecimalVFmt, Ref>{fmt_}, fixed{fixed_} { }

  template <typename S> inline void print(S &s) const {
    if (ZuUnlikely(!*fixed)) return;
    uint128_t iv, fv;
    if (ZuUnlikely(fixed.value < 0)) {
      s << '-';
      iv = -fixed.value;
    } else
      iv = fixed.value;
    fv = iv % ZuDecimal::scale();
    iv /= ZuDecimal::scale();
    s << ZuBoxed(iv).vfmt(this->fmt);
    if (fv) s << '.' << ZuBoxed(fv).fmt(ZuFmt::Frac<18>());
  }
};
ZuInline ZuDecimalVFmt<> ZuDecimal::vfmt() const
{
  return ZuDecimalVFmt<>{*this};
}
ZuInline ZuDecimalVFmt<1> ZuDecimal::vfmt(ZuVFmt &fmt) const
{
  return ZuDecimalVFmt<1>{*this, fmt};
}
template <bool Ref>
struct ZuPrint<ZuDecimalVFmt<Ref> > : public ZuPrintFn { };

template <> struct ZuTraits<ZuDecimal> : public ZuTraits<int128_t> {
  typedef ZuDecimal T;
  enum { IsPrimitive = 0, IsComparable = 1, IsHashable = 1 };
};

// ZuCmp has to be specialized since null() is otherwise !t (instead of !*t)
template <> struct ZuCmp<ZuDecimal> {
  typedef ZuDecimal T;
  ZuInline static int cmp(const T &t1, const T &t2) { return t1.cmp(t2); }
  ZuInline static bool equals(const T &t1, const T &t2) { return t1 == t2; }
  ZuInline static bool null(const T &t) { return !*t; }
  ZuInline static const T &null() { static const T t; return t; }
};

#endif /* ZuDecimal_HPP */
