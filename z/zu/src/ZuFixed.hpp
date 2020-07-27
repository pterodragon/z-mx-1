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

// 64bit decimal variable point with variable exponent,
// 18 significant digits and 10^-exponent scaling:
//   (18 - exponent) integer digits
//   exponent fractional digits (i.e. number of decimal places)
// Note: exponent is negated

#ifndef ZuFixed_HPP
#define ZuFixed_HPP

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

// ZuFixedVal value range is +/- 10^18
typedef ZuBox<int64_t> ZuFixedVal; // fixed point value (mantissa numerator)
typedef ZuBox<uint8_t> ZuFixedExp; // exponent, i.e. number of decimal places

#define ZuFixedMin (ZuDecVal{static_cast<int64_t>(-999999999999999999LL)})
#define ZuFixedMax (ZuDecVal{static_cast<int64_t>(999999999999999999LL)})
// ZuFixedValReset is the distinct sentinel value used to reset values to null
#define ZuFixedReset (ZuDecVal{static_cast<int64_t>(-1000000000000000000LL)})

// combination of value and exponent, used as a temporary for conversions & I/O
// constructors / scanning:
//   ZuFixed(<integer>, exponent)		// {1042, 2} -> 10.42
//   ZuFixed(<floating point>, exponent)	// {10.42, 2} -> 10.42
//   ZuFixed(<string>, exponent)		// {"10.42", 2} -> 10.42
//   ZuFixedVal x = ZuFixed{"42.42", 2}.value	// x == 4242
//   ZuFixed xn{x, 2}; xn *= ZuFixed{2000, 3}; x = xn.value // x == 8484
// printing:
//   s << ZuFixed{...}			// print (default)
//   s << ZuFixed{...}.fmt(ZuFmt...)	// print (compile-time formatted)
//   s << ZuFixed{...}.vfmt(ZuVFmt...)	// print (variable run-time formatted)
//   s << ZuFixed(x, 2)			// s << "42.42"
//   x = 4200042;				// 42000.42
//   s << ZuFixed(x, 2).fmt(ZuFmt::Comma<>())	// s << "42,000.42"

template <typename Fmt> struct ZuFixed_Fmt;	// internal
class ZuFixed_VFmt;				// internal

struct ZuFixed {
  ZuFixedVal	value;	// mantissa
  unsigned	exponent = 0;

  ZuInline ZuFixed() { }

  template <typename V>
  ZuInline ZuFixed(V value_, unsigned exponent_,
      typename ZuIsIntegral<V>::T *_ = 0) :
    value(value_), exponent(exponent_) { }

  template <typename V>
  ZuInline ZuFixed(V value_, unsigned exponent_,
      typename ZuIsFloatingPoint<V>::T *_ = 0) :
    value((double)value_ * ZuDecimalFn::pow10_64(exponent_)),
    exponent(exponent_) { }

