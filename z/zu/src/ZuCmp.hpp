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

// generic comparison (including distinguished null values)

// UDTs must implement the <, >, == and ! operators
//
// For more efficiency, specialize ZuTraits and implement the cmp() method
// (replaces calls to operators < and >; operators == and ! are still called)
//
// template <> ZuTraits<UDT> : public ZuGenericTraits<UDT> {
//   enum { IsComparable = 1 };
// };
// class UDT {
//   ...
//   inline int cmp(const UDT &t) { ... }
//   // returns -ve if *this < t, 0 if *this == t, +ve if *this > t
//   ...
// };
//
// unless ZuCmp<UDT> is explicitly specialized, UDT::operator !(UDT())
// should always return true
//
// to specialize ZuCmp<UDT>, implement the following:
//
// template <> struct ZuCmp<UDT> {
//   inline static int cmp(const UDT &t1, const UDT &t2) { ... }
//   // returns -ve if t1 < t2, 0 if t1 == t2, +ve if t1 > t2
//   inline static bool equals(const UDT &t1, const UDT &t2) { ... }
//   // returns t1 == t2
//   inline static bool null(const UDT &t) { ... }
//   // returns !t
//   inline static const UDT &null() { static UDT udt = ...; return udt; }
//   // returns distinguished "null" value for UDT
// };
//
// ZuCmp<T>::null() returns a distinguished "null" value for T that may
// (or may not) lie within the normal range of values for T; the default
// ZuCmp<T>::null() performs as follows:
//
// Type			Null	Notes
// ----			----	-----
// UDT			T()	result of default constructor
// bool			*	cast of minimum of same-sized signed int type
// char			0	ASCII NUL
// signed integer	MIN	minimum representable value
// unsigned integer	MAX	maximum representable value
// floating point	NaN	IEEE 754 NaN
// pointer, C string	0	aka NULL
//
// Note: (*) bool has no portably distinguishable null value, this is a hack;
//	 furthermore, ZuCmp<bool>::null(true) will probably evaluate as true
//
// Note: int8_t  is a typedef for signed char
//	 uint8_t is a typedef for unsigned char
//	 char is a third, distinct type from both signed char and unsigned char
//	 (but which has the same representation, values, alignment and size
//	 as one of them, depending on the implementation)

#ifndef ZuCmp_HPP
#define ZuCmp_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <string.h>
#include <wchar.h>

#include <ZuTraits.hpp>
#include <ZuInt.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

template <typename T> struct ZuCmp;

// deliberately undefined template

template <typename T> struct ZuCmp_Cannot;

// null sentinel values (min for signed, max for unsigned) for integral types

template <typename T, int Size, bool Signed> struct ZuCmp_IntNull;
template <typename T> struct ZuCmp_IntNull<T, 1, false>
  { ZuInline static constexpr const T null() { return (T)0xff; } };
template <typename T> struct ZuCmp_IntNull<T, 1, true>
  { ZuInline static constexpr const T null() { return (T)-0x80; } };
template <typename T> struct ZuCmp_IntNull<T, 2, false>
  { ZuInline static constexpr const T null() { return (T)0xffff; } };
template <typename T> struct ZuCmp_IntNull<T, 2, true>
  { ZuInline static constexpr const T null() { return (T)-0x8000; } };
template <typename T> struct ZuCmp_IntNull<T, 4, false>
  { ZuInline static constexpr const T null() { return (T)~0; } };
template <typename T> struct ZuCmp_IntNull<T, 4, true>
  { ZuInline static constexpr const T null() { return (T)-0x80000000; } };
template <typename T> struct ZuCmp_IntNull<T, 8, false>
  { ZuInline static constexpr const T null() { return (T)~0ULL; } };
template <typename T> struct ZuCmp_IntNull<T, 8, true> {
  ZuInline static constexpr const T null() { return (T)-0x8000000000000000LL; }
};

// comparison of larger-sized (>= sizeof(int)) integral types

