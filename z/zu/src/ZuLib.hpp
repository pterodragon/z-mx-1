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

// std::decay without dragging in STL cruft
template <typename T_>
struct ZuDecay { using T = typename ZuStrip<typename ZuDeref<T_>::T>::T; };

// shorthand constexpr std::forward without STL cruft
template <typename T>
constexpr T &&ZuFwd(typename ZuDeref<T>::T &v) noexcept { // fwd lvalue
  return static_cast<T &&>(v);
}
template <typename T>
constexpr T &&ZuFwd(typename ZuDeref<T>::T &&v) noexcept { // fwd rvalue
  return static_cast<T &&>(v);
}
// use to forward auto &&x parameters (usually template lambda parameters)
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

// move/copy universal reference
// ZuMvCp<U>::mvcp(ZuFwd<U>(u), [](auto &&v) { }, [](const auto &v) { });
template <typename T_> struct ZuMvCp {
  using T = typename ZuDecay<T_>::T;

  template <typename Mv, typename Cp>
  static auto mvcp(const T &v, Mv, Cp cp_) { return cp_(v); }
  template <typename Mv, typename Cp>
  static auto mvcp(T &&v, Mv mv_, Cp) { return mv_(ZuMv(v)); }

  template <typename Mv>
  static void mv(const T &v, Mv); // undefined
  template <typename Mv>
  static auto mv(T &&v, Mv mv_) { return mv_(ZuMv(v)); }

  template <typename Cp>
  static auto cp(const T &v, Cp cp_) { return cp_(v); }
  template <typename Cp>
  static void cp(T &&v, Cp); // undefined
};

// type list
template <typename ...Args> struct ZuTypeList {
  enum { N = sizeof...(Args) };
  template <typename ...Args_> struct Prepend {
    using T = ZuTypeList<Args_..., Args...>;
  };
  template <typename ...Args_> struct Prepend<ZuTypeList<Args_...>> {
    using T = ZuTypeList<Args_..., Args...>;
  };
  template <typename ...Args_> struct Append {
    using T = ZuTypeList<Args..., Args_...>;
  };
  template <typename ...Args_> struct Append<ZuTypeList<Args_...>> {
    using T = ZuTypeList<Args..., Args_...>;
  };
};

// index -> type
template <unsigned I, typename ...> struct ZuType;
template <typename T0, typename ...Args>
struct ZuType<0, T0, Args...> {
  using T = T0;
};
template <unsigned I, typename O, typename ...Args>
struct ZuType<I, O, Args...> {
  using T = typename ZuType<I - 1, Args...>::T;
};
template <unsigned I, typename ...Args>
struct ZuType<I, ZuTypeList<Args...>> :
  public ZuType<I, Args...> { };

// type -> index
template <typename, typename ...> struct ZuTypeIndex;
template <typename T, typename ...Args>
struct ZuTypeIndex<T, T, Args...> {
  enum { I = 0 };
};
template <typename T, typename O, typename ...Args>
struct ZuTypeIndex<T, O, Args...> {
  enum { I = 1 + ZuTypeIndex<T, Args...>::I };
};
template <typename T, typename ...Args>
struct ZuTypeIndex<T, ZuTypeList<Args...>> :
  public ZuTypeIndex<T, Args...> { };

// map
template <template <typename> class, typename ...> struct ZuTypeMap;
template <template <typename> class Map, typename T0>
struct ZuTypeMap<Map, T0> {
  using T = ZuTypeList<typename Map<T0>::T>;
};
template <template <typename> class Map, typename T0, typename ...Args>
struct ZuTypeMap<Map, T0, Args...> {
  using T =
    typename ZuTypeList<typename Map<T0>::T>::template Append<
      typename ZuTypeMap<Map, Args...>::T>::T;
};
template <template <typename> class Map, typename ...Args>
struct ZuTypeMap<Map, ZuTypeList<Args...>> :
  public ZuTypeMap<Map, Args...> { };

// grep
template <bool, typename> struct ZuTypeGrep_;
template <typename T0> struct ZuTypeGrep_<true, T0> {
  using T = ZuTypeList<T0>;
};
template <typename T0> struct ZuTypeGrep_<false, T0> {
  using T = ZuTypeList<>;
};
template <template <typename> class, typename ...> struct ZuTypeGrep;
template <template <typename> class Filter, typename T0>
struct ZuTypeGrep<Filter, T0> {
  using T = typename ZuTypeGrep_<Filter<T0>::OK, T0>::T;
};
template <template <typename> class Filter, typename T0, typename ...Args>
struct ZuTypeGrep<Filter, T0, Args...> {
  using T =
    typename ZuTypeGrep_<Filter<T0>::OK, T0>::T::template Append<
      typename ZuTypeGrep<Filter, Args...>::T>::T;
};
template <template <typename> class Filter, typename ...Args>
struct ZuTypeGrep<Filter, ZuTypeList<Args...>> :
  public ZuTypeGrep<Filter, Args...> { };

