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
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuSwitch.hpp>
#include <zlib/ZuPP.hpp>

namespace Zu_ {
  template <typename ...Args> class Tuple;
}
template <typename ...Args> using ZuTuple = Zu_::Tuple<Args...>;

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

namespace Zu_ {
template <typename T0_>
class Tuple<T0_> : public ZuTuple1_ {
public:
  using T0 = T0_;
  using U0 = typename ZuDeref<T0>::T;
  template <unsigned I> using Type = ZuTuple_Type0<I, T0>;
  template <unsigned I> using Type_ = ZuTuple_Type0_<I, U0>;
  using Traits = ZuTraits<T0>;
  enum { N = 1 };

  template <typename T>
  using Index = ZuTypeIndex<T, T0>;

  Tuple() = default;
  Tuple(const Tuple &) = default;
  Tuple &operator =(const Tuple &) = default;
  Tuple(Tuple &&) = default;
  Tuple &operator =(Tuple &&) = default;
  ~Tuple() = default;

private:
  template <typename T, typename> struct Bind_P0 {
    using U0 = typename T::U0;
    ZuInline static const U0 &p0(const T &v) { return v.m_p0; }
    ZuInline static U0 &&p0(T &&v) { return static_cast<U0 &&>(v.m_p0); }
  };
  template <typename T, typename P0> struct Bind_P0<T, P0 &> {
    using U0 = typename T::U0;
    ZuInline static U0 &p0(T &v) { return v.m_p0; }
  };
  template <typename T, typename P0> struct Bind_P0<T, const P0 &> {
    using U0 = typename T::U0;
    ZuInline static const U0 &p0(const T &v) { return v.m_p0; }
  };
  template <typename T, typename P0> struct Bind_P0<T, volatile P0 &> {
    using U0 = typename T::U0;
    ZuInline static volatile U0 &p0(volatile T &v) { return v.m_p0; }
  };
  template <typename T, typename P0> struct Bind_P0<T, const volatile P0 &> {
    using U0 = typename T::U0;
    ZuInline static const volatile U0 &p0(const volatile T &v) {
      return v.m_p0;
    }
  };
  template <typename T>
  struct Bind : public Bind_P0<T, T0> { };

public:
  template <typename T>
  ZuInline Tuple(T &&v, typename ZuIfT<
	ZuTuple1_Cvt<typename ZuDecay<T>::T, Tuple>::OK
      >::T *_ = 0) :
    m_p0(Bind<typename ZuDecay<T>::T>::p0(ZuFwd<T>(v))) { }

  template <typename T>
  ZuInline Tuple(T &&v, typename ZuIfT<
      !ZuTuple1_Cvt<typename ZuDecay<T>::T, Tuple>::OK &&
	(!ZuTraits<T0>::IsReference ||
	  ZuConversion<typename ZuDecay<U0>::T, typename ZuDecay<T>::T>::Is)
      >::T *_ = 0) :
    m_p0(ZuFwd<T>(v)) { }

  template <typename T> ZuInline
  typename ZuIfT<ZuTuple1_Cvt<typename ZuDecay<T>::T, Tuple>::OK, Tuple &>::T
  operator =(T &&v) {
    m_p0 = Bind<typename ZuDecay<T>::T>::p0(ZuFwd<T>(v));
    return *this;
  }

  template <typename T>
  ZuInline
  typename ZuIfT<
    !ZuTuple1_Cvt<typename ZuDecay<T>::T, Tuple>::OK &&
    (!ZuTraits<T0>::IsReference ||
      ZuConversion<typename ZuDecay<U0>::T, typename ZuDecay<T>::T>::Is),
    Tuple &
  >::T operator =(T &&v) {
    m_p0 = ZuFwd<T>(v);
    return *this;
  }

  template <typename P0>
  ZuInline int cmp(const Tuple<P0> &p) const {
    return ZuCmp<T0>::cmp(m_p0, p.template p<0>());
  }
  template <typename P0>
  ZuInline bool less(const Tuple<P0> &p) const {
    return ZuCmp<T0>::less(m_p0, p.template p<0>());
  }
  template <typename P0>
  ZuInline bool equals(const Tuple<P0> &p) const {
    return ZuCmp<T0>::equals(m_p0, p.template p<0>());
  }

  ZuInline bool operator ==(const Tuple &p) const { return equals(p); }
  ZuInline bool operator !=(const Tuple &p) const { return !equals(p); }
  ZuInline bool operator >(const Tuple &p) const { return p.less(*this); }
  ZuInline bool operator >=(const Tuple &p) const { return !less(p); }
  ZuInline bool operator <(const Tuple &p) const { return less(p); }
  ZuInline bool operator <=(const Tuple &p) const { return !p.less(*this); }

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
  ZuInline typename ZuIfT<!I, Tuple &>::T p(P &&p) {
    m_p0 = ZuFwd<P>(p);
    return *this;
  }

