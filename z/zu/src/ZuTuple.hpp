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
#include <zlib/ZuSwitch.hpp>
#include <zlib/ZuPP.hpp>

template <typename ...Args> struct ZuTuple_Index_ { };
template <typename, typename> struct ZuTuple_Index;
template <typename T, typename ...Args>
struct ZuTuple_Index<T, ZuTuple_Index_<T, Args...>> {
  enum { I = 0 };
};
template <typename T, typename O, typename ...Args>
struct ZuTuple_Index<T, ZuTuple_Index_<O, Args...>> {
  enum { I = 1 + ZuTuple_Index<T, ZuTuple_Index_<Args...>>::I };
};

template <typename ...Args> class ZuTuple;

struct ZuTuple1_ { }; // tuple containing single value

template <typename T, typename P, bool IsTuple0> struct ZuTuple1_Cvt_;
template <typename T, typename P> struct ZuTuple1_Cvt_<T, P, 0> {
  enum { OK = 0 };
};
template <typename T, typename P> struct ZuTuple1_Cvt_<T, P, 1> {
  enum {
    OK = ZuConversion<typename T::T0, typename P::T0>::Exists
  };
};
template <typename T, typename P> struct ZuTuple1_Cvt :
  public ZuTuple1_Cvt_<typename ZuDeref<T>::T, P,
    ZuConversion<ZuTuple1_, typename ZuDeref<T>::T>::Base> { };

template <unsigned, typename> struct ZuTuple_Type0;
template <typename Left>
struct ZuTuple_Type0<0, Left> { using T = Left; };
template <unsigned, typename> struct ZuTuple_Type0_;
template <typename Left_>
struct ZuTuple_Type0_<0, Left_> { using T = Left_; };

template <typename U0> struct ZuTuple1_Print_ {
  ZuTuple1_Print_() = delete;
  ZuTuple1_Print_(const ZuTuple1_Print_ &) = delete;
  ZuTuple1_Print_ &operator =(const ZuTuple1_Print_ &) = delete;
  ZuTuple1_Print_(ZuTuple1_Print_ &&) = delete;
  ZuTuple1_Print_ &operator =(ZuTuple1_Print_ &&) = delete;
  ZuTuple1_Print_(const U0 &p0_, const ZuString &delim_) :
      p0{p0_}, delim{delim_} { }
  const U0		&p0;
  const ZuString	&delim;
};
template <typename U0, bool Nested>
struct ZuTuple1_Print;
template <typename U0>
struct ZuTuple1_Print<U0, false> :
  public ZuTuple1_Print_<U0>, public ZuPrintable {
  ZuTuple1_Print(const U0 &p0, const ZuString &delim) :
      ZuTuple1_Print_<U0>{p0, delim} { }
  template <typename S> void print(S &s) const {
    s << this->p0;
  }
};
template <typename U0>
struct ZuTuple1_Print<U0, true> :
  public ZuTuple1_Print_<U0>, public ZuPrintable {
  ZuTuple1_Print(const U0 &p0, const ZuString &delim) :
      ZuTuple1_Print_<U0>{p0, delim} { }
  template <typename S> void print(S &s) const {
    s << this->p0.print(this->delim);
  }
};

template <typename T0>
class ZuTuple<T0> : public ZuTuple1_ {
public:
  using U0 = typename ZuDeref<T0>::T;
  template <unsigned I> using Type = ZuTuple_Type0<I, T0>;
  template <unsigned I> using Type_ = ZuTuple_Type0_<I, U0>;
  using Traits = ZuTraits<T0>;
  enum { N = 1 };

  template <typename T>
  using Index = ZuTuple_Index<T, ZuTuple_Index_<T0>>;

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

  template <typename T>
  ZuInline explicit ZuTuple(T p,
      typename ZuIfT<ZuTuple1_Cvt<T, ZuTuple>::OK>::T *_ = 0) :
    m_p0(ZuMv(p.m_p0)) { }

