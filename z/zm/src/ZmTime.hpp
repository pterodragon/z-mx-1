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

// nanosecond precision time class

#ifndef ZmTime_HPP
#define ZmTime_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuInt.hpp>
#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuConversion.hpp>

#include <ZmPlatform.hpp>

#include <time.h>

#ifdef _WIN32
#define ZmTime_FT_Epoch	0x019db1ded53e8000ULL	// 00:00:00 Jan 1 1970

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
  time_t  tv_sec;
  long    tv_nsec;
};
#endif
#endif

class ZmAPI ZmTime : public timespec {
  struct Private { };

public:
  enum Now_ { Now };		// disambiguator
  enum Nano_ { Nano };		// ''

  ZuInline ZmTime() : timespec{0, 0} { }

  ZuInline ZmTime(const ZmTime &t) : timespec{t.tv_sec, t.tv_nsec} { }
  ZuInline ZmTime &operator =(const ZmTime &t) {
    if (this == &t) return *this;
    tv_sec = t.tv_sec, tv_nsec = t.tv_nsec;
    return *this;
  }

  template <typename T> struct MatchInt_ {
    enum { OK =
      ZuConversion<int, T>::Same ||
      ZuConversion<unsigned, T>::Same ||
      ZuConversion<long, T>::Same ||
      ZuConversion<time_t, T>::Same };
  };
  template <typename T, typename R = void, bool OK = MatchInt_<T>::OK>
  struct MatchInt;
  template <typename T_, typename R> struct MatchInt<T_, R, 1> {
    typedef T_ T;
  };

  ZuInline ZmTime(Now_) { now(); }
  template <typename T>
  ZuInline ZmTime(Now_, T i, typename MatchInt<T, Private>::T *_ = 0) {
    now(i);
  }
  ZuInline ZmTime(Now_, double d) { now(d); }

  template <typename T>
  ZuInline ZmTime(T v, typename MatchInt<T, Private>::T *_ = 0) :
    timespec{v, 0} { }
  ZuInline ZmTime(time_t t, long n) : timespec{t, n} { }

  ZuInline ZmTime(double d) :
    timespec{(time_t)d, (long)((d - tv_sec) * 1000000000)} { }

  ZuInline ZmTime(Nano_, int64_t nano) : 
    timespec{(time_t)(nano / 1000000000), (long)(nano % 1000000000)} { }

#ifndef _WIN32
  ZuInline ZmTime(const timespec &t) : timespec{t.tv_sec, t.tv_nsec} { }
  ZuInline ZmTime &operator =(const timespec &t) {
    new (this) ZmTime(t);
    return *this;
  }
  ZuInline ZmTime(const timeval &t) : timespec{t.tv_sec, t.tv_usec * 1000} { }
  ZuInline ZmTime &operator =(const timeval &t) {
    new (this) ZmTime(t);
    return *this;
  }
#else
  ZuInline ZmTime(FILETIME f) {
    int64_t *ZuMayAlias(f_) = (int64_t *)&f;
    int64_t t = *f_;
    t -= ZmTime_FT_Epoch;
    tv_sec = t / 10000000;
    tv_nsec = (int32_t)((t % 10000000) * 100);
  }
  ZuInline ZmTime &operator =(FILETIME t) {
    new (this) ZmTime(t);
    return *this;
  }
#endif

  static void calibrate(int n);
#ifndef _WIN32
  ZuInline ZmTime &now() {
    clock_gettime(CLOCK_REALTIME, this);
    return *this;
  }
#else
  int64_t now_();
  ZuInline ZmTime &now() {
    int64_t t = now_();
    tv_sec = t / 10000000;
    tv_nsec = (int32_t)((t % 10000000) * 100);
    return *this;
  }
#endif /* !_WIN32 */

  template <typename T>
  ZuInline typename MatchInt<T, ZmTime &>::T now(T i) { return now() += i; }
  ZuInline ZmTime &now(double d) { return now() += d; }

  ZuInline time_t time() const { return(tv_sec); }
  ZuInline operator time_t() const { return(tv_sec); }
  ZuInline double dtime() const {
    return (double)tv_sec + (double)tv_nsec / (double)1000000000;
  }
  ZuInline int32_t millisecs() const {
    return (int32_t)(tv_sec * 1000 + tv_nsec / 1000000);
  }
  ZuInline int32_t microsecs() const {
    return (int32_t)(tv_sec * 1000000 + tv_nsec / 1000);
  }
  ZuInline int64_t nanosecs() const {
    return (int64_t)tv_sec * 1000000000 + (int64_t)tv_nsec;
  }

#ifndef _WIN32
  ZuInline operator timeval() const {
    return timeval{tv_sec, tv_nsec / 1000};
  }
#else
  ZuInline operator FILETIME() const {
    FILETIME f;
    int64_t *ZuMayAlias(f_) = (int64_t *)&f;
    *f_ = (int64_t)tv_sec * 10000000 + tv_nsec / 100 + ZmTime_FT_Epoch;
    return f;
  }
#endif

  inline ZmTime &operator =(time_t t) {
    tv_sec = t;
    tv_nsec = 0;
    return *this;
  }
  inline ZmTime &operator =(double d) {
    tv_sec = (time_t)d;
    tv_nsec = (long)((d - (double)tv_sec) * (double)1000000000);
    return *this;
  }