template <typename T, bool isSmallInt> struct ZuCmp_Integral {
  ZuInline static int cmp(T i1, T i2) {
    return (i1 > i2) ? 1 : (i1 < i2) ? -1 : 0;
  }
  ZuInline static bool equals(T i1, T i2) { return i1 == i2; }
  ZuInline static bool null(T i) { return i == null(); }
  inline static const T &null() {
    static const T i =
      ZuCmp_IntNull<T, sizeof(T), ZuTraits<T>::IsSigned>::null();
    return i;
  }
  ZuInline static T epsilon(T f) { return 0; }
};

// comparison of small-sized (< sizeof(int)) integral types

template <typename T> struct ZuCmp_Integral<T, true> {
  ZuInline static int cmp(T i1, T i2) { return (int)i1 - (int)i2; }
  ZuInline static bool equals(T i1, T i2) { return i1 == i2; }
  ZuInline static bool null(T i) { return i == null(); }
  inline static const T &null() {
    static const T i =
      ZuCmp_IntNull<T, sizeof(T), ZuTraits<T>::IsSigned>::null();
    return i;
  }
  ZuInline static T epsilon(T f) { return 0; }
};

// comparison of floating point types

template <typename T> struct ZuCmp_Floating {
  ZuInline static int cmp(T f1, T f2) { return f1 > f2 ? 1 : f1 < f2 ? -1 : 0; }
  ZuInline static bool equals(T f1, T f2) { return f1 == f2; }
  ZuInline static T null() { return ZuTraits<T>::nan(); }
  ZuInline static bool null(T f) { return ZuTraits<T>::nan(f); }
  ZuInline static T inf() { return ZuTraits<T>::inf(); }
  ZuInline static bool inf(T f) { return ZuTraits<T>::inf(f); }
  ZuInline static T epsilon(T f) { return ZuTraits<T>::epsilon(f); }
};

// comparison of real numbers (integral and floating point)

template <typename T, bool isIntegral> struct ZuCmp_Real;
template <typename T> struct ZuCmp_Real<T, false> :
  public ZuCmp_Floating<T> { };
template <typename T> struct ZuCmp_Real<T, true> :
  public ZuCmp_Integral<T, sizeof(T) < sizeof(int)> { };

// comparison of primitive types

template <typename T, bool IsReal, bool IsVoid>
struct ZuCmp_Primitive : public ZuCmp_Cannot<T> { };
template <typename T> struct ZuCmp_Primitive<T, true, false> :
  public ZuCmp_Real<T, ZuTraits<T>::IsIntegral> { };

// default comparison of non-primitive types - uses operators >, <, ==, !

template <typename T, bool IsComparable, bool Fwd> struct ZuCmp_NonPrimitive {
  template <typename P1, typename P2>
  ZuInline static int cmp(const P1 &p1, const P2 &p2) {
    return (p1 > p2) ? 1 : (p1 < p2) ? -1 : 0;
  }
  template <typename P1, typename P2>
  ZuInline static bool equals(const P1 &p1, const P2 &p2) { return p1 == p2; }
  template <typename P> inline static bool null(const P &p) { return !p; }
  ZuInline static const T &null() { static const T t; return t; }
};

template <typename P1, bool IsPrimitive> struct ZuCmp_NonPrimitive___ :
    public ZuCmp_NonPrimitive<P1, ZuTraits<P1>::IsComparable, false> { };
template <typename P1>
struct ZuCmp_NonPrimitive___<P1, true> : public ZuCmp<P1> { };
template <typename T, typename P1> struct ZuCmp_NonPrimitive__ :
  public ZuCmp_NonPrimitive___<P1, ZuTraits<P1>::IsPrimitive> { };
