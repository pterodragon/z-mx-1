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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// Julian (serial) date based date/time class - JD(UT1) in IAU nomenclature

// * handles dates from Jan 1st 4713BC to Oct 14th 1465002AD
//   Note: 4713BC is -4712 when printed as an ISO8601 date, since
//   historians do not include a 0 year (1BC immediately precedes 1AD)

// * date/times are internally represented as a Julian date with
//   intraday time in POSIX time_t seconds (effectively UT1 seconds)
//   from midnight in the GMT timezone, and a number of nanoseconds

// * intraday timespan of each day is midnight-to-midnight instead
//   of the historical / astronomical noon-to-noon convention for
//   Julian dates

// * year/month/day conversions account for the reformation of
//   15th October 1582 and the subsequent adoption of the Gregorian
//   calendar; the default effective date is 14th September 1752
//   when Britain and the American colonies adopted the Gregorian
//   calendar by Act of Parliament (consistent with Unix cal); this
//   can be changed by calling ZtDate::reformation(year, month, day)

// * UTC time is composed of atomic time seconds which are of fixed
//   duration; leap seconds have been intermittently added to UTC time
//   since 1972 to correct for perturbations in the Earth's rotation;
//   ZtDate is based on POSIX time_t which is UT1 time, defined
//   directly in terms of the Earth's rotation and composed of
//   seconds which have slightly variable duration; all POSIX times
//   driven by typical computer clocks therefore require occasional
//   adjustment due to the difference between UTC and UT1 - see
//   http://www.boulder.nist.gov/timefreq/pubs/bulletin/leapsecond.htm -
//   this is typically done by running an NTP client on the system;
//   when performing calculations involving time values that are exchanged
//   with external systems requiring accuracy to the atomic second (such
//   as astronomical systems, etc.), the caller may need to compensate for
//   the difference between atomic time and POSIX time_t, in particular:
//
//   * when repeatedly adding/subtracting atomic time values to advance or
//     reverse a POSIX time, it is possible to accumulate more than one
//     second of error due to the accumulated difference between UTC and
//     UT1; to compensate, the caller should add/remove a leap second when
//     the POSIX time crosses the midnight that a leap second was applied
//     (typically applied by NTP software)
//
//   * when calculating the elapsed atomic time difference between two
//     absolute POSIX times, leap seconds that were applied to UTC for the
//     intervening period should be added/removed

// * conversion functions accept an arbitrary intraday offset in seconds
//   for adjustment between GMT and another time zone; for local time
//   caller should use the value returned by offset() which works for
//   the entire date range supported by ZtDate and performs DST and
//   other adjustments according to the default timezone in effect or
//   the timezone specified by the caller; timezone names are the same as
//   the values that can be assigned to the TZ environment variable:
//
//     ZtDate date;		// date/time
//
//     date.now();
//
//     ZtDateFmt::ISO gmt;			// GMT/UTC
//     ZtDateFmt::ISO local(date.offset());	// default local time (tzset())
//     ZtDateFmt::ISO gb(date.offset("GB"));	// local time in GB
//     ZtDateFmt::ISO est(date.offset("EST"));	// local time in EST
//
//     puts(date.iso(gmt));	// print time in ISO8601 format
//
// * WARNING - functions with this annotation call tzset() if the tz argument
//   is non-zero; these functions should not be called with a high frequency
//   by high performance applications with a non-zero tz since they:
//   - acquire and release a global lock, since tzset() is not thread-safe
//   - are potentially time-consuming
//   - may access and modify global environment variables
//   - may access system configuration files and external timezone databases

#ifndef ZtDate_HPP
#define ZtDate_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <ZtLib.hpp>
#endif

#include <time.h>
#include <limits.h>

#include <ZuIfT.hpp>
#include <ZuTraits.hpp>
#include <ZuString.hpp>
#include <ZuConversion.hpp>
#include <ZuTuple.hpp>
#include <ZuString.hpp>
#include <ZuStringN.hpp>
#include <ZuPrint.hpp>
#include <ZuBox.hpp>

#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmTime.hpp>
#include <ZmSpecific.hpp>

#include <ZtString.hpp>

#define ZtDate_NullJulian	INT_MIN
#define ZtDate_MaxJulian	(0x1fffffff - 68572)

class ZtDate;

template <int size> class ZtDate_time_t;
template <> class ZtDate_time_t<4> {
friend class ZtDate;
private:
  ZuInline static constexpr int32_t minimum() { return -0x80000000; } 
  ZuInline static constexpr int32_t maximum() { return 0x7fffffff; } 
public:
  ZuInline static bool isMinimum(int32_t t) { return t == minimum(); }
  ZuInline static bool isMaximum(int32_t t) { return t == maximum(); }
  inline static int32_t time(int julian, int second) {
    if (julian == ZtDate_NullJulian) return 0;
    if (julian < 2415732) {
      return minimum();
    } else if (julian == 2415732) {
      if (second < 74752) {
	return minimum();
      }
      julian++, second -= 86400;
    } else if (julian > 2465443 || (julian == 2465443 && second > 11647)) {
      return maximum();
    }
    return (julian - 2440588) * 86400 + second;
  }
};
template <> class ZtDate_time_t<8> {
public:
  ZuInline static constexpr bool isMinimum(int64_t t) { return false; }
  ZuInline static constexpr bool isMaximum(int64_t t) { return false; }
  ZuInline static int64_t time(int julian, int second) {
    if (julian == ZtDate_NullJulian) return 0;
    return ((int64_t)julian - (int64_t)2440588) * (int64_t)86400 +
      (int64_t)second;
  }
};

namespace ZtDateFmt {
  template <unsigned Width, char Trim> struct FIX_ {
    template <typename S> ZuInline static void fracPrint(S &s, unsigned nsec) {
      s << '.' << ZuBoxed(nsec).fmt(ZuFmt::Frac<Width, Trim>());
    }
  };
  template <unsigned Width> struct FIX_<Width, '\0'> {
    template <typename S> ZuInline static void fracPrint(S &s, unsigned nsec) {
      if (ZuLikely(nsec))
	s << '.' << ZuBoxed(nsec).fmt(ZuFmt::Frac<Width, '\0'>());
    }
  };
  template <char Trim> struct FIX_<0, Trim> {
    template <typename S> ZuInline static void fracPrint(S &, unsigned) { }
  };
  template <int Prec_, class Null_>
  class FIX : public FIX_<Prec_ < 0 ? -Prec_ : Prec_, Prec_ < 0 ? '\0' : '0'> {
  friend class ::ZtDate;
  public:
    enum { Prec = Prec_ };
    typedef Null_ Null;

    ZuAssert(Prec >= -9 && Prec <= 9);

    ZuInline FIX() : m_ptr(0), m_julian(0), m_sec(0) {
      memcpy(m_yyyymmdd, "00010101", 8);
      memcpy(m_hhmmss, "00:00:00", 8);
    }

