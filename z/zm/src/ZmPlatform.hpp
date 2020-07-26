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

// platform primitives

#ifndef ZmPlatform_HPP
#define ZmPlatform_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <alloca.h>
#else
#include <winsock2.h>
#include <ole2.h>
#include <olectl.h>
#include <process.h>
#endif

#include <stdarg.h>

#include <zlib/ZuInt.hpp>
#include <zlib/ZuStringN.hpp>

#ifdef linux
#include <sys/types.h>
#include <linux/unistd.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251 4800 4996)
#endif

#define ZmIDStrSize	60	// max size of a heap/hash ID incl. terminator

using ZmIDString = ZuStringN<ZmIDStrSize>;

class ZmTime;

#define ZmCacheLineSize 64
#ifdef _MSC_VER
#define ZmCacheAlign /**/
#else
#define ZmCacheAlign __attribute__((aligned(ZmCacheLineSize)))
#endif

class ZmAPI ZmPlatform {
  ZmPlatform();
  ZmPlatform(const ZmPlatform &);
  ZmPlatform &operator =(const ZmPlatform &);	// prevent mis-use

public:
  enum { CacheLineSize = ZmCacheLineSize };

#ifndef _WIN32
  using ProcessID = pid_t;
#ifdef linux
  using ThreadID = pid_t;
#else
  using ThreadID = pthread_t;
#endif
#else /* !_WIN32 */
  using ProcessID = DWORD;
  using ThreadID = DWORD;
#endif /* !_WIN32 */

// process ID
#ifndef _WIN32
  static ProcessID getPID() { return getpid(); }
#else
  static ProcessID getPID() { return GetCurrentProcessId(); }
#endif

// thread ID

  static ThreadID getTID_();
  ZuInline static ThreadID getTID() {
    thread_local ThreadID tid = getTID_();
    return tid;
  }

// #cpus (number of cores)
#ifdef _WIN32
private:
  typedef BOOL (WINAPI *PIsWow64Process)(HANDLE, PBOOL);
  typedef void (WINAPI *PGetNativeSystemInfo)(LPSYSTEM_INFO);
public:
  static unsigned getncpu() {
    SYSTEM_INFO si;
    {
      HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
      if (!kernel32)
	GetSystemInfo(&si);
      else {
	PIsWow64Process isWow64Process =
	  (PIsWow64Process)GetProcAddress(kernel32, "IsWow64Process");
	BOOL isWow64;
	if (!isWow64Process ||
	    !isWow64Process(GetCurrentProcess(), &isWow64) ||
	    !isWow64)
	  GetSystemInfo(&si);
	else {
	    PGetNativeSystemInfo getNativeSystemInfo =
	      (PGetNativeSystemInfo)GetProcAddress(kernel32,
						   "GetNativeSystemInfo");
	    if (!getNativeSystemInfo)
	      GetSystemInfo(&si);
	    else
	      getNativeSystemInfo(&si);
	}
      }
    }
    int n = si.dwNumberOfProcessors;
    return n < 1 ? 1 : n;
  }
#endif
#ifdef linux
  static unsigned getncpu() {
    int n = sysconf(_SC_NPROCESSORS_ONLN);
    return n < 1 ? 1 : n;
  }
#endif

// sleep & yield
#ifndef _WIN32
  static void sleep(ZmTime timeout);
  ZuInline static void yield() { pthread_yield(); }
#else
  static void sleep(ZmTime timeout);
  ZuInline static void yield() { ::Sleep(0); }
#endif

// (hard) exit process
#ifndef _WIN32
  ZuInline static void exit(int code) { ::_exit(code); }
#else
  ZuInline static void exit(int code) { ::ExitProcess(code); }
#endif
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmPlatform_HPP */