template <typename T> struct ZuCmp_NonPrimitive__<T, T> {
  template <typename P2>
  ZuInline static int cmp(const T &t, const P2 &p2) { return t.cmp(p2); }
};
template <typename T, typename P1, bool Fwd>
struct ZuCmp_NonPrimitive_ : public ZuCmp_NonPrimitive__<T, P1> { };
template <typename T, typename P1> struct ZuCmp_NonPrimitive_<T, P1, false> {
  template <typename P2>
  ZuInline static int cmp(const T &t, const P2 &p2) { return t.cmp(p2); }
};
template <typename T, bool Fwd> struct ZuCmp_NonPrimitive<T, true, Fwd> {
  template <typename P1, typename P2>
  ZuInline static int cmp(const P1 &p1, const P2 &p2) {
    return ZuCmp_NonPrimitive_<T, typename ZuTraits<P1>::T, Fwd>::cmp(p1, p2);
  }
  template <typename P1, typename P2>
  ZuInline static bool equals(const P1 &p1, const P2 &p2) { return p1 == p2; }
  template <typename P> inline static bool null(const P &p) { return !p; }
  inline static const T &null() { static const T t; return t; }
};

// comparison of pointers (including smart pointers)

template <typename T> struct ZuCmp_Pointer {
  typedef typename ZuTraits<T>::Elem P;
  ZuInline static int cmp(const P *p1, const P *p2) {
    return (char *)p1 > (char *)p2 ? 1 : (char *)p1 < (char *)p2 ? -1 : 0;
  }
  ZuInline static bool equals(const P *p1, const P *p2) { return p1 == p2; }
  ZuInline static bool null(const P *p) { return !p; }
  inline static const T &null() { static const T t = 0; return t; }
};

template <typename T, bool IsCString>
struct ZuCmp_PrimitivePointer : public ZuCmp_Cannot<T> { };

template <typename T> struct ZuCmp_PrimitivePointer<T, false> :
  public ZuCmp_Pointer<T> { };

// non-string comparison

template <typename T, bool IsPrimitive, bool IsPointer> struct ZuCmp_NonString;

template <typename T> struct ZuCmp_NonString<T, false, false> :
  public ZuCmp_NonPrimitive<T, ZuTraits<T>::IsComparable, true> { };

template <typename T> struct ZuCmp_NonString<T, false, true> :
  public ZuCmp_Pointer<T> { };

template <typename T> struct ZuCmp_NonString<T, true, false> :
  public ZuCmp_Primitive<T, ZuTraits<T>::IsReal, ZuTraits<T>::IsVoid> { };

template <typename T> struct ZuCmp_NonString<T, true, true> :
  public ZuCmp_PrimitivePointer<T, ZuTraits<T>::IsCString> { };

// string comparison

template <typename T1, typename T2,
	  bool T1IsCStringAndT2IsCString,
	  bool T1IsString,
	  bool T1IsWStringAndT2IsWString> struct ZuCmp_StrCmp;