// reduce (recursive pair-wise reduction)
template <template <typename...> class, typename ...>
struct ZuTypeReduce;
template <template <typename...> class Reduce, typename T0>
struct ZuTypeReduce<Reduce, T0> {
  using T = typename Reduce<T0>::T;
};
template <template <typename...> class Reduce, typename T0, typename T1>
struct ZuTypeReduce<Reduce, T0, T1> {
  using T = typename Reduce<T0, T1>::T;
};
template <
  template <typename...> class Reduce,
  typename T0, typename T1, typename ...Args>
struct ZuTypeReduce<Reduce, T0, T1, Args...> {
  using T =
    typename Reduce<T0, typename ZuTypeReduce<Reduce, T1, Args...>::T>::T;
};
template <template <typename...> class Reduce, typename ...Args>
struct ZuTypeReduce<Reduce, ZuTypeList<Args...>> :
  public ZuTypeReduce<Reduce, Args...> { };

// split typelist left, 0..N-1
template <unsigned N, typename ...Args> struct ZuTypeLeft;
template <unsigned N, typename Arg0, typename ...Args>
struct ZuTypeLeft<N, Arg0, Args...> {
  using T = typename ZuTypeLeft<N - 1, Args...>::T::template Prepend<Arg0>::T;
};
template <typename Arg0, typename ...Args>
struct ZuTypeLeft<0, Arg0, Args...> {
  using T = ZuTypeList<>;
};
template <typename ...Args>
struct ZuTypeLeft<0, Args...> {
  using T = ZuTypeList<>;
};
template <unsigned N, typename ...Args>
struct ZuTypeLeft<N, ZuTypeList<Args...>> : public ZuTypeLeft<N, Args...> { };

// split typelist right, N..
template <unsigned N, typename ...Args> struct ZuTypeRight;
template <unsigned N, typename Arg0, typename ...Args>
struct ZuTypeRight<N, Arg0, Args...> {
  using T = typename ZuTypeRight<N - 1, Args...>::T;
};
template <typename Arg0, typename ...Args>
struct ZuTypeRight<0, Arg0, Args...> {
  using T = ZuTypeList<Arg0, Args...>;
};
template <typename ...Args>
struct ZuTypeRight<0, Args...> {
  using T = ZuTypeList<Args...>;
};
template <unsigned N, typename ...Args>
struct ZuTypeRight<N, ZuTypeList<Args...>> : public ZuTypeRight<N, Args...> { };

// compile-time merge sort typelist using Index<T>::I
template <template <typename> class Index, typename Left, typename Right>
struct ZuTypeMerge;
template <template <typename> class, typename, typename, bool>
struct ZuTypeMerge_;
template <
  template <typename> class Index,
  typename LeftArg0, typename ...LeftArgs,
  typename RightArg0, typename ...RightArgs>
struct ZuTypeMerge_<Index,
    ZuTypeList<LeftArg0, LeftArgs...>,
    ZuTypeList<RightArg0, RightArgs...>, false> {
  using T = typename ZuTypeMerge<Index,
    ZuTypeList<LeftArgs...>,
    ZuTypeList<RightArg0, RightArgs...>>::T::template Prepend<LeftArg0>::T;
};
template <
  template <typename> class Index,
  typename LeftArg0, typename ...LeftArgs,
  typename RightArg0, typename ...RightArgs>
struct ZuTypeMerge_<Index,
    ZuTypeList<LeftArg0, LeftArgs...>,
    ZuTypeList<RightArg0, RightArgs...>, true> {
  using T = typename ZuTypeMerge<Index,
    ZuTypeList<LeftArg0, LeftArgs...>,
    ZuTypeList<RightArgs...>>::T::template Prepend<RightArg0>::T;
};
template <
  template <typename> class Index,
  typename LeftArg0, typename ...LeftArgs,
  typename RightArg0, typename ...RightArgs>
struct ZuTypeMerge<Index,
  ZuTypeList<LeftArg0, LeftArgs...>,
  ZuTypeList<RightArg0, RightArgs...>> :
    public ZuTypeMerge_<Index, 
      ZuTypeList<LeftArg0, LeftArgs...>,
      ZuTypeList<RightArg0, RightArgs...>,
      (Index<LeftArg0>::I > Index<RightArg0>::I)> { };
