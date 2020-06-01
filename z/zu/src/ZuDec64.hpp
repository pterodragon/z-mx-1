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

// 64bit decimal variable point with variable decimal exponent,
// 18 significant digits and 10^-exponent scaling:
//   (18 - exponent) integer digits
//   exponent fractional digits (i.e. decimal places)
// Note: exponent is negated

#ifndef ZuDec64_HPP
#define ZuDec64_HPP

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

// ZuDec64Val value range is +/- 10^18
typedef ZuBox<int64_t> ZuDec64Val; // fixed point value (numerator)
typedef ZuBox<uint8_t> ZuDec64Exp; // exponent, i.e. number of decimal places

#define ZuDec64Min (ZuDecVal{static_cast<int64_t>(-999999999999999999LL)})
#define ZuDec64Max (ZuDecVal{static_cast<int64_t>(999999999999999999LL)})
// ZuDec64ValReset is the distinct sentinel value used to reset values to null
#define ZuDec64Reset (ZuDecVal{static_cast<int64_t>(-1000000000000000000LL)})

// combination of value and exponent, used as a temporary for conversions & I/O
// constructors / scanning:
//   ZuDec64(<integer>, exponent)		// {1042, 2} -> 10.42
//   ZuDec64(<floating point>, exponent)	// {10.42, 2} -> 10.42
//   ZuDec64(<string>, exponent)		// {"10.42", 2} -> 10.42
//   ZuDec64Val x = ZuDec64{"42.42", 2}.value	// x == 4242
//   ZuDec64 xn{x, 2}; xn *= ZuDec64{2000, 3}; x = xn.value // x == 8484
// printing:
//   s << ZuDec64{...}			// print (default)
//   s << ZuDec64{...}.fmt(ZuFmt...)	// print (compile-time formatted)
//   s << ZuDec64{...}.vfmt(ZuVFmt...)	// print (variable run-time formatted)
//   s << ZuDec64(x, 2)			// s << "42.42"
//   x = 4200042;				// 42000.42
//   s << ZuDec64(x, 2).fmt(ZuFmt::Comma<>())	// s << "42,000.42"

template <typename Fmt> struct ZuDec64_Fmt;	// internal
template <bool Ref = 0> struct ZuDec64_VFmt;	// internal

struct ZuDec64 {
  ZuDec64Val	value;
  unsigned	exponent = 0;

  ZuInline ZuDec64() { }

  template <typename V>
  ZuInline ZuDec64(V value_, unsigned exponent_,
      typename ZuIsIntegral<V>::T *_ = 0) :
    value(value_), exponent(exponent_) { }

  template <typename V>
  ZuInline ZuDec64(V value_, unsigned exponent_,
      typename ZuIsFloatingPoint<V>::T *_ = 0) :
    value((MxFloat)value_ * ZuDecimalFn::pow10_64(exponent_)),
    exponent(exponent_) { }

  // multiply: exponent of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  ZuDec64 operator *(const ZuDec64 &v) const {
    int128_t i = (typename ZuDec64Val::T)value;
    i *= (typename ZuDec64Val::T)v.value;
    i /= ZuDecimalFn::pow10_64(v.exponent);
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return ZuDec64{ZuDec64Val{}, exponent};
    return ZuDec64{static_cast<int64_t>(i), exponent};
  }

  // divide: exponent of result is taken from the LHS
  // a 128bit integer intermediary is used to avoid overflow
  ZuDec64 operator /(const ZuDec64 &v) const {
    int128_t i = (typename ZuDec64Val::T)value;
    i *= ZuDecimalFn::pow10_64(v.exponent);
    i /= (typename ZuDec64Val::T)v.value;
    if (ZuUnlikely(i >= 1000000000000000000ULL))
      return ZuDec64{ZuDec64Val(), exponent};
    return ZuDec64{static_cast<int64_t>(i), exponent};
  }

  // scan from string
  template <typename S>
  ZuDec64(const S &s_, int exponent_ = -1,
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
    value = ZuDec64Val();
    return;
  }

  // convert to floating point
  template <typename Float = ZuBox<double>>
  ZuInline Float fp() const {
    if (ZuUnlikely(!*value)) return Float{};
    return Float{value} / Float{ZuDecimalFn::pow10_64(exponent)};
  }

  // adjust to another exponent
  ZuInline ZuDec64Val adjust(unsigned exponent) const {
    if (ZuLikely(exponent == this->exponent)) return value;
    if (!*value) return ZuDec64Val();
    return (ZuDec64{1.0, exponent} * (*this)).value;
  }

  // ! is zero, unary * is !null
  ZuInline bool operator !() const { return !value; }
  ZuOpBool

  ZuInline bool operator *() const { return *value; }

  template <typename S> void print(S &s) const;

  template <typename Fmt> ZuDec64_Fmt<Fmt> fmt(Fmt) const;
  ZuDec64_VFmt<> vfmt() const;
  ZuDec64_VFmt<1> vfmt(ZuVFmt &) const;
};
template <typename Fmt> struct ZuDec64_Fmt {
  const ZuDec64	&fixed;

  template <typename S> void print(S &s) const {
    ZuDec64Val iv = fixed.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(fixed.exponent);
    ZuDec64Val fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).fmt(Fmt());
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixed.exponent);
  }
};
template <typename Fmt>
struct ZuPrint<ZuDec64_Fmt<Fmt> > : public ZuPrintFn { };
template <class Fmt>
ZuInline ZuDec64_Fmt<Fmt> ZuDec64::fmt(Fmt) const
{
  return ZuDec64_Fmt<Fmt>{*this};
}
template <typename S> inline void ZuDec64::print(S &s) const
{
  s << ZuDec64_Fmt<ZuFmt::Default>{*this};
}
template <> struct ZuPrint<ZuDec64> : public ZuPrintFn { };
template <bool Ref>
struct ZuDec64_VFmt : public ZuVFmtWrapper<ZuDec64_VFmt<Ref>, Ref> {
  const ZuDec64	&fixed;

  ZuInline ZuDec64_VFmt(const ZuDec64 &fixed_) :
    fixed{fixed_} { }
  ZuInline ZuDec64_VFmt(const ZuDec64 &fixed_, ZuVFmt &fmt_) :
    ZuVFmtWrapper<ZuDec64_VFmt, Ref>{fmt_}, fixed{fixed_} { }

  template <typename S> void print(S &s) const {
    ZuDec64Val iv = fixed.value;
    if (ZuUnlikely(!*iv)) return;
    if (ZuUnlikely(iv < 0)) { s << '-'; iv = -iv; }
    uint64_t factor = ZuDecimalFn::pow10_64(fixed.exponent);
    ZuDec64Val fv = iv % factor;
    iv /= factor;
    s << ZuBoxed(iv).vfmt(this->fmt);
    if (fv) s << '.' << ZuBoxed(fv).vfmt().frac(fixed.exponent);
  }
};
ZuInline ZuDec64_VFmt<> ZuDec64::vfmt() const
{
  return ZuDec64_VFmt<>{*this};
}
ZuInline ZuDec64_VFmt<1> ZuDec64::vfmt(ZuVFmt &fmt) const
{
  return ZuDec64_VFmt<1>{*this, fmt};
}
template <bool Ref>
struct ZuPrint<ZuDec64_VFmt<Ref> > : public ZuPrintFn { };

#endif /* ZuDec64_HPP */
