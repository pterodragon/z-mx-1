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

// 128bit (64bit/64bit) decimal fixed point

#ifndef ZuFixed_HPP
#define ZuFixed_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuDecimal.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuFmt.hpp>
#include <zlib/ZuBox.hpp>

template <typename Fmt> struct ZuFixedFmt;	// internal
template <bool Ref = 0> struct ZuFixedVFmt;	// internal

struct ZuFixed {
private:
  static constexpr const int64_t reset_() {
    return (int64_t)-1000000000000000000LL;
  }
  static constexpr const int64_t null_() {
    return (int64_t)-0x8000000000000000LL;
  }
  static constexpr const long double pow10_18() { // 10^18
    return 1000000000000000000.0L;
  }
  template <unsigned U> using Pow10 = ZuDecimal::Pow10<U>;

public:
  int64_t	i_;	// integer portion
  int64_t	f_;	// decimal fractional portion (10^-18)

  ZuInline ZuFixed() : i_(null_()), f_(0) { }
  ZuInline ZuFixed(int64_t i__, uint64_t f__) : i_(i__), f_(f__) { }

  template <typename V>
  ZuInline ZuFixed(V v, typename ZuIsIntegral<V>::T *_ = 0) : i_(v), f_(0) { }

  template <typename V>
  ZuInline ZuFixed(V v, typename ZuIsFloatingPoint<V>::T *_ = 0) :
    i_(v), f_((long double)(v - (V)i) * pow10_18()) { }

  ZuInline ZuFixed operator +(const ZuFixed &v) const {
    int64_t i = i_ + v.i_;
    int64_t f = f_ + v.f_;
    if (i < 0) {
      if (f >= 0)
	++i, f -= Pow10<18U>::pow10();
    } else {
    }
    if (v.i_ >= 0) {
      r.i_ = i_ + v.i_;
      r.f_ = f_ + v.f_;
      if (r.f_ >= Pow10<18U>::pow10()) {
	r.f_ -= Pow10<18U>::pow10();
	++r.i_;
      }
    return r;

  }
  ZuInline ZuFixed &operator +=(const ZuFixed &v) {
    // FIXME
    return *this;
  }

  ZuInline ZuFixed operator -(const ZuFixed &v) const {
    // FIXME
  }
  ZuInline ZuFixed &operator -=(const ZuFixed &v) {
    // FIXME
    return *this;
  }

  ZuInline ZuFixed operator *(const ZuFixed &v) const {
    bool negative;
    int64_t i, v_i;
    int64_t f, v_f;
    if (i_ < 0) {
      i = -i_, f = -f_;
      if (negative = v.i_ >= 0)
	v_i = v.i_, v_f = v.f_;
      else
	v_i = -v.i_, v_f = -v.f_;
    } else {
      i = i_, f = f_;
      if (negative = v.i_ < 0)
	v_i = -v.i_, v_f = -v.f_;
      else
	v_i = v.i_, v_f = v.f_;
    }
    uint128_t c = ((uint128_t)i * v_f) + ((uint128_t)v_i * f);
    i = (i * v_i) + (c / Pow10<18U>::pow10());
    uint128_t u = ((uint128_t)f * v_f);
    f = (c % Pow10<18U>::pow10()) + (u / dec18_int64());
    if (negative) i = -i, f = -f;
    return ZuFixed{i, f};
  }
  ZuInline ZuFixed &operator *=(const ZuFixed &v) {
    // FIXME
    return *this;
  }

  ZuInline ZuFixed operator inv() const {
    int64_t i = i_, f = f_;
    bool negative = i_ < 0;
    if (negative) i = -i, f = -f;
    uint128_t t = ((uint128_t)i * Pow10<18U>::pow10()) + f;
    uint128_t u = Pow10<36U>::pow10();
    i = u / t;
    f = ((u % t) * Pow10<18U>::pow10()) / t;
    if (negative) i = -i, f = -f;
    return ZuFixed{i, f};
  }

  ZuInline ZuFixed operator /(const ZuFixed &f) const {
    return *this * f.inv();
  }
  ZuInline ZuFixed &operator /=(const ZuFixed &v) {
    // FIXME
    return *this;
  }

  // scan from string
  template <typename S>
  inline ZuFixed(const S &s_, typename ZuIsString<S>::T *_ = 0) {
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
      uint64_t i = 0;
      unsigned n = s.length();
      if (ZuUnlikely(s[0] == '.')) {
	if (ZuUnlikely(n == 1)) { value = 0; return; }
	goto frac;
      }
      n = Zu_atou(i, s.data(), n);
      if (ZuUnlikely(!n)) goto null;
      if (ZuUnlikely(n > 18)) goto null; // overflow
      s.offset(n);
      if ((n = s.length()) > 1 && s[0] == '.') {
  frac:
	if (--n > 18) n = 18;
	n = Zu_atou(f_, &s[1], n);
	if (f_ && n < 18) f_ *= ZuDecimal::pow10_64(18 - n);
      }
      i_ = i;
      if (ZuUnlikely(negative)) i_ = -i_, f_ = -f_;
    }
    return;
  null:
    i_ = null_();
    f_ = 0;
    return;
  }

