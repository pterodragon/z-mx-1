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

template <auto Fn> struct ZuLambdaFn__;
template <
  typename L, typename R_, typename ...Args, R_ (L::*Fn_)(Args...) const>
struct ZuLambdaFn__<Fn_> {
  using R = R_;
  typedef R (*Fn)(Args...);
  enum { OK = ZuConversion<L, Fn>::Exists };
};
template <typename L>
struct ZuLambdaFn_ : public ZuLambdaFn__<&L::operator()> { };
template <typename L, bool OK = ZuLambdaFn_<L>::OK> struct ZuLambdaFn;
template <typename L, typename R> struct ZuLambdaFn_Invoke {
  template <typename ...Args>
  static R invoke(Args &&... args) {
    return (*(const L *)(void *)0)(ZuFwd<Args>(args)...);
  }
};
template <typename L> struct ZuLambdaFn_Invoke<L, void> {
  template <typename ...Args>
  static void invoke(Args &&... args) {
    (*(const L *)(void *)0)(ZuFwd<Args>(args)...);
  }
};
template <typename L>
struct ZuLambdaFn<L, true> :
    public ZuLambdaFn_Invoke<L, typename ZuLambdaFn_<L>::R> {
  using Fn = typename ZuLambdaFn_<L>::Fn;
  static constexpr auto fn(L l) { return static_cast<Fn>(l); }
};

#endif /* ZuLambdaFn_HPP */
