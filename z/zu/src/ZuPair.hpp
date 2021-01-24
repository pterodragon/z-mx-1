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

// generic pair

#ifndef ZuPair_HPP
#define ZuPair_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuIfT.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuString.hpp>

struct ZuPair_ { };

template <typename T, typename P, bool IsPair> struct ZuPair_Cvt_;
template <typename T, typename P> struct ZuPair_Cvt_<T, P, 0> {
  enum { OK = 0 };
};
template <typename T, typename P> struct ZuPair_Cvt_<T, P, 1> {
  enum {
    OK = ZuConversion<typename T::T0, typename P::T0>::Exists &&
	 ZuConversion<typename T::T1, typename P::T1>::Exists
  };
};
template <typename T, typename P> struct ZuPair_Cvt :
  public ZuPair_Cvt_<typename ZuDecay<T>::T, P,
    ZuConversion<ZuPair_, typename ZuDecay<T>::T>::Base> { };

// fwd-declare the traits
namespace Zu_ {
  template <typename T0, typename T1> class Pair;
}
template <typename T0, typename T1>
using ZuPair = Zu_::Pair<T0, T1>;
template <typename T0, typename T1>
struct ZuTraits<ZuPair<T0, T1>> : public ZuBaseTraits<ZuPair<T0, T1>> {
  enum {
    IsPOD = ZuTraits<T0>::IsPOD && ZuTraits<T1>::IsPOD,
    IsComparable = 1,
    IsHashable = 1
  };
};

template <unsigned, typename, typename> struct ZuPair_Type;
template <typename T0, typename T1>
struct ZuPair_Type<0, T0, T1> { using T = T0; };
template <typename T0, typename T1>
struct ZuPair_Type<1, T0, T1> { using T = T1; };
template <unsigned, typename, typename> struct ZuPair_Type_;
template <typename U0, typename U1>
struct ZuPair_Type_<0, U0, U1> { using T = U0; };
template <typename U0, typename U1>
struct ZuPair_Type_<1, U0, U1> { using T = U1; };

template <typename U0, typename U1> struct ZuPair_Print_ {
  ZuPair_Print_() = delete;
  ZuPair_Print_(const ZuPair_Print_ &) = delete;
  ZuPair_Print_ &operator =(const ZuPair_Print_ &) = delete;
  ZuPair_Print_(ZuPair_Print_ &&) = delete;
  ZuPair_Print_ &operator =(ZuPair_Print_ &&) = delete;
  ZuPair_Print_(const U0 &p0_, const U1 &p1_, const ZuString &delim_) :
      p0{p0_}, p1{p1_}, delim{delim_} { }
  const U0	&p0;
  const U1	&p1;
  ZuString	delim;
};
template <typename U0, typename U1, bool LeftPair, bool RightPair>
struct ZuPair_Print;
template <typename U0, typename U1>
struct ZuPair_Print<U0, U1, false, false> :
  public ZuPair_Print_<U0, U1>, public ZuPrintable {
  ZuPair_Print(const U0 &p0, const U1 &p1, const ZuString &delim) :
      ZuPair_Print_<U0, U1>{p0, p1, delim} { }
  template <typename S> void print(S &s) const {
    s << this->p0 << this->delim << this->p1;
  }
};
template <typename U0, typename U1>
struct ZuPair_Print<U0, U1, false, true> :
  public ZuPair_Print_<U0, U1>, public ZuPrintable {
  ZuPair_Print(const U0 &p0, const U1 &p1, const ZuString &delim) :
      ZuPair_Print_<U0, U1>{p0, p1, delim} { }
  template <typename S> void print(S &s) const {
    s << this->p0 << this->delim << this->p1.print(this->delim);
  }
};
template <typename U0, typename U1>
struct ZuPair_Print<U0, U1, true, false> :
  public ZuPair_Print_<U0, U1>, public ZuPrintable {
  ZuPair_Print(const U0 &p0, const U1 &p1, const ZuString &delim) :
      ZuPair_Print_<U0, U1>{p0, p1, delim} { }
  template <typename S> void print(S &s) const {
    s << this->p0.print(this->delim) << this->delim << this->p1;
  }
};
template <typename U0, typename U1>
struct ZuPair_Print<U0, U1, true, true> :
  public ZuPair_Print_<U0, U1>, public ZuPrintable {
  ZuPair_Print(const U0 &p0, const U1 &p1, const ZuString &delim) :
      ZuPair_Print_<U0, U1>{p0, p1, delim} { }
  template <typename S> void print(S &s) const {
    s <<
      this->p0.print(this->delim) << this->delim <<
      this->p1.print(this->delim);
  }
};