  ZuInline void normalize() {
    if (tv_nsec >= 1000000000) tv_nsec -= 1000000000, ++tv_sec;
    else {
      if (tv_nsec < 0) {
	tv_nsec += 1000000000, --tv_sec;
	if (ZuUnlikely(tv_nsec < 0)) tv_nsec += 1000000000, --tv_sec;
      }
    }
  }

  ZuInline ZmTime operator -() const {
    return ZmTime{-tv_sec - 1, 1000000000L - tv_nsec};
  }

  template <typename T>
  ZuInline typename MatchInt<T, ZmTime>::T operator +(T v) const {
    return ZmTime(tv_sec + v, tv_nsec);
  }
  ZuInline ZmTime operator +(double d) const {
    return ZmTime::operator +(ZmTime(d));
  }
  ZuInline ZmTime operator +(const ZmTime &t_) const {
    ZmTime t{tv_sec + t_.tv_sec, tv_nsec + t_.tv_nsec};
    t.normalize();
    return t;
  }
  template <typename T>
  ZuInline typename MatchInt<T, ZmTime &>::T operator +=(T v) {
    tv_sec += v;
    return *this;
  }
  ZuInline ZmTime &operator +=(double d) {
    return ZmTime::operator +=(ZmTime(d));
  }
  ZuInline ZmTime &operator +=(const ZmTime &t_) {
    tv_sec += t_.tv_sec, tv_nsec += t_.tv_nsec;
    normalize();
    return *this;
  }
  template <typename T>
  ZuInline typename MatchInt<T, ZmTime>::T operator -(T v) const {
    return ZmTime(tv_sec - v, tv_nsec);
  }
  ZuInline ZmTime operator -(double d) const {
    return ZmTime::operator -(ZmTime(d));
  }
  ZuInline ZmTime operator -(const ZmTime &t_) const {
    ZmTime t{tv_sec - t_.tv_sec, tv_nsec - t_.tv_nsec};
    t.normalize();
    return t;
  }
  template <typename T>
  ZuInline typename MatchInt<T, ZmTime &>::T operator -=(T v) {
    tv_sec -= v;
    return *this;
  }
  ZuInline ZmTime &operator -=(double d) {
    return ZmTime::operator -=(ZmTime(d));
  }
  ZuInline ZmTime &operator -=(const ZmTime &t_) {
    tv_sec -= t_.tv_sec, tv_nsec -= t_.tv_nsec;
    normalize();
    return *this;
  }

  ZuInline ZmTime operator *(double d) {
    return ZmTime(dtime() * d);
  }
  ZuInline ZmTime &operator *=(double d) {
    return operator =(dtime() * d);
  }
  ZuInline ZmTime operator /(double d) {
    return ZmTime(dtime() / d);
  }
  ZuInline ZmTime &operator /=(double d) {
    return operator =(dtime() / d);
  }

  ZuInline int operator ==(const ZmTime &t) const {
    return tv_sec == t.tv_sec && tv_nsec == t.tv_nsec;
  }
  ZuInline int operator !=(const ZmTime &t) const {
    return tv_sec != t.tv_sec || tv_nsec != t.tv_nsec;
  }
  ZuInline int operator >(const ZmTime &t) const {
    return tv_sec > t.tv_sec || (tv_sec == t.tv_sec && tv_nsec > t.tv_nsec);
  }
  ZuInline int operator >=(const ZmTime &t) const {
    return tv_sec > t.tv_sec || (tv_sec == t.tv_sec && tv_nsec >= t.tv_nsec);
  }
  ZuInline int operator <(const ZmTime &t) const {
    return tv_sec < t.tv_sec || (tv_sec == t.tv_sec && tv_nsec < t.tv_nsec);
  }
  ZuInline int operator <=(const ZmTime &t) const {
    return tv_sec < t.tv_sec || (tv_sec == t.tv_sec && tv_nsec <= t.tv_nsec);
  }
  ZuInline int operator !() const {
    return !(tv_sec || tv_nsec);
  }

  ZuInline time_t sec() const { return tv_sec; }
  ZuInline time_t &sec() { return tv_sec; }
  ZuInline long nsec() const { return tv_nsec; }
  ZuInline long &nsec() { return tv_nsec; }

  ZuInline uint32_t hash() const {
    return ZuHash<time_t>::hash(tv_sec) ^ ZuHash<long>::hash(tv_nsec);
  }

  ZuInline int cmp(const ZmTime &t) const {
    if (int i = ZuCmp<time_t>::cmp(tv_sec, t.tv_sec)) return i;
    return ZuCmp<long>::cmp(tv_nsec, t.tv_nsec);
  }
};

ZuInline ZmTime ZmTimeNow() { return ZmTime(ZmTime::Now); }
template <typename T>
ZuInline typename ZmTime::MatchInt<T, ZmTime>::T ZmTimeNow(T i) {
  return ZmTime(ZmTime::Now, i);
}
ZuInline ZmTime ZmTimeNow(double d) { return ZmTime(ZmTime::Now, d); }

template <> struct ZuTraits<ZmTime> : public ZuGenericTraits<ZmTime> {
  enum { IsPOD = 1, IsHashable = 1, IsComparable = 1 };
};

#endif /* ZmTime_HPP */
