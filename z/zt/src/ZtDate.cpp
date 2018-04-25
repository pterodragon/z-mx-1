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

// Julian day based date/time class

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include <ZuBox.hpp>

#include <ZmSingleton.hpp>

#include <ZtPlatform.hpp>

#include <ZtDate.hpp>

int ZtDate::m_reformationYear = 1752;
int ZtDate::m_reformationMonth = 9;
int ZtDate::m_reformationDay = 14;
int ZtDate::m_reformationJulian = 2361222;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

void ZtDate::reformation(int year, int month, int day)
{
  m_reformationJulian = 0;

  m_reformationYear = 0, m_reformationMonth = 0, m_reformationDay = 0;

  ZtDate r(year, month, day);

  m_reformationJulian = r.julian();

  r.ymd(m_reformationYear, m_reformationMonth, m_reformationDay);
}

struct tm *ZtDate::tm(struct tm *tm_) const
{
  memset(tm_, 0, sizeof(struct tm));

  ymd(tm_->tm_year, tm_->tm_mon, tm_->tm_mday);
  tm_->tm_year -= 1900, tm_->tm_mon--;
  hms(tm_->tm_hour, tm_->tm_min, tm_->tm_sec);

  return tm_;
}


void ZtDate::ymd(int &year, int &month, int &day) const
{
  if (ZuLikely(m_julian >= m_reformationJulian)) {
    int i, j, l, n;

    l = julian() + 68569;
    n = (l<<2) / 146097;
    l = l - ((146097 * n + 3)>>2);
    i = (4000 * (l + 1)) / 1461001;
    l = l - ((1461 * i)>>2) + 31;
    j = (80 * l) / 2447;
    day = l - (2447 * j) / 80;
    l = j / 11;
    month = j + 2 - 12 * l;
    year = (100 * (n - 49) + i + l);
  } else {
    int i, j, k, l, n;

    j = julian() + 1402;
    k = (j - 1) / 1461;
    l = j - 1461 * k;
    n = (l - 1) / 365 - l / 1461;
    i = l - 365 * n + 30;
    j = (80 * i) / 2447;
    day = i - (2447 * j) / 80;
    i = j / 11;
    month = j + 2 - 12 * i;
    year = (k<<2) + n + i - 4716;
  }
}

void ZtDate::hms(int &hour, int &minute, int &sec) const
{
  int sec_ = m_sec;

  hour = sec_ / 3600, sec_ %= 3600,
  minute = sec_ / 60, sec = sec_ % 60;
}

void ZtDate::hmsn(int &hour, int &minute, int &sec, int &nsec) const
{
  hms(hour, minute, sec);
  nsec = m_nsec;
}

// week (0-53) wkDay (1-7)
// 1st Monday in year is 1st day of week 1
void ZtDate::ywd(int year, int days, int &week, int &wkDay) const
{
  int wkDay_ = julian() % 7;
  if (wkDay_ < 0) wkDay_ += 7;
  wkDay = wkDay_ + 1;
  week = days < wkDay_ ? 0 : ((days - wkDay_) / 7 + 1);
}

// week (0-53) wkDay (1-7)
// 1st Sunday in year is 1st day of week 1
void ZtDate::ywdSun(int year, int days, int &week, int &wkDay) const
{
  int wkDay_ = (julian() + 1) % 7;
  if (wkDay_ < 0) wkDay_ += 7;
  wkDay = wkDay_ + 1;
  week = days < wkDay_ ? 0 : ((days - wkDay_) / 7 + 1);
}

// week (1-53) wkDay (1-7)
// 1st Thursday in year is 4th day of week 1
void ZtDate::ywdISO(
    int year, int days, int &wkYear, int &week, int &wkDay) const
{
  int wkDay_ = julian() % 7;
  if (wkDay_ < 0) wkDay_ += 7;
  wkDay = wkDay_ + 1;
  int days_;
  if (days < wkDay_ - 3)
    days_ = this->days(wkYear = year - 1, 1, 1);
  else
    days_ = days, wkYear = year;
  week = ((days_ - wkDay_) + 3) / 7 + 1;
}

