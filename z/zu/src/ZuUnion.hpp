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
// *(u.new_<0>()) = 42;
// u.p<0>(42);
// u.p<0>() = 42;
// v = u;
// if (v.type() == 0) { printf("%d\n", v.p<0>()); }
// v.p<1>(42.0);
// if (v.type() == 1) { printf("%g\n", v.p<1>()); }
//
// struct Fn {
//   void operator()(int i) const { printf("%d\n", i); }
//   void operator()(double d) const { printf("%g\n", d); }
//   void operator()(const ZuNull &) const { puts("(null)"); }
// };
// ZuSwitch::dispatch(i, [&u](auto i) { Fn()(u.p<i>()); });

#ifndef ZuUnion_HPP
#define ZuUnion_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuNew.hpp>
#include <zlib/ZuIfT.hpp>
#include <zlib/ZuCan.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuLargest.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuSwitch.hpp>
#include <zlib/ZuPP.hpp>

template <typename T, bool CanStar> class ZuUnion_OpStar;
template <typename T> class ZuUnion_OpStar<T, 1> {
  ZuInline static bool star(const void *t) { return *(*(const T *)t); }
};
template <typename T> class ZuUnion_OpStar<T, 0> {
  ZuInline static bool star(const void *t) {
    return !ZuCmp<T>::null(*(const T *)t);
  }
};
template <typename T, bool CanBang> class ZuUnion_OpBang;
template <typename T> class ZuUnion_OpBang<T, 1> {
  ZuInline static bool bang(const void *t) { return !(*(const T *)t); }
};
template <typename T> class ZuUnion_OpBang<T, 0> {
  ZuInline static bool bang(const void *t) {
    return ZuCmp<T>::null(*(const T *)t);
  }
};
template <typename T, bool IsPrimitive, bool IsPointer> class ZuUnion_Ops_;
ZuCan(operator *, ZuUnion_CanStar);
ZuCan(operator !, ZuUnion_CanBang);
template <typename T> class ZuUnion_Ops_<T, 0, 0> :
  public ZuUnion_OpStar<T, ZuUnion_CanStar<T, bool (T::*)() const>::OK>,
  public ZuUnion_OpBang<T, ZuUnion_CanBang<T, bool (T::*)() const>::OK> {
public:
  ZuInline static void ctor(void *dst) { new (dst) T(ZuCmp<T>::null()); }
  template <typename P>
  ZuInline static void ctor(void *dst, const P &p) { new (dst) T(p); }
  ZuInline static void dtor(void *dst) { ((T *)dst)->~T(); }
};
template <typename T> class ZuUnion_Ops_<T, 1, 0> {
public:
  ZuInline static void ctor(void *dst) { *(T *)dst = ZuCmp<T>::null(); }
  template <typename P>
  ZuInline static void ctor(void *dst, const P &p) { *(T *)dst = p; }
  ZuInline static void dtor(void *dst) { }
  ZuInline static bool star(const void *t) {
    return !ZuCmp<T>::null(*(const T *)t);
  }
  ZuInline static bool bang(const void *t) { return !(*(const T *)t); }
};
template <typename T> class ZuUnion_Ops_<T, 1, 1> {
public:
  ZuInline static void ctor(void *dst) { *(T *)dst = ZuCmp<T>::null(); }
  template <typename P>
  ZuInline static void ctor(void *dst, const P &p) { *(T *)dst = p; }
  ZuInline static void dtor(void *dst) { }
  ZuInline static bool star(const void *t) { return (bool)(*(const T *)t); }
  ZuInline static bool bang(const void *t) { return !(*(const T *)t); }
};
template <typename T>
struct ZuUnion_Ops :
    public ZuUnion_Ops_<T, ZuTraits<T>::IsPrimitive, ZuTraits<T>::IsPointer> {
  template <typename P> ZuInline static void assign(void *dst, const P &p)
    { *(T *)dst = p; }
  template <typename P> ZuInline static bool equals(const void *t, const P &p)
    { return ZuCmp<T>::equals(*(const T *)t, p); }
  template <typename P> ZuInline static int cmp(const void *t, const P &p)
    { return ZuCmp<T>::cmp(*(const T *)t, p); }
  ZuInline static uint32_t hash(const void *t)
    { return ZuHash<T>::hash(*(const T *)t); }
};

