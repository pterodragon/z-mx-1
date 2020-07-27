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

// generic discriminated union
//
// using U = ZuUnion<int, double>;
// U u, v;
// *(u.init<int>()) = 42;
// u.p<0>(42);
// u.p<0>() = 42;
// v = u;
// if (v.type() == 0) { printf("%d\n", v.p<1>()); }
// v.p<1>(42.0);
// if (v.contains<double>()) { printf("%g\n", v.v<double>()); }
// u.~U();
// *reinterpret_cast<int *>(&u) = 43; // *(u.ptr_<Index<int>::I>()) = ...
// u.type_(U::Index<int>::I);
// printf("%d\n", u.v<int>());
//
// namespace {
//   void print(int i) const { printf("%d\n", i); }
//   void print(double d) const { printf("%g\n", d); }
// };
// u.cdispatch([](auto &&i) { print(ZuAutoFwd(i)); });
// ZuSwitch::dispatch<U::N>(u.type(), [&u](auto i) { print(u.p<i>()); });

#ifndef ZuUnion_HPP
#define ZuUnion_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

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
  ZuInline static bool star(const void *p) {
    return *(*static_cast<const T *>(p));
  }
};
template <typename T> class ZuUnion_OpStar<T, 0> {
  ZuInline static bool star(const void *p) {
    return !ZuCmp<T>::null(*static_cast<const T *>(p));
  }
};
template <typename T, bool CanBang> class ZuUnion_OpBang;
template <typename T> class ZuUnion_OpBang<T, 1> {
  ZuInline static bool bang(const void *p) {
    return !(*static_cast<const T *>(p));
  }
};
template <typename T> class ZuUnion_OpBang<T, 0> {
  ZuInline static bool bang(const void *p) {
    return ZuCmp<T>::null(*static_cast<const T *>(p));
  }
};
template <typename T, bool IsPrimitive, bool IsPointer> class ZuUnion_Ops_;
ZuCan(operator *, ZuUnion_CanStar);
ZuCan(operator !, ZuUnion_CanBang);
template <typename T> class ZuUnion_Ops_<T, 0, 0> :
  public ZuUnion_OpStar<T, ZuUnion_CanStar<T, bool (T::*)() const>::OK>,
  public ZuUnion_OpBang<T, ZuUnion_CanBang<T, bool (T::*)() const>::OK> {
public:
  ZuInline static void ctor(void *p) { new (p) T(); }
  template <typename V>
  ZuInline static void ctor(void *p, V &&v) { new (p) T(ZuFwd<V>(v)); }
  ZuInline static void dtor(void *p) { static_cast<T *>(p)->~T(); }
};
template <typename T> class ZuUnion_Ops_<T, 1, 0> {
public:
  ZuInline static void ctor(void *p) {
    *static_cast<T *>(p) = ZuCmp<T>::null();
  }
  template <typename V>
  ZuInline static void ctor(void *p, V &&v) {
    *static_cast<T *>(p) = ZuFwd<V>(v);
  }
  ZuInline static void dtor(void *p) { }
  ZuInline static bool star(const void *p) {
    return !ZuCmp<T>::null(*static_cast<const T *>(p));
  }
  ZuInline static bool bang(const void *p) {
    return !(*static_cast<const T *>(p));
  }
};
template <typename T> class ZuUnion_Ops_<T, 1, 1> {
public:
  ZuInline static void ctor(void *p) {
    *static_cast<T *>(p) = nullptr;
  }
  template <typename V>
  ZuInline static void ctor(void *p, V &&v) {
    *static_cast<T *>(p) = ZuFwd<V>(v);
  }
  ZuInline static void dtor(void *p) { }
  ZuInline static bool star(const void *p) {
    return (bool)(*static_cast<const T *>(p));
  }
  ZuInline static bool bang(const void *p) {
    return !(*static_cast<const T *>(p));
  }
};
template <typename T>
struct ZuUnion_Ops :
    public ZuUnion_Ops_<T,
      ZuTraits<T>::IsPrimitive,
      ZuTraits<T>::IsPrimitive && ZuTraits<T>::IsPointer> {
  template <typename V> ZuInline static void assign(void *p, V &&v)
    { *static_cast<T *>(p) = ZuFwd<V>(v); }
  template <typename V> ZuInline static bool less(const void *p, const V &v)
    { return ZuCmp<T>::less(*static_cast<const T *>(p), v); }
  template <typename V> ZuInline static bool equals(const void *p, const V &v)
    { return ZuCmp<T>::equals(*static_cast<const T *>(p), v); }
  template <typename V> ZuInline static int cmp(const void *p, const V &v)
    { return ZuCmp<T>::cmp(*static_cast<const T *>(p), v); }
  ZuInline static uint32_t hash(const void *p)
    { return ZuHash<T>::hash(*static_cast<const T *>(p)); }
};