template <typename T1, typename T2, bool T1IsString>
struct ZuCmp_StrCmp<T1, T2, 1, T1IsString, 0> {
  inline static int cmp(const T1 &s1_, const T2 &s2_) {
    const char *s1 = ZuTraits<T1>::data(s1_);
    const char *s2 = ZuTraits<T2>::data(s2_);
    if (!s1) {
      if (!s2) return 0;
      s1 = "";
    } else {
      if (!s2) s2 = "";
    }
    return strcmp(s1, s2);
  }
  inline static bool equals(const T1 &s1_, const T2 &s2_) {
    const char *s1 = ZuTraits<T1>::data(s1_);
    const char *s2 = ZuTraits<T2>::data(s2_);
    if (!s1) {
      if (!s2) return true;
      s1 = "";
    } else {
      if (!s2) s2 = "";
    }
    return !strcmp(s1, s2);
  }
};
template <typename T1, typename T2>
struct ZuCmp_StrCmp<T1, T2, 0, 1, 0> {
  inline static int cmp(const T1 &s1_, const T2 &s2_) {
    const char *s1 = ZuTraits<T1>::data(s1_);
    const char *s2 = ZuTraits<T2>::data(s2_);
    if (!s1) return s2 ? -1 : 0;
    if (!s2) return 1;
    int l1 = ZuTraits<T1>::length(s1_), l2 = ZuTraits<T2>::length(s2_);
    int i = strncmp(s1, s2, l1 > l2 ? l2 : l1);
    if (i) return i;
    return l1 > l2 ? 1 : l1 < l2 ? -1 : 0;
  }
  inline static bool equals(const T1 &s1_, const T2 &s2_) {
    const char *s1 = ZuTraits<T1>::data(s1_);
    const char *s2 = ZuTraits<T2>::data(s2_);
    if (!s1) return !s2;
    if (!s2) return false;
    int l1 = ZuTraits<T1>::length(s1_), l2 = ZuTraits<T2>::length(s2_);
    if (l1 != l2) return false;
    return !strncmp(s1, s2, l1);
  }
};
template <typename T1, typename T2, bool T1IsString>
struct ZuCmp_StrCmp<T1, T2, 1, T1IsString, 1> {
  inline static int cmp(const T1 &w1_, const T2 &w2_) {
    const wchar_t *w1 = ZuTraits<T1>::data(w1_);
    const wchar_t *w2 = ZuTraits<T2>::data(w2_);
    if (!w1) return w2 ? -1 : 0;
    if (!w2) return 1;
    return wcscmp(w1, w2);
  }
  inline static bool equals(const T1 &w1_, const T2 &w2_) {
    const wchar_t *w1 = ZuTraits<T1>::data(w1_);
    const wchar_t *w2 = ZuTraits<T2>::data(w2_);
    if (!w1) return !w2;
    if (!w2) return false;
    return !wcscmp(w1, w2);
  }
};
template <typename T1, typename T2>
struct ZuCmp_StrCmp<T1, T2, 0, 1, 1> {
  inline static int cmp(const T1 &w1_, const T2 &w2_) {
    const wchar_t *w1 = ZuTraits<T1>::data(w1_);
    const wchar_t *w2 = ZuTraits<T2>::data(w2_);
    if (!w1) return w2 ? -1 : 0;
    if (!w2) return 1;
    int l1 = ZuTraits<T1>::length(w1_), l2 = ZuTraits<T2>::length(w2_);
    int i = wcsncmp(w1, w2, l1 > l2 ? l2 : l1);
    if (i) return i;
    return l1 > l2 ? 1 : l1 < l2 ? -1 : 0;
  }
  inline static bool equals(const T1 &w1_, const T2 &w2_) {
    const wchar_t *w1 = ZuTraits<T1>::data(w1_);
    const wchar_t *w2 = ZuTraits<T2>::data(w2_);
    if (!w1) return !w2;
    if (!w2) return false;
    int l1 = ZuTraits<T1>::length(w1_), l2 = ZuTraits<T2>::length(w1_);
    if (l1 != l2) return false;
    return !wcsncmp(w1, w2, l1);
  }
};

template <typename T, bool IsCString, bool IsString, bool IsWString>
struct ZuCmp_String;

template <typename T, bool IsString>
struct ZuCmp_String<T, 1, IsString, 0> {
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsCString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      !ZuTraits<S2>::IsWString, int>::T cmp(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2,
			ZuTraits<S2>::IsCString,
			ZuTraits<S1>::IsString, 0>::cmp(s1, s2);
  }
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsCString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      !ZuTraits<S2>::IsWString, bool>::T equals(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2,
			ZuTraits<S2>::IsCString,
			ZuTraits<S1>::IsString, 0>::equals(s1, s2);
  }
  ZuInline static bool null(const char *s) { return !s || !*s; }
  inline static const T &null() {
    static const T t = (const T)(const char *)0; return t;
  }
};
template <typename T>
struct ZuCmp_String<T, 0, 1, 0> {
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      !ZuTraits<S2>::IsWString, int>::T cmp(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2, 0, 1, 0>::cmp(s1, s2);
  }
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      !ZuTraits<S2>::IsWString, bool>::T equals(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2, 0, 1, 0>::equals(s1, s2);
  }
  ZuInline static bool null(const T &s) { return !s; }
  inline static const T &null() { static const T t; return t; }
};
template <typename T, bool IsString>
struct ZuCmp_String<T, 1, IsString, 1> {
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsCString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      ZuTraits<S2>::IsWString, int>::T cmp(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2,
			ZuTraits<S2>::IsCString,
			ZuTraits<S1>::IsString, 1>::cmp(s1, s2);
  }
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsCString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      ZuTraits<S2>::IsWString, bool>::T equals(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2,
			ZuTraits<S2>::IsCString,
			ZuTraits<S1>::IsString, 1>::equals(s1, s2);
  }
  ZuInline static bool null(const wchar_t *w) { return !w || !*w; }
  inline static const T &null() {
    static const T t = (const T)(const wchar_t *)0;
    return t;
  }
};
template <typename T>
struct ZuCmp_String<T, 0, 1, 1> {
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      ZuTraits<S2>::IsWString, int>::T cmp(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2, 0, 1, 1>::cmp(s1, s2);
  }
  template <typename S1, typename S2>
  ZuInline static typename ZuIfT<
      ZuTraits<S1>::IsString &&
      (ZuTraits<S2>::IsCString || ZuTraits<S2>::IsString) &&
      ZuTraits<S2>::IsWString, bool>::T equals(const S1 &s1, const S2 &s2) {
    return ZuCmp_StrCmp<S1, S2, 0, 1, 1>::equals(s1, s2);
  }
  ZuInline static bool null(const T &s) { return !s; }
  inline static const T &null() { static const T t; return t; }
};

