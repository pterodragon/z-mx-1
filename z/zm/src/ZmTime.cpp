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

// high resolution timer

#include <ZmTime.hpp>
#include <ZmAtomic.hpp>
#include <ZmLock.hpp>
#include <ZmTime.hpp>
#include <ZmThread.hpp>
#include <ZmSingleton.hpp>

#ifndef _WIN32

void ZmTime::calibrate(int n) { }

#else /* !_WIN32 */

struct ZmPlatform_WinTimer {
  ZmPlatform_WinTimer() {
    calibrate(64);
  }

  ~ZmPlatform_WinTimer() { }

  static ZmPlatform_WinTimer *instance() {
    return ZmSingleton<ZmPlatform_WinTimer>::instance();
  }

  void calibrate(int n) {
    ZmGuard<ZmPLock> guard(m_lock);

    int64_t ftTotal = 0, ftNow, ftCheck;
    int64_t pcTotal = 0, pcDelta, pcNow, pcIntrinsic;

    long double k10 = (long double)10000;
    long double m10 = (long double)10000000;

    // establish intrinsic function call time

    for (int i = 0; i < 100; i++)
      QueryPerformanceCounter((LARGE_INTEGER *)&pcNow);
    QueryPerformanceCounter((LARGE_INTEGER *)&m_pcStart);
    for (int i = 0; i < 10000; i++)
      QueryPerformanceCounter((LARGE_INTEGER *)&pcNow);
    pcIntrinsic = (int64_t)((long double)(pcNow - m_pcStart) / k10);

    // "burn in"

    GetSystemTimeAsFileTime((FILETIME *)&ftCheck);
    do {
      GetSystemTimeAsFileTime((FILETIME *)&ftNow);
      QueryPerformanceCounter((LARGE_INTEGER *)&pcNow);
    } while (ftNow == ftCheck);

    // establish start times

    m_ftStart = ftNow;
    m_pcStart = pcNow - pcIntrinsic;

    // calculate and record

    ftCheck = m_ftStart;
    int i = 0, j = 0;
    for (;;) {
      GetSystemTimeAsFileTime((FILETIME *)&ftNow);
      QueryPerformanceCounter((LARGE_INTEGER *)&pcNow);
      pcNow -= pcIntrinsic;
      pcDelta = pcNow - m_pcStart;
      pcTotal += pcDelta;
      ftTotal += ftNow - m_ftStart;
      j++;
      if (ftNow != ftCheck) {
	if (++i == n) break;
	ftCheck = ftNow;
      }
    }

    // calculate *effective* m_freq
    // - QueryPerformanceFrequency() is inaccurate due to clock drift

    m_freq = ((long double)pcDelta * m10) / (long double)(ftNow - m_ftStart);

    // establish accurate FILETIME and Performance Counter offsets

    m_ftStart -= ZmTime_FT_Epoch;
    m_ftStart += (int64_t)((long double)ftTotal / (long double)j);
    m_ftStart =
      (int64_t)(((long double)m_ftStart * m_freq) / (long double)10000000);

    m_pcStart += (int64_t)((long double)pcTotal / (long double)j);
  }

  int64_t now() {
    int64_t t;
    QueryPerformanceCounter((LARGE_INTEGER *)&t);
    t -= m_pcStart;
    t += m_ftStart;
    return (int64_t)(((long double)t * (long double)10000000) / m_freq);
  }

  ZmPLock	m_lock;

  long double	m_freq;
  int64_t	m_pcStart;
  int64_t	m_ftStart;
};

int64_t ZmTime::now_() {
  return ZmPlatform_WinTimer::instance()->now();
}

void ZmTime::calibrate(int n) {
  ZmPlatform_WinTimer::instance()->calibrate(n);
}

#endif /* !_WIN32 */

// sleep()

#ifndef _WIN32
void ZmPlatform::sleep(ZmTime timeout) {
  ZmTime remaining;

  while (nanosleep(&timeout, &remaining)) {
    if (errno != EINTR) break;
    timeout = remaining;
  }
}
#else
void ZmPlatform::sleep(ZmTime timeout) {
  Sleep(timeout.millisecs());
}
#endif
