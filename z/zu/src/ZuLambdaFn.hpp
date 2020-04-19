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

// compile-time stateless lambda <-> function pointer conversion

#ifndef ZuLambdaFn_HPP
#define ZuLambdaFn_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuConversion.hpp>

template <bool, typename, typename, typename ...> struct ZuLambdaFn__;
template <typename L, typename R_, typename ...Args>
struct ZuLambdaFn__<true, L, R_, Args...> {
  using R = R_;
  typedef R (*Fn)(Args...);
  template <typename ...Args_>
  static R invoke(Args_ &&... args) {
    return (*reinterpret_cast<const L *>(0))(ZuFwd<Args_>(args)...);
  }
  static R invoke_(Args... args) {
    return (*reinterpret_cast<const L *>(0))(ZuFwd<Args>(args)...);
  }
};
template <typename L, typename ...Args>
struct ZuLambdaFn__<true, L, void, Args...> {
  using R = void;
  typedef void (*Fn)(Args...);
  template <typename ...Args_>
  static void invoke(Args_ &&... args) {
    (*reinterpret_cast<const L *>(0))(ZuFwd<Args_>(args)...);
  }
  static void invoke_(Args... args) {
    (*reinterpret_cast<const L *>(0))(ZuFwd<Args>(args)...);
  }
};

template <typename L, typename R, typename ...Args>
struct ZuLambdaFn_Stateless {
  typedef R (*Fn)(Args...);
  enum { OK = ZuConversion<L, Fn>::Exists };
};

template <auto> struct ZuLambdaFn_;
template <typename L, typename R, typename ...Args, R (L::*Fn)(Args...) const>
struct ZuLambdaFn_<Fn> : public ZuLambdaFn__<
    ZuLambdaFn_Stateless<L, R, Args...>::OK, L, R, Args...> { };

template <typename L>
struct ZuLambdaFn : public ZuLambdaFn_<&L::operator()> {
  using Fn = typename ZuLambdaFn_<&L::operator()>::Fn;
  static constexpr Fn fn(L l) { return static_cast<Fn>(l); }
  static constexpr Fn fn() { return &ZuLambdaFn_<&L::operator()>::invoke_; }
};

#endif /* ZuLambdaFn_HPP */
