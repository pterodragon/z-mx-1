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

// generic discriminated union
//
// ZuUnion<int, double> u, v;
// *(u.new1()) = 42;
// u.p1(42);
// u.p1() = 42;
// v = u;
// if (v.type() == 1) { printf("%d\n", v.p1()); }
// v.p2(42.0);
// if (v.type() == 2) { printf("%g\n", v.p2()); }
//
// struct Fn : public ZuUnionFn<> {
//   inline void operator()(int i) const { printf("%d\n", i); }
//   inline void operator()(double d) const { printf("%g\n", d); }
//   inline void operator()(const ZuNull &) const { puts("(null)"); }
// };
// v.fn(Fn());
//
// // ZuUnionFn<ReturnType, Const, PConst>
// //   ReturnType - type returned by functor (default: void)
// //   Const - if functor is const (default: true)
// //   PConst - if (optional) parameter is const (default: true)

#ifndef ZuUnion_HPP
#define ZuUnion_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuNew.hpp>
#include <ZuIfT.hpp>
#include <ZuTraits.hpp>
#include <ZuLargest.hpp>
#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuConversion.hpp>
#include <ZuTuple.hpp>
#include <ZuPP.hpp>

template <typename T, bool IsPrimitive> class ZuUnion_Ops_;
template <typename T> class ZuUnion_Ops_<T, false> {
public:
  inline static void ctor(void *dst) { new (dst) T(ZuCmp<T>::null()); }
  template <typename P>
  inline static void ctor(void *dst, const P &p) { new (dst) T(p); }
  inline static void dtor(void *dst) { ((T *)dst)->~T(); }
};
template <typename T> class ZuUnion_Ops_<T, true> {
public:
  inline static void ctor(void *dst) { *(T *)dst = ZuCmp<T>::null(); }
  template <typename P>
  inline static void ctor(void *dst, const P &p) { *(T *)dst = p; }
  inline static void dtor(void *dst) { }
};
template <typename T>
struct ZuUnion_Ops : public ZuUnion_Ops_<T, ZuTraits<T>::IsPrimitive> {
  template <typename P> inline static void assign(void *dst, const P &p)
    { *(T *)dst = p; }
  template <typename P> inline static bool equals(const void *t, const P &p)
    { return ZuCmp<T>::equals(*(const T *)t, p); }
  template <typename P> inline static int cmp(const void *t, const P &p)
    { return ZuCmp<T>::cmp(*(const T *)t, p); }
  inline static bool null(const void *t)
    { return ZuCmp<T>::null(*(const T *)t); }
  inline static uint32_t hash(const void *t)
    { return ZuHash<T>::hash(*(const T *)t); }
};

#define ZuUnion_TemplateArgDef(I) typename T##I##_ = ZuNull
#define ZuUnion_TemplateArg(I) T##I##_
template <ZuPP_CList(ZuUnion_TemplateArgDef, ZuPP_N), int _ = 0> struct ZuUnion;

#undef ZuUnion_TemplateArgDef
#define ZuUnion_TemplateArgDef(I) typename T##I##_
template <ZuPP_CList(ZuUnion_TemplateArgDef, ZuPP_N)>
struct ZuTraits<ZuUnion<ZuPP_CList(ZuUnion_TemplateArg, ZuPP_N)> > :
    public ZuTraits<ZuTuple<ZuPP_CList(ZuUnion_TemplateArg, ZuPP_N)> > {
  typedef ZuUnion<ZuPP_CList(ZuUnion_TemplateArg, ZuPP_N)> T;
};

// SFINAE techniques...
#define ZuUnion_TemplateArgNull(I) ZuNull
#define ZuUnion_Typedef(I, _) typedef typename ZuDeref<T##I##_>::T T##I;
#define ZuUnion_Dtor(I, _) \
      case I: ZuUnion_Ops<T##I>::dtor(m_u); break;
#define ZuUnion_Ctor(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)u.m_u; \
	ZuUnion_Ops<T##I>::ctor(m_u, *ptr); \
      } break;
#define ZuUnion_Assign(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)u.m_u; \
	ZuUnion_Ops<T##I>::assign(m_u, *ptr); \
      } break;
#define ZuUnion_Equals(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)p.m_u; \
	return ZuUnion_Ops<T##I>::equals(m_u, *ptr); \
      }
#define ZuUnion_Equals2(I, _) \
      case I: return ZuUnion_Ops<T##I>::equals(m_u, p);
#define ZuUnion_Cmp(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)p.m_u; \
	return ZuUnion_Ops<T##I>::cmp(m_u, *ptr); \
      }