  template <typename T> ZuInline ZuTuple(T &&v,
      typename ZuIfT<
	!ZuTuple1_Cvt<typename ZuDecay<T>::T, ZuTuple>::OK &&
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuDecay<U0>::T,
			 typename ZuDecay<T>::T>::Is)>::T *_ = 0) :
    m_p0(ZuFwd<T>(v)) { }

  template <typename T> ZuInline
  typename ZuIfT<ZuTuple1_Cvt<T, ZuTuple>::OK, ZuTuple &>::T
  operator =(T p) {
    m_p0 = ZuMv(p.m_p0);
    return *this;
  }

  template <typename T> ZuInline
      typename ZuIfT<
	!ZuTuple1_Cvt<typename ZuDecay<T>::T, ZuTuple>::OK &&
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuDecay<U0>::T,
			 typename ZuDecay<T>::T>::Is), ZuTuple &>::T
  operator =(T &&v) {
    m_p0 = ZuFwd<T>(v);
    return *this;
  }

  ZuInline bool operator ==(const ZuTuple &p) const { return equals(p); }
  ZuInline bool operator !=(const ZuTuple &p) const { return !equals(p); }
  ZuInline bool operator >(const ZuTuple &p) const { return cmp(p) > 0; }
  ZuInline bool operator >=(const ZuTuple &p) const { return cmp(p) >= 0; }
  ZuInline bool operator <(const ZuTuple &p) const { return cmp(p) < 0; }
  ZuInline bool operator <=(const ZuTuple &p) const { return cmp(p) <= 0; }

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

  template <typename T>
  ZuInline auto v() const {
    return p<Index<T>::I>();
  }
  template <typename T>
  ZuInline auto v() {
    return p<Index<T>::I>();
  }
  template <typename T, typename P>
  ZuInline auto v(P &&p) {
    return p<Index<T>::I>(ZuFwd<P>(p));
  }

  using Print = ZuTuple1_Print<U0, ZuConversion<ZuPair_, U0>::Base>;
  ZuInline Print print() const {
    return Print{m_p0, "|"};
  }
  ZuInline Print print(const ZuString &delim) const {
    return Print{m_p0, delim};
  }

  template <typename L>
  auto dispatch(unsigned i, L l) {
    if (i) return decltype(l(m_p0)){};
    return l(m_p0);
  }
  template <typename L>
  auto cdispatch(unsigned i, L l) const {
    if (i) return decltype(l(m_p0)){};
    return l(m_p0);
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

  template <typename T>
  using Index = ZuTuple_Index<T, ZuTuple_Index_<T0, T1>>;

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

  template <typename T>
  ZuInline auto v() const {
    return p<Index<T>::I>();
  }
  template <typename T>
  ZuInline auto v() {
    return p<Index<T>::I>();
  }
  template <typename T, typename P>
  ZuInline auto v(P &&p) {
    return p<Index<T>::I>(ZuFwd<P>(p));
  }

  template <typename L>
  auto dispatch(unsigned i, L l) {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(p<i>());
	});
  }
  template <typename L>
  auto cdispatch(unsigned i, L l) const {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(p<i>());
	});
  }
};

template <unsigned I, typename Left, typename Right>
struct ZuTuple_Type : public Right::template Type<I - 1> { };
template <unsigned I, typename Left, typename Right>
struct ZuTuple_Type_ : public Right::template Type_<I - 1> { };
template <typename Left, typename Right>
struct ZuTuple_Type<0, Left, Right> { using T = Left; };
template <typename Left, typename Right>
struct ZuTuple_Type_<0, Left, Right> { using T = typename ZuDeref<Left>::T; };

template <typename T0, typename T1, typename ...Args>
class ZuTuple<T0, T1, Args...> : public ZuPair<T0, ZuTuple<T1, Args...>> {
  using Left = T0;
  using Right = ZuTuple<T1, Args...>;
  using Pair = ZuPair<Left, Right>;

public:
  template <unsigned I> using Type = ZuTuple_Type<I, Left, Right>;
  template <unsigned I> using Type_ = ZuTuple_Type_<I, Left, Right>;
  using Traits = ZuTraits<Pair>;
  enum { N = Right::N + 1 };

  template <typename T>
  using Index = ZuTuple_Index<T, ZuTuple_Index_<T0, T1, Args...>>;

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
  ZuInline
  typename ZuIfT<(I && I < N), const typename Type_<I>::T &>::T p() const {
    return Pair::template p<1>().template p<I - 1>();
  }
  template <unsigned I>
  ZuInline typename ZuIfT<(I && I < N), typename Type_<I>::T &>::T p() {
    return Pair::template p<1>().template p<I - 1>();
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<(I && I < N), ZuTuple &>::T p(P &&p) {
    Pair::template p<1>().template p<I - 1>(ZuFwd<P>(p));
    return *this;
  }

  template <typename T>
  ZuInline auto v() const {
    return p<Index<T>::I>();
  }
  template <typename T>
  ZuInline auto v() {
    return p<Index<T>::I>();
  }
  template <typename T, typename P>
  ZuInline auto v(P &&p) {
    return p<Index<T>::I>(ZuFwd<P>(p));
  }

  template <typename L>
  auto dispatch(unsigned i, L l) {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(p<i>());
	});
  }
  template <typename L>
  auto cdispatch(unsigned i, L l) const {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(p<i>());
	});
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
