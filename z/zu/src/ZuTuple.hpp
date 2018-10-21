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

// generic tuple

#ifndef ZuTuple_HPP
#define ZuTuple_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuTraits.hpp>
#include <ZuPair.hpp>
#include <ZuNull.hpp>
#include <ZuPP.hpp>

#define ZuTuple_TemplateArgDef(I) typename T##I
#define ZuTuple_TemplateArg(I) T##I
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)> struct ZuTuple;
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)> struct ZuTuple_;
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)> struct ZuTuple_T {
  typedef ZuTuple_<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> T;
};

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) ZuNull
#undef ZuPP_CList_3
#define ZuPP_CList_3(L) L(3)
template <typename T1, typename T2>
struct ZuTuple_T<T1, T2, ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)> {
#undef ZuPP_CList_3
#define ZuPP_CList_3(L) ZuPP_CList_2(L), L(3)
  typedef ZuPair<T1, T2> T;
};

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename T##I##_
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)> struct ZuTuple_ :
#undef ZuTuple_TemplateArg
#define ZuTuple_TemplateArg(I) T##I##_
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) L(2)
    public ZuPair<T1_,
      ZuTuple<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N), ZuNull> > {
  typedef ZuTuple<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N), ZuNull> R;
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) ZuPP_CList_1(L), L(2)
  typedef ZuPair<T1_, R> P;

#define ZuTuple_Typedef(I, _) typedef typename ZuDeref<T##I##_>::T T##I;
ZuPP_List1(ZuTuple_Typedef, _, ZuPP_N)

  ZuInline ZuTuple_() { }

  ZuInline ZuTuple_(const ZuTuple_ &t) : P(t) { }
  ZuInline ZuTuple_ &operator =(const ZuTuple_ &t) {
    if (this != &t) P::operator =(t);
    return *this;
  }

  ZuInline ZuTuple_(ZuTuple_ &&t) noexcept : P(static_cast<P &&>(t)) { }
  ZuInline ZuTuple_ &operator =(ZuTuple_ &&t) noexcept {
    P::operator =(static_cast<P &&>(t));
    return *this;
  }

  template <typename T>
  ZuInline ZuTuple_(T &&t, typename ZuNotSame<ZuTuple_, T>::T *_ = 0) :
    P(ZuFwd<T>(t)) { }
  template <typename T>
  ZuInline typename ZuNotSame<ZuTuple_, T, ZuTuple_ &>::T operator =(T &&t) {
    P::operator =(ZuFwd<T>(t));
    return *this;
  }

  template <typename T>
  ZuInline bool operator ==(T &&t) const { return equals(ZuFwd<T>(t)); }
  template <typename T>
  ZuInline bool operator !=(T &&t) const { return !equals(ZuFwd<T>(t)); }
  template <typename T>
  ZuInline bool operator >(T &&t) const { return cmp(ZuFwd<T>(t)) > 0; }
  template <typename T>
  ZuInline bool operator >=(T &&t) const { return cmp(ZuFwd<T>(t)) >= 0; }
  template <typename T>
  ZuInline bool operator <(T &&t) const { return cmp(ZuFwd<T>(t)) < 0; }
  template <typename T>
  ZuInline bool operator <=(T &&t) const { return cmp(ZuFwd<T>(t)) <= 0; }

  template <typename T>
  ZuInline bool equals(T &&t) const { return P::equals(ZuFwd<T>(t)); }

  template <typename T>
  ZuInline int cmp(T &&t) const { return P::cmp(ZuFwd<T>(t)); }

  ZuInline bool operator !() const { return P::operator !(); }
  ZuOpBool

  ZuInline uint32_t hash() const { return P::hash(); }

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename P##I
#define ZuTuple_ArgParam(I)	 P##I &&p##I
#define ZuTuple_Arg(I)	ZuFwd<P##I>(p##I)
#undef ZuPP_List1_2
#define ZuPP_List1_2(L, C) L(2, C)
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) L(2)
#define ZuTuple_Ctor(N, C) \
  template <typename P1, ZuPP_CList(ZuTuple_TemplateArgDef, N)> \
  ZuInline ZuTuple_(P1 &&p1, ZuPP_CList(ZuTuple_ArgParam, N)) : \
      P(ZuFwd<P1>(p1), R(ZuPP_CList(ZuTuple_Arg, N))) { }