template <typename ...Args> struct ZuUnion_;
template <typename Arg0> struct ZuUnion_<Arg0> {
  template <unsigned I> struct Type;
  template <unsigned I> struct Type_;
  template <> struct Type<0> { using T = Arg0; };
  template <> struct Type_<0> { using T = typename ZuDeref<Arg0>::T; };
  enum { N = 1 };
};
template <typename Arg0, typename ...Args> struct ZuUnion_<Arg0, Args...> {
  template <unsigned I>
  struct Type : public ZuUnion_<Args...>::template Type<I - 1> { };
  template <unsigned I>
  struct Type_ : public ZuUnion_<Args...>::template Type_<I - 1> { };
  template <> struct Type<0> { using T = Arg0; };
  template <> struct Type_<0> { using T = typename ZuDeref<Arg0>::T; };
  enum { N = ZuUnion_<Args...>::N + 1 };
};

template <typename ...Args> class ZuUnion {
public:
  using Largest = typename ZuLargest<Args...>::T;
  enum { Size = sizeof(Largest) };
  template <unsigned I>
  using Type = typename ZuUnion_<ZuNull, Args...>::template Type<I>;
  template <unsigned I>
  using Type_ = typename ZuUnion_<ZuNull, Args...>::template Type_<I>;
  enum { N = ZuUnion_<ZuNull, Args...>::N };

  ZuUnion() { this->type(0); }

  ~ZuUnion() {
    ZuSwitch::dispatch<N>(this->type(), [this](auto i) {
      using T = typename Type<i>::T;
      ZuUnion_Ops<T>::dtor(m_u);
    });
  }

  ZuUnion(const ZuUnion &u) {
    ZuSwitch::dispatch<N>(this->type(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = (const T *)u.m_u;
      ZuUnion_Ops<T>::ctor(m_u, *ptr);
    });
  }

  ZuUnion &operator =(const ZuUnion &u) {
    if (this == &u) return *this;
    if (this->type() != u.type()) {
      this->~ZuUnion();
      new (this) ZuUnion(u);
      return *this;
    }
    ZuSwitch::dispatch<N>(this->type(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = (const T *)u.m_u;
      ZuUnion_Ops<T>::assign(m_u, *ptr);
    });
    return *this;
  }

  void null() {
    ZuSwitch::dispatch<N>(this->type(), [this](auto i) {
      using T = typename Type<i>::T;
      ZuUnion_Ops<T>::dtor(m_u);
    });
    this->type(0);
  }

  template <typename P>
  bool operator ==(const P &p) const { return equals(p); }
  template <typename P>
  bool operator !=(const P &p) const { return !equals(p); }
  template <typename P>
  bool operator >(const P &p) const { return cmp(p) > 0; }
  template <typename P>
  bool operator >=(const P &p) const { return cmp(p) >= 0; }
  template <typename P>
  bool operator <(const P &p) const { return cmp(p) < 0; }
  template <typename P>
  bool operator <=(const P &p) const { return cmp(p) <= 0; }

  template <typename P>
  typename ZuIs<ZuUnion, P, bool>::T equals(const P &p) const {
    if (this == &p) return true;
    if (this->type() != p.type()) return false;
    return ZuSwitch::dispatch<N>(this->type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = (const T *)p.m_u;
      return ZuUnion_Ops<T>::equals(m_u, *ptr);
    });
  }
  template <typename P>
  typename ZuIsNot<ZuUnion, P, bool>::T equals(const P &p) const {
    return ZuSwitch::dispatch<N>(this->type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::equals(m_u, p);
    });
  }

  template <typename P>
  typename ZuIs<ZuUnion, P, int>::T cmp(const P &p) const {
    if (this == &p) return true;
    if (int i = ZuCmp<uint8_t>::cmp(this->type(), p.type())) return i;
    return ZuSwitch::dispatch<N>(this->type(), [this, &p](auto i) -> int {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = (const T *)p.m_u;
      return ZuUnion_Ops<T>::cmp(m_u, *ptr);
    });
  }
  template <typename P>
  typename ZuIsNot<ZuUnion, P, int>::T cmp(const P &p) const {
    return ZuSwitch::dispatch<N>(this->type(), [this, &p](auto i) -> int {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::cmp(m_u, p);
    });
  }

  bool operator *() const {
    return ZuSwitch::dispatch<N>(this->type(), [this](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::star(m_u);
    });
  }

  bool operator !() const {
    return ZuSwitch::dispatch<N>(this->type(), [this](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::bang(m_u);
    });
  }

  uint32_t hash() const {
    return ZuSwitch::dispatch<N>(this->type(), [this](auto i) -> uint32_t {
      using T = typename Type<i>::T;
      return ZuHash<uint8_t>::hash(i) ^ ZuUnion_Ops<T>::hash(m_u);
    });
  }

  ZuInline unsigned type() const { return m_u[Size]; }

  template <unsigned I>
  const typename Type_<I>::T &p() const {
    using T = typename Type<I>::T;
    static const T null = ZuCmp<T>::null();
    if (this->type() != I) return null;
    const T *ZuMayAlias(ptr) = (const T *)(const void *)m_u;
    return *ptr;
  }
  template <unsigned I, typename P>
  ZuUnion &p(P &&p) {
    using T = typename Type<I>::T;
    if (this->type() == I) {
      T *ZuMayAlias(ptr) = (T *)(void *)m_u;
      *ptr = ZuFwd<P>(p);
      return *this;
    }
    this->~ZuUnion();
    this->type(I);
    ZuUnion_Ops<T>::ctor(m_u, ZuFwd<P>(p));
    return *this;
  }
  template <unsigned I>
  typename Type_<I>::T &p() {
    using T = typename Type<I>::T;
    T *ZuMayAlias(ptr) = (T *)(void *)m_u;
    if (this->type() == I) return *ptr;
    this->~ZuUnion();
    this->type(I);
    ZuUnion_Ops<T>::ctor(m_u);
    return *ptr;
  }
  template <unsigned I>
  typename Type<I>::T *ptr() {
    using T = typename Type<I>::T;
    if (this->type() != I) return 0;
    T *ZuMayAlias(ptr) = (T *)(void *)m_u;
    return ptr;
  }
  template <unsigned I>
  typename Type<I>::T *new_() {
    using T = typename Type<I>::T;
    this->~ZuUnion();
    this->type(I);
    T *ZuMayAlias(ptr) = (T *)(void *)m_u;
    return ptr;
  }