template <unsigned I, typename Left> struct ZuUnion_Type0;
template <unsigned I, typename Left> struct ZuUnion_Type0_;
template <typename Left>
struct ZuUnion_Type0<0, Left> { using T = Left; };
template <typename Left>
struct ZuUnion_Type0_<0, Left> { using T = typename ZuDeref<Left>::T; };

template <typename ...Args> class ZuUnion_;
template <typename Arg0> class ZuUnion_<Arg0> {
public:
  template <unsigned I> using Type = ZuUnion_Type0<I, Arg0>;
  template <unsigned I> using Type_ = ZuUnion_Type0_<I, Arg0>;
  enum { N = 1 };
};

template <unsigned I, typename Left, typename Right>
struct ZuUnion_Type : public Right::template Type<I - 1> { };
template <unsigned I, typename Left, typename Right>
struct ZuUnion_Type_ : public Right::template Type_<I - 1> { };
template <typename Left, typename Right>
struct ZuUnion_Type<0, Left, Right> { using T = Left; };
template <typename Left, typename Right>
struct ZuUnion_Type_<0, Left, Right> { using T = typename ZuDeref<Left>::T; };

template <typename Arg0, typename ...Args>
class ZuUnion_<Arg0, Args...> {
  using Left = Arg0;
  using Right = ZuUnion_<Args...>;

public:
  template <unsigned I> using Type = ZuUnion_Type<I, Left, Right>;
  template <unsigned I> using Type_ = ZuUnion_Type_<I, Left, Right>;
  enum { N = Right::N + 1 };
};

template <typename ...Args> struct ZuUnion_Index_ { };
template <typename, typename> struct ZuUnion_Index;
template <typename T, typename ...Args>
struct ZuUnion_Index<T, ZuUnion_Index_<T, Args...>> {
  enum { I = 0 };
};
template <typename T, typename O, typename ...Args>
struct ZuUnion_Index<T, ZuUnion_Index_<O, Args...>> {
  enum { I = 1 + ZuUnion_Index<T, ZuUnion_Index_<Args...>>::I };
};