ZuPP_List1(ZuTuple_Ctor, _, ZuPP_N)

#undef ZuPP_List1_2
#define ZuPP_List1_2(L, C) ZuPP_List1_1(L, C) L(2, C)
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) ZuPP_CList_1(L), L(2)

#define ZuTuple_Method(N, O) \
  ZuInline const T##O &p##O() const { return P::p2().p##N(); } \
  ZuInline T##O &p##O() { return P::p2().p##N(); } \
  template <typename Q> ZuInline void p##O(Q &&q) { P::p2().p##N(ZuFwd<Q>(q)); }
ZuPP_SList(ZuTuple_Method, ZuPP_N)
};

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename T##I##_ = ZuNull
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) L(2)
template <typename T1_, ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)>
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) ZuPP_CList_1(L), L(2)
#undef ZuTuple_TemplateArg
#define ZuTuple_TemplateArg(I) T##I##_
struct ZuTuple : public ZuTuple_T<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)>::T {
  typedef typename ZuTuple_T<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)>::T T;

#undef ZuTuple_Typedef
#define ZuTuple_Typedef(I, _) typedef T##I##_ T##I;
ZuPP_List1(ZuTuple_Typedef, _, ZuPP_N)

  ZuInline ZuTuple() { }

  ZuInline ZuTuple(const ZuTuple &t) : T(t) { }

  ZuInline ZuTuple(ZuTuple &&t) : T(static_cast<T &&>(t)) { }

  ZuInline ZuTuple &operator =(const ZuTuple &t) {
    if (this != &t) T::operator =(t);
    return *this;
  }

  ZuInline ZuTuple &operator =(ZuTuple &&t) {
    if (this != &t) T::operator =(static_cast<T &&>(t));
    return *this;
  }

  template <typename P> ZuInline ZuTuple(P &&p) : T(ZuFwd<P>(p)) { }

  template <typename P> ZuInline ZuTuple &operator =(P &&p) {
    T::operator =(ZuFwd<P>(p));
    return *this;
  }

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename P##I
#undef ZuPP_List1_2
#define ZuPP_List1_2(L, C) L(2, C)
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) L(2)
#undef ZuTuple_Ctor
#define ZuTuple_Ctor(N, C) \
  template <typename P1, ZuPP_CList(ZuTuple_TemplateArgDef, N)> \
  ZuInline ZuTuple(P1 &&p1, ZuPP_CList(ZuTuple_ArgParam, N)) : \
      T(ZuFwd<P1>(p1), ZuPP_CList(ZuTuple_Arg, N)) { }
ZuPP_List1(ZuTuple_Ctor, _, ZuPP_N)
#undef ZuPP_List1_2
#define ZuPP_List1_2(L, C) ZuPP_List1_1(L, C) L(2, C)
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) ZuPP_CList_1(L), L(2)
};

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename T##I
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)>
#undef ZuTuple_TemplateArg
#define ZuTuple_TemplateArg(I) T##I
struct ZuTraits<ZuTuple<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> > :
    public ZuTraits<
      typename ZuTuple_T<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)>::T> {
  typedef ZuTuple<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> T;
};

#undef ZuTuple_TemplateArgDef
#define ZuTuple_TemplateArgDef(I) typename T##I
template <ZuPP_CList(ZuTuple_TemplateArgDef, ZuPP_N)>
#undef ZuTuple_TemplateArg
#define ZuTuple_TemplateArg(I) T##I
struct ZuTraits<ZuTuple_<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> > :
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) L(2)
    public ZuTraits<
      ZuPair<T1, ZuTuple<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> > > {
#undef ZuPP_CList_2
#define ZuPP_CList_2(L) ZuPP_CList_1(L), L(2)
  typedef ZuTuple_<ZuPP_CList(ZuTuple_TemplateArg, ZuPP_N)> T;
};

template <typename ...Args>
auto ZuInline ZuMkTuple(Args &&... args) {
  return ZuTuple<decltype(ZuFwd<Args>(args))...>{ZuFwd<Args>(args)...};
}

template <typename... Args> ZuTuple(Args...) -> ZuTuple<Args...>;

#endif /* ZuTuple_HPP */
