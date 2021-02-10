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

// nanosecond precision time class

#ifndef ZmTime_HPP
#define ZmTime_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuConversion.hpp>

#include <zlib/ZmPlatform.hpp>

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
public:
  enum Now_ { Now };		// disambiguator
  enum Nano_ { Nano };		// ''

  ZmTime() : timespec{0, 0} { }

  ZmTime(const ZmTime &t) : timespec{t.tv_sec, t.tv_nsec} { }
  ZmTime &operator =(const ZmTime &t) {
    if (this == &t) return *this;
    tv_sec = t.tv_sec, tv_nsec = t.tv_nsec;
    return *this;
  }

  template <typename T> struct IsInt {
    enum { OK =
      ZuConversion<int, T>::Same ||
      ZuConversion<unsigned, T>::Same ||
      ZuConversion<long, T>::Same ||
      ZuConversion<time_t, T>::Same };
  };
  template <typename T, typename R = void, bool OK = IsInt<T>::OK>
  struct MatchInt;
  template <typename T_, typename R> struct MatchInt<T_, R, 1> {
    using T = T_;
  };

public:
  ZmTime(Now_) { now(); }
  template <typename T>
  ZmTime(Now_, T i, typename MatchInt<T>::T *_ = 0) {
    now(i);
  }
  ZmTime(Now_, double d) { now(d); }
  ZmTime(Now_, const ZmTime &d) { now(d); }

  template <typename T>
  ZmTime(T v, typename MatchInt<T>::T *_ = 0) :
    timespec{v, 0} { }
  ZmTime(time_t t, long n) : timespec{t, n} { }

  ZmTime(double d) :
    timespec{(time_t)d, (long)((d - (double)(time_t)d) * 1000000000)} { }

  ZmTime(Nano_, int64_t nano) : 
    timespec{(time_t)(nano / 1000000000), (long)(nano % 1000000000)} { }

#ifndef _WIN32
  ZmTime(const timespec &t) : timespec{t.tv_sec, t.tv_nsec} { }
  ZmTime &operator =(const timespec &t) {
    new (this) ZmTime{t};
    return *this;
  }
  ZmTime(const timeval &t) : timespec{t.tv_sec, t.tv_usec * 1000} { }
  ZmTime &operator =(const timeval &t) {
    new (this) ZmTime{t};
    return *this;
  }
#else
  ZmTime(FILETIME f) {
    int64_t *ZuMayAlias(f_) = reinterpret_cast<int64_t *>(&f);
    int64_t t = *f_;
    t -= ZmTime_FT_Epoch;
    tv_sec = t / 10000000;
    tv_nsec = (int32_t)((t % 10000000) * 100);
  }
  ZmTime &operator =(FILETIME t) {
    new (this) ZmTime{t};
    return *this;
  }
#endif

#ifndef _WIN32
  ZmTime &now() {
    clock_gettime(CLOCK_REALTIME, this);
    return *this;
  }
#else
  ZmTime &now();

  static uint64_t cpuFreq();
#endif /* !_WIN32 */

  template <typename T>
  typename MatchInt<T, ZmTime &>::T now(T i) { return now() += i; }
  ZmTime &now(double d) { return now() += d; }
  ZmTime &now(const ZmTime &d) { return now() += d; }

  time_t time() const { return(tv_sec); }
  operator time_t() const { return(tv_sec); }
  double dtime() const {
    return (double)tv_sec + (double)tv_nsec / (double)1000000000;
  }
  int32_t millisecs() const {
    return (int32_t)(tv_sec * 1000 + tv_nsec / 1000000);
  }
  int32_t microsecs() const {
    return (int32_t)(tv_sec * 1000000 + tv_nsec / 1000);
  }
  int64_t nanosecs() const {
    return (int64_t)tv_sec * 1000000000 + (int64_t)tv_nsec;
  }

#ifndef _WIN32
  operator timeval() const {
    return timeval{tv_sec, tv_nsec / 1000};
  }
#else
  operator FILETIME() const {
    FILETIME f;
    int64_t *ZuMayAlias(f_) = reinterpret_cast<int64_t *>(&f);
    *f_ = (int64_t)tv_sec * 10000000 + tv_nsec / 100 + ZmTime_FT_Epoch;
    return f;
  }