namespace Zu_ {
template <typename T0_, typename T1_> class Pair : public ZuPair_ {
  template <typename, typename> friend class Pair;

public:
  using T0 = T0_;
  using T1 = T1_;
  using U0 = typename ZuDeref<T0_>::T;
  using U1 = typename ZuDeref<T1_>::T;
  template <unsigned I> using Type = ZuPair_Type<I, T0, T1>;
  template <unsigned I> using Type_ = ZuPair_Type_<I, U0, U1>;

  Pair() = default;
  Pair(const Pair &) = default;
  Pair &operator =(const Pair &) = default;
  Pair(Pair &&) = default;
  Pair &operator =(Pair &&) = default;
  ~Pair() = default;

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
  template <typename T, typename> struct Bind_P1 {
    using U1 = typename T::U1;
    ZuInline static const U1 &p1(const T &v) { return v.m_p1; }
    ZuInline static U1 &&p1(T &&v) { return static_cast<U1 &&>(v.m_p1); }
  };
  template <typename T, typename P1> struct Bind_P1<T, P1 &> {
    using U1 = typename T::U1;
    ZuInline static U1 &p1(T &v) { return v.m_p1; }
  };
  template <typename T, typename P1> struct Bind_P1<T, const P1 &> {
    using U1 = typename T::U1;
    ZuInline static const U1 &p1(const T &v) { return v.m_p1; }
  };
  template <typename T, typename P1> struct Bind_P1<T, volatile P1 &> {
    using U1 = typename T::U1;
    ZuInline static volatile U1 &p1(volatile T &v) { return v.m_p1; }
  };
  template <typename T, typename P1> struct Bind_P1<T, const volatile P1 &> {
    using U1 = typename T::U1;
    ZuInline static const volatile U1 &p1(const volatile T &v) {
      return v.m_p1;
    }
  };
  template <typename T>
  struct Bind : public Bind_P0<T, T0>, public Bind_P1<T, T1> { };

public:
  template <typename T>
  ZuInline Pair(T &&v, typename ZuIfT<
	ZuPair_Cvt<typename ZuDecay<T>::T, Pair>::OK
      >::T *_ = 0) :
    m_p0(Bind<typename ZuDecay<T>::T>::p0(ZuFwd<T>(v))),
    m_p1(Bind<typename ZuDecay<T>::T>::p1(ZuFwd<T>(v))) { }

protected:
  template <typename T>
  ZuInline
  typename ZuIfT<
    ZuPair_Cvt<typename ZuDecay<T>::T, Pair>::OK
  >::T assign(T &&v) {
    m_p0 = Bind<typename ZuDecay<T>::T>::p0(ZuFwd<T>(v));
    m_p1 = Bind<typename ZuDecay<T>::T>::p1(ZuFwd<T>(v));
  }

public:
  template <typename T> ZuInline Pair &operator =(T &&v) noexcept {
    assign(ZuFwd<T>(v));
    return *this;
  }

  template <typename P0, typename P1>
  ZuInline Pair(P0 &&p0, P1 &&p1,
      typename ZuIfT<
	(!ZuTraits<T0>::IsReference ||
	  ZuConversion<typename ZuDecay<U0>::T,
		       typename ZuDecay<P0>::T>::Is) &&
	(!ZuTraits<T1>::IsReference ||
	  ZuConversion<typename ZuDecay<U1>::T,
		       typename ZuDecay<P1>::T>::Is)>::T *_ = 0) :
    m_p0(ZuFwd<P0>(p0)), m_p1(ZuFwd<P1>(p1)) { }

  template <typename P0, typename P1>
  ZuInline int cmp(const Pair<P0, P1> &p) const {
    int i;
    if (i = ZuCmp<T0>::cmp(m_p0, p.template p<0>())) return i;
    return ZuCmp<T1>::cmp(m_p1, p.template p<1>());
  }
  template <typename P0, typename P1>
  ZuInline bool less(const Pair<P0, P1> &p) const {
    return
      !ZuCmp<T0>::less(p.template p<0>(), m_p0) &&
      ZuCmp<T1>::less(m_p1, p.template p<1>());
  }
  template <typename P0, typename P1>
  ZuInline bool equals(const Pair<P0, P1> &p) const {
    return
      ZuCmp<T0>::equals(m_p0, p.template p<0>()) &&
      ZuCmp<T1>::equals(m_p1, p.template p<1>());
  }

  ZuInline bool operator ==(const Pair &p) const { return equals(p); }
  ZuInline bool operator !=(const Pair &p) const { return !equals(p); }
  ZuInline bool operator >(const Pair &p) const { return p.less(*this); }
  ZuInline bool operator >=(const Pair &p) const { return !less(p); }
  ZuInline bool operator <(const Pair &p) const { return less(p); }
  ZuInline bool operator <=(const Pair &p) const { return !p.less(*this); }

  ZuInline bool operator !() const { return !m_p0 || !m_p1; }
  ZuOpBool

  ZuInline uint32_t hash() const {
    return ZuHash<T0>::hash(m_p0) ^ ZuHash<T1>::hash(m_p1);
  }

  template <unsigned I>
  ZuInline typename ZuIfT<I == 0, const U0 &>::T p() const {
    return m_p0;
  }
  template <unsigned I>
  ZuInline typename ZuIfT<I == 0, U0 &>::T p() {
    return m_p0;
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<I == 0, Pair &>::T p(P &&p) {
    m_p0 = ZuFwd<P>(p);
    return *this;
  }

  template <unsigned I>
  ZuInline typename ZuIfT<I == 1, const U1 &>::T p() const {
    return m_p1;
  }
  template <unsigned I>
  ZuInline typename ZuIfT<I == 1, U1 &>::T p() {
    return m_p1;
  }
  template <unsigned I, typename P>
  ZuInline typename ZuIfT<I == 1, Pair &>::T p(P &&p) {
    m_p1 = ZuFwd<P>(p);
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
  ZuInline typename ZuIfT<ZuConversion<T, T0>::Same, Pair &>::T v(P &&p) {
    m_p0 = ZuFwd<P>(p);
    return *this;
  }

  template <typename T>
  ZuInline typename ZuIfT<
      !ZuConversion<T, T0>::Same && ZuConversion<T, T1>::Same,
      const U1 &>::T v() const {
    return m_p1;
  }
  template <typename T>
  ZuInline typename ZuIfT<
      !ZuConversion<T, T0>::Same && ZuConversion<T, T1>::Same,
      U1 &>::T v() {
    return m_p1;
  }
  template <typename T, typename P>
  ZuInline typename ZuIfT<
      !ZuConversion<T, T0>::Same && ZuConversion<T, T1>::Same,
      Pair &>::T v(P &&p) {
    m_p1 = ZuFwd<P>(p);
    return *this;
  }

  using Print = ZuPair_Print<U0, U1,
	ZuConversion<ZuPair_, U0>::Base, ZuConversion<ZuPair_, U1>::Base>;
  ZuInline Print print() const {
    return Print{m_p0, m_p1, "|"};
  }
  ZuInline Print print(const ZuString &delim) const {
    return Print{m_p0, m_p1, delim};
  }

private:
  T0		m_p0;
  T1		m_p1;
};
} // namespace Zu_

template <typename T0, typename T1>
auto ZuInline ZuFwdPair(T0 &&v1, T1 &&v2) {
  return ZuPair<T0 &&, T1 &&>(ZuFwd<T0>(v1), ZuFwd<T1>(v2));
}

template <typename T0, typename T1>
auto ZuInline ZuMvPair(T0 v1, T1 v2) {
  return ZuPair<T0, T1>(ZuMv(v1), ZuMv(v2));
}

template <typename T0, typename T1>
struct ZuPrint<ZuPair<T0, T1> > : public ZuPrintDelegate {
  template <typename S>
  static void print(S &s, const ZuPair<T0, T1> &p) { s << p.print(); }
};

namespace Zu_ {
  template <typename ...Args> Pair(Args...) -> Pair<Args...>;
}

// STL structured binding cruft
#include <type_traits>
namespace std {
  template <class> struct tuple_size;
  template <typename T0, typename T1>
  struct tuple_size<ZuPair<T0, T1>> :
  public integral_constant<size_t, 2> { };

  template <size_t, class> struct tuple_element;
  template <typename T0, typename T1>
  struct tuple_element<0, ZuPair<T0, T1>> { using type = T0; };
  template <typename T0, typename T1>
  struct tuple_element<1, ZuPair<T0, T1>> { using type = T1; };
}
namespace Zu_ {
  using size_t = std::size_t;
  namespace {
    template <size_t I, typename T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;
  }
  template <size_t I, typename T0, typename T1>
  constexpr tuple_element_t<I, Pair<T0, T1>> &
  get(Pair<T0, T1> &p) noexcept { return p.template p<I>(); }
  template <size_t I, typename T0, typename T1>
  constexpr const tuple_element_t<I, Pair<T0, T1>> &
  get(const Pair<T0, T1> &p) noexcept { return p.template p<I>(); }
  template <size_t I, typename T0, typename T1>
  constexpr tuple_element_t<I, Pair<T0, T1>> &&
  get(Pair<T0, T1> &&p) noexcept {
    return static_cast<tuple_element_t<I, Pair<T0, T1>> &&>(
	p.template p<I>());
  }
  template <size_t I, typename T0, typename T1>
  constexpr const tuple_element_t<I, Pair<T0, T1>> &&
  get(const Pair<T0, T1> &&p) noexcept {
    return static_cast<const tuple_element_t<I, Pair<T0, T1>> &&>(
	p.template p<I>());
  }

  template <typename T, typename U>
  constexpr T &get(Pair<T, U> &p) noexcept {
    return p.template p<0>();
  }
  template <typename T, typename U>
  constexpr const T &get(const Pair<T, U> &p) noexcept {
    return p.template p<0>();
  }
  template <typename T, typename U>
  constexpr T &&get(Pair<T, U> &&p) noexcept {
    return static_cast<T &&>(p.template p<0>());
  }
  template <typename T, typename U>
  constexpr const T &&get(const Pair<T, U> &&p) noexcept {
    return static_cast<const T &&>(p.template p<0>());
  }

  template <typename T, typename U>
  constexpr T &get(Pair<U, T> &p) noexcept {
    return p.template p<1>();
  }
  template <typename T, typename U>
  constexpr const T &get(const Pair<U, T> &p) noexcept {
    return p.template p<1>();
  }
  template <typename T, typename U>
  constexpr T &&get(Pair<U, T> &&p) noexcept {
    return static_cast<T &&>(p.template p<1>());
  }
  template <typename T, typename U>
  constexpr const T &&get(const Pair<U, T> &&p) noexcept {
    return static_cast<const T &&>(p.template p<1>());
  }
} // namespace Zu_

#endif /* ZuPair_HPP */