// generic comparison function

template <typename T, bool IsString> struct ZuCmp_;

template <typename T> struct ZuCmp_<T, false> :
  public ZuCmp_NonString<T,
    ZuTraits<T>::IsPrimitive, ZuTraits<T>::IsPointer> { };

template <typename T> struct ZuCmp_<T, true> :
  public ZuCmp_String<T,
    ZuTraits<T>::IsCString, ZuTraits<T>::IsString, ZuTraits<T>::IsWString> { };

// generic template

template <typename T> struct ZuCmp :
  public ZuCmp_<typename ZuTraits<T>::T,
    ZuTraits<T>::IsCString || ZuTraits<T>::IsString> { };

// equivalent signed integral types for various possible sizes of bool

template <int Size> struct ZuCmp_BoolInt;
template <> struct ZuCmp_BoolInt<1> { typedef int8_t T; };
template <> struct ZuCmp_BoolInt<2> { typedef int16_t T; };
template <> struct ZuCmp_BoolInt<4> { typedef int32_t T; };
template <> struct ZuCmp_BoolInt<8> { typedef int64_t T; };

// comparison of bool

template <> struct ZuCmp<bool> {
  typedef ZuCmp_BoolInt<sizeof(bool)>::T I;

  ZuInline static int cmp(bool b1, bool b2)
    { return (b1 && !b2) ? 1 : (!b1 && b2) ? -1 : 0; }
  ZuInline static bool equals(bool b1, bool b2) { return b1 == b2; }
  ZuInline static bool null(bool b) {
    const I *ZuMayAlias(b_) = (const I *)&b;
    return ZuCmp<I>::null(*b_);
  }
  inline static const bool &null() {
    static const I i = ZuCmp<I>::null();
    const bool *ZuMayAlias(i_) = (const bool *)&i;
    return *i_;
  }
};

// comparison of char (null value should be '\0')

template <> struct ZuCmp<char> {
  ZuInline static int cmp(char c1, char c2) { return (int)c1 - (int)c2; }
  ZuInline static bool equals(char c1, char c2) { return c1 == c2; }
  ZuInline static bool null(char c) { return !c; }
  ZuInline static const char &null() { static const char c = 0; return c; }
};

// comparison of void

template <> struct ZuCmp<void> { inline static void null() { } };

// same as ZuCmp<T> but with 0 as the null value

template <typename T> struct ZuCmp0 : public ZuCmp<T> {
  ZuInline static bool null(T t) { return !t; }
  inline static const T &null() { static const T v = (T)0; return v; }
};

// same as ZuCmp<T> but with -1 (or any negative value) as the null value

template <typename T> struct ZuCmp_1 : public ZuCmp<T> {
  ZuInline static bool null(T v) { return v < 0; }
  inline static const T &null() { static const T v = (T)-1; return v; }
};

// same as ZuCmp<T> but with N as the null value

template <typename T, int N> struct ZuCmpN : public ZuCmp<T> {
  ZuInline static bool null(T v) { return v == (T)N; }
  inline static const T &null() { static const T v = (T)N; return v; }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuCmp_HPP */
