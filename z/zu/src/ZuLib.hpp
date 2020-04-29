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

// Zu library main header

#ifndef ZuLib_HPP
#define ZuLib_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _WIN32

#ifndef WINVER
#define WINVER 0x0800
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0800
#endif

#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0800
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

#ifndef __MSVCRT_VERSION__
#define __MSVCRT_VERSION__ 0x0A00
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define ZuExport_API __declspec(dllexport)
#define ZuExport_Explicit
#define ZuImport_API __declspec(dllimport)
#define ZuImport_Explicit extern

#ifdef _MSC_VER
#pragma warning(disable:4231 4503)
#endif

#ifdef ZU_EXPORTS
#define ZuAPI ZuExport_API
#define ZuExplicit ZuExport_Explicit
#else
#define ZuAPI ZuImport_API
#define ZuExplicit ZuImport_Explicit
#endif
#define ZuExtern extern ZuAPI

#else /* _WIN32 */

#define ZuAPI
#define ZuExplicit
#define ZuExtern extern

#endif /* _WIN32 */

// to satisfy the pedants
#include <limits.h>
#if CHAR_BIT != 8
#error "Broken platform - CHAR_BIT is not 8 - a byte is not 8 bits!"
#endif
#if UINT_MAX < 0xffffffff
#error "Broken platform - UINT_MAX is < 0xffffffff - an int is < 32 bits!"
#endif

#ifdef __GNUC__

#define ZuLikely(x) __builtin_expect(!!(x), 1)
#define ZuUnlikely(x) __builtin_expect(!!(x), 0)

#ifdef ZDEBUG
#define ZuInline inline
#define ZuNoInline inline
#else
#define ZuInline inline __attribute__((always_inline))
#define ZuNoInline inline __attribute__((noinline))
#endif

#else

#define ZuLikely(x) (x)
#define ZuUnlikely(x) (x)

#ifdef _MSC_VER
#define ZuInline __forceinline
#else
#define ZuInline
#endif
#define ZuNoInline

#endif

#ifdef __GNUC__
#define ZuMayAlias(x) __attribute__((__may_alias__)) x
#else
#define ZuMayAlias(x) x
#endif

#if defined (__cplusplus)
// std::remove_reference without dragging in STL cruft
template <typename T_>
struct ZuDeref { using T = T_; };
template <typename T_>
struct ZuDeref<T_ &> { using T = T_; };
template <typename T_>
struct ZuDeref<const T_ &> { using T = const T_; };
template <typename T_>
struct ZuDeref<volatile T_ &> { using T = volatile T_; };
template <typename T_>
struct ZuDeref<const volatile T_ &> { using T = const volatile T_; };
template <typename T_>
struct ZuDeref<T_ &&> { using T = T_; };

// std::remove_cv (strip qualifiers) without dragging in STL cruft
template <typename T_>
struct ZuStrip { using T = T_; };
template <typename T_>
struct ZuStrip<const T_> { using T = T_; };
template <typename T_>
struct ZuStrip<volatile T_> { using T = T_; };
template <typename T_>
struct ZuStrip<const volatile T_> { using T = T_; };

// shorthand constexpr std::forward without STL cruft
template <typename T>
constexpr T &&ZuFwd(typename ZuDeref<T>::T &v) noexcept { // fwd lvalue
  return static_cast<T &&>(v);
}
template <typename T>
constexpr T &&ZuFwd(typename ZuDeref<T>::T &&v) noexcept { // fwd rvalue
  return static_cast<T &&>(v);
}
// use to forward auto &&x parameters (GNU extension)
#define ZuAutoFwd(x) ZuFwd<decltype(x)>(x)
// shorthand constexpr std::move without STL cruft
template <typename T>
constexpr typename ZuDeref<T>::T &&ZuMv(T &&t) noexcept {
  return static_cast<typename ZuDeref<T>::T &&>(t);
}

// generic RAII guard
template <typename L> struct ZuGuard {
  ZuInline ZuGuard(L fn_) : fn(ZuMv(fn_)) { }
  ZuInline ~ZuGuard() { fn(); }
  ZuGuard(const ZuGuard &) = delete;
  ZuGuard &operator =(const ZuGuard &) = delete;
  ZuGuard(ZuGuard &&) = default;
  ZuGuard &operator =(ZuGuard &&) = default;
  L fn;
};
#endif

#if defined(linux) || defined(__mips64)
#include <endian.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define Zu_BIGENDIAN 1
#else
#define Zu_BIGENDIAN 0
#endif
#else
#ifdef _WIN32
#define Zu_BIGENDIAN 0
#endif
#endif

// safe bool idiom
#define ZuOpBool \
  operator const void *() const { \
    return !*this ? (const void *)0 : (const void *)this; \
  }

// generic swap
template <typename T>
void ZuSwap(T &v1, T &v2) {
  T tmp = ZuMv(v1);
  v1 = ZuMv(v2);
  v2 = ZuMv(tmp);
}

#endif /* ZuLib_HPP */