ZuString ZtDate::dayShortName(int i)
{
  static const char *s[] =
    { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  if (--i < 0 || i >= 7) return ZuString("???", 3);
  return ZuString(s[i], 3);
}

ZuString ZtDate::dayLongName(int i)
{
  static const char *s[] =
    { "Monday", "Tuesday", "Wednesday", "Thursday",
      "Friday", "Saturday", "Sunday" };
  static uint8_t l[] = { 6, 7, 9, 8, 6, 8, 6 };
  if (--i < 0 || i >= 7) return ZuString("???", 3);
  return ZuString(s[i], l[i]);
}

ZuString ZtDate::monthShortName(int i)
{
  static const char *s[] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
      "Oct", "Nov", "Dec" };
  if (--i < 0 || i >= 12) return ZuString("???", 3);
  return ZuString(s[i], 3);
}

ZuString ZtDate::monthLongName(int i)
{
  static const char *s[] =
    { "January", "February", "March", "April", "May", "June", "July",
      "August", "September", "October", "November", "December" };
  static uint8_t l[] = { 7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8  };
  if (--i < 0 || i >= 12) return ZuString("???", 3);
  return ZuString(s[i], l[i]);
}

class ZtDate_TzLock : public ZmLock { };

template <> struct ZmCleanup<ZtDate_TzLock> {
  enum { Level = ZmCleanupLevel::Platform };
};

class ZtDate_TzGuard : public ZmGuard<ZmLock> {
public:
  ZtDate_TzGuard(const char *tz);
  ~ZtDate_TzGuard();

private:
  char	*m_tz;
  char	*m_oldTz;
};

ZtDate_TzGuard::ZtDate_TzGuard(const char *tz) :
  ZmGuard<ZmLock>(*ZmSingleton<ZtDate_TzLock>::instance()),
  m_tz(0),
  m_oldTz(0)
{
  if (tz) {
    if (m_oldTz = getenv("TZ")) m_oldTz -= 3; // potentially non-portable

    m_tz = (char *)malloc(strlen(tz) + 4);
    ZmAssert(m_tz);
    if (!m_tz) throw std::bad_alloc();
    strcpy(m_tz, "TZ=");
    strcpy(m_tz + 3, tz);

    ZtPlatform::putenv(m_tz);
  }

  ZtPlatform::tzset();
}

ZtDate_TzGuard::~ZtDate_TzGuard()
{
  if (m_tz) {
    if (m_oldTz)
      ZtPlatform::putenv(m_oldTz);
    else
      ZtPlatform::putenv("TZ=");
    free(m_tz);

    ZtPlatform::tzset();
  }
}

int ZtDate::offset(const char *tz) const
{
  if (!tz) return 0;
  time_t t = Native::time(m_julian, m_sec);
  {
    ZtDate_TzGuard tzGuard(tz);
    if (ZuUnlikely(Native::isMinimum(t) || Native::isMaximum(t)))
      return -timezone;
    struct tm *tm_ = localtime(&t);
    time_t t_ = Native::time(
	  julian(tm_->tm_year + 1900, tm_->tm_mon + 1, tm_->tm_mday),
	  second(tm_->tm_hour, tm_->tm_min, tm_->tm_sec));
    if (ZuUnlikely(Native::isMinimum(t_) || Native::isMaximum(t_)))
      return -timezone;
    return (int)(t_ - t);
  }
}

int ZtDate::julian(int year, int month, int day)
{
  if (year > m_reformationYear ||
	(year == m_reformationYear &&
	    (month > m_reformationMonth ||
		(month == m_reformationMonth && day >= m_reformationDay)))) {
    int o = (month <= 2 ? -1 : 0);

    return ((1461 * (year + 4800 + o))>>2) +
      (367 * (month - 2 - 12 * o)) / 12 -
      ((3 * ((year + 4900 + o) / 100))>>2) +
      day - 32075;
  } else {
    return 367 * year - ((7 * (year + 5001 + (month - 9) / 7))>>2) +
      (275 * month) / 9 + day + 1729777;
  }
}