  template <typename T>
  ZuInline typename ZuIfT<ZuConversion<T, T0>::Same, const U0 &>::T v() const {
    return m_p0;
  }
  template <typename T>
  ZuInline typename ZuIfT<ZuConversion<T, T0>::Same, U0 &>::T v() {
    return m_p0;
  }
  template <typename T, typename P>
  ZuInline typename ZuIfT<ZuConversion<T, T0>::Same, Tuple &>::T v(P &&p) {
    m_p0 = ZuFwd<P>(p);
    return *this;
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
class Tuple<T0, T1> : public Pair<T0, T1> {
  using Base = Pair<T0, T1>;

public:
  template <unsigned I> struct Type : public Base::template Type<I> { };
  template <unsigned I> struct Type_ : public Base::template Type_<I> { };
  using Traits = ZuTraits<Base>;
  enum { N = 2 };

  template <typename T>
  using Index = ZuTypeIndex<T, T0, T1>;

  Tuple() = default;
  Tuple(const Tuple &) = default;
  Tuple &operator =(const Tuple &) = default;
  Tuple(Tuple &&) = default;
  Tuple &operator =(Tuple &&) = default;
  ~Tuple() = default;

  template <typename T> ZuInline Tuple(T &&v) : Base{ZuFwd<T>(v)} { }

  template <typename T> ZuInline Tuple &operator =(T &&v) noexcept {
    Base::assign(ZuFwd<T>(v));
    return *this;
  }

  template <typename P0, typename P1>
  ZuInline Tuple(P0 &&p0, P1 &&p1) : Base{ZuFwd<P0>(p0), ZuFwd<P1>(p1)} { }

  template <unsigned I>
  ZuInline const typename Type_<I>::T &p() const {
    return Base::template p<I>();
  }
  template <unsigned I>
  ZuInline typename Type_<I>::T &p() {
    return Base::template p<I>();
  }
  template <unsigned I, typename P>
  ZuInline Tuple &p(P &&p) {
    Base::template p<I>(ZuFwd<P>(p));
    return *this;
  }

  template <typename T>
  ZuInline const typename Type<Index<T>::I>::T &v() const {
    return p<Index<T>::I>();
  }
  template <typename T>
  ZuInline typename Type<Index<T>::I>::T &v() {
    return p<Index<T>::I>();
  }
  template <typename T, typename P>
  ZuInline Tuple &v(P &&p) {
    return p<Index<T>::I>(ZuFwd<P>(p));
  }

  template <typename L>
  auto dispatch(unsigned i, L l) {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(this->p<i>());
	});
  }
  template <typename L>
  auto cdispatch(unsigned i, L l) const {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(this->p<i>());
	});
  }
};
} // namespace Zu_

template <unsigned I, typename Left, typename Right>
struct ZuTuple_Type : public Right::template Type<I - 1> { };
template <unsigned I, typename Left, typename Right>
struct ZuTuple_Type_ : public Right::template Type_<I - 1> { };
template <typename Left, typename Right>
struct ZuTuple_Type<0, Left, Right> { using T = Left; };
template <typename Left, typename Right>
struct ZuTuple_Type_<0, Left, Right> { using T = typename ZuDeref<Left>::T; };

namespace Zu_ {
template <typename T0, typename T1, typename ...Args>
class Tuple<T0, T1, Args...> : public Pair<T0, Tuple<T1, Args...>> {
  using Left = T0;
  using Right = Tuple<T1, Args...>;
  using Base = Pair<Left, Right>;

public:
  template <unsigned I> using Type = ZuTuple_Type<I, Left, Right>;
  template <unsigned I> using Type_ = ZuTuple_Type_<I, Left, Right>;
  using Traits = ZuTraits<Base>;
  enum { N = Right::N + 1 };

  template <typename T>
  using Index = ZuTypeIndex<T, T0, T1, Args...>;

  Tuple() = default;
  Tuple(const Tuple &) = default;
  Tuple &operator =(const Tuple &) = default;
  Tuple(Tuple &&) = default;
  Tuple &operator =(Tuple &&) = default;
  ~Tuple() = default;

  template <typename T> ZuInline Tuple(T &&v) : Base{ZuFwd<T>(v)} { }

  template <typename T> ZuInline Tuple &operator =(T &&v) noexcept {
    Base::assign(ZuFwd<T>(v));
    return *this;
  }

  template <typename P0, typename P1, typename ...Args_>
  ZuInline Tuple(P0 &&p0, P1 &&p1, Args_ &&... args) :
      Base{ZuFwd<P0>(p0), Right{ZuFwd<P1>(p1), ZuFwd<Args_>(args)...}} { }