template <typename ...Args> class ZuUnion {
public:
  using Largest = typename ZuLargest<Args...>::T;
  enum { Size = sizeof(Largest) };
  template <unsigned I>
  using Type = typename ZuUnion_<Args...>::template Type<I>;
  template <unsigned I>
  using Type_ = typename ZuUnion_<Args...>::template Type_<I>;
  enum { N = ZuUnion_<Args...>::N };

  template <typename T>
  using Index = ZuUnion_Index<T, ZuUnion_Index_<Args...>>;

  ZuUnion() {
    using T0 = typename Type<0>::T;
    type_(0);
    new (&m_u[0]) T0{};
  }

  ~ZuUnion() {
    ZuSwitch::dispatch<N>(type(), [this](auto i) {
      using T = typename Type<i>::T;
      ZuUnion_Ops<T>::dtor(m_u);
    });
  }

  ZuUnion(const ZuUnion &u) {
    ZuSwitch::dispatch<N>(type_(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(u.m_u);
      ZuUnion_Ops<T>::ctor(m_u, *ptr);
    });
  }
  ZuUnion(ZuUnion &&u) {
    ZuSwitch::dispatch<N>(type_(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      T *ZuMayAlias(ptr) = reinterpret_cast<T *>(u.m_u);
      ZuUnion_Ops<T>::ctor(m_u, ZuMv(*ptr));
    });
  }

  ZuUnion &operator =(const ZuUnion &u) {
    if (this == &u) return *this;
    if (type() != u.type()) {
      this->~ZuUnion();
      new (this) ZuUnion(u);
      return *this;
    }
    ZuSwitch::dispatch<N>(type_(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(u.m_u);
      ZuUnion_Ops<T>::assign(m_u, *ptr);
    });
    return *this;
  }
  ZuUnion &operator =(ZuUnion &&u) {
    if (type() != u.type()) {
      this->~ZuUnion();
      new (this) ZuUnion(ZuMv(u));
      return *this;
    }
    ZuSwitch::dispatch<N>(type_(u.type()), [this, &u](auto i) {
      using T = typename Type<i>::T;
      T *ZuMayAlias(ptr) = reinterpret_cast<T *>(u.m_u);
      ZuUnion_Ops<T>::assign(m_u, ZuMv(*ptr));
    });
    return *this;
  }

  template <typename T>
  ZuInline static T *new_(void *ptr) {
    reinterpret_cast<ZuUnion *>(ptr)->type_(Index<T>::I);
    return reinterpret_cast<T *>(ptr);
  }

  template <typename T> void *init() {
    this->~ZuUnion();
    this->type_(Index<T>::I);
    return m_u;
  }

  ZuInline unsigned type_(unsigned i) { return m_u[Size] = i; }

  void null() {
    ZuSwitch::dispatch<N>(type(), [this](auto i) {
      using T = typename Type<i>::T;
      ZuUnion_Ops<T>::dtor(m_u);
    });
    type_(0);
  }

  template <typename P>
  typename ZuIs<ZuUnion, P, int>::T cmp(const P &p) const {
    if (this == &p) return true;
    if (int i = ZuCmp<uint8_t>::cmp(type(), p.type())) return i;
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> int {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(p.m_u);
      return ZuUnion_Ops<T>::cmp(m_u, *ptr);
    });
  }
  template <typename P>
  typename ZuIsNot<ZuUnion, P, int>::T cmp(const P &p) const {
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> int {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::cmp(m_u, p);
    });
  }

  template <typename P>
  typename ZuIs<ZuUnion, P, bool>::T less(const P &p) const {
    if (this == &p) return true;
    if (type() != p.type()) return false;
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(p.m_u);
      return ZuUnion_Ops<T>::less(m_u, *ptr);
    });
  }
  template <typename P>
  typename ZuIsNot<ZuUnion, P, bool>::T less(const P &p) const {
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::less(m_u, p);
    });
  }

  template <typename P>
  typename ZuIs<ZuUnion, P, bool>::T equals(const P &p) const {
    if (this == &p) return true;
    if (type() != p.type()) return false;
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(p.m_u);
      return ZuUnion_Ops<T>::equals(m_u, *ptr);
    });
  }
  template <typename P>
  typename ZuIsNot<ZuUnion, P, bool>::T equals(const P &p) const {
    return ZuSwitch::dispatch<N>(type(), [this, &p](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::equals(m_u, p);
    });
  }

  ZuInline bool operator ==(const ZuUnion &p) const { return equals(p); }
  ZuInline bool operator !=(const ZuUnion &p) const { return !equals(p); }
  ZuInline bool operator >(const ZuUnion &p) const { return p.less(*this); }
  ZuInline bool operator >=(const ZuUnion &p) const { return !less(p); }
  ZuInline bool operator <(const ZuUnion &p) const { return less(p); }
  ZuInline bool operator <=(const ZuUnion &p) const { return !p.less(*this); }

  bool operator *() const {
    return ZuSwitch::dispatch<N>(type(), [this](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::star(m_u);
    });
  }

  bool operator !() const {
    return ZuSwitch::dispatch<N>(type(), [this](auto i) -> bool {
      using T = typename Type<i>::T;
      return ZuUnion_Ops<T>::bang(m_u);
    });
  }

  uint32_t hash() const {
    return ZuSwitch::dispatch<N>(type(), [this](auto i) -> uint32_t {
      using T = typename Type<i>::T;
      return ZuHash<uint8_t>::hash(i) ^ ZuUnion_Ops<T>::hash(m_u);
    });
  }

  ZuInline unsigned type() const { return m_u[Size]; }

  template <typename T>
  ZuInline bool contains() const { return type() == Index<T>::I; }

  template <unsigned I>
  const typename Type_<I>::T &p() const {
    using T = typename Type<I>::T;
    static const T null = ZuCmp<T>::null();
    if (type() != I) return null;
    const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(m_u);
    return *ptr;
  }
  template <unsigned I, typename P>
  ZuUnion &p(P &&p) {
    using T = typename Type<I>::T;
    if (type() == I) {
      T *ZuMayAlias(ptr) = reinterpret_cast<T *>(m_u);
      *ptr = ZuFwd<P>(p);
      return *this;
    }
    this->~ZuUnion();
    type_(I);
    ZuUnion_Ops<T>::ctor(m_u, ZuFwd<P>(p));
    return *this;
  }
  template <unsigned I>
  typename Type_<I>::T &p() {
    using T = typename Type<I>::T;
    T *ZuMayAlias(ptr) = reinterpret_cast<T *>(m_u);
    if (type() == I) return *ptr;
    this->~ZuUnion();
    type_(I);
    ZuUnion_Ops<T>::ctor(m_u);
    return *ptr;
  }

  template <typename T>
  ZuInline const typename Type_<Index<T>::I>::T &v() const {
    return this->template p<Index<T>::I>();
  }
  template <typename T, typename P>
  ZuInline ZuUnion &v(P &&p) {
    return this->template p<Index<T>::I>(ZuFwd<P>(p));
  }
  template <typename T>
  ZuInline typename Type_<Index<T>::I>::T &v() {
    return this->template p<Index<T>::I>();
  }

  template <unsigned I>
  const typename Type<I>::T *ptr() const {
    if (type() != I) return nullptr;
    return ptr_<I>();
  }
  template <unsigned I>
  typename Type<I>::T *ptr() {
    if (type() != I) return nullptr;
    return ptr_<I>();
  }
  template <unsigned I>
  const typename Type<I>::T *ptr_() const {
    using T = typename Type<I>::T;
    const T *ZuMayAlias(ptr) = reinterpret_cast<const T *>(m_u);
    return ptr;
  }
  template <unsigned I>
  typename Type<I>::T *ptr_() {
    using T = typename Type<I>::T;
    T *ZuMayAlias(ptr) = reinterpret_cast<T *>(m_u);
    return ptr;
  }

  template <typename L>
  auto dispatch(L l) {
    return ZuSwitch::dispatch<N>(
	type(), [this, &l](auto i) mutable {
	  return l(this->template p<i>());
	});
  }
  template <typename L>
  auto cdispatch(L l) const {
    return ZuSwitch::dispatch<N>(
	type(), [this, &l](auto i) mutable {
	  return l(this->template p<i>());
	});
  }