#endif

  ZmTime &operator =(time_t t) {
    tv_sec = t;
    tv_nsec = 0;
    return *this;
  }
  ZmTime &operator =(double d) {
    tv_sec = (time_t)d;
    tv_nsec = (long)((d - (double)tv_sec) * (double)1000000000);
    return *this;
  }

  void normalize() {
    if (tv_nsec >= 1000000000) tv_nsec -= 1000000000, ++tv_sec;
    else {
      if (tv_nsec < 0) {
	tv_nsec += 1000000000, --tv_sec;
	if (ZuUnlikely(tv_nsec < 0)) tv_nsec += 1000000000, --tv_sec;
      }
    }
  }

  ZmTime operator -() const {
    return ZmTime{-tv_sec - 1, 1000000000L - tv_nsec};
  }

  template <typename T>
  typename MatchInt<T, ZmTime>::T operator +(T v) const {
    return ZmTime{tv_sec + v, tv_nsec};
  }
  ZmTime operator +(double d) const {
    return ZmTime::operator +(ZmTime{d});
  }
  ZmTime operator +(const ZmTime &t_) const {
    ZmTime t{tv_sec + t_.tv_sec, tv_nsec + t_.tv_nsec};
    t.normalize();
    return t;
  }
  template <typename T>
  typename MatchInt<T, ZmTime &>::T operator +=(T v) {
    tv_sec += v;
    return *this;
  }
  ZmTime &operator +=(double d) {
    return ZmTime::operator +=(ZmTime{d});
  }
  ZmTime &operator +=(const ZmTime &t_) {
    tv_sec += t_.tv_sec, tv_nsec += t_.tv_nsec;
    normalize();
    return *this;
  }
  template <typename T>
  typename MatchInt<T, ZmTime>::T operator -(T v) const {
    return ZmTime{tv_sec - v, tv_nsec};
  }
  ZmTime operator -(double d) const {
    return ZmTime::operator -(ZmTime{d});
  }
  ZmTime operator -(const ZmTime &t_) const {
    ZmTime t{tv_sec - t_.tv_sec, tv_nsec - t_.tv_nsec};
    t.normalize();
    return t;
  }
  template <typename T>
  typename MatchInt<T, ZmTime &>::T operator -=(T v) {
    tv_sec -= v;
    return *this;
  }
  ZmTime &operator -=(double d) {
    return ZmTime::operator -=(ZmTime{d});
  }
  ZmTime &operator -=(const ZmTime &t_) {
    tv_sec -= t_.tv_sec, tv_nsec -= t_.tv_nsec;
    normalize();
    return *this;
  }

  ZmTime operator *(double d) {
    return ZmTime{dtime() * d};
  }
  ZmTime &operator *=(double d) {
    return operator =(dtime() * d);
  }
  ZmTime operator /(double d) {
    return ZmTime{dtime() / d};
  }
  ZmTime &operator /=(double d) {
    return operator =(dtime() / d);
  }

  int operator ==(const ZmTime &t) const {
    return tv_sec == t.tv_sec && tv_nsec == t.tv_nsec;
  }
  int operator !=(const ZmTime &t) const {
    return tv_sec != t.tv_sec || tv_nsec != t.tv_nsec;
  }
  int operator >(const ZmTime &t) const {
    return tv_sec > t.tv_sec || (tv_sec == t.tv_sec && tv_nsec > t.tv_nsec);
  }
  int operator >=(const ZmTime &t) const {
    return tv_sec > t.tv_sec || (tv_sec == t.tv_sec && tv_nsec >= t.tv_nsec);
  }
  int operator <(const ZmTime &t) const {
    return tv_sec < t.tv_sec || (tv_sec == t.tv_sec && tv_nsec < t.tv_nsec);
  }
  int operator <=(const ZmTime &t) const {
    return tv_sec < t.tv_sec || (tv_sec == t.tv_sec && tv_nsec <= t.tv_nsec);
  }

  int operator !() const {
    return !(tv_sec || tv_nsec);
  }
  ZuOpBool

  time_t sec() const { return tv_sec; }
  time_t &sec() { return tv_sec; }
  long nsec() const { return tv_nsec; }
  long &nsec() { return tv_nsec; }

  uint32_t hash() const {
    return ZuHash<time_t>::hash(tv_sec) ^ ZuHash<long>::hash(tv_nsec);
  }

  int cmp(const ZmTime &t) const {
    if (int i = ZuCmp<time_t>::cmp(tv_sec, t.tv_sec)) return i;
    return ZuCmp<long>::cmp(tv_nsec, t.tv_nsec);
  }

  struct Traits : public ZuBaseTraits<ZmTime> { enum { IsPOD = 1 }; };
  friend Traits ZuTraitsType(ZmTime *);
};

inline ZmTime ZmTimeNow() { return ZmTime{ZmTime::Now}; }
template <typename T>
inline typename ZmTime::MatchInt<T, ZmTime>::T ZmTimeNow(T i) {
  return ZmTime{ZmTime::Now, i};
}
inline ZmTime ZmTimeNow(double d) { return ZmTime{ZmTime::Now, d}; }
inline ZmTime ZmTimeNow(const ZmTime &d) { return ZmTime{ZmTime::Now, d}; }

#endif /* ZmTime_HPP */
