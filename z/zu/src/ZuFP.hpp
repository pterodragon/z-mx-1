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

// IEEE floating point type traits

#ifndef ZuFP_HPP
#define ZuFP_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <math.h>
#include <float.h>

#ifdef _MSC_VER
#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif
#define isnan _isnan
#endif

#include <zlib/ZuInt.hpp>

// floating point support for all IEEE 754 sizes
// - number of decimal places in each type's mantissa
// - NaN and infinity generators for each type
// - epsilon function for each type

// ZuFP<T>
//
// Bits - number of bits in mantissa
// MinDigits - minimum number of decimal significant figures in mantissa
// MaxDigits - maximum ''
// T inf() - return positive infinity (use -inf() for negative infinity)
// bool inf(T v) - true if v is positive infinite
// T nan() - NaN ("not a number" - null sentinel value)
// bool nan(T v) - true if v is not a number
// T epsilon(T v) - return decimal epsilon of v
//   (this is the worst case range within which values would compare equal
//    if converted into decimal and back again)

template <typename T, unsigned Size = sizeof(T)> struct ZuFP;

template <typename T> struct ZuFP__;
template <> struct ZuFP__<float> {
  ZuInline static float floor_(float f) { return floorf(f); }
  ZuInline static float log10_(float f) { return log10f(f); }
  ZuInline static float frexp_(float f, int *n) { return frexpf(f, n); }
  ZuInline static float ldexp_(float f, int n) { return ldexpf(f, n); }
  ZuInline static float fabs_(float f) { return fabsf(f); }
};
template <> struct ZuFP__<double> {
  ZuInline static double floor_(double f) { return floor(f); }
  ZuInline static double log10_(double f) { return log10(f); }
  ZuInline static double frexp_(double f, int *n) { return frexp(f, n); }
  ZuInline static double ldexp_(double f, int n) { return ldexp(f, n); }
  ZuInline static double fabs_(double f) { return fabs(f); }
};
template <> struct ZuFP__<long double> {
  ZuInline static long double floor_(long double f) { return floorl(f); }
  ZuInline static long double log10_(long double f) { return log10l(f); }
  ZuInline static long double frexp_(long double f, int *n)
    { return frexpl(f, n); }
  ZuInline static long double ldexp_(long double f, int n)
    { return ldexpl(f, n); }
  ZuInline static long double fabs_(long double f) { return fabsl(f); }
};

// CRTP mixin
template <class FP, typename T> struct ZuFP_ : public ZuFP__<T> {
  ZuInline static T inf() { return (T)INFINITY; }
  ZuInline static bool inf(T v) { return v == inf(); }
  ZuInline static T epsilon(T v) {
    if (ZuUnlikely(FP::nan(v))) return v;
    if (ZuUnlikely(FP::inf(v))) return v;
    if (ZuUnlikely(FP::inf(-v))) return -v;
    if (v == (T)0) return v;
    return FP::epsilon_(v);
  }
};

template <typename T>
struct ZuFP<T, 2U> : public ZuFP_<ZuFP<T, 2U>, T> { // 10+5
  enum { Bits = 10, MinDigits = 4, MaxDigits = 4 };
  ZuInline static T nan() {
    static const uint16_t i = ~(uint16_t)0;
    const T *ZuMayAlias(i_) = reinterpret_cast<const T *>(&i);
    return *i_;
  }
  ZuInline static bool nan(T v) { return isnan((double)v); }
  ZuInline static T epsilon_(T v) {
    v = ZuFP::fabs_(v);
    T b = v;
    uint16_t *ZuMayAlias(v_) = reinterpret_cast<uint16_t *>(&v);
    *v_ += 5;
    return v - b;
  }
};
template <typename T>
struct ZuFP<T, 4U> : public ZuFP_<ZuFP<T, 4U>, T> { // 23+8
  enum { Bits = 23, MinDigits = 7, MaxDigits = 8 };
  ZuInline static T nan() {
    static const uint32_t i = ~0;
    const T *ZuMayAlias(i_) = reinterpret_cast<const T *>(&i);
    return *i_;
  }
  ZuInline static bool nan(T v) { return isnan((double)v); }
  ZuInline static T epsilon_(T v) {
    v = ZuFP::fabs_(v);
    T b = v;
    uint32_t *ZuMayAlias(v_) = reinterpret_cast<uint32_t *>(&v);
    *v_ += 5;
    return v - b;
  }
};
template <typename T>
struct ZuFP<T, 8U> : public ZuFP_<ZuFP<T, 8U>, T> { // 52+11
  enum { Bits = 52, MinDigits = 16, MaxDigits = 16 };
  ZuInline static T nan() {
    static const uint64_t i = ~0ULL;
    const T *ZuMayAlias(i_) = reinterpret_cast<const T *>(&i);
    return *i_;
  }
  ZuInline static bool nan(T v) { return isnan((double)v); }
  ZuInline static T epsilon_(T v) {
    v = ZuFP::fabs_(v);
    T b = v;
    uint64_t *ZuMayAlias(v_) = reinterpret_cast<uint64_t *>(&v);
    *v_ += 5;
    return v - b;
  }
};
template <class ZuFP, typename T>
struct ZuFP_64 : public ZuFP_<ZuFP, T> { // 64+15
  enum { Bits = 64, MinDigits = 20, MaxDigits = 20 };
#pragma pack(push, 1)
  struct I_ {
    uint64_t mantissa;
    uint16_t exponent; // actually 15bits, sign bit is MSB
    ZuInline void inc() {
      uint64_t prev = mantissa;
      if (ZuUnlikely((mantissa += 5) < prev))
	(mantissa |= (1ULL<<63U)), ++exponent;
    }
  };
#pragma pack(pop)
  ZuInline static T epsilon_(T v) {
    v = ZuFP::fabs_(v);
    T b = v;
    I_ *ZuMayAlias(v_) = reinterpret_cast<I_ *>(&v);
    v_->inc();
    return v - b;
  }
};
template <typename T>
struct ZuFP<T, 12U> : public ZuFP_64<ZuFP<T, 12U>, T> { // 64+15 (12 bytes)
  ZuInline static T nan() {
    static const struct { uint64_t i64; uint32_t i32; } i = { ~0ULL, ~0 };
    const T *ZuMayAlias(i_) = reinterpret_cast<const T *>(&i);
    return *i_;
  }
  ZuInline static bool nan(T v) { return isnan((double)v); }
};
template <typename T>
struct ZuFP<T, 16U> : public ZuFP_64<ZuFP<T, 16U>, T> { // 64+15 (16 bytes)
  ZuInline static T nan() {
    static const struct { uint64_t i64; uint64_t j64; } i = { ~0ULL, ~0ULL };
    const T *ZuMayAlias(i_) = reinterpret_cast<const T *>(&i);
    return *i_;
  }
  ZuInline static bool nan(T v) { return isnan((double)v); }
};

#endif /* ZuFP_HPP */
