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

// generic tuple

#ifndef ZuTuple_HPP
#define ZuTuple_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuPP.hpp>

template <typename ...Args> class ZuTuple;

template <typename T0>
class ZuTuple<T0> {
public:
  using U0 = typename ZuDeref<T0>::T;
  template <unsigned> struct Type;
  template <unsigned> struct Type_;
  template <> struct Type<0> { using T = T0; };
  template <> struct Type_<0> { using T = U0; };
  using Traits = ZuTraits<T0>;
  enum { N = 1 };

  ZuInline ZuTuple() { }

  ZuInline ZuTuple(const ZuTuple &p) : m_p0(p.m_p0) { }

  ZuInline ZuTuple(ZuTuple &&p) : m_p0(static_cast<T0 &&>(p.m_p0)) { }

  ZuInline ZuTuple &operator =(const ZuTuple &p) {
    if (this != &p) m_p0 = p.m_p0;
    return *this;
  }

  ZuInline ZuTuple &operator =(ZuTuple &&p) noexcept {
    m_p0 = static_cast<T0 &&>(p.m_p0);
    return *this;
  }

  template <typename T> ZuInline ZuTuple(T &&v,
      typename ZuIfT<
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuTraits<U0>::T,
			 typename ZuTraits<T>::T>::Is)>::T *_ = 0) :
    m_p0(ZuFwd<T>(v)) { }

  template <typename T> inline
      typename ZuIfT<
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuTraits<U0>::T,
			 typename ZuTraits<T>::T>::Is), ZuTuple &>::T
  operator =(T &&v) noexcept {
    m_p0 = ZuFwd<T>(v);
    return *this;
  }

  template <typename P0>
  ZuInline bool operator ==(const ZuTuple<P0> &p) const { return equals(p); }
  template <typename P0>
  ZuInline bool operator !=(const ZuTuple<P0> &p) const { return !equals(p); }
  template <typename P0>
  ZuInline bool operator >(const ZuTuple<P0> &p) const { return cmp(p) > 0; }
  template <typename P0>
  ZuInline bool operator >=(const ZuTuple<P0> &p) const { return cmp(p) >= 0; }
  template <typename P0>
  ZuInline bool operator <(const ZuTuple<P0> &p) const { return cmp(p) < 0; }
  template <typename P0>
  ZuInline bool operator <=(const ZuTuple<P0> &p) const { return cmp(p) <= 0; }

  template <typename P0>
  ZuInline bool equals(const ZuTuple<P0> &p) const {
    return ZuCmp<T0>::equals(m_p0, p.template p<0>());
  }

  template <typename P0>
  ZuInline int cmp(const ZuTuple<P0> &p) const {
    return ZuCmp<T0>::cmp(m_p0, p.template p<0>());
  }

  ZuInline bool operator !() const { return !m_p0; }
  ZuOpBool

  ZuInline uint32_t hash() const {
    return ZuHash<T0>::hash(m_p0);
  }

  template <unsigned I>
  ZuInline typename ZuIfT<!I, const U0 &>::T p() const {
    return m_p0;
  }
  template <unsigned I>
  ZuInline typename ZuIfT<!I, U0 &>::T p() {
    return m_p0;
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<!I, ZuTuple &>::T p(P &&p) {
    m_p0 = ZuFwd<P>(p);
    return *this;
  }

private:
  T0		m_p0;
};

template <typename T0, typename T1>
class ZuTuple<T0, T1> : public ZuPair<T0, T1> {
  using Pair = ZuPair<T0, T1>;

public:
  template <unsigned I> struct Type : public Pair::template Type<I> { };
  template <unsigned I> struct Type_ : public Pair::template Type_<I> { };
  using Traits = ZuTraits<Pair>;
  enum { N = 2 };

  using Pair::Pair;
  using Pair::operator =;

  ZuInline ZuTuple(const Pair &v) : Pair(v) { };
  ZuInline ZuTuple(Pair &&v) : Pair(ZuMv(v)) { };

  template <unsigned I>
  ZuInline const typename Type_<I>::T &p() const {
    return Pair::template p<I>();
  }
  template <unsigned I>
  ZuInline typename Type_<I>::T &p() {
    return Pair::template p<I>();
  }
  template <unsigned I, typename P>
  ZuInline ZuTuple &p(P &&p) {
    Pair::template p<I>(ZuFwd<P>(p));
    return *this;
  }
};