  template <unsigned I>
  ZuInline typename ZuIfT<!I, const typename Type_<I>::T &>::T p() const {
    return Base::template p<0>();
  }
  template <unsigned I>
  ZuInline typename ZuIfT<!I, typename Type_<I>::T &>::T p() {
    return Base::template p<0>();
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<!I, Tuple &>::T p(P &&p) {
    Base::template p<0>(ZuFwd<P>(p));
    return *this;
  }

  template <unsigned I>
  ZuInline
  typename ZuIfT<(I && I < N), const typename Type_<I>::T &>::T p() const {
    return Base::template p<1>().template p<I - 1>();
  }
  template <unsigned I>
  ZuInline typename ZuIfT<(I && I < N), typename Type_<I>::T &>::T p() {
    return Base::template p<1>().template p<I - 1>();
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<(I && I < N), Tuple &>::T p(P &&p) {
    Base::template p<1>().template p<I - 1>(ZuFwd<P>(p));
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
	  return l(this->p<i>());
	});
  }
  template <typename L>
  auto cdispatch(unsigned i, L l) const {
    return ZuSwitch::dispatch<N>(
	i, [this, &l](auto i) mutable {
	  return l(this->p<i>());
	});
  }
};
} // namespace Zu_

template <typename ...Args>
struct ZuTraits<ZuTuple<Args...>> : public ZuTuple<Args...>::Traits {
  using T = ZuTuple<Args...>;
};

template <typename ...Args>
auto ZuInline ZuFwdTuple(Args &&... args) {
  return ZuTuple<Args &&...>{ZuFwd<Args>(args)...};
}

template <typename ...Args>
auto ZuInline ZuMvTuple(Args... args) {
  return ZuTuple<Args...>{ZuMv(args)...};
}

namespace Zu_ {
  template <typename ...Args> Tuple(Args...) -> Tuple<Args...>;
}

// STL structured binding cruft
#include <type_traits>
namespace std {
  template <class> struct tuple_size;
  template <typename ...Args>
  struct tuple_size<ZuTuple<Args...>> :
  public integral_constant<size_t, sizeof...(Args)> { };

  template <size_t, typename> struct tuple_element;
  template <size_t I, typename ...Args>
  struct tuple_element<I, ZuTuple<Args...>> {
    using type = typename ZuTuple<Args...>::template Type<I>::T;
  };
}
namespace Zu_ {
  using size_t = std::size_t;
  namespace {
    template <size_t I, typename T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;
  }
  template <size_t I, typename ...Args>
  constexpr tuple_element_t<I, Tuple<Args...>> &
  get(Tuple<Args...> &p) noexcept { return p.template p<I>(); }
  template <size_t I, typename ...Args>
  constexpr const tuple_element_t<I, Tuple<Args...>> &
  get(const Tuple<Args...> &p) noexcept { return p.template p<I>(); }
  template <size_t I, typename ...Args>
  constexpr tuple_element_t<I, Tuple<Args...>> &&
  get(Tuple<Args...> &&p) noexcept {
    return static_cast<tuple_element_t<I, Tuple<Args...>> &&>(
	p.template p<I>());
  }
  template <size_t I, typename ...Args>
  constexpr const tuple_element_t<I, Tuple<Args...>> &&
  get(const Tuple<Args...> &&p) noexcept {
    return static_cast<const tuple_element_t<I, Tuple<Args...>> &&>(
	p.template p<I>());
  }

  template <typename T, typename ...Args>
  constexpr T &get(Tuple<Args...> &p) noexcept {
    return p.template v<T>();
  }
  template <typename T, typename ...Args>
  constexpr const T &get(const Tuple<Args...> &p) noexcept {
    return p.template v<T>();
  }
  template <typename T, typename ...Args>
  constexpr T &&get(Tuple<Args...> &&p) noexcept {
    return static_cast<T &&>(p.template v<T>());
  }
  template <typename T, typename ...Args>
  constexpr const T &&get(const Tuple<Args...> &&p) noexcept {
    return static_cast<const T &&>(p.template v<T>());
  }
} // namespace Zu_

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
  Type() = default; \
  Type(const Type &) = default; \
  Type &operator =(const Type &) = default; \
  Type(Type &&) = default; \
  Type &operator =(Type &&) = default; \
  ~Type() = default; \
  template <typename ...Args> \
  Type(Args &&... args) : Tuple{ZuFwd<Args>(args)...} { } \
  template <typename ...Args> \
  Type &operator =(ZuTuple<Args...> &&v) { \
    Tuple::operator =(ZuFwd<ZuTuple<Args...>>(v)); \
    return *this; \
  } \
  ZuPP_Eval(ZuPP_MapIndex(ZuTuple_FieldFn, 0, __VA_ARGS__)) \
  struct Traits : public ZuTraits<Type##_> { using T = Type; }; \
  friend Traits ZuTraitsType(const Type *); \
}

#endif /* ZuTuple_HPP */
