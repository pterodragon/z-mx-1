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

// platform-specific

#ifndef ZtPlatform_HPP
#define ZtPlatform_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <locale.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

#include <zlib/ZuInt.hpp>

class ZtAPI ZtPlatform {
  ZtPlatform();
  ZtPlatform(const ZtPlatform &);
  ZtPlatform &operator =(const ZtPlatform &);	// prevent mis-use

public:
// environment and timezone manipulation
#ifndef _WIN32
  static int putenv(const char *s) { return ::putenv((char *)s); }
  static void tzset(void) { ::tzset(); }
#else
  static int putenv(const char *s) { return ::_putenv((char *)s); }
  static void tzset(void) { ::_tzset(); }
#endif

// memory growth algorithm for growing string and array buffers

  static unsigned grow(unsigned o, unsigned n) {
    if (ZuUnlikely(o > n)) return o;

    const unsigned v = (sizeof(void *)<<1); // malloc overhead

    o += v, n += v;

    if (n < 128) return 128 - v;	// minimum 128 bytes

    const unsigned m = (1U<<16) - 1;	// 64K mask

    if (o < 64) o = 64;

    if (o <= m && n < (o<<1)) return (o<<1) - v;// double old size up to 64k

    return ((n + m) & ~m) - v;	// round up to nearest 64k - overhead
  }
};

#endif /* ZtPlatform_HPP */
