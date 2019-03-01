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

#ifdef _WIN32

#include <intrin.h>

class ZmPlatform_WinTimer {
public:
  ZmPlatform_WinTimer() { calibrate(); }

  int64_t now() const {
    int64_t t;
    long double m10 = 10000000.0L;
    QueryPerformanceCounter((LARGE_INTEGER *)&t);
    t -= m_qpcStart;
    t += m_ftStart;
    return (int64_t)(((long double)t * m10) / m_ftFreq);
  }

  long double cpuFreq() const {
    return m_cpuFreq;
  }

private:
  void calibrate() {
    ZmGuard<ZmPLock> guard(m_lock);

    int64_t ftTotal = 0, ftNow, ftCheck;
    int64_t qpcTotal = 0, qpcDelta, qpcNow, qpcCheck, qpcStamp;
    int64_t cpuStamp;

    long double m10 = 10000000.0L;

    // "burn in" - tight loop until FT ticks up

    GetSystemTimeAsFileTime((FILETIME *)&ftCheck);
    do {
      QueryPerformanceCounter((LARGE_INTEGER *)&qpcCheck);
      do {
	QueryPerformanceCounter((LARGE_INTEGER *)&qpcNow);
	cpuStamp = __rdtsc();
      } while (qpcNow == qpcCheck);
      qpcStamp = qpcNow;
      GetSystemTimeAsFileTime((FILETIME *)&ftNow);
    } while (ftNow == ftCheck);

    // establish start times

    m_ftStart = ftNow;
    m_qpcStart = qpcNow;

    // calculate and record for 100 FT ticks

    ftCheck = m_ftStart;
    unsigned i = 0, j = 0;
    for (;;) {
      QueryPerformanceCounter((LARGE_INTEGER *)&qpcCheck);
      do {
	QueryPerformanceCounter((LARGE_INTEGER *)&qpcNow);
      } while (qpcNow == qpcCheck);
      GetSystemTimeAsFileTime((FILETIME *)&ftNow);
      QueryPerformanceCounter((LARGE_INTEGER *)&qpcDelta);
      qpcNow += ((qpcDelta - qpcNow)>>1);
      qpcDelta = qpcNow - m_qpcStart;
      qpcTotal += qpcDelta;
      ftTotal += ftNow - m_ftStart;
      j++;
      if (ftNow != ftCheck) {
	if (++i >= 100) break;
	ftCheck = ftNow;
      }
    }

    // calculate *effective* m_ftFreq for FT

    m_ftFreq = ((long double)qpcDelta * m10) / (long double)(ftNow - m_ftStart);

    // establish accurate FT and PC offsets

    long double jd = j;

    m_ftStart -= ZmTime_FT_Epoch;
    m_ftStart += (int64_t)((long double)ftTotal / jd);
    m_ftStart = (int64_t)(((long double)m_ftStart * m_ftFreq) / m10);
    m_qpcStart += (int64_t)((long double)qpcTotal / jd);

    // obtain CPU frequency, preferring CPUID to elapsed TSC
 
    int info[4];
    __cpuid(info, 0);
    if (info[0] < 0x15) {
fallback:
      int64_t qpcFreq;
      int64_t cpuDelta;
      QueryPerformanceFrequency((LARGE_INTEGER *)&qpcFreq);
      QueryPerformanceCounter((LARGE_INTEGER *)&qpcCheck);
      do {
	QueryPerformanceCounter((LARGE_INTEGER *)&qpcNow);
	cpuDelta = __rdtsc();
      } while (qpcNow == qpcCheck);
      cpuDelta -= cpuStamp;
      qpcDelta = qpcNow - qpcStamp;
      qpcFreq /= 1000;
      m_cpuFreq = (long double)((cpuDelta * qpcFreq) / qpcDelta) * 1000.0L;
    } else {
      // below code ported from Linux kernel
      __cpuid(info, 0x1);
      unsigned family = (info[0] >> 8) & 0xf;
      unsigned model = (info[0] >> 4) & 0xf;
      if (family == 0xf) family += (info[0] >> 20) & 0xff;
      if (family >= 0x6) model += ((info[0] >> 16) & 0xf) << 4;
      __cpuid(info, 0x15);
      unsigned crystal_khz = info[2] / 1000;
      if (!crystal_khz) {
	switch (model) {
	  case 0x4e: // INTEL_FAM6_SKYLAKE_MOBILE
	  case 0x5e: // INTEL_FAM6_SKYLAKE_DESKTOP
	  case 0x8e: // INTEL_FAM6_KABYLAKE_MOBILE
	  case 0x9e: // INTEL_FAM6_KABYLAKE_DESKTOP
	    crystal_khz = 24000;
	    break;
	  case 0x5f: // INTEL_FAM6_ATOM_GOLDMONT_X
	    crystal_khz = 25000;
	    break;
	  case 0x5c: // INTEL_FAM6_ATOM_GOLDMONT
	    crystal_khz = 19200;
	    break;
	  default:
	    goto fallback;
	}
      }
      m_cpuFreq = (long double)((crystal_khz * info[1]) / info[0]) * 1000.0L;
    }
  }

private:
  ZmPLock	m_lock;

  long double	m_ftFreq;	// FILETIME frequency
  int64_t	m_qpcStart;	// QueryPerformanceCounter start
  int64_t	m_ftStart;	// FILETIME start
  long double	m_cpuFreq;	// CPU frequency (TSC cycles per second)
};

static ZmPlatform_WinTimer ZmPlatform_winTimer;

int64_t ZmTime::now_() {
  return ZmPlatform_winTimer.now();
}

long double ZmTime::cpuFreq() {
  return ZmPlatform_winTimer.cpuFreq();
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
