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

#ifndef ZuField_HPP
#define ZuField_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuLambdaFn.hpp>

template <auto, auto> struct ZuFieldFn_;
template <
  typename U, typename R, typename P,
  R (U::*Get)() const, void (U::*Set)(P)>
struct ZuFieldFn_<Get, Set> {
  using T = typename ZuDecay<R>::T;
  static R get(const void *o) {
    return (static_cast<const U *>(o)->*Get)();
  }
  template <typename V>
  static void set(void *o, V &&v) {
    (static_cast<U *>(o)->*Set)(ZuFwd<V>(v));
  }
};
template <typename U, typename R>
constexpr auto ZuFieldFn_Get(R (U::*fn)() const) noexcept {
  return fn;
}
template <typename U, typename P>
constexpr auto ZuFieldFn_Set(void (U::*fn)(P)) noexcept {
  return fn;
}

template <auto Member> struct ZuFieldData_;
template <typename U, typename M, M U::*Member>
struct ZuFieldData_<Member> {
  using T = typename ZuDecay<decltype(ZuDeclVal<const U &>().*Member)>::T;
  static const auto &get(const void *o) {
    return static_cast<const U *>(o)->*Member;
  }
  template <typename V>
  static void set(void *o, V &&v) {
    static_cast<U *>(o)->*Member = ZuFwd<V>(v);
  }
};

template <typename U, typename ID, typename Accessor>
struct ZuField_ : public ID, public Accessor { };

#define ZuFieldID(T, ID) \
  struct T##_##ID { \
    static constexpr const char *id() { return #ID; } };

#define ZuFieldData(T, ID) \
  ZuField_<T, T##_##ID, ZuFieldData_<&T::ID>>
#define ZuFieldFn(T, ID) \
  ZuField_<T, T##_##ID, \
    ZuFieldFn_<ZuFieldFn_Get(&T::ID), ZuFieldFn_Set(&T::ID)>>

#define ZuField_ID_(T, Type, ID, ...) ZuFieldID(T, ID)
#define ZuField_ID(T, Args) ZuPP_Defer(ZuField_ID_)(T, ZuPP_Strip(Args))

#define ZuField_T_(T, Type, ...) ZuField##Type(T, __VA_ARGS__)
#define ZuField_T(T, Args) ZuPP_Defer(ZuField_T_)(T, ZuPP_Strip(Args))

template <typename T> ZuTypeList<> ZuFields_(T *); // default

#define ZuFields(U, ...) \
  namespace { \
    ZuPP_Eval(ZuPP_MapArg(ZuField_ID, U, __VA_ARGS__)) \
  } \
  ZuTypeList<ZuPP_Eval(ZuPP_MapArgComma(ZuField_T, U, __VA_ARGS__))> \
  ZuFields_(U *);

template <typename T> using ZuFields = decltype(ZuFields_(ZuDeclVal<T *>()));

#endif /* ZuField_HPP */