  // multiply: exponent of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  ZuFixed operator *(const ZuFixed &v) const {
    int128_t i = (typename ZuFixedVal::T)value;
    i *= (typename ZuFixedVal::T)v.value;
    i /= ZuDecimalFn::pow10_64(v.exponent);
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return ZuFixed{ZuFixedVal{}, exponent};
    return ZuFixed{static_cast<int64_t>(i), exponent};
  }

  // divide: exponent of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  ZuFixed operator /(const ZuFixed &v) const {
    int128_t i = (typename ZuFixedVal::T)value;
    i *= ZuDecimalFn::pow10_64(v.exponent);
    i /= (typename ZuFixedVal::T)v.value;
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return ZuFixed{ZuFixedVal(), exponent};
    return ZuFixed{static_cast<int64_t>(i), exponent};
  }

  // scan from string
  template <typename S>
  ZuFixed(const S &s_, int exponent_ = -1,
      typename ZuIsString<S>::T *_ = 0) {
    ZuString s(s_);
    if (ZuUnlikely(!s || exponent_ > 18)) goto null;
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
	if (exponent_ < 0) exponent_ = n - 1;
	goto frac;
      }
      n = Zu_atou(iv, s.data(), n);
      if (ZuUnlikely(!n)) goto null;
      if (ZuUnlikely(n > (18 - (exponent_ < 0 ? 0 : exponent_))))
	goto null; // overflow
      s.offset(n);
      if (exponent_ < 0) exponent_ = 18 - n;
      if ((n = s.length()) > 1 && s[0] == '.') {
  frac:
	if (--n > exponent_) n = exponent_;
	n = Zu_atou(fv, &s[1], n);
	if (fv && exponent_ > n)
	  fv *= ZuDecimalFn::pow10_64(exponent_ - n);
      }
      value = iv * ZuDecimalFn::pow10_64(exponent_) + fv;
      if (ZuUnlikely(negative)) value = -value;
    }
    exponent = exponent_;
    return;
  null:
    value = ZuFixedVal();
    return;
  }

  // convert to floating point
  template <typename Float = ZuBox<double>>
  ZuInline Float fp() const {
    if (ZuUnlikely(!*value)) return Float{};
    return Float{value} / Float{ZuDecimalFn::pow10_64(exponent)};
  }

  // adjust to another exponent
  ZuInline ZuFixedVal adjust(unsigned exponent) const {
    if (ZuLikely(exponent == this->exponent)) return value;
    if (!*value) return ZuFixedVal{};
    if (exponent > this->exponent)
      return value * ZuDecimalFn::pow10_64(exponent - this->exponent);
    return value / ZuDecimalFn::pow10_64(this->exponent - exponent);
  }

  // comparisons
  ZuInline int cmp(const ZuFixed &v) const {
    if (ZuLikely(exponent == v.exponent || !*value || !*v.value))
      return value.cmp(v.value);
    int128_t i = (typename ZuFixedVal::T)value;
    int128_t j = (typename ZuFixedVal::T)v.value;
    if (exponent < v.exponent)
      i *= ZuDecimalFn::pow10_64(v.exponent - exponent);
    else
      j *= ZuDecimalFn::pow10_64(exponent - v.exponent);
    return (i > j) - (i < j);
  }
  ZuInline bool less(const ZuFixed &v) const {
    if (ZuLikely(exponent == v.exponent || !*value || !*v.value))
      return value < v.value;
    int128_t i = (typename ZuFixedVal::T)value;
    int128_t j = (typename ZuFixedVal::T)v.value;
    if (exponent < v.exponent)
      i *= ZuDecimalFn::pow10_64(v.exponent - exponent);
    else
      j *= ZuDecimalFn::pow10_64(exponent - v.exponent);
    return i < j;
  }
  ZuInline bool equals(const ZuFixed &v) const {
    if (ZuLikely(exponent == v.exponent || !*value || !*v.value))
      return value == v.value;
    int128_t i = (typename ZuFixedVal::T)value;
    int128_t j = (typename ZuFixedVal::T)v.value;
    if (exponent < v.exponent)
      i *= ZuDecimalFn::pow10_64(v.exponent - exponent);
    else
      j *= ZuDecimalFn::pow10_64(exponent - v.exponent);
    return i == j;
  }
  ZuInline bool operator ==(const ZuFixed &v) const { return equals(v); }
  ZuInline bool operator !=(const ZuFixed &v) const { return !equals(v); }
  ZuInline bool operator >(const ZuFixed &v) const { return cmp(v) > 0; }
  ZuInline bool operator >=(const ZuFixed &v) const { return cmp(v) >= 0; }
  ZuInline bool operator <(const ZuFixed &v) const { return cmp(v) < 0; }
  ZuInline bool operator <=(const ZuFixed &v) const { return cmp(v) <= 0; }

  // ! is zero, unary * is !null
  ZuInline bool operator !() const { return !value; }
  ZuOpBool

  ZuInline bool operator *() const { return *value; }

  template <typename S> void print(S &s) const;

  template <typename Fmt> ZuFixed_Fmt<Fmt> fmt(Fmt) const;
  ZuFixed_VFmt vfmt() const;
  template <typename VFmt>
  ZuFixed_VFmt vfmt(VFmt &&) const;
};
template <typename Fmt> struct ZuFixed_Fmt {
  const ZuFixed	&fixed;

  template <typename S> void print(S &s) const {
    ZuFixedVal iv = fixed.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(fixed.exponent);
    ZuFixedVal fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).fmt(Fmt());
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixed.exponent);
  }
};
template <typename Fmt>
struct ZuPrint<ZuFixed_Fmt<Fmt> > : public ZuPrintFn { };
template <class Fmt>
ZuInline ZuFixed_Fmt<Fmt> ZuFixed::fmt(Fmt) const
{
  return ZuFixed_Fmt<Fmt>{*this};
}
template <typename S> inline void ZuFixed::print(S &s) const
{
  s << ZuFixed_Fmt<ZuFmt::Default>{*this};
}
template <> struct ZuPrint<ZuFixed> : public ZuPrintFn { };
class ZuFixed_VFmt : public ZuVFmtWrapper<ZuFixed_VFmt> {
public:
  ZuInline ZuFixed_VFmt(const ZuFixed &fixed) : m_fixed{fixed} { }
  template <typename VFmt>
  ZuInline ZuFixed_VFmt(const ZuFixed &fixed, VFmt &&fmt) :
    ZuVFmtWrapper<ZuFixed_VFmt>{ZuFwd<VFmt>(fmt)}, m_fixed{fixed} { }

  template <typename S> void print(S &s) const {
    ZuFixedVal iv = m_fixed.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(m_fixed.exponent);
    ZuFixedVal fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).vfmt(this->fmt);
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(m_fixed.exponent);
  }

private:
  const ZuFixed	&m_fixed;
};
inline ZuFixed_VFmt ZuFixed::vfmt() const {
  return ZuFixed_VFmt{*this};
}
template <typename VFmt>
inline ZuFixed_VFmt ZuFixed::vfmt(VFmt &&fmt) const {
  return ZuFixed_VFmt{*this, ZuFwd<VFmt>(fmt)};
}
template <>
struct ZuPrint<ZuFixed_VFmt> : public ZuPrintFn { };

#endif /* ZuFixed_HPP */
