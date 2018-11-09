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

// generic pair

#ifndef ZuPair_HPP
#define ZuPair_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuAssert.hpp>
#include <ZuIfT.hpp>
#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuNull.hpp>
#include <ZuConversion.hpp>
#include <ZuPrint.hpp>

// SFINAE technique...
struct ZuPair_ { };

template <typename T, typename P, bool IsPair> struct ZuPair_Cvt_;
template <typename T, typename P> struct ZuPair_Cvt_<T, P, 0> {
  enum { OK = 0 };
};
template <typename T, typename P> struct ZuPair_Cvt_<T, P, 1> {
  enum {
    OK = ZuConversion<typename T::T1, typename P::T1>::Exists &&
	 ZuConversion<typename T::T2, typename P::T2>::Exists
  };
};
template <typename T, typename P> struct ZuPair_Cvt :
  public ZuPair_Cvt_<typename ZuDeref<T>::T, P,
    ZuConversion<ZuPair_, typename ZuDeref<T>::T>::Base> { };

template <typename T1, typename T2> class ZuPair;

template <typename T1, typename T2>
struct ZuTraits<ZuPair<T1, T2> > : public ZuGenericTraits<ZuPair<T1, T2> > {
  enum {
    IsPOD = ZuTraits<T1>::IsPOD && ZuTraits<T2>::IsPOD,
    IsComparable = ZuTraits<T1>::IsComparable && ZuTraits<T2>::IsComparable,
    IsHashable = ZuTraits<T1>::IsHashable && ZuTraits<T2>::IsHashable
  };
};