template <typename T0, typename T1, typename ...Args>
class ZuTuple<T0, T1, Args...> : public ZuPair<T0, ZuTuple<T1, Args...>> {
  using Left = T0;
  using Right = ZuTuple<T1, Args...>;
  using Pair = ZuPair<Left, Right>;

public:
  template <unsigned I> struct Type : public Right::template Type<I - 1> { };
  template <unsigned I> struct Type_ : public Right::template Type_<I - 1> { };
  template <> struct Type<0> : public Pair::template Type<0> { };
  template <> struct Type_<0> : public Pair::template Type_<0> { };
  using Traits = ZuTraits<Pair>;
  enum { N = Right::N + 1 };

  using Pair::Pair;
  using Pair::operator =;

  ZuInline ZuTuple(const Pair &v) : Pair(v) { };
  ZuInline ZuTuple(Pair &&v) : Pair(ZuMv(v)) { };

  template <typename P0, typename P1, typename ...Args_>
  ZuInline ZuTuple(P0 &&p0, P1 &&p1, Args_ &&... args) :
      Pair{ZuFwd<P0>(p0), Right{ZuFwd<P1>(p1), ZuFwd<Args_>(args)...}} { }

  template <unsigned I>
  ZuInline typename ZuIfT<!I, const typename Type_<I>::T &>::T p() const {
    return Pair::template p<0>();
  }
  template <unsigned I>
  ZuInline typename ZuIfT<!I, typename Type_<I>::T &>::T p() {
    return Pair::template p<0>();
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<!I, ZuTuple &>::T p(P &&p) {
    Pair::template p<0>(ZuFwd<P>(p));
    return *this;
  }

  template <unsigned I>
  ZuInline typename ZuIfT<!!I, const typename Type_<I>::T &>::T p() const {
    return Pair::template p<1>().template p<I - 1>();
  }
  template <unsigned I>
  ZuInline typename ZuIfT<!!I, typename Type_<I>::T &>::T p() {
    return Pair::template p<1>().template p<I - 1>();
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<!!I, ZuTuple &>::T p(P &&p) {
    Pair::template p<1>().template p<I - 1>(ZuFwd<P>(p));
    return *this;
  }
};

template <typename ...Args>
struct ZuTraits<ZuTuple<Args...>> : public ZuTuple<Args...>::Traits {
  using T = ZuTuple<Args...>;
};

template <typename ...Args>
auto ZuInline ZuMkTuple(Args &&... args) {
  return ZuTuple<Args...>{ZuFwd<Args>(args)...};
}

template <typename... Args> ZuTuple(Args...) -> ZuTuple<Args...>;

// ZuDeclTuple(Type, (Type0, Fn0), (Type1, Fn1), ...) creates
// a ZuTuple<Type0, ...> with named member functions Fn0, Fn1, etc.
// that alias p<0>, p<1>, etc.
//
// ZuDeclTuple(Person, (ZtString, name), (unsigned, age), (bool, gender));
// Person p = Person().name("Fred").age(1).gender(1);
// p.age() = 42;
// std::cout << p.name() << '\n';

#define ZuTuple_FieldType(args) \
  ZuPP_Defer(ZuTuple_FieldType_)()(ZuPP_Strip(args))
#define ZuTuple_FieldType_() ZuTuple_FieldType__
#define ZuTuple_FieldType__(type, fn) ZuPP_Strip(type)

#define ZuTuple_FieldFn(N, args) \
  ZuPP_Defer(ZuTuple_FieldFn_)()(N, ZuPP_Strip(args))
#define ZuTuple_FieldFn_() ZuTuple_FieldFn__
#define ZuTuple_FieldFn__(N, type, fn) \
  ZuInline const auto &fn() const { return this->template p<N>(); } \
  ZuInline auto &fn() { return this->template p<N>(); } \
  template <typename P> \
  ZuInline auto &fn(P &&v) { this->template p<N>(ZuFwd<P>(v)); return *this; }

#define ZuDeclTuple(Type, ...) \
using Type##_ = \
  ZuTuple<ZuPP_Eval(ZuPP_MapComma(ZuTuple_FieldType, __VA_ARGS__))>; \
class Type : public Type##_ { \
  using Tuple = Type##_; \
public: \
  using Tuple::Tuple; \
  using Tuple::operator =; \
  ZuInline Type(const Tuple &v) : Tuple(v) { }; \
  ZuInline Type(Tuple &&v) : Tuple(ZuMv(v)) { }; \
  ZuPP_Eval(ZuPP_MapIndex(ZuTuple_FieldFn, 0, __VA_ARGS__)) \
  struct Traits : public ZuTraits<Type##_> { using T = Type; }; \
  friend Traits ZuTraitsType(const Type *); \
}

#endif /* ZuTuple_HPP */