#if 0
void ZtDate::offset_(int offset)
{
  if ((m_sec += offset) < 0)
    m_sec += 86400, --m_julian;
  else if (m_sec >= 86400)
    m_sec -= 86400, ++m_julian;
}
#endif

void ZtDate::offset_(
    int year, int month, int day,
    int hour, int minute, int second, const char *tz)
{
  if (!tz) return;

  ZtDate_TzGuard tzGuard(tz);

  if (year < 1900) {
    *this += timezone;
  } else {
    struct tm tm_;

    memset(&tm_, 0, sizeof(struct tm));
    tm_.tm_year = year - 1900;
    tm_.tm_mon = month - 1;
    tm_.tm_mday = day;
    tm_.tm_hour = hour;
    tm_.tm_min = minute;
    tm_.tm_sec = second;
    tm_.tm_isdst = -1;

    time_t t = mktime(&tm_);

    if (t < 0)
      *this += timezone;
    else
      *this += (int)(t - this->time());
  }
}

void ZtDate::normalize(unsigned &year, unsigned &month)
{
  if (month < 1) {
    year -= (12 - month) / 12;
    month = 12 - ((12 - month) % 12);
    return;
  }

  if (month > 12) {
    year += (month - 1) / 12;
    month = ((month - 1) % 12) + 1;
    return;
  }
}
void ZtDate::normalize(int &year, int &month)
{
  if (month < 1) {
    year -= (12 - month) / 12;
    month = 12 - ((12 - month) % 12);
    return;
  }

  if (month > 12) {
    year += (month - 1) / 12;
    month = ((month - 1) % 12) + 1;
    return;
  }
}

void ZtDate::normalize(unsigned &day, unsigned &hour, unsigned &minute,
      unsigned &sec, unsigned &nsec)
{
  if (nsec > 999999999) {
    sec += nsec / 1000000000;
    nsec = nsec % 1000000000;
  }

  if (sec > 59) {
    minute += sec / 60;
    sec = sec % 60;
  }

  if (minute > 59) {
    hour += minute / 60;
    minute = minute % 60;
  }

  if (hour > 23) {
    day += hour / 24;
    hour = hour % 24;
  }
}

void ZtDate::normalize(int &day, int &hour, int &minute, int &sec, int &nsec)
{
  if (nsec < 0) {
    sec -= (999999999 - nsec) / 1000000000;
    nsec = 999999999 - ((999999999 - nsec) % 1000000000);
  } else if (nsec > 999999999) {
    sec += nsec / 1000000000;
    nsec = nsec % 1000000000;
  }

  if (sec < 0) {
    minute -= (59 - sec) / 60;
    sec = 59 - ((59 - sec) % 60);
  } else if (sec > 59) {
    minute += sec / 60;
    sec = sec % 60;
  }

  if (minute < 0) {
    hour -= (59 - minute) / 60;
    minute = 59 - ((59 - minute) % 60);
  } else if (minute > 59) {
    hour += minute / 60;
    minute = minute % 60;
  }

  if (hour < 0) {
    day -= (23 - hour) / 24;
    hour = 23 - ((23 - hour) % 24);
  } else if (hour > 23) {
    day += hour / 24;
    hour = hour % 24;
  }
}

