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

#include <zlib/ZuAssert.hpp>
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
  public ZuPair_Cvt_<typename ZuDeref<T>::T, P,
    ZuConversion<ZuPair_, typename ZuDeref<T>::T>::Base> { };

template <typename T0, typename T1> class ZuPair;

template <typename T0, typename T1>
struct ZuTraits<ZuPair<T0, T1> > : public ZuGenericTraits<ZuPair<T0, T1> > {
  enum {
    IsPOD = ZuTraits<T0>::IsPOD && ZuTraits<T1>::IsPOD,
    IsComparable = ZuTraits<T0>::IsComparable && ZuTraits<T1>::IsComparable,
    IsHashable = ZuTraits<T0>::IsHashable && ZuTraits<T1>::IsHashable
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
  const U0		&p0;
  const U1		&p1;
  const ZuString	&delim;
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

template <typename T0_, typename T1_> class ZuPair : public ZuPair_ {
  template <typename, typename> friend class ZuPair;

public:
  using T0 = T0_;
  using T1 = T1_;
  using U0 = typename ZuDeref<T0_>::T;
  using U1 = typename ZuDeref<T1_>::T;
  template <unsigned I> using Type = ZuPair_Type<I, T0, T1>;
  template <unsigned I> using Type_ = ZuPair_Type_<I, U0, U1>;

  ZuInline ZuPair() { }

  ZuInline ZuPair(const ZuPair &p) : m_p0(p.m_p0), m_p1(p.m_p1) { }

  ZuInline ZuPair(ZuPair &&p) :
    m_p0(static_cast<T0 &&>(p.m_p0)), m_p1(static_cast<T1 &&>(p.m_p1)) { }

  ZuInline ZuPair &operator =(const ZuPair &p) {
    if (this != &p) m_p0 = p.m_p0, m_p1 = p.m_p1;
    return *this;
  }

  ZuInline ZuPair &operator =(ZuPair &&p) noexcept {
    m_p0 = static_cast<T0 &&>(p.m_p0), m_p1 = static_cast<T1 &&>(p.m_p1);
    return *this;
  }

  template <typename T> ZuInline ZuPair(T p,
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T *_ = 0) :
    m_p0(ZuMv(p.m_p0)), m_p1(ZuMv(p.m_p1)) { }

  template <typename T> ZuInline ZuPair(T &&v,
      typename ZuIfT<
	!ZuPair_Cvt<T, ZuPair>::OK &&
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuTraits<U0>::T,
			 typename ZuTraits<T>::T>::Is)>::T *_ = 0) :
    m_p0(ZuFwd<T>(v)), m_p1(ZuCmp<U1>::null()) { }

private:
  template <typename T> ZuInline
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T assign(T p)
    { m_p0 = ZuMv(p.m_p0), m_p1 = ZuMv(p.m_p1); }

  template <typename T> ZuInline
      typename ZuIfT<
	!ZuPair_Cvt<T, ZuPair>::OK &&
	  (!ZuTraits<T0>::IsReference ||
	    ZuConversion<typename ZuTraits<U0>::T,
			 typename ZuTraits<T>::T>::Is)
      >::T assign(T &&v) { m_p0 = ZuFwd<T>(v), m_p1 = ZuCmp<U1>::null(); }

public:
  template <typename T> ZuInline ZuPair &operator =(T &&v) noexcept {
    assign(ZuFwd<T>(v));
    return *this;
  }

  template <typename P0, typename P1>
  ZuInline ZuPair(P0 &&p0, P1 &&p1,
      typename ZuIfT<
	(!ZuTraits<T0>::IsReference ||
	  ZuConversion<typename ZuTraits<U0>::T,
		       typename ZuTraits<P0>::T>::Is) && 
	(!ZuTraits<T1>::IsReference ||
	  ZuConversion<typename ZuTraits<U1>::T,
		       typename ZuTraits<P1>::T>::Is)>::T *_ = 0) :
    m_p0(ZuFwd<P0>(p0)), m_p1(ZuFwd<P1>(p1)) { }

  template <typename P0, typename P1>
  ZuInline bool operator ==(const ZuPair<P0, P1> &p) const
    { return equals(p); }
  template <typename P0, typename P1>
  ZuInline bool operator !=(const ZuPair<P0, P1> &p) const
    { return !equals(p); }
  template <typename P0, typename P1>
  ZuInline bool operator >(const ZuPair<P0, P1> &p) const
    { return cmp(p) > 0; }
  template <typename P0, typename P1>
  ZuInline bool operator >=(const ZuPair<P0, P1> &p) const
    { return cmp(p) >= 0; }
  template <typename P0, typename P1>
  ZuInline bool operator <(const ZuPair<P0, P1> &p) const
    { return cmp(p) < 0; }
  template <typename P0, typename P1>
  ZuInline bool operator <=(const ZuPair<P0, P1> &p) const
    { return cmp(p) <= 0; }

  template <typename P0, typename P1>
  ZuInline bool equals(const ZuPair<P0, P1> &p) const {
    return
      ZuCmp<T0>::equals(m_p0, p.template p<0>()) &&
      ZuCmp<T1>::equals(m_p1, p.template p<1>());
  }

  template <typename P0, typename P1>
  ZuInline int cmp(const ZuPair<P0, P1> &p) const {
    int i;
    if (i = ZuCmp<T0>::cmp(m_p0, p.template p<0>())) return i;
    return ZuCmp<T1>::cmp(m_p1, p.template p<1>());
  }

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
  ZuInline typename ZuIfT<I == 0, ZuPair &>::T p(P &&p) {
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
  ZuInline typename ZuIfT<I == 1, ZuPair &>::T p(P &&p) {
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

template <typename T0, typename T1>
auto ZuInline ZuMkPair(T0 t1, T1 t2) {
  return ZuPair<T0, T1>(ZuMv(t1), ZuMv(t2));
}

template <typename T0, typename T1>
struct ZuPrint<ZuPair<T0, T1> > : public ZuPrintDelegate {
  template <typename S>
  static void print(S &s, const ZuPair<T0, T1> &p) {
    s << p.template p<0>() << ':' << p.template p<1>();
  }
};

#endif /* ZuPair_HPP */