private:
  ZuInline unsigned type(unsigned i) { return m_u[Size] = i; }

  uint8_t	m_u[Size + 1];
};

template <typename Union, typename ...Args> struct ZuUnion_Traits;
template <typename Union>
struct ZuUnion_Traits<Union> : public ZuGenericTraits<Union> { };
template <typename Union, typename Arg0>
struct ZuUnion_Traits<Union, Arg0> : public ZuGenericTraits<Union> {
  enum {
    IsPOD = ZuTraits<Arg0>::IsPOD,
    IsComparable = ZuTraits<Arg0>::IsComparable,
    IsHashable = ZuTraits<Arg0>::IsHashable
  };
};
template <typename Union, typename Arg0, typename ...Args>
struct ZuUnion_Traits<Union, Arg0, Args...> : public ZuGenericTraits<Union> {
private:
  using Left = ZuTraits<Arg0>;
  using Right = ZuUnion_Traits<Union, Args...>;
public:
  enum {
    IsPOD = Left::IsPOD && Right::IsPOD,
    IsComparable = Left::IsComparable && Right::IsComparable,
    IsHashable = Left::IsHashable && Right::IsHashable
  };
};
template <typename ...Args>
struct ZuTraits<ZuUnion<Args...>> :
    public ZuUnion_Traits<ZuUnion<Args...>, Args...> { };

#define ZuUnion_FieldType(args) \
  ZuPP_Defer(ZuUnion_FieldType_)()(ZuPP_Strip(args))
#define ZuUnion_FieldType_() ZuUnion_FieldType__
#define ZuUnion_FieldType__(type, fn) ZuPP_Strip(type)

#define ZuUnion_FieldFn(N, args) \
  ZuPP_Defer(ZuUnion_FieldFn_)()(N, ZuPP_Strip(args))
#define ZuUnion_FieldFn_() ZuUnion_FieldFn__
#define ZuUnion_FieldFn__(N, type, fn) \
  ZuInline const auto &fn() const { return this->template p<N>(); } \
  ZuInline auto &fn() { return this->template p<N>(); } \
  template <typename P> \
  ZuInline auto &fn(P &&v) { this->template p<N>(ZuFwd<P>(v)); return *this; } \
  ZuInline auto ptr_##fn() { return this->template ptr<N>(); } \
  ZuInline auto new_##fn() { return this->template new_<N>(); }

#define ZuDeclUnion(Type, ...) \
using Type##_ = \
  ZuUnion<ZuPP_Eval(ZuPP_MapComma(ZuUnion_FieldType, __VA_ARGS__))>; \
class Type : public Type##_ { \
  using Union = Type##_; \
public: \
  using Union::Union; \
  using Union::operator =; \
  ZuInline Type(const Union &v) : Union(v) { }; \
  ZuInline Type(Union &&v) : Union(ZuMv(v)) { }; \
  ZuPP_Eval(ZuPP_MapIndex(ZuUnion_FieldFn, 1, __VA_ARGS__)) \
  struct Traits : public ZuTraits<Type##_> { using T = Type; }; \
  friend Traits ZuTraitsFn(const Type *); \
}

#endif /* ZuUnion_HPP */