private:
  uint8_t	m_u[Size + 1];
};

template <typename Union, typename ...Args> struct ZuUnion_Traits;
template <typename Union>
struct ZuUnion_Traits<Union> : public ZuGenericTraits<Union> { };
template <typename Union, typename Arg0>
struct ZuUnion_Traits<Union, Arg0> : public ZuGenericTraits<Union> {
  enum {
    IsPOD = ZuTraits<Arg0>::IsPOD,
    IsComparable = 1,
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
    IsComparable = 1,
    IsHashable = Left::IsHashable && Right::IsHashable
  };
};
template <typename ...Args>
struct ZuTraits<ZuUnion<Args...>> :
    public ZuUnion_Traits<ZuUnion<Args...>, Args...> { };

// STL variant cruft
#include <type_traits>
#include <variant>
namespace std {
  template <size_t, typename> struct tuple_element;
  template <size_t I, typename ...Args>
  struct tuple_element<I, ZuUnion<Args...>> {
    using type = typename ZuUnion<Args...>::template Type<I>::T;
  };

  template <size_t I, typename ...Args>
  constexpr tuple_element_t<I, ZuUnion<Args...>> &
  get(ZuUnion<Args...> &p) {
    if (ZuUnlikely(p.type() != I)) throw bad_variant_access{};
    return p.template p<I>();
  }
  template <size_t I, typename ...Args>
  constexpr const tuple_element_t<I, ZuUnion<Args...>> &
  get(const ZuUnion<Args...> &p) {
    if (ZuUnlikely(p.type() != I)) throw bad_variant_access{};
    return p.template p<I>();
  }
  template <size_t I, typename ...Args>
  constexpr tuple_element_t<I, ZuUnion<Args...>> &&
  get(ZuUnion<Args...> &&p) {
    if (ZuUnlikely(p.type() != I)) throw bad_variant_access{};
    return static_cast<tuple_element_t<I, ZuUnion<Args...>> &&>(
	p.template p<I>());
  }
  template <size_t I, typename ...Args>
  constexpr const tuple_element_t<I, ZuUnion<Args...>> &&
  get(const ZuUnion<Args...> &&p) {
    if (ZuUnlikely(p.type() != I)) throw bad_variant_access{};
    return static_cast<const tuple_element_t<I, ZuUnion<Args...>> &&>(
	p.template p<I>());
  }