  // convert to floating point
  ZuInline long double fp() {
    if (ZuUnlikely(i_ == null_())) return ZuFP<long double>::nan();
    return (long double)i_ + (long double)f_ / pow10_18();
  }

  // comparisons
  ZuInline bool equals(const ZuFixed &v) const { /* FIXME */ }
  ZuInline int cmp(const ZuFixed &v) const { /* FIXME */ }
  ZuInline bool operator ==(const ZuFixed &v) const { return equals(v); }
  ZuInline bool operator !=(const ZuFixed &v) const { return !equals(v); }
  ZuInline bool operator >(const ZuFixed &v) const { return cmp(v) > 0; }
  ZuInline bool operator >=(const ZuFixed &v) const { return cmp(v) >= 0; }
  ZuInline bool operator <(const ZuFixed &v) const { return cmp(v) < 0; }
  ZuInline bool operator <=(const ZuFixed &v) const { return cmp(v) <= 0; }

  // ! is zero, unary * is !null
  ZuInline bool operator !() const { return !i_ && !f_; }
  ZuOpBool

  ZuInline bool operator *() const { return i_ != null_(); }

  template <typename S> void print(S &s) const;

  template <typename Fmt> ZuFixedFmt<Fmt> fmt(Fmt) const;
  ZuFixedVFmt<> vfmt() const;
  ZuFixedVFmt<1> vfmt(ZuVFmt &) const;
};
template <typename Fmt> struct ZuFixedFmt {
  const ZuFixed	&fixed;

  template <typename S> inline void print(S &s) const {
    if (ZuUnlikely(!*fixed)) return;
    int64_t i = fixed.i_, f = fixed.f_;
    if (ZuUnlikely(i < 0)) { s << '-'; i = -i, f = -f; }
    s << ZuBoxed(i).fmt(Fmt());
    if (f) s << '.' << ZuBoxed(f).fmt(ZuFmt::Frac<18>());
  }
};
template <typename Fmt>
struct ZuPrint<ZuFixedFmt<Fmt> > : public ZuPrintFn { };
template <class Fmt>
ZuInline ZuFixedFmt<Fmt> ZuFixed::fmt(Fmt) const
{
  return ZuFixedFmt<Fmt>{*this};
}
template <typename S> inline void ZuFixed::print(S &s) const
{
  s << ZuFixedFmt<ZuFmt::Default>{*this};
}
template <> struct ZuPrint<ZuFixed> : public ZuPrintFn { };
template <bool Ref>
struct ZuFixedVFmt : public ZuVFmtWrapper<ZuFixedVFmt<Ref>, Ref> {
  const ZuFixed	&fixed;

  ZuInline ZuFixedVFmt(const ZuFixed &fixed_) :
    fixed{fixed_} { }
  ZuInline ZuFixedVFmt(const ZuFixed &fixed_, ZuVFmt &fmt_) :
    ZuVFmtWrapper<ZuFixedVFmt, Ref>{fmt_}, fixed{fixed_} { }

  template <typename S> inline void print(S &s) const {
    if (ZuUnlikely(!*fixed)) return;
    int64_t i = fixed.i_, f = fixed.f_;
    if (ZuUnlikely(i < 0)) { s << '-'; i = -i, f = -f; }
    s << ZuBoxed(i).vfmt(this->fmt);
    if (f) s << '.' << ZuBoxed(f).fmt(ZuFmt::Frac<18>());
  }
};
ZuInline ZuFixedVFmt<> ZuFixed::vfmt() const
{
  return ZuFixedVFmt<>{*this};
}
ZuInline ZuFixedVFmt<1> ZuFixed::vfmt(ZuVFmt &fmt) const
{
  return ZuFixedVFmt<1>{*this, fmt};
}
template <bool Ref>
struct ZuPrint<ZuFixedVFmt<Ref> > : public ZuPrintFn { };

#endif /* ZuFixed_HPP */