template <template <typename> class Index, typename ...Args>
struct ZuTypeMerge<Index, ZuTypeList<>, ZuTypeList<Args...>> {
  using T = ZuTypeList<Args...>;
};
template <template <typename> class Index, typename ...Args>
struct ZuTypeMerge<Index, ZuTypeList<Args...>, ZuTypeList<>> {
  using T = ZuTypeList<Args...>;
};
template <template <typename> class Index>
struct ZuTypeMerge<Index, ZuTypeList<>, ZuTypeList<>> {
  using T = ZuTypeList<>;
};
template <template <typename> class Index, typename ...Args>
struct ZuTypeSort {
  enum { N = sizeof...(Args) };
  using T = typename ZuTypeMerge<Index,
    typename ZuTypeSort<Index, typename ZuTypeLeft<(N>>1), Args...>::T>::T,
    typename ZuTypeSort<Index, typename ZuTypeRight<(N>>1), Args...>::T>::T
  >::T;
};
template <template <typename> class Index, typename Arg0>
struct ZuTypeSort<Index, Arg0> {
  using T = ZuTypeList<Arg0>;
};
template <template <typename> class Index>
struct ZuTypeSort<Index> {
  using T = ZuTypeList<>;
};
template <template <typename> class Index, typename ...Args>
struct ZuTypeSort<Index, ZuTypeList<Args...>> :
    public ZuTypeSort<Index, Args...> { };

// apply typelist to template
template <template <typename...> class Type, typename ...Args>
struct ZuTypeApply { using T = Type<Args...>; };
template <template <typename...> class Type, typename ...Args>
struct ZuTypeApply<Type, ZuTypeList<Args...>> :
  public ZuTypeApply<Type, Args...> { };

// invoke lambda on all elements of typelist []<typename T>() { ...; }
template <typename ...>
struct ZuTypeAll {
  template <typename L> static ZuInline void invoke(L) { }
  template <typename L> static ZuInline void invoke_(L &) { }
};
template <typename T0>
struct ZuTypeAll<T0> {
  template <typename L> static ZuInline void invoke(L l) { invoke_(l); }
  template <typename L> static ZuInline void invoke_(L &l) {
    l.template operator ()<T0>();
  }
};
template <typename T0, typename ...Args>
struct ZuTypeAll<T0, Args...> {
  template <typename L> static ZuInline void invoke(L l) { invoke_(l); }
  template <typename L> static ZuInline void invoke_(L &l) {
    l.template operator ()<T0>();
    ZuTypeAll<Args...>::invoke_(l);
  }
};
template <typename ...Args>
struct ZuTypeAll<ZuTypeList<Args...>> : public ZuTypeAll<Args...> { };

// compile-time policy tag for intrusively shadowed (not owned) objects
class ZuShadow { }; // tag

// constexpr instantiable numeric constant
template <unsigned I_> struct ZuConstant {
  enum { I = I_ };
  constexpr operator unsigned() const noexcept { return I; }
  constexpr unsigned operator()() const noexcept { return I; }
};

// shorthand for std::declval
template <typename U> struct ZuDeclVal__ { using T = U; };
template <typename T> auto ZuDeclVal_(int) -> typename ZuDeclVal__<T&&>::T;
template <typename T> auto ZuDeclVal_(...) -> typename ZuDeclVal__<T>::T;
template <typename U> decltype(ZuDeclVal_<U>(0)) ZuDeclVal();
 
// shorthand for std::void_t
template <typename ...> struct ZuVoid { using T = void; };

// cv checking
template <typename U, typename R = void> struct ZuIsConst;
template <typename U, typename R>
struct ZuIsConst<const U, R> { using T = R; };
template <typename U, typename R = void>
struct ZuNotConst { using T = R; };
template <typename U, typename R> struct ZuNotConst<const U, R> { };

template <typename U, typename R = void> struct ZuIsVolatile;
template <typename U, typename R>
struct ZuIsVolatile<volatile U, R> { using T = R; };
template <typename U, typename R = void>
struct ZuNotVolatile { using T = R; };
template <typename U, typename R> struct ZuNotVolatile<volatile U, R> { };

// T *var = ZuAlloca(var, n);
#ifdef _MSC_VER
#define ZuAlloca(var, n) \
  nullptr; __try { \
    var = reinterpret_cast<decltype(var)>( \
	_alloca(n * sizeof(decltype(*var)))); \
  } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) { \
    _resetstkoflw(); \
    var = nullptr; \
  } (void)0
#else
#ifndef _WIN32
#include <alloca.h>
#endif

#define ZuAlloca(var, n) \
  reinterpret_cast<decltype(var)>(alloca(n * sizeof(decltype(*var))))
#endif

#endif /* ZuLib_HPP */