#define ZuUnion_Cmp2(I, _) \
      case I: return ZuUnion_Ops<T##I>::cmp(m_u, p);
#define ZuUnion_Null(I, _) \
      case I: return ZuUnion_Ops<T##I>::null(m_u);
#define ZuUnion_Hash(I, _) \
      case I: return ZuHash<uint8_t>::hash(this->type()) ^ ZuUnion_Ops<T##I>::hash(m_u);
#define ZuUnion_ConstFn(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)m_u; \
	return f(*ptr); \
      }
#define ZuUnion_Fn(I, _) \
      case I: return f(*(T##I *)m_u);
#define ZuUnion_ConstFnP(I, _) \
      case I: { \
	const T##I *ZuMayAlias(ptr) = (const T##I *)m_u; \
	return f(p, *ptr); \
      }
#define ZuUnion_FnP(I, _) \
      case I: { \
	T##I *ZuMayAlias(ptr) = (T##I *)m_u; \
	return f(p, *ptr); \
      }
#define ZuUnion_Method(I, _) \
  inline const T##I &p##I() const { \
    static const T##I null = ZuCmp<T##I>::null(); \
    if (this->type() != I) return null; \
    const T##I *ZuMayAlias(ptr) = (const T##I *)(const void *)m_u; \
    return *ptr; \
  } \
  template <typename P> inline ZuUnion &p##I(P &&p) { \
    if (this->type() == I) { \
      T##I *ZuMayAlias(ptr) = (T##I *)(void *)m_u; \
      *ptr = ZuFwd<P>(p); \
      return *this; \
    } \
    this->~ZuUnion(); \
    this->type(I); \
    ZuUnion_Ops<T##I>::ctor(m_u, ZuFwd<P>(p)); \
    return *this; \
  } \
  inline T##I &p##I() { \
    T##I *ZuMayAlias(ptr) = (T##I *)(void *)m_u; \
    if (this->type() == I) return *ptr; \
    this->~ZuUnion(); \
    this->type(I); \
    ZuUnion_Ops<T##I>::ctor(m_u); \
    return *ptr; \
  } \
  inline T##I *ptr##I() { \
    if (this->type() != I) return 0; \
    T##I *ZuMayAlias(ptr) = (T##I *)(void *)m_u; \
    return ptr; \
  } \
  inline T##I *new##I() { \
    this->~ZuUnion(); \
    this->type(I); \
    T##I *ZuMayAlias(ptr) = (T##I *)(void *)m_u; \
    return ptr; \
  }

struct ZuUnionFn_ { };
template <typename T_ = void, bool Const_ = true, bool PConst_ = true>
struct ZuUnionFn : public ZuUnionFn_ {
  typedef T_ T;
  enum _ { Const = Const_, PConst = PConst_ };
};