    ZuInline void ptr(const ZtDate *p) { m_ptr = p; }
    ZuInline const ZtDate *ptr() const { return m_ptr; }

  private:
    const ZtDate	*m_ptr;

    mutable int		m_julian;
    mutable int		m_sec;
    mutable char	m_yyyymmdd[8];
    mutable char	m_hhmmss[8];
  };
  struct CSV {
  friend class ::ZtDate;
  public:
    ZuInline CSV() : m_ptr(0), m_offset(0), m_pad(0), m_julian(0), m_sec(0) {
      memcpy(m_yyyymmdd, "0001/01/01", 10);
      memcpy(m_hhmmss, "00:00:00", 8);
    }

    ZuInline void ptr(const ZtDate *p) { m_ptr = p; }
    ZuInline const ZtDate *ptr() const { return m_ptr; }

    ZuInline void offset(int o) { m_offset = o; }
    ZuInline int offset() const { return m_offset; }

    ZuInline void pad(char c) { m_pad = c; }
    ZuInline char pad() const { return m_pad; }

  private:
    const ZtDate	*m_ptr;
    int			m_offset;
    char		m_pad;

    mutable int		m_julian;
    mutable int		m_sec;
    mutable char	m_yyyymmdd[10];
    mutable char	m_hhmmss[8];
  };
  struct ISO {
  friend class ::ZtDate;
  public:
    ZuInline ISO(int offset = 0) :
	m_ptr(0), m_offset(offset), m_julian(0), m_sec(0) {
      memcpy(m_yyyymmdd, "0001-01-01", 10);
      memcpy(m_hhmmss, "00:00:00", 8);
    }

    ZuInline void ptr(const ZtDate *p) { m_ptr = p; }
    ZuInline const ZtDate *ptr() const { return m_ptr; }

    ZuInline void offset(int o) { m_offset = o; }
    ZuInline int offset() const { return m_offset; }

  private:
    const ZtDate	*m_ptr;
    int			m_offset;

    mutable int		m_julian;
    mutable int		m_sec;
    mutable char	m_yyyymmdd[10];
    mutable char	m_hhmmss[8];
  };
  struct Strftime {
    inline Strftime(
	const ZtDate &date_, const char *format_, int offset_) :
      date(date_), format(format_), offset(offset_) { }

    const ZtDate	&date;
    const char		*format;
    int			offset;
  };
};

template <> struct ZuTraits<ZtDate> : public ZuGenericTraits<ZtDate> {
  enum { IsPOD = 1, IsHashable = 1, IsComparable = 1 };
};

class ZtAPI ZtDate {
  typedef ZtDate_time_t<sizeof(time_t)> Native;

public:
  template <int Prec, class Null>
  using FIXFmt = ZtDateFmt::FIX<Prec, Null>;
  typedef ZtDateFmt::CSV CSVFmt;
  typedef ZtDateFmt::ISO ISOFmt;
  typedef ZtDateFmt::Strftime Strftime;

  enum Now_ { Now };		// disambiguator
  enum Julian_ { Julian };	// ''

  enum FIX_ { FIX };		// ''
  enum CSV_ { CSV };		// ''

  inline ZtDate() : m_julian(ZtDate_NullJulian), m_sec(0), m_nsec(0) { }

  inline ZtDate(const ZtDate &date) :
    m_julian(date.m_julian), m_sec(date.m_sec), m_nsec(date.m_nsec) { }

  inline ZtDate &operator =(const ZtDate &date) {
    if (this == &date) return *this;
    m_julian = date.m_julian;
    m_sec = date.m_sec;
    m_nsec = date.m_nsec;
    return *this;
  }

  inline ZtDate(Now_ _) { now(); }

  // ZmTime

  template <typename T>
  inline ZtDate(const T &t, typename ZuSame<ZmTime, T>::T *_ = 0) {
    init(t.sec()), m_nsec = t.nsec();
  }
  template <typename T>
  inline typename ZuSame<ZmTime, T, ZtDate &>::T operator =(const T &t) {
    init(t.sec());
    m_nsec = t.nsec();
    return *this;
  }

  // time_t

  template <typename T>
  inline ZtDate(const T &t, typename ZuSame<time_t, T>::T *_ = 0) {
    init(t), m_nsec = 0;
  }
  template <typename T>
  inline typename ZuSame<time_t, T, ZtDate &>::T operator =(const T &t) {
    init(t);
    m_nsec = 0;
    return *this;
  }

  // double

  template <typename T>
  inline ZtDate(T d, typename ZuSame<double, T>::T *_ = 0) {
    time_t t = (time_t)d;
    init(t);
    m_nsec = (int)((d - (double)t) * (double)1000000000);
  }
  template <typename T>
  inline typename ZuSame<double, T, ZtDate &>::T operator =(T d) {
    time_t t = (time_t)d;
    init(t);
    m_nsec = (int)((d - (double)t) * (double)1000000000);
    return *this;
  }

  // struct tm