void ZtDate::ctorISO(ZuString s, const char *tz)
{
  {
    unsigned year, month, day, hour, minute, sec, nsec;
    const char *ptr = s.data();
    const char *end = ptr + s.length();
    unsigned c;
    bool bc = 0;

  bc:
    if (ZuUnlikely(end - ptr < 10)) goto invalid;
    c = *ptr++;
    if (ZuUnlikely(c == '-')) { bc = 1; goto bc; }
    c = (char)c - '0'; year = c * 1000;
    c = *ptr++ - '0'; year += c * 100;
    c = *ptr++ - '0'; year += c * 10;
    c = *ptr++ - '0'; year += c;
    if (ZuUnlikely(*ptr++ != '-')) goto invalid;
    c = *ptr++ - '0'; month = c * 10;
    c = *ptr++ - '0'; month += c;
    if (ZuUnlikely(*ptr++ != '-')) goto invalid;
    c = *ptr++ - '0'; day = c * 10;
    c = *ptr++ - '0'; day += c;

    if (ptr >= end || *ptr++ != 'T') {
      int year_ = year, month_ = month;
      if (ZuUnlikely(bc)) year_ = -year_;
      normalize(year_, month_);
      m_julian = julian(year_, month_, day);
      m_sec = 0;
      m_nsec = 0;
      if (ZuUnlikely(tz)) offset_(year_, month_, day, 0, 0, 0, tz);
      return;
    }

    if (ZuUnlikely(end - ptr < 8)) goto invalid;
    c = *ptr++ - '0'; hour = c * 10;
    c = *ptr++ - '0'; hour += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; minute = c * 10;
    c = *ptr++ - '0'; minute += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; sec = c * 10;
    c = *ptr++ - '0'; sec += c;

    nsec = 0;
    if (ZuLikely(ptr < end)) {
      if (ZuUnlikely(end - ptr < 2)) goto tz;
      if (ZuUnlikely((c = *ptr++) != '.')) { --ptr; goto tz; }
      unsigned pow = 100000000;
      c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) goto invalid;
      nsec = c * pow;
      while (ptr < end) {
	c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) { --ptr; break; }
	nsec += c * (pow /= 10);
      }
    }

  tz:
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    m_julian = julian(year, month, day);
    m_sec = second(hour, minute, sec);
    m_nsec = nsec;

    if (ptr >= end) {
      if (ZuUnlikely(tz)) offset_(year, month, day, hour, minute, sec, tz);
      return;
    }

    if ((c = *ptr++) == 'Z') return;

    int offset;

    if (c == '+') {
      offset = 1;
    } else if (ZuLikely(c == '-')) {
      offset = -1;
    } else
      goto invalid;

    if (ZuUnlikely(end - ptr < 2)) goto invalid;

    unsigned offsetHours, offsetMinutes = 0;

    c = *ptr++ - '0'; offsetHours = c * 10;
    c = *ptr++ - '0'; offsetHours += c;
    if (ZuLikely(ptr >= end)) goto offset;
    c = *ptr++;
    if (c == ':') {
      if (ZuUnlikely(end - ptr < 2)) goto invalid;
      c = *ptr++;
    } else
      if (ZuUnlikely(end - ptr < 1)) goto invalid;
    c = (char)c - '0'; offsetMinutes = c * 10;
    c = *ptr++ - '0'; offsetMinutes += c;

  offset:
    if (offset >= 0)
      offset = -(int)(offsetHours * 60 + offsetMinutes) * 60;
    else
      offset = (offsetHours * 60 + offsetMinutes) * 60;

    *this += offset;
    return;
  }

invalid:
  m_julian = ZtDate_NullJulian, m_sec = 0, m_nsec = 0;
}