  template <typename T, typename ...Args>
  constexpr T &get(ZuUnion<Args...> &p) {
    if (ZuUnlikely(p.type() != ZuUnion<Args...>::template Index<T>::I))
      throw bad_variant_access{};
    return p.template v<T>();
  }
  template <typename T, typename ...Args>
  constexpr const T &get(const ZuUnion<Args...> &p) {
    if (ZuUnlikely(p.type() != ZuUnion<Args...>::template Index<T>::I))
      throw bad_variant_access{};
    return p.template v<T>();
  }
  template <typename T, typename ...Args>
  constexpr T &&get(ZuUnion<Args...> &&p) {
    if (ZuUnlikely(p.type() != ZuUnion<Args...>::template Index<T>::I))
      throw bad_variant_access{};
    return static_cast<T &&>(p.template v<T>());
  }
  template <typename T, typename ...Args>
  constexpr const T &&get(const ZuUnion<Args...> &&p) {
    if (ZuUnlikely(p.type() != ZuUnion<Args...>::template Index<T>::I))
      throw bad_variant_access{};
    return static_cast<const T &&>(p.template v<T>());
  }

  template <class> struct tuple_size;
  template <typename ...Args>
  struct tuple_size<ZuUnion<Args...>> :
  public integral_constant<std::size_t, sizeof...(Args)> { };
}

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
  ZuInline auto init_##fn() { return this->template init<ZuPP_Strip(type)>(); }

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
  ZuPP_Eval(ZuPP_MapIndex(ZuUnion_FieldFn, 0, __VA_ARGS__)) \
  struct Traits : public ZuTraits<Type##_> { using T = Type; }; \
  friend Traits ZuTraitsType(const Type *); \
}

#endif /* ZuUnion_HPP */