  inline ZtDate(const struct tm *tm_) {
    int year = tm_->tm_year + 1900, month = tm_->tm_mon + 1, day = tm_->tm_mday;
    int hour = tm_->tm_hour, minute = tm_->tm_min, sec = tm_->tm_sec;
    int nsec = 0;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate &operator =(const struct tm *tm_) {
    new (this) ZtDate(tm_);
    return *this;
  }

  // multiple parameter constructors

  inline ZtDate(time_t t, int nsec) { init(t); m_nsec = nsec; }

  inline explicit ZtDate(Julian_ _, int julian, int sec, int nsec) :
    m_julian(julian), m_sec(sec), m_nsec(nsec) { }

  inline ZtDate(int year, int month, int day) {
    normalize(year, month);
    ctor(year, month, day);
  }
  inline ZtDate(int year, int month, int day, int hour, int minute, int sec) {
    normalize(year, month);
    int nsec = 0;
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(
      int year, int month, int day, int hour, int minute, int sec, int nsec) {
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(int hour, int minute, int sec, int nsec) {
    ctor(hour, minute, sec, nsec);
  }

  // the tz parameter in the following ctors can be 0 for GMT, "" for the
  // local timezone (as obtained by tzset()), or a specific timezone
  // name - the passed in date/time is converted from the date/time
  // in the specified timezone to the internal GMT representation

  inline ZtDate(struct tm *tm_, const char *tz) { // WARNING
    int year = tm_->tm_year + 1900, month = tm_->tm_mon + 1, day = tm_->tm_mday;
    int hour = tm_->tm_hour, minute = tm_->tm_min, sec = tm_->tm_sec;
    int nsec = 0;

    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(int year, int month, int day, const char *tz) { // WARNING
    normalize(year, month);
    ctor(year, month, day);
    if (tz) offset_(year, month, day, 0, 0, 0, tz);
  }
  inline ZtDate(
      int year, int month, int day,
      int hour, int minute, int sec, const char *tz) { // WARNING
    normalize(year, month);
    int nsec = 0;
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(
      int year, int month, int day,
      int hour, int minute, int sec, int nsec, const char *tz) { // WARNING
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(
      int hour, int minute, int sec, int nsec, const char *tz) { // WARNING
    ctor(hour, minute, sec, nsec);
    if (tz) {
      int year, month, day;
      ymd(year, month, day);
      offset_(year, month, day, hour, minute, sec, tz);
    }
  }

  // FIX format: YYYYMMDD-HH:MM:SS.nnnnnnnnn

  template <typename S>
  inline ZtDate(FIX_ _, const S &s, typename ZuIsString<S>::T *__ = 0) {
    ctorFIX(s);
  }

  // CSV format: YYYY/MM/DD HH:MM:SS with an optional timezone parameter

  template <typename S>
  inline ZtDate(CSV_ _, const S &s, typename ZuIsString<S>::T *__ = 0) {
    ctorCSV(s, 0, 0);
  }
  template <typename S, typename TZ>
  inline ZtDate(CSV_ _, const S &s, const TZ &tz, typename ZuIfT<
      ZuTraits<S>::IsString && ZuTraits<TZ>::IsCString>::T *__ = 0) {
    ctorCSV(s, tz, 0);
  }
  template <typename S, typename TZ>
  inline ZtDate(CSV_ _, const S &s, const TZ &tzOffset, typename ZuIfT<
      ZuTraits<S>::IsString && ZuConversion<int, TZ>::Same>::T *__ = 0) {
    ctorCSV(s, 0, tzOffset);
  }

  // the ISO8601 ctor accepts the two standard ISO8601 date/time formats
  // "yyyy-mm-dd" and "yyyy-mm-ddThh:mm:ss[.n]Z", where Z is an optional
  // timezone: "Z" (GMT), "+hhmm", "+hh:mm", "-hhmm", or "-hh:mm";
  // if a timezone is present in the string it overrides any tz
  // parameter passed in by the caller

  template <typename S>
  inline ZtDate(const S &s, typename ZuIsString<S>::T *_ = 0) {
    ctorISO(s, 0);
  }
  template <typename S, typename TZ>
  inline ZtDate(const S &s, const TZ &tz, typename ZuIfT<
      ZuTraits<S>::IsString && ZuTraits<TZ>::IsCString>::T *_ = 0) {
    ctorISO(s, tz);
  }

  // common integral forms

  enum YYYYMMDD_ { YYYYMMDD };
  enum YYMMDD_ { YYMMDD };
  enum HHMMSSmmm_ { HHMMSSmmm };
  enum HHMMSS_ { HHMMSS };

  inline ZtDate(int year, int month, int day, HHMMSSmmm_, int time) {
    int hour, minute, sec, nsec;
    hour = time / 10000000; minute = (time / 100000) % 100;
    sec = (time / 1000) % 100; nsec = (time % 1000) * 1000000;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(int year, int month, int day, HHMMSS_, int time) {
    int hour, minute, sec, nsec = 0;
    hour = time / 10000; minute = (time / 100) % 100; sec = time % 100;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(YYYYMMDD_, int date, HHMMSSmmm_, int time) {
    int year, month, day, hour, minute, sec, nsec;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000000; minute = (time / 100000) % 100;
    sec = (time / 1000) % 100; nsec = (time % 1000) * 1000000;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(YYYYMMDD_, int date, HHMMSS_, int time) {
    int year, month, day, hour, minute, sec, nsec = 0;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000; minute = (time / 100) % 100; sec = time % 100;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(YYMMDD_, int date, HHMMSSmmm_, int time) {
    int year, month, day, hour, minute, sec, nsec;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000000; minute = (time / 100000) % 100;
    sec = (time / 1000) % 100; nsec = (time % 1000) * 1000000;
    year += (year < 70) ? 2000 : 1900;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(YYMMDD_, int date, HHMMSS_, int time) {
    int year, month, day, hour, minute, sec, nsec = 0;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000; minute = (time / 100) % 100; sec = time % 100;
    year += (year < 70) ? 2000 : 1900;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
  }
  inline ZtDate(
      YYYYMMDD_, int date, HHMMSSmmm_, int time, const char *tz) { // WARNING
    int year, month, day, hour, minute, sec, nsec;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000000; minute = (time / 100000) % 100;
    sec = (time / 1000) % 100; nsec = (time % 1000) * 1000000;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(
      YYYYMMDD_, int date, HHMMSS_, int time, const char *tz) { // WARNING
    int year, month, day, hour, minute, sec, nsec = 0;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000; minute = (time / 100) % 100; sec = time % 100;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(
      YYMMDD_, int date, HHMMSSmmm_, int time, const char *tz) { // WARNING
    int year, month, day, hour, minute, sec, nsec;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000000; minute = (time / 100000) % 100;
    sec = (time / 1000) % 100; nsec = (time % 1000) * 1000000;
    year += (year < 70) ? 2000 : 1900;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }
  inline ZtDate(
      YYMMDD_, int date, HHMMSS_, int time, const char *tz) { // WARNING
    int year, month, day, hour, minute, sec, nsec = 0;
    year = date / 10000; month = (date / 100) % 100; day = date % 100;
    hour = time / 10000; minute = (time / 100) % 100; sec = time % 100;
    year += (year < 70) ? 2000 : 1900;
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    ctor(year, month, day, hour, minute, sec, nsec);
    if (tz) offset_(year, month, day, hour, minute, sec, tz);
  }

  inline int yyyymmdd() const {
    int year, month, day;
    ymd(year, month, day);
    year *= 10000;
    if (year >= 0) return year + month * 100 + day;
    return year - month * 100 - day;
  }
  inline int yymmdd() const {
    int year, month, day;
    ymd(year, month, day);
    year = (year % 100) * 10000;
    if (year >= 0) return year + month * 100 + day;
    return year - month * 100 - day;
  }
  inline int hhmmssmmm() const {
    int hour, minute, sec, nsec;
    hmsn(hour, minute, sec, nsec);
    return hour * 10000000 + minute * 100000 + sec * 1000 + nsec / 1000000;
  }
  inline int hhmmss() const {
    int hour, minute, sec;
    hms(hour, minute, sec);
    return hour * 10000 + minute * 100 + sec;
  }

// accessors for external persistency providers

  inline const int &julian() const { return m_julian; }
  inline int &julian() { return m_julian; }
  inline const int &sec() const { return m_sec; }
  inline int &sec() { return m_sec; }
  inline const int &nsec() const { return m_nsec; }
  inline int &nsec() { return m_nsec; }

// set effective date of reformation (default 14th September 1752)

  // reformation() is purposely not MT safe since MT safety is an
  // unlikely requirement for a function only intended to be called
  // once during program initialization, MT safety would degrade performance
  // considerably by lock acquisition on every conversion

  static void reformation(int year, int month, int day);

// conversions

  // time(), dtime() and zmTime() can result in an out of range conversion
  // to time_t

  inline operator time_t() { return this->time(); }
  ZuInline time_t time() const {
    return (time_t)Native::time(m_julian, m_sec);
  }
  inline double dtime() const {
    if (ZuUnlikely(m_julian == ZtDate_NullJulian)) return ZuCmp<double>::null();
    time_t t = Native::time(m_julian, m_sec);
    if (ZuUnlikely(Native::isMinimum(t))) return -ZuCmp<double>::inf();
    if (ZuUnlikely(Native::isMaximum(t))) return ZuCmp<double>::inf();
    return((double)t + (double)m_nsec / (double)1000000000);
  }
  inline ZmTime zmTime() const {
    if (ZuUnlikely(m_julian == ZtDate_NullJulian)) return ZmTime();
    return ZmTime(this->time(), m_nsec);
  }
  inline operator ZmTime() { return zmTime(); }

  struct tm *tm(struct tm *tm) const;

  void ymd(int &year, int &month, int &day) const;
  void hms(int &hour, int &minute, int &sec) const;
  void hmsn(int &hour, int &minute, int &sec, int &nsec) const;

  // number of days relative to a base date
  inline int days(int year, int month, int day) const {
    return m_julian - julian(year, month, day);
  }
  // days arg below should be the value of days(year, 1, 1, offset)
  // week (0-53) wkDay (1-7) 1st Monday in year is 1st day of week 1
  void ywd(
      int year, int days, int &week, int &wkDay) const;
  // week (0-53) wkDay (1-7) 1st Sunday in year is 1st day of week 1
  void ywdSun(
      int year, int days, int &week, int &wkDay) const;
  // week (1-53) wkDay (1-7) 1st Thursday in year is 4th day of week 1
  void ywdISO(
      int year, int days, int &wkYear, int &week, int &wkDay) const;

  static ZuString dayShortName(int i); // 1-7
  static ZuString dayLongName(int i); // 1-7
  static ZuString monthShortName(int i); // 1-12
  static ZuString monthLongName(int i); // 1-12

  template <int Prec, class Null>
  ZuInline const FIXFmt<Prec, Null> &fix(FIXFmt<Prec, Null> &fmt) const {
    fmt.ptr(this);
    return fmt;
  }
  template <typename S_, typename FIXFmt>
  inline void fixPrint(S_ &s, const FIXFmt &fmt) const {
    if (!*this) { s << typename FIXFmt::Null(); return; }
    if (ZuUnlikely(m_julian != fmt.m_julian)) {
      fmt.m_julian = m_julian;
      int y, m, d;
      ymd(y, m, d);
      if (ZuUnlikely(y < 1)) y = 1;
      else if (ZuUnlikely(y > 9999)) y = 9999;
      fmt.m_yyyymmdd[0] = y / 1000 + '0';
      fmt.m_yyyymmdd[1] = (y / 100) % 10 + '0';
      fmt.m_yyyymmdd[2] = (y / 10) % 10 + '0';
      fmt.m_yyyymmdd[3] = y % 10 + '0';
      fmt.m_yyyymmdd[4] = m / 10 + '0';
      fmt.m_yyyymmdd[5] = m % 10 + '0';
      fmt.m_yyyymmdd[6] = d / 10 + '0';
      fmt.m_yyyymmdd[7] = d % 10 + '0';
    }
    s << ZuString(fmt.m_yyyymmdd, 8) << '-';
    if (ZuUnlikely(m_sec != fmt.m_sec)) {
      fmt.m_sec = m_sec;
      int H, M, S;
      hms(H, M, S);
      fmt.m_hhmmss[0] = H / 10 + '0';
      fmt.m_hhmmss[1] = H % 10 + '0';
      fmt.m_hhmmss[3] = M / 10 + '0';
      fmt.m_hhmmss[4] = M % 10 + '0';
      fmt.m_hhmmss[6] = S / 10 + '0';
      fmt.m_hhmmss[7] = S % 10 + '0';
    }
    s << ZuString(fmt.m_hhmmss, 8);
    fmt.fracPrint(s, m_nsec);
  }

  ZuInline const CSVFmt &csv(CSVFmt &fmt) {
    fmt.ptr(this);
    return fmt;
  }
  template <typename S_>
  inline void csvPrint(S_ &s, const CSVFmt &fmt) const {
    if (!*this) return;
    ZtDate date = *this + fmt.m_offset;
    if (ZuUnlikely(date.m_julian != fmt.m_julian)) {
      fmt.m_julian = date.m_julian;
      int y, m, d;
      date.ymd(y, m, d);
      if (ZuUnlikely(y < 1)) { s << '-'; y = 1 - y; }
      else if (ZuUnlikely(y > 9999)) y = 9999;
      fmt.m_yyyymmdd[0] = y / 1000 + '0';
      fmt.m_yyyymmdd[1] = (y / 100) % 10 + '0';
      fmt.m_yyyymmdd[2] = (y / 10) % 10 + '0';
      fmt.m_yyyymmdd[3] = y % 10 + '0';
      fmt.m_yyyymmdd[5] = m / 10 + '0';
      fmt.m_yyyymmdd[6] = m % 10 + '0';
      fmt.m_yyyymmdd[8] = d / 10 + '0';
      fmt.m_yyyymmdd[9] = d % 10 + '0';
    }
    s << ZuString(fmt.m_yyyymmdd, 10) << ' ';
    if (ZuUnlikely(date.m_sec != fmt.m_sec)) {
      fmt.m_sec = date.m_sec;
      int H, M, S;
      hms(H, M, S);
      fmt.m_hhmmss[0] = H / 10 + '0';
      fmt.m_hhmmss[1] = H % 10 + '0';
      fmt.m_hhmmss[3] = M / 10 + '0';
      fmt.m_hhmmss[4] = M % 10 + '0';
      fmt.m_hhmmss[6] = S / 10 + '0';
      fmt.m_hhmmss[7] = S % 10 + '0';
    }
    s << ZuString(fmt.m_hhmmss, 8);
    if (unsigned N = date.m_nsec) {
      char buf[9];
      if (fmt.m_pad) {
	Zu_ntoa::Base10_print_frac(N, 9, fmt.m_pad, buf);
	s << '.' << ZuString(buf, 9);
      } else {
	if (N = Zu_ntoa::Base10_print_frac_truncate(N, 9, buf))
	  s << '.' << ZuString(buf, N);
      }
    }
  }

  // iso() always generates a full date/time format parsable by the ctor
  ZuInline const ISOFmt &iso(ISOFmt &fmt) const {
    fmt.ptr(this);
    return fmt;
  }
  template <typename S_>
  inline void isoPrint(S_ &s, const ISOFmt &fmt) const {
    if (!*this) return;
    ZtDate date = *this + fmt.m_offset;
    if (ZuUnlikely(date.m_julian != fmt.m_julian)) {
      fmt.m_julian = date.m_julian;
      int y, m, d;
      date.ymd(y, m, d);
      if (ZuUnlikely(y < 1)) { s << '-'; y = 1 - y; }
      else if (ZuUnlikely(y > 9999)) y = 9999;
      fmt.m_yyyymmdd[0] = y / 1000 + '0';
      fmt.m_yyyymmdd[1] = (y / 100) % 10 + '0';
      fmt.m_yyyymmdd[2] = (y / 10) % 10 + '0';
      fmt.m_yyyymmdd[3] = y % 10 + '0';
      fmt.m_yyyymmdd[5] = m / 10 + '0';
      fmt.m_yyyymmdd[6] = m % 10 + '0';
      fmt.m_yyyymmdd[8] = d / 10 + '0';
      fmt.m_yyyymmdd[9] = d % 10 + '0';
    }
    s << ZuString(fmt.m_yyyymmdd, 10) << 'T';
    if (ZuUnlikely(date.m_sec != fmt.m_sec)) {
      fmt.m_sec = date.m_sec;
      int H, M, S;
      hms(H, M, S);
      fmt.m_hhmmss[0] = H / 10 + '0';
      fmt.m_hhmmss[1] = H % 10 + '0';
      fmt.m_hhmmss[3] = M / 10 + '0';
      fmt.m_hhmmss[4] = M % 10 + '0';
      fmt.m_hhmmss[6] = S / 10 + '0';
      fmt.m_hhmmss[7] = S % 10 + '0';
    }
    s << ZuString(fmt.m_hhmmss, 8);
    if (unsigned N = date.m_nsec) {
      char buf[9];
      N = Zu_ntoa::Base10_print_frac_truncate(N, 9, buf);
      if (N) s << '.' << ZuString(buf, N);
    }
    if (fmt.m_offset) {
      int offset_ = (fmt.m_offset < 0) ? -fmt.m_offset : fmt.m_offset;
      int oH = offset_ / 3600, oM = (offset_ % 3600) / 60;
      char buf[5];
      buf[0] = oH / 10 + '0';
      buf[1] = oH % 10 + '0';
      buf[2] = ':';
      buf[3] = oM / 10 + '0';
      buf[4] = oM % 10 + '0';
      s << ((fmt.m_offset < 0) ? '-' : '+') << ZuString(buf, 5);
    } else
      s << 'Z';
  }

  // this strftime(3) is neither system timezone- nor locale- dependent;
  // timezone is specified by the (optional) offset parameter, output is
  // equivalent to the C library strftime under the 'C' locale; it does
  // not call tzset() and is thread-safe; conforms to, variously:
  //   (C90) - ANSI C '90
  //   (C99) - ANSI C '99
  //   (SU) - Single Unix Specification
  //   (MS) - Microsoft CRT
  //   (GNU) - glibc (not all glibc-specific extensions are supported)
  //   (TZ) - Arthur Olson's timezone library
  // the following conversion specifiers are supported:
  //   %# (MS) alt. format
  //   %E (SU) alt. format
  //   %O (SU) alt. digits (has no effect)
  //   %0-9 (GNU) field width (precision) specifier
  //   %a (C90) day of week - short name
  //   %A (C90) day of week - long name
  //   %b (C90) month - short name
  //   %h (SU) '' 
  //   %B (C90) month - long name
  //   %c (C90) Unix asctime() / ctime() (%a %b %e %T %Y)
  //   %C (SU) century
  //   %d (C90) day of month
  //   %x (C90) %m/%d/%y
  //   %D (SU) ''
  //   %e (SU) day of month - space padded
  //   %F (C99) %Y-%m-%d per ISO 8601
  //   %g (TZ) ISO week date year (2 digits)
  //   %G (TZ) '' (4 digits)
  //   %H (C90) hour (24hr)
  //   %I (C90) hour (12hr)
  //   %j (C90) day of year
  //   %m (C90) month
  //   %M (C90) minute
  //   %n (SU) newline
  //   %p (C90) AM/PM
  //   %P (GNU) am/pm
  //   %r (SU) %I:%M:%S %p
  //   %R (SU) %H:%M
  //   %s (TZ) number of seconds since the Epoch
  //   %S (C90) second
  //   %t (SU) TAB
  //   %X (C90) %H:%M:%S
  //   %T (SU) ''
  //   %u (SU) week day as decimal (1-7), 1 is Monday (7 is Sunday)
  //   %U (C90) week (00-53), 1st Sunday in year is 1st day of week 1
  //   %V (SU) week (01-53), per ISO week date
  //   %w (C90) week day as decimal (0-6), 0 is Sunday
  //   %W (C90) week (00-53), 1st Monday in year is 1st day of week 1
  //   %y (C90) year (2 digits)
  //   %Y (C90) year (4 digits)
  //   %z (GNU) RFC 822 timezone offset
  //   %Z (C90) timezone
  //   %% (C90) percent sign
  inline Strftime strftime(const char *format, int offset = 0) const {
    return Strftime(*this, format, offset);
  }

  int offset(const char *tz = 0) const; // WARNING

// operators

  // ZtDate is an absolute time, not a time interval, so the difference
  // between two ZtDates is a ZmTime;
  // for the same reason no operator +(ZtDate) is defined
  ZuInline ZmTime operator -(const ZtDate &date) const {
    int day = m_julian - date.m_julian;
    int sec = m_sec - date.m_sec;
    int nsec = m_nsec - date.m_nsec;

    if (nsec < 0) nsec += 1000000000, --sec;
    if (sec < 0) sec += 86400, --day;

    return ZmTime(day * 86400 + sec, nsec);
  }

  template <typename T>
  ZuInline typename ZuSame<ZmTime, T, ZtDate>::T operator +(const T &t) const {
    int julian, sec, nsec;

    sec = m_sec;
    nsec = m_nsec + t.nsec();
    if (nsec < 0)
      nsec += 1000000000, --sec;
    else if (nsec >= 1000000000)
      nsec -= 1000000000, ++sec;

    {
      int sec_ = t.sec();

      if (sec_ < 0) {
        sec_ = -sec_;
	julian = m_julian - sec_ / 86400;
	sec -= sec_ % 86400;

	if (sec < 0) sec += 86400, --julian;
      } else {
	julian = m_julian + sec_ / 86400;
	sec += sec_ % 86400;

	if (sec >= 86400) sec -= 86400, ++julian;
      }
    }

    return ZtDate(Julian, julian, sec, nsec);
  }
  template <typename T> inline typename ZuIfT<
    ZuConversion<time_t, T>::Same ||
    ZuConversion<long, T>::Same ||
    ZuConversion<int, T>::Same, ZtDate>::T operator +(T sec_) const {
    int julian, sec = sec_;

    if (sec < 0) {
      sec = -sec;
      if (ZuLikely(sec < 86400))
	julian = m_julian, sec = m_sec - sec;
      else
	julian = m_julian - (int)(sec / 86400), sec = m_sec - sec % 86400;

      if (sec < 0) sec += 86400, --julian;
    } else {
      if (ZuLikely(sec < 86400))
	julian = m_julian, sec = m_sec + sec;
      else
	julian = m_julian + (int)(sec / 86400), sec = m_sec + sec % 86400;

      if (sec >= 86400) sec -= 86400, ++julian;
    }

    return ZtDate(Julian, julian, sec, m_nsec);
  }

  template <typename T>
  inline typename ZuSame<ZmTime, T, ZtDate &>::T operator +=(const T &t) {
    int julian, sec, nsec;

    sec = m_sec;
    nsec = m_nsec + t.nsec();
    if (nsec < 0)
      nsec += 1000000000, --sec;
    else if (nsec >= 1000000000)
      nsec -= 1000000000, ++sec;

    {
      int sec_ = t.sec();

      if (sec_ < 0) {
        sec_ = -sec_;
	julian = m_julian - sec_ / 86400;
	sec -= sec_ % 86400;

	if (sec < 0) sec += 86400, --julian;
      } else {
	julian = m_julian + sec_ / 86400;
	sec += sec_ % 86400;

	if (sec >= 86400) sec -= 86400, ++julian;
      }
    }

    m_julian = julian;
    m_sec = sec;
    m_nsec = nsec;

    return *this;
  }
  template <typename T> inline typename ZuIfT<
    ZuConversion<time_t, T>::Same ||
    ZuConversion<long, T>::Same ||
    ZuConversion<int, T>::Same, ZtDate &>::T operator +=(T sec_) {
    int julian, sec = sec_;

    if (sec < 0) {
      sec = -sec;
      if (ZuLikely(sec < 86400))
	julian = m_julian, sec = m_sec - sec;
      else
	julian = m_julian - (int)(sec / 86400), sec = m_sec - sec % 86400;

      if (sec < 0) sec += 86400, --julian;
    } else {
      if (ZuLikely(sec < 86400))
	julian = m_julian, sec = m_sec + sec;
      else
	julian = m_julian + (int)(sec / 86400), sec = m_sec + sec % 86400;

      if (sec >= 86400) sec -= 86400, ++julian;
    }

    m_julian = julian;
    m_sec = sec;

    return *this;
  }

  template <typename T>
  ZuInline typename ZuSame<ZmTime, T, ZtDate>::T operator -(const T &t) const {
    return ZtDate::operator +(-t);
  }
  template <typename T> ZuInline typename ZuIfT<
    ZuConversion<time_t, T>::Same ||
    ZuConversion<long, T>::Same ||
    ZuConversion<int, T>::Same, ZtDate>::T operator -(T sec_) {
    return ZtDate::operator +(-sec_);
  }
  template <typename T>
  ZuInline typename ZuSame<ZmTime, T, ZtDate &>::T operator -=(const T &t) {
    return ZtDate::operator +=(-t);
  }
  template <typename T> ZuInline typename ZuIfT<
    ZuConversion<time_t, T>::Same ||
    ZuConversion<long, T>::Same ||
    ZuConversion<int, T>::Same, ZtDate &>::T operator -=(T sec_) {
    return ZtDate::operator +=(-sec_);
  }

  inline int operator ==(const ZtDate &date) const {
    return m_julian == date.m_julian &&
      m_sec == date.m_sec && m_nsec == date.m_nsec;
  }
  inline int operator !=(const ZtDate &date) const {
    return m_julian != date.m_julian ||
      m_sec != date.m_sec || m_nsec != date.m_nsec;
  }

  inline int operator >(const ZtDate &date) const {
    return m_julian > date.m_julian ||
      (m_julian == date.m_julian && (m_sec > date.m_sec ||
      (m_sec == date.m_sec && m_nsec > date.m_nsec)));
  }
  inline int operator >=(const ZtDate &date) const {
    return m_julian > date.m_julian ||
      (m_julian == date.m_julian && (m_sec > date.m_sec ||
      (m_sec == date.m_sec && m_nsec >= date.m_nsec)));
  }
  inline int operator <(const ZtDate &date) const {
    return m_julian < date.m_julian ||
      (m_julian == date.m_julian && (m_sec < date.m_sec ||
      (m_sec == date.m_sec && m_nsec < date.m_nsec)));
  }
  inline int operator <=(const ZtDate &date) const {
    return m_julian < date.m_julian ||
      (m_julian == date.m_julian && (m_sec < date.m_sec ||
      (m_sec == date.m_sec && m_nsec <= date.m_nsec)));
  }

  inline bool operator !() const {
    return (int)m_julian == (int)ZtDate_NullJulian;
  }
  ZuOpBool

// utility functions

  inline ZtDate &now() {
    ZmTime t(ZmTime::Now);

    init(t.sec());
    m_nsec = t.nsec();
    return *this;
  }

  inline uint32_t hash() const { return m_julian ^ m_sec ^ m_nsec; }

  inline int cmp(const ZtDate &date) const {
    return
      m_julian > date.m_julian ? 1 : m_julian < date.m_julian ? -1 :
      m_sec    > date.m_sec    ? 1 : m_sec    < date.m_sec    ? -1 :
      m_nsec   > date.m_nsec   ? 1 : m_nsec   < date.m_nsec   ? -1 : 0;
  }

private:
  inline void ctor(int year, int month, int day) {
    normalize(year, month);
    m_julian = julian(year, month, day);
    m_sec = 0, m_nsec = 0;
  }
  inline void ctor(int year, int month, int day,
		   int hour, int minute, int sec, int nsec) {
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    m_julian = julian(year, month, day);
    m_sec = second(hour, minute, sec);
    m_nsec = nsec;
  }
  inline void ctor(int hour, int minute, int sec, int nsec) {
    m_julian = (int32_t)((ZmTime(ZmTime::Now).sec() / 86400) + 2440588);
    {
      int day = 0;
      normalize(day, hour, minute, sec, nsec);
      m_julian += day;
    }
    m_sec = second(hour, minute, sec);
    m_nsec = nsec;
  }

private:
  // parse fixed-width integers
  template <unsigned Width> struct Int {
    template <typename T, typename S>
    ZuInline static unsigned scan(T &v, S &s) {
      unsigned i = v.scan(ZuFmt::Right<Width>(), s);
      if (ZuLikely(i > 0)) s.offset(i);
      return i;
    }
  };
  // parse nanoseconds
  template <typename T, typename S>
  inline static unsigned scanFrac(T &v, S &s) {
    unsigned i = v.scan(ZuFmt::Frac<9>(), s);
    if (ZuLikely(i > 0)) s.offset(i);
    return i;
  }

public:
  void ctorISO(ZuString s, const char *tz);
  void ctorFIX(ZuString s);
  void ctorCSV(ZuString s, const char *tz, int tzOffset);

  void normalize(unsigned &year, unsigned &month);
  void normalize(int &year, int &month);

  void normalize(unsigned &day, unsigned &hour, unsigned &minute,
      unsigned &sec, unsigned &nsec);
  void normalize(int &day, int &hour, int &minute, int &sec, int &nsec);

  static int julian(int year, int month, int day);

  ZuInline static int second(int hour, int minute, int second) {
    return hour * 3600 + minute * 60 + second;
  }

  void offset_(int year, int month, int day,
	       int hour, int minute, int second, const char *tz);

  ZuInline void init(time_t t) {
    m_julian = (int32_t)((t / 86400) + 2440588);
    m_sec = (int32_t)(t % 86400);
    m_nsec = 0;
  }

  int32_t	m_julian;	// julian day
  int32_t	m_sec;		// time within day, in seconds, from midnight
  int32_t	m_nsec;		// nanoseconds

// effective date of the reformation (default 14th September 1752)

  static int		m_reformationJulian;
  static int		m_reformationYear;
  static int		m_reformationMonth;
  static int		m_reformationDay;
};

inline ZtDate ZtDateNow() { return ZtDate(ZtDate::Now); }

template <int Prec, class Null>
struct ZuPrint<ZtDateFmt::FIX<Prec, Null> > : public ZuPrintDelegate {
  template <typename S>
  ZuInline static void print(S &s, const ZtDateFmt::FIX<Prec, Null> &fmt) {
    fmt.ptr()->fixPrint(s, fmt);
  }
};

template <> struct ZuPrint<ZtDateFmt::CSV> : public ZuPrintDelegate {
  template <typename S>
  ZuInline static void print(S &s, const ZtDateFmt::CSV &fmt) {
    fmt.ptr()->csvPrint(s, fmt);
  }
};

template <> struct ZuPrint<ZtDateFmt::ISO> : public ZuPrintDelegate {
  template <typename S>
  ZuInline static void print(S &s, const ZtDateFmt::ISO &fmt) {
    fmt.ptr()->isoPrint(s, fmt);
  }
};

template <> struct ZuPrint<ZtDateFmt::Strftime> : public ZuPrintDelegate {
private:
  template <typename Boxed>
  inline static auto vfmt(ZuVFmt &fmt,
      const Boxed &boxed, unsigned width, bool alt) {
    if (alt)
      fmt.reset();
    else
      fmt.right(width);
    return boxed.vfmt(fmt);
  }
  template <typename Boxed>
  inline static auto vfmt(ZuVFmt &fmt,
      const Boxed &boxed, ZuBox<unsigned> width, int deflt, bool alt) {
    if (alt)
      fmt.reset();
    else {
      if (!*width) width = deflt;
      fmt.right(width);
    }
    return boxed.vfmt(fmt);
  }
  template <typename Boxed>
  inline static auto vfmt(ZuVFmt &fmt,
      const Boxed &boxed, ZuBox<unsigned> width, int deflt, bool alt,
      char pad) {
    if (alt)
      fmt.reset();
    else {
      if (!*width) width = deflt;
      fmt.right(width, pad);
    }
    return boxed.vfmt(fmt);
  }

public:
  template <typename S_>
  inline static void print(S_ &s, const ZtDateFmt::Strftime &v) {
    const char *format = v.format;

    if (!format || !*format) return;

    ZtDate date = v.date + v.offset;

    ZuBox<int> year, month, day, hour, minute, second;
    ZuBox<int> days, week, wkDay, hour12;
    ZuBox<int> wkYearISO, weekISO, weekSun;
    ZuBox<time_t> seconds;

    // Conforming to, variously:
    // (C90) - ANSI C '90
    // (C99) - ANSI C '99
    // (SU) - Single Unix Specification
    // (MS) - Microsoft CRT
    // (GNU) - glibc (not all glibc-specific extensions are supported)
    // (TZ) - Arthur Olson's timezone library
   
    ZuVFmt fmt;

    while (char c = *format++) {
      if (c != '%') { s << c; continue; }
      bool alt = false;
      ZuBox<unsigned> width;
  fmtchar:
      c = *format++;
      switch (c) {
	case '#': // (MS) alt. format
	case 'E': // (SU) ''
	  alt = true;
	case 'O': // (SU) alt. digits
	  goto fmtchar;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': // (GNU) field width specifier
	  format += width.scan(format);
	  goto fmtchar;
	case 'a': // (C90) day of week - short name
	  if (!*wkDay) wkDay = date.julian() % 7 + 1;
	  s << ZtDate::dayShortName(wkDay);
	  break;
	case 'A': // (C90) day of week - long name
	  if (!*wkDay) wkDay = date.julian() % 7 + 1;
	  s << ZtDate::dayLongName(wkDay);
	  break;
	case 'b': // (C90) month - short name
	case 'h': // (SU) '' 
	  if (!*month) date.ymd(year, month, day);
	  s << ZtDate::monthShortName(month);
	  break;
	case 'B': // (C90) month - long name
	  if (!*month) date.ymd(year, month, day);
	  s << ZtDate::monthLongName(month);
	  break;
	case 'c': // (C90) Unix asctime() / ctime() (%a %b %e %T %Y)
	  if (!*year) date.ymd(year, month, day);
	  if (!*hour) date.hms(hour, minute, second);
	  if (!*wkDay) wkDay = date.julian() % 7 + 1;
	  s << ZtDate::dayShortName(wkDay) << ' ' <<
	    ZtDate::monthShortName(month) << ' ' <<
	    vfmt(fmt, day, 2, alt) << ' ' <<
	    vfmt(fmt, hour, 2, alt) << ':' <<
	    vfmt(fmt, minute, 2, alt) << ':' <<
	    vfmt(fmt, second, 2, alt) << ' ' <<
	    vfmt(fmt, year, 4, alt);
	  break;
	case 'C': // (SU) century
	  if (!*year) date.ymd(year, month, day);
	  {
	    ZuBox<int> century = year / 100;
	    s << vfmt(fmt, century, width, 2, alt);
	  }
	  break;
	case 'd': // (C90) day of month
	  if (!*day) date.ymd(year, month, day);
	  s << vfmt(fmt, day, width, 2, alt);
	  break;
	case 'x': // (C90) %m/%d/%y
	case 'D': // (SU) ''
	  if (!*year) date.ymd(year, month, day);
	  {
	    ZuBox<int> year_ = year % 100;
	    s << vfmt(fmt, month, 2, alt) << '/' <<
	      vfmt(fmt, day, 2, alt) << '/' <<
	      vfmt(fmt, year_, 2, alt);
	  }
	  break;
	case 'e': // (SU) day of month - space padded
	  if (!*year) date.ymd(year, month, day);
	  s << vfmt(fmt, day, width, 2, alt, ' ');
	  break;
	case 'F': // (C99) %Y-%m-%d per ISO 8601
	  if (!*year) date.ymd(year, month, day);
	  s << vfmt(fmt, year, 4, alt) << '-' <<
	    vfmt(fmt, month, 2, alt) << '-' <<
	    vfmt(fmt, day, 2, alt);
	  break;
	case 'g': // (TZ) ISO week date year (2 digits)
	case 'G': // (TZ) '' (4 digits)
	  if (!*wkYearISO) {
	    if (!*year) date.ymd(year, month, day);
	    if (!*days) days = date.days(year, 1, 1);
	    date.ywdISO(year, days, wkYearISO, weekISO, wkDay);
	  }
	  {
	    ZuBox<int> wkYearISO_ = wkYearISO;
	    if (c == 'g') wkYearISO_ %= 100;
	    s << vfmt(fmt, wkYearISO_, width, (c == 'g' ? 2 : 4), alt);
	  }
	  break;
	case 'H': // (C90) hour (24hr)
	  if (!*hour) date.hms(hour, minute, second);
	  s << vfmt(fmt, hour, width, 2, alt);
	  break;
	case 'I': // (C90) hour (12hr)
	  if (!*hour12) {
	    if (!*hour) date.hms(hour, minute, second);
	    hour12 = hour % 12;
	    if (!hour12) hour12 += 12;
	  }
	  s << vfmt(fmt, hour12, width, 2, alt);
	  break;
	case 'j': // (C90) day of year
	  if (!*year) date.ymd(year, month, day);
	  if (!*days) days = date.days(year, 1, 1);
	  {
	    ZuBox<int> days_ = days + 1;
	    s << vfmt(fmt, days_, width, 3, alt);
	  }
	  break;
	case 'm': // (C90) month
	  if (!*month) date.ymd(year, month, day);
	  s << vfmt(fmt, month, width, 2, alt);
	  break;
	case 'M': // (C90) minute
	  if (!*minute) date.hms(hour, minute, second);
	  s << vfmt(fmt, minute, width, 2, alt);
	  break;
	case 'n': // (SU) newline
	  s << '\n';
	  break;
	case 'p': // (C90) AM/PM
	  if (!*hour) date.hms(hour, minute, second);
	  s << (hour >= 12 ? "PM" : "AM");
	  break;
	case 'P': // (GNU) am/pm
	  if (!*hour) date.hms(hour, minute, second);
	  s << (hour >= 12 ? "pm" : "am");
	  break;
	case 'r': // (SU) %I:%M:%S %p
	  if (!*hour) date.hms(hour, minute, second);
	  if (!*hour12) {
	    hour12 = hour % 12;
	    if (!hour12) hour12 += 12;
	  }
	  s << vfmt(fmt, hour12, 2, alt) << ':' <<
	    vfmt(fmt, minute, 2, alt) << ':' <<
	    vfmt(fmt, second, 2, alt) << ' ' <<
	    (hour >= 12 ? "PM" : "AM");
	  break;
	case 'R': // (SU) %H:%M
	  if (!*hour) date.hms(hour, minute, second);
	  s << vfmt(fmt, hour, 2, alt) << ':' <<
	    vfmt(fmt, minute, 2, alt);
	  break;
	case 's': // (TZ) number of seconds since the Epoch
	  if (!*seconds) seconds = date.time();
	  if (!alt && *width)
	    s << seconds.vfmt().right(width);
	  else
	    s << seconds;
	  break;
	case 'S': // (C90) second
	  if (!*second) date.hms(hour, minute, second);
	  s << vfmt(fmt, second, width, 2, alt);
	  break;
	case 't': // (SU) TAB
	  s << '\t';
	  break;
	case 'X': // (C90) %H:%M:%S
	case 'T': // (SU) ''
	  if (!*hour) date.hms(hour, minute, second);
	  s << vfmt(fmt, hour, 2, alt) << ':' <<
	    vfmt(fmt, minute, 2, alt) << ':' <<
	    vfmt(fmt, second, 2, alt);
	  break;
	case 'u': // (SU) week day as decimal (1-7), 1 is Monday (7 is Sunday)
	  if (!*wkDay) wkDay = date.julian() % 7 + 1;
	  s << vfmt(fmt, wkDay, width, 1, alt);
	  break;
	case 'U': // (C90) week (00-53), 1st Sunday in year is 1st day of week 1
	  if (!*weekSun) {
	    if (!*year) date.ymd(year, month, day);
	    if (!*days) days = date.days(year, 1, 1);
	    {
	      int wkDay_;
	      date.ywdSun(year, days, weekSun, wkDay_);
	    }
	  }
	  s << vfmt(fmt, weekSun, width, 2, alt);
	  break;
	case 'V': // (SU) week (01-53), per ISO week date
	  if (!*weekISO) {
	    if (!*year) date.ymd(year, month, day);
	    if (!*days) days = date.days(year, 1, 1);
	    date.ywdISO(year, days, wkYearISO, weekISO, wkDay);
	  }
	  s << vfmt(fmt, weekISO, width, 2, alt);
	  break;
	case 'w': // (C90) week day as decimal, 0 is Sunday
	  if (!*wkDay) wkDay = date.julian() % 7 + 1;
	  {
	    ZuBox<int> wkDay_ = wkDay;
	    if (wkDay_ == 7) wkDay_ = 0;
	    s << vfmt(fmt, wkDay_, width, 1, alt);
	  }
	  break;
	case 'W': // (C90) week (00-53), 1st Monday in year is 1st day of week 1
	  if (!*week) {
	    if (!*year) date.ymd(year, month, day);
	    if (!*days) days = date.days(year, 1, 1);
	    date.ywd(year, days, week, wkDay);
	  }
	  s << vfmt(fmt, week, width, 2, alt);
	  break;
	case 'y': // (C90) year (2 digits)
	  if (!*year) date.ymd(year, month, day);
	  {
	    ZuBox<int> year_ = year % 100;
	    s << vfmt(fmt, year_, width, 2, alt);
	  }
	  break;
	case 'Y': // (C90) year (4 digits)
	  if (!*year) date.ymd(year, month, day);
	  s << vfmt(fmt, year, width, 4, alt);
	  break;
	case 'z': // (GNU) RFC 822 timezone offset
	case 'Z': // (C90) timezone
	  {
	    ZuBox<int> offset_ = v.offset;
	    if (offset_ < 0) { s += '-'; offset_ = -offset_; }
	    offset_ = (offset_ / 3600) * 100 + (offset_ % 3600) / 60;
	    s << offset_;
	  }
	  break;
	case '%':
	  s << '%';
	  break;
      }
    }
  }
};

#endif /* ZtDate_HPP */