template <typename T1_, typename T2_> class ZuPair : public ZuPair_ {
public:
  typedef T1_ T1;
  typedef T2_ T2;
  typedef typename ZuDeref<T1_>::T U1;
  typedef typename ZuDeref<T2_>::T U2;

  ZuInline ZuPair() { }

  ZuInline ZuPair(const ZuPair &p) : m_p1(p.m_p1), m_p2(p.m_p2) { }

  ZuInline ZuPair(ZuPair &&p) :
    m_p1(static_cast<T1 &&>(p.m_p1)), m_p2(static_cast<T2 &&>(p.m_p2)) { }

  ZuInline ZuPair &operator =(const ZuPair &p) {
    if (this != &p) m_p1 = p.m_p1, m_p2 = p.m_p2;
    return *this;
  }

  ZuInline ZuPair &operator =(ZuPair &&p) {
    m_p1 = static_cast<T1 &&>(p.m_p1), m_p2 = static_cast<T2 &&>(p.m_p2);
    return *this;
  }

  template <typename T> ZuInline ZuPair(const T &t,
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T *_ = 0) :
    m_p1(t.m_p1), m_p2(t.m_p2) { }
  template <typename T> ZuInline ZuPair(T &&t,
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T *_ = 0) noexcept :
    m_p1(ZuFwd<decltype(t.m_p1)>(t.m_p1)),
    m_p2(ZuFwd<decltype(t.m_p2)>(t.m_p2)) { }
  template <typename T> ZuInline ZuPair(T &&t,
      typename ZuIfT<
	!ZuPair_Cvt<T, ZuPair>::OK &&
	  (!ZuTraits<T1>::IsReference ||
	    ZuConversion<typename ZuTraits<U1>::T,
			 typename ZuTraits<T>::T>::Is)>::T *_ = 0) :
    m_p1(ZuFwd<T>(t)), m_p2(ZuCmp<U2>::null()) { }

private:
  template <typename T> inline
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T assign(const T &t)
    { m_p1 = t.m_p1, m_p2 = t.m_p2; }
  template <typename T> inline
      typename ZuIfT<ZuPair_Cvt<T, ZuPair>::OK>::T assign(T &&t) {
    m_p1 = ZuFwd<decltype(t.m_p1)>(t.m_p1);
    m_p2 = ZuFwd<decltype(t.m_p2)>(t.m_p2);
  }
  template <typename T> inline
      typename ZuIfT<
	!ZuPair_Cvt<T, ZuPair>::OK &&
	  (!ZuTraits<T1>::IsReference ||
	    ZuConversion<typename ZuTraits<U1>::T,
			 typename ZuTraits<T>::T>::Is)
      >::T assign(T &&t) { m_p1 = ZuFwd<T>(t), m_p2 = ZuCmp<U2>::null(); }

public:
  template <typename T> ZuInline ZuPair &operator =(T &&t) noexcept {
    assign(ZuFwd<T>(t));
    return *this;
  }

  template <typename P1, typename P2>
  ZuInline ZuPair(P1 &&p1, P2 &&p2,
      typename ZuIfT<
	(!ZuTraits<T1>::IsReference ||
	  ZuConversion<typename ZuTraits<U1>::T,
		       typename ZuTraits<P1>::T>::Is) && 
	(!ZuTraits<T2>::IsReference ||
	  ZuConversion<typename ZuTraits<U2>::T,
		       typename ZuTraits<P2>::T>::Is)>::T *_ = 0) :
    m_p1(ZuFwd<P1>(p1)), m_p2(ZuFwd<P2>(p2)) { }

  template <typename P1, typename P2>
  ZuInline bool operator ==(const ZuPair<P1, P2> &p) const
    { return equals(p); }
  template <typename P1, typename P2>
  ZuInline bool operator !=(const ZuPair<P1, P2> &p) const
    { return !equals(p); }
  template <typename P1, typename P2>
  ZuInline bool operator >(const ZuPair<P1, P2> &p) const
    { return cmp(p) > 0; }
  template <typename P1, typename P2>
  ZuInline bool operator >=(const ZuPair<P1, P2> &p) const
    { return cmp(p) >= 0; }
  template <typename P1, typename P2>
  ZuInline bool operator <(const ZuPair<P1, P2> &p) const
    { return cmp(p) < 0; }
  template <typename P1, typename P2>
  ZuInline bool operator <=(const ZuPair<P1, P2> &p) const
    { return cmp(p) <= 0; }

  template <typename P1, typename P2>
  ZuInline bool equals(const ZuPair<P1, P2> &p) const {
    return ZuCmp<T1>::equals(m_p1, p.p1()) && ZuCmp<T2>::equals(m_p2, p.p2());
  }

  template <typename P1, typename P2>
  ZuInline int cmp(const ZuPair<P1, P2> &p) const {
    int i;
    if (i = ZuCmp<T1>::cmp(m_p1, p.p1())) return i;
    return ZuCmp<T2>::cmp(m_p2, p.p2());
  }

  ZuInline bool operator !() const { return !m_p1 || !m_p2; }
  ZuOpBool

  ZuInline uint32_t hash() const {
    return ZuHash<T1>::hash(m_p1) ^ ZuHash<T2>::hash(m_p2);
  }

  ZuInline const U1 &p1() const { return m_p1; }
  ZuInline U1 &p1() { return m_p1; }
  template <typename P1> ZuInline auto &p1(P1 &&p1) {
    m_p1 = ZuFwd<P1>(p1);
    return *this;
  }
  ZuInline const U2 &p2() const { return m_p2; }
  ZuInline U2 &p2() { return m_p2; }
  template <typename P2> ZuInline auto &p2(P2 &&p2) {
    m_p2 = ZuFwd<P2>(p2);
    return *this;
  }

  T1		m_p1;
  T2		m_p2;
};

template <typename T1, typename T2>
auto ZuInline ZuMkPair(T1 &&t1, T2 &&t2) {
  return ZuPair<decltype(ZuFwd<T1>(t1)), decltype(ZuFwd<T2>(t2))>(
      ZuFwd<T1>(t1), ZuFwd<T2>(t2));
}

template <typename T1, typename T2>
struct ZuPrint<ZuPair<T1, T2> > : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const ZuPair<T1, T2> &p) {
    s << p.p1() << ':' << p.p2();
  }
};

#endif /* ZuPair_HPP */