#define ZuUnion_Specialization(N) \
template <ZuPP_CList(ZuUnion_TemplateArgDef, N)> \
struct ZuUnion<ZuPP_CList(ZuUnion_TemplateArg, N) \
               ZuPP_CListT(ZuUnion_TemplateArgNull, N), 0> { \
ZuPP_List1(ZuUnion_Typedef, _, N) \
 \
  typedef typename ZuLargest<ZuPP_CList(ZuUnion_TemplateArg, N)>::T Largest; \
  enum _ { Size = sizeof(Largest) }; \
 \
  inline ZuUnion() { this->type(0); } \
 \
  inline ~ZuUnion() { \
    switch ((int)this->type()) { \
      default: return; \
      ZuPP_List1(ZuUnion_Dtor, _, N) \
    } \
  } \
 \
  inline ZuUnion(const ZuUnion &u) { \
    switch ((int)(this->type(u.type()))) { \
      default: return; \
      ZuPP_List1(ZuUnion_Ctor, _, N) \
    } \
  } \
  inline ZuUnion &operator =(const ZuUnion &u) { \
    if (this == &u) return *this; \
    if (this->type() != u.type()) { \
      this->~ZuUnion(); \
      new (this) ZuUnion(u); \
      return *this; \
    } \
    switch ((int)this->type()) { \
      default: return *this; \
      ZuPP_List1(ZuUnion_Assign, _, N) \
    } \
    return *this; \
  } \
 \
  inline void null() { \
    switch ((int)this->type()) { \
      ZuPP_List1(ZuUnion_Dtor, _, N) \
    } \
    this->type(0); \
  } \
 \
  template <class F> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && F::Const, \
			      typename F::T>::T fn(const F &f) const { \
    switch ((int)this->type()) { \
      default: return f(ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFn, _, N) \
    } \
  } \
  template <class F> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && !F::Const, \
			      typename F::T>::T fn(F &f) const { \
    switch ((int)this->type()) { \
      default: return f(ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFn, _, N) \
    } \
  } \
  template <class F> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && F::Const, \
			      typename F::T>::T fn(const F &f) { \
    switch ((int)this->type()) { \
      default: return f(ZuNull()); \
      ZuPP_List1(ZuUnion_Fn, _, N) \
    } \
  } \
  template <class F> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && !F::Const, \
			      typename F::T>::T fn(F &f) { \
    switch ((int)this->type()) { \
      default: return f(ZuNull()); \
      ZuPP_List1(ZuUnion_Fn, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	F::Const && F::PConst, typename F::T>::T \
		  fn(const F &f, const P &p) const { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	!F::Const && F::PConst, typename F::T>::T \
		  fn(F &f, const P &p) const { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
			F::Const && F::PConst, typename F::T>::T \
		  fn(const F &f, const P &p) { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_FnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
			!F::Const && F::PConst, typename F::T>::T \
		  fn(F &f, const P &p) { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_FnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	F::Const && !F::PConst, typename F::T>::T \
		  fn(const F &f, P &p) const { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	!F::Const && !F::PConst, typename F::T>::T \
		  fn(F &f, P &p) const { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_ConstFnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	F::Const && !F::PConst, typename F::T>::T \
		  fn(const F &f, P &p) { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_FnP, _, N) \
    } \
  } \
  template <class F, typename P> \
  inline typename ZuIfT<ZuConversion<ZuUnionFn_, F>::Base && \
		 	!F::Const && !F::PConst, typename F::T>::T \
		  fn(F &f, P &p) { \
    switch ((int)this->type()) { \
      default: return f(p, ZuNull()); \
      ZuPP_List1(ZuUnion_FnP, _, N) \
    } \
  } \
 \
  template <typename P> \
  inline bool operator ==(const P &p) const { return equals(p); } \
  template <typename P> \
  inline bool operator !=(const P &p) const { return !equals(p); } \
  template <typename P> \
  inline bool operator >(const P &p) const { return cmp(p) > 0; } \
  template <typename P> \
  inline bool operator >=(const P &p) const { return cmp(p) >= 0; } \
  template <typename P> \
  inline bool operator <(const P &p) const { return cmp(p) < 0; } \
  template <typename P> \
  inline bool operator <=(const P &p) const { return cmp(p) <= 0; } \
 \
  template <typename P> \
  inline typename ZuIs<ZuUnion, P, bool>::T equals(const P &p) const { \
    if (this == &p) return true; \
    if (this->type() != p.type()) return false; \
    switch ((int)this->type()) { \
      default: return true; \
      ZuPP_List1(ZuUnion_Equals, _, N) \
    } \
  } \
  template <typename P> \
  inline typename ZuIsNot<ZuUnion, P, bool>::T equals(const P &p) const { \
    switch ((int)this->type()) { \
      default: return false; \
      ZuPP_List1(ZuUnion_Equals2, _, N) \
    } \
  } \
 \
  template <typename P> \
  inline typename ZuIs<ZuUnion, P, int>::T cmp(const P &p) const { \
    if (this == &p) return true; \
    if (int i = ZuCmp<uint8_t>::cmp(this->type(), p.type())) return i; \
    switch ((int)this->type()) { \
      default: return 0; \
      ZuPP_List1(ZuUnion_Cmp, _, N) \
    } \
  } \
  template <typename P> \
  inline typename ZuIsNot<ZuUnion, P, int>::T cmp(const P &p) const { \
    switch ((int)this->type()) { \
      default: return -1; \
      ZuPP_List1(ZuUnion_Cmp2, _, N) \
    } \
  } \
 \
  inline bool operator !() const { \
    switch ((int)this->type()) { \
      default: return true; \
      ZuPP_List1(ZuUnion_Null, _, N) \
    } \
  } \
 \
  inline uint32_t hash() const { \
    switch ((int)this->type()) { \
      default: return ZuHash<uint8_t>::hash(type()); \
      ZuPP_List1(ZuUnion_Hash, _, N) \
    } \
  } \
 \
  inline uint8_t type() const { return m_u[Size]; } \
 \
ZuPP_List1(ZuUnion_Method, _, N) \
 \
private: \
  inline uint8_t type(uint8_t i) { return m_u[Size] = i; } \
 \
  uint8_t	m_u[Size + 1]; \
};
ZuPP_List(ZuUnion_Specialization, ZuPP_N)

#endif /* ZuUnion_HPP */