void ZtDate::ctorFIX(ZuString s)
{
  {
    unsigned year, month, day, hour, minute, sec, nsec;
    const char *ptr = s.data();
    const char *end = ptr + s.length();
    unsigned c;

    if (ZuUnlikely(end - ptr < 17)) goto invalid;
    c = *ptr++ - '0'; year = c * 1000;
    c = *ptr++ - '0'; year += c * 100;
    c = *ptr++ - '0'; year += c * 10;
    c = *ptr++ - '0'; year += c;
    c = *ptr++ - '0'; month = c * 10;
    c = *ptr++ - '0'; month += c;
    c = *ptr++ - '0'; day = c * 10;
    c = *ptr++ - '0'; day += c;
    if (ZuUnlikely(
	!year || year > 9999U ||
	!month || month > 12U ||
	!day || day > 31U)) goto invalid;
    if (ZuUnlikely(*ptr++ != '-')) goto invalid;
    c = *ptr++ - '0'; hour = c * 10;
    c = *ptr++ - '0'; hour += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; minute = c * 10;
    c = *ptr++ - '0'; minute += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; sec = c * 10;
    c = *ptr++ - '0'; sec += c;
    if (ZuUnlikely(
	hour > 23U || minute > 59U || sec > 59U)) goto invalid;

    nsec = 0;
    if (ZuLikely(ptr < end)) {
      if (ZuUnlikely(end - ptr < 2 || *ptr++ != '.')) goto end;
      unsigned pow = 100000000;
      c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) goto invalid;
      nsec = c * pow;
      while (ptr < end) {
	c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) break;
	nsec += c * (pow /= 10);
      }
    }

  end:
    // normalize(year, month);
    // normalize(day, hour, minute, sec, nsec);
    m_julian = julian(year, month, day);
    m_sec = second(hour, minute, sec);
    m_nsec = nsec;

    return;
  }

invalid:
  m_julian = ZtDate_NullJulian, m_sec = 0, m_nsec = 0;
}

void ZtDate::ctorCSV(ZuString s, const char *tz, int tzOffset)
{
  {
    unsigned year, month, day, hour, minute, sec, nsec;
    const char *ptr = s.data();
    const char *end = ptr + s.length();
    unsigned c;
    bool bc = 0;

  bc:
    if (ZuUnlikely(end - ptr < 10)) goto invalid;
    c = *ptr++;
    if (ZuUnlikely(c == '-')) { bc = 1; goto bc; }
    c = (char)c - '0'; year = c * 1000;
    c = *ptr++ - '0'; year += c * 100;
    c = *ptr++ - '0'; year += c * 10;
    c = *ptr++ - '0'; year += c;
    if (ZuUnlikely(*ptr++ != '/')) goto invalid;
    c = *ptr++ - '0'; month = c * 10;
    c = *ptr++ - '0'; month += c;
    if (ZuUnlikely(*ptr++ != '/')) goto invalid;
    c = *ptr++ - '0'; day = c * 10;
    c = *ptr++ - '0'; day += c;

    if (ptr >= end || *ptr++ != ' ') {
      int year_ = year, month_ = month;
      if (ZuUnlikely(bc)) year_ = -year_;
      normalize(year_, month_);
      m_julian = julian(year_, month_, day);
      m_sec = 0;
      m_nsec = 0;
      if (ZuUnlikely(tz)) offset_(year_, month_, day, 0, 0, 0, tz);
      return;
    }

    if (ZuUnlikely(end - ptr < 8)) goto invalid;
    c = *ptr++ - '0'; hour = c * 10;
    c = *ptr++ - '0'; hour += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; minute = c * 10;
    c = *ptr++ - '0'; minute += c;
    if (ZuUnlikely(*ptr++ != ':')) goto invalid;
    c = *ptr++ - '0'; sec = c * 10;
    c = *ptr++ - '0'; sec += c;

    nsec = 0;
    if (ZuLikely(ptr < end)) {
      if (ZuUnlikely(end - ptr < 2)) goto end;
      if (ZuUnlikely((c = *ptr++) != '.')) goto end;
      unsigned pow = 100000000;
      c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) goto invalid;
      nsec = c * pow;
      while (ptr < end) {
	c = *ptr++ - '0'; if (ZuUnlikely(c >= 10)) break;
	nsec += c * (pow /= 10);
      }
    }

  end:
    normalize(year, month);
    normalize(day, hour, minute, sec, nsec);
    m_julian = julian(year, month, day);
    m_sec = second(hour, minute, sec);
    m_nsec = nsec;

    if (tz)
      offset_(year, month, day, hour, minute, sec, tz);
    else if (tzOffset)
      *this += tzOffset;
    return;
  }

invalid:
  m_julian = ZtDate_NullJulian, m_sec = 0, m_nsec = 0;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
