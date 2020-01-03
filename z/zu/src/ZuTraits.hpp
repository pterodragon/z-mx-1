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

// type traits

// template <> struct ZuTraits<UDT> : public ZuGenericTraits<UDT> {
//   enum { IsPOD = 1, IsHashable = 1, IsComparable = 1 }; // traits overrides
// };
// class UDT {
//   ...
//   inline int cmp(const UDT &t) { ... }
//   // returns -ve if *this < t, 0 if *this == t, +ve if *this > t
//   ...
// };

#ifndef ZuTraits_HPP
#define ZuTraits_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <wchar.h>
#include <string.h>
#include <limits.h>
#include <string.h>

#include <zlib/ZuFP.hpp>

// generic traits (overridden by specializations)

template <typename T, typename _ = void>
struct ZuTraits_Composite {
  enum { Is = 0 };
};
template <typename T>
struct ZuTraits_Composite<T, decltype((int T::*){}, void())> {
  enum { Is = 1 };
};

template <typename T> struct ZuTraits_Enum {
  enum { Is = __is_enum(T) }; // supported by VC++, gcc and clang
};

template <typename T_, bool Enum> struct ZuGenericTraits_ {
  enum {
    IsReference		= 0,	IsRvalueRef	= 0,	IsPointer	= 0,
    IsPrimitive		= Enum,	IsReal		= Enum,	IsSigned	= Enum,
    IsIntegral		= Enum,	IsFloatingPoint	= 0,	IsPOD		= Enum,
    IsString		= 0,	IsCString	= 0,	IsWString	= 0,
    IsVoid		= 0,	IsBool		= 0,	IsArray		= 0,
    IsHashable		= 0,	IsComparable	= 0,
    IsBoxed		= 0,	IsFixedPoint	= 0,
    IsComposite	= ZuTraits_Composite<T_>::Is,	// class/struct/union
    IsEnum = Enum
  };

  typedef T_ T;
  typedef void Elem;
};
template <typename T>
struct ZuGenericTraits : public ZuGenericTraits_<T, ZuTraits_Enum<T>::Is> { };

template <typename T> ZuGenericTraits<T> inline ZuTraitsFn(const T *);
template <typename T>
using ZuDefaultTraits = decltype(ZuTraitsFn((const T *)(const void *)0));

template <typename T> struct ZuTraits : public ZuDefaultTraits<T> { };

template <typename T> struct ZuTraits<const T> : public ZuTraits<T> { };
template <typename T> struct ZuTraits<volatile T> : public ZuTraits<T> { };
template <typename T>
struct ZuTraits<const volatile T> : public ZuTraits<T> { };

// real numbers

template <typename T> struct ZuTraits_Real : public ZuGenericTraits<T> {
  enum {
    IsPrimitive = 1, IsReal = 1, IsPOD = 1, IsHashable = 1, IsComparable = 1
  };
};

// signed integers

template <typename T> struct ZuTraits_Signed : public ZuTraits_Real<T> {
  enum { IsIntegral = 1, IsSigned = 1 };
};

// unsigned integers

template <typename T> struct ZuTraits_Unsigned : public ZuTraits_Real<T> {
  enum { IsIntegral = 1 };
};

// primitive integral types

template <> struct ZuTraits<bool> : public ZuTraits_Unsigned<bool> {
  enum { IsBool = 1 };
};
#if CHAR_MIN
template <> struct ZuTraits<char> : public ZuTraits_Signed<char> { };
#else
template <> struct ZuTraits<char> : public ZuTraits_Unsigned<char> { };
#endif
template <>
struct ZuTraits<signed char> : public ZuTraits_Signed<signed char> { };
template <>
struct ZuTraits<unsigned char> : public ZuTraits_Unsigned<unsigned char> { };
#if WCHAR_MIN
template <> struct ZuTraits<wchar_t> : public ZuTraits_Signed<wchar_t> { };
#else
template <>
struct ZuTraits<wchar_t> : public ZuTraits_Unsigned<wchar_t> { };
#endif
template <> struct ZuTraits<short> : public ZuTraits_Signed<short> { };
template <>
struct ZuTraits<unsigned short> : public ZuTraits_Unsigned<unsigned short> { };
template <> struct ZuTraits<int> : public ZuTraits_Signed<int> { };
template <>
struct ZuTraits<unsigned int> : public ZuTraits_Unsigned<unsigned int> { };
template <> struct ZuTraits<long> : public ZuTraits_Signed<long> { };
template <>
struct ZuTraits<unsigned long> : public ZuTraits_Unsigned<unsigned long> { };
template <>
struct ZuTraits<long long> : public ZuTraits_Signed<long long> { };
template <>
struct ZuTraits<unsigned long long> :
  public ZuTraits_Unsigned<unsigned long long> { };
template <>
struct ZuTraits<__int128_t> : public ZuTraits_Signed<__int128_t> { };
template <>
struct ZuTraits<__uint128_t> : public ZuTraits_Unsigned<__uint128_t> { };

// primitive floating point types

template <typename T>
struct ZuTraits_Floating : public ZuTraits_Real<T>, public ZuFP<T> {
  enum { IsSigned = 1, IsFloatingPoint = 1 };
};

template <> struct ZuTraits<float> : public ZuTraits_Floating<float> { };
template <> struct ZuTraits<double> : public ZuTraits_Floating<double> { };
template <>
struct ZuTraits<long double> : public ZuTraits_Floating<long double> { };

// references

// Note: ZuTraits<T>::T both dereferences and strips cv qualifiers,
// while ZuDeref<T>::T preserves cv qualifiers

template <typename T_> struct ZuTraits<T_ &> : public ZuTraits<T_> {
  enum { IsReference = 1 };
  typedef T_ T;
};

template <typename T> struct ZuTraits<const T &> : public ZuTraits<T &> { };
template <typename T> struct ZuTraits<volatile T &> : public ZuTraits<T &> { };
template <typename T>
struct ZuTraits<const volatile T &> : public ZuTraits<T &> { };

template <typename T_> struct ZuTraits<T_ &&> : public ZuTraits<T_> {
  enum { IsRvalueRef = 1 };
  typedef T_ T;
};

// pointers

template <typename T, typename Elem_>
struct ZuTraits_Pointer : public ZuGenericTraits<T> {
  enum {
    IsPrimitive = 1, IsPOD = 1, IsPointer = 1, IsHashable = 1, IsComparable = 1
  };
  typedef Elem_ Elem;
};

template <typename T>
struct ZuTraits<T *> : public ZuTraits_Pointer<T *, T> { };
template <typename T>
struct ZuTraits<const T *> :
  public ZuTraits_Pointer<const T *, const T> { };
template <typename T>
struct ZuTraits<volatile T *> :
  public ZuTraits_Pointer<volatile T *, volatile T> { };
template <typename T>
struct ZuTraits<const volatile T *> :
  public ZuTraits_Pointer<const volatile T *, const volatile T> { };

// primitive arrays

template <typename T, typename Elem_>
struct ZuTraits_Array : public ZuGenericTraits<T> {
  using Elem = Elem_;
  using ElemTraits = ZuTraits<Elem>;
  enum {
    IsPrimitive = 1, // the array is primitive, the element might not be
    IsPOD = ElemTraits::IsPOD,
    IsArray = 1,
    IsHashable = ElemTraits::IsHashable,
    IsComparable = ElemTraits::IsComparable
  };
  inline static const Elem *data(const T &a) { return &a[0]; }
  inline static unsigned length(const T &a) { return sizeof(a) / sizeof(a[0]); }
};

template <typename T>
struct ZuTraits<T []> : public ZuTraits_Array<T [], T> { };
template <typename T>
struct ZuTraits<const T []> : public ZuTraits_Array<const T [], const T> { };
template <typename T>
struct ZuTraits<volatile T []> :
  public ZuTraits_Array<volatile T [], volatile T> { };
template <typename T>
struct ZuTraits<const volatile T []> :
  public ZuTraits_Array<const volatile T [], const volatile T> { };

template <typename T, int N>
struct ZuTraits<T [N]> : public ZuTraits_Array<T [N], T> { };
template <typename T, int N>
struct ZuTraits<const T [N]> : public ZuTraits_Array<const T [N], const T> { };
template <typename T, int N>
struct ZuTraits<volatile T [N]> :
  public ZuTraits_Array<volatile T [N], volatile T> { };
template <typename T, int N>
struct ZuTraits<const volatile T [N]> :
  public ZuTraits_Array<const volatile T [N], const volatile T> { };

template <typename T, int N>
struct ZuTraits<T (&)[N]> : public ZuTraits_Array<T [N], T> { };
template <typename T, int N>
struct ZuTraits<const T (&)[N]> :
  public ZuTraits_Array<const T [N], const T> { };
template <typename T, int N>
struct ZuTraits<volatile T (&)[N]> :
  public ZuTraits_Array<volatile T [N], volatile T> { };
template <typename T, int N>
struct ZuTraits<const volatile T (&)[N]> :
  public ZuTraits_Array<const volatile T [N], const volatile T> { };

// strings

template <class Base> struct ZuTraits_CString : public Base {
  enum { IsCString = 1, IsString = 1 };
  inline static const char *data(const char *s) { return s; }
  inline static unsigned length(const char *s) { return s ? strlen(s) : 0; }
};

template <> struct ZuTraits<char *> :
  public ZuTraits_CString<ZuTraits_Pointer<char *, char> > { };
template <> struct ZuTraits<const char *> :
  public ZuTraits_CString<ZuTraits_Pointer<const char *, const char> > { };
template <> struct ZuTraits<volatile char *> :
  public ZuTraits_CString<ZuTraits_Pointer<volatile char *, volatile char> > { };
template <> struct ZuTraits<const volatile char *> :
  public ZuTraits_CString<ZuTraits_Pointer<
      const volatile char *, const volatile char> > { };

#ifndef _MSC_VER
template <> struct ZuTraits<char []> :
  public ZuTraits_CString<ZuTraits_Array<char [], char> > { };
template <>
struct ZuTraits<const char []> :
  public ZuTraits_CString<ZuTraits_Array<const char [], const char> > { };
template <>
struct ZuTraits<volatile char []> :
  public ZuTraits_CString<ZuTraits_Array<volatile char [], volatile char> > { };
template <>
struct ZuTraits<const volatile char []> :
  public ZuTraits_CString<ZuTraits_Array<
      const volatile char [], const volatile char> > { };
#endif

template <int N> struct ZuTraits<char [N]> :
  public ZuTraits_CString<ZuTraits_Array<char [N], char> > { };
template <int N>
struct ZuTraits<const char [N]> :
  public ZuTraits_CString<ZuTraits_Array<const char [N], const char> > { };
template <int N>
struct ZuTraits<volatile char [N]> :
  public ZuTraits_CString<ZuTraits_Array<
      volatile char [N], volatile char> > { };
template <int N>
struct ZuTraits<const volatile char [N]> :
  public ZuTraits_CString<ZuTraits_Array<
      const volatile char [N], const volatile char> > { };

template <int N> struct ZuTraits<char (&)[N]> :
  public ZuTraits_CString<ZuTraits_Array<char [N], char> > { };
template <int N>
struct ZuTraits<const char (&)[N]> :
  public ZuTraits_CString<ZuTraits_Array<const char [N], const char> > { };
template <int N>
struct ZuTraits<volatile char (&)[N]> :
  public ZuTraits_CString<ZuTraits_Array<
      volatile char [N], volatile char> > { };
template <int N>
struct ZuTraits<const volatile char (&)[N]> :
  public ZuTraits_CString<ZuTraits_Array<
      const volatile char [N], const volatile char> > { };

template <class Base> struct ZuTraits_WString : public Base {
  enum { IsCString = 1, IsString = 1, IsWString = 1 };
  inline static const wchar_t *data(const wchar_t *s) { return s; }
  inline static unsigned length(const wchar_t *s) { return s ? wcslen(s) : 0; }
};

template <> struct ZuTraits<wchar_t *> :
  public ZuTraits_WString<ZuTraits_Pointer<wchar_t *, wchar_t> > { };
template <> struct ZuTraits<const wchar_t *> :
  public ZuTraits_WString<ZuTraits_Pointer<
      const wchar_t *, const wchar_t> > { };
template <> struct ZuTraits<volatile wchar_t *> :
  public ZuTraits_WString<ZuTraits_Pointer<
      volatile wchar_t *, volatile wchar_t> > { };
template <> struct ZuTraits<const volatile wchar_t *> :
  public ZuTraits_WString<ZuTraits_Pointer<
      const volatile wchar_t *, const volatile wchar_t> > { };

#ifndef _MSC_VER
template <> struct ZuTraits<wchar_t []> :
  public ZuTraits_WString<ZuTraits_Array<wchar_t [], wchar_t> > { };
template <>
struct ZuTraits<const wchar_t []> :
  public ZuTraits_WString<ZuTraits_Array<
      const wchar_t [], const wchar_t> > { };
template <>
struct ZuTraits<volatile wchar_t []> :
  public ZuTraits_WString<ZuTraits_Array<
      volatile wchar_t [], volatile wchar_t> > { };
template <>
struct ZuTraits<const volatile wchar_t []> :
  public ZuTraits_WString<ZuTraits_Array<
      const volatile wchar_t [], const volatile wchar_t> > { };
#endif

template <int N> struct ZuTraits<wchar_t [N]> :
  public ZuTraits_WString<ZuTraits_Array<wchar_t [N], wchar_t> > { };
template <int N>
struct ZuTraits<const wchar_t [N]> :
  public ZuTraits_WString<ZuTraits_Array<
      const wchar_t [N], const wchar_t> > { };
template <int N>
struct ZuTraits<volatile wchar_t [N]> :
  public ZuTraits_WString<ZuTraits_Array<
      volatile wchar_t [N], volatile wchar_t> > { };
template <int N>
struct ZuTraits<const volatile wchar_t [N]> :
  public ZuTraits_WString<ZuTraits_Array<
      const volatile wchar_t [N], const volatile wchar_t> > { };

template <int N> struct ZuTraits<wchar_t (&)[N]> :
  public ZuTraits_WString<ZuTraits_Array<wchar_t [N], wchar_t> > { };
template <int N>
struct ZuTraits<const wchar_t (&)[N]> :
  public ZuTraits_WString<ZuTraits_Array<
      const wchar_t [N], const wchar_t> > { };
template <int N>
struct ZuTraits<volatile wchar_t (&)[N]> :
  public ZuTraits_WString<ZuTraits_Array<
      volatile wchar_t [N], volatile wchar_t> > { };
template <int N>
struct ZuTraits<const volatile wchar_t (&)[N]> :
  public ZuTraits_WString<ZuTraits_Array<
      const volatile wchar_t [N], const volatile wchar_t> > { };

// void

template <> struct ZuTraits<void> : public ZuGenericTraits<void> {
  enum { IsPrimitive = 1, IsPOD = 1, IsVoid = 1 };
};

#include <zlib/ZuIfT.hpp>

// SFINAE techniques...
#define ZuTraits_SFINAE(R) \
template <typename S, typename T = void> \
struct ZuIs##R : public ZuIfT<ZuTraits<S>::Is##R, T> { }; \
template <typename S, typename T = void> \
struct ZuNot##R : public ZuIfT<!ZuTraits<S>::Is##R, T> { };

ZuTraits_SFINAE(Primitive)
ZuTraits_SFINAE(Real)
ZuTraits_SFINAE(POD)
ZuTraits_SFINAE(Integral)
ZuTraits_SFINAE(Signed)
ZuTraits_SFINAE(CString)
ZuTraits_SFINAE(Pointer)
ZuTraits_SFINAE(Reference)
ZuTraits_SFINAE(WString)
ZuTraits_SFINAE(Array)
ZuTraits_SFINAE(PrimitiveArray)
ZuTraits_SFINAE(Void)
ZuTraits_SFINAE(Hashable)
ZuTraits_SFINAE(Comparable)
ZuTraits_SFINAE(Bool)
ZuTraits_SFINAE(String)
ZuTraits_SFINAE(Boxed)
ZuTraits_SFINAE(FloatingPoint)
ZuTraits_SFINAE(FixedPoint)

#undef ZuTraits_SFINAE

template <typename S, typename T = void> struct ZuIsCharString :
  public ZuIfT<ZuTraits<S>::IsString && !ZuTraits<S>::IsWString, T> { };

// STL / Boost interoperability
template <typename T_, typename Char>
struct ZuStdStringTraits_ : public ZuGenericTraits<T_> {
  enum { IsString = 1 };
  typedef T_ T;
  typedef Char Elem;
#if 0
  inline static T make(const Char *data, unsigned length) {
    if (ZuUnlikely(!data)) return T();
    return T(data, (size_t)length);
  }
#endif
  inline static const Char *data(const T &s) { return s.data(); }
  inline static unsigned length(const T &s) { return s.length(); }
};
template <typename T_>
struct ZuStdStringTraits : public ZuStdStringTraits_<T_, char> { };
template <typename T_>
struct ZuStdWStringTraits : public ZuStdStringTraits_<T_, wchar_t>
  { enum { IsWString = 1 }; };

template <typename T_, typename Elem_>
struct ZuStdArrayTraits_ : public ZuGenericTraits<T_> {
  enum { IsArray = 1 };
  typedef T_ T;
  typedef Elem_ Elem;
  inline static const Elem *data(const T &a) { return a.data(); }
  inline static unsigned length(const T &a) { return a.size(); }
};
template <typename T, typename Elem>
struct ZuStdVectorTraits : public ZuStdArrayTraits_<T, Elem> {
#if 0
  inline static T make(const Elem *data, unsigned length) {
    T vector((size_t)length);
    for (unsigned i = 0; i < length; i++) vector[i] = data[i];
    return vector;
  }
#endif
};
template <typename T, typename Elem, size_t N>
struct ZuStdArrayTraits : public ZuStdArrayTraits_<T, Elem> {
#if 0
  inline static T make(const Elem *data, unsigned length) {
    T array;
    if ((size_t)length > N) length = N;
    for (unsigned i = 0; i < length; i++) array[i] = data[i];
    return array;
  }
#endif
};

#include <zlib/ZuStdString.hpp>

namespace std {
  template <typename, class> class vector;
  template <typename, size_t> class array;
}
template <typename Traits, typename Alloc>
struct ZuTraits<std::basic_string<char, Traits, Alloc> > :
    public ZuStdStringTraits<std::basic_string<char, Traits, Alloc> > { };
template <typename Traits, typename Alloc>
struct ZuTraits<std::basic_string<wchar_t, Traits, Alloc> > :
    public ZuStdWStringTraits<std::basic_string<wchar_t, Traits, Alloc> > { };
template <typename T, typename Alloc>
struct ZuTraits<std::vector<T, Alloc> > :
    public ZuStdVectorTraits<std::vector<T, Alloc>, T> { };
template <typename T, size_t N>
struct ZuTraits<std::array<T, N> > :
    public ZuStdArrayTraits<std::array<T, N>, T, N> { };

namespace boost {
  template <typename, typename> class basic_string_ref;
  template <typename, size_t> class array;
}
template <typename Traits>
struct ZuTraits<boost::basic_string_ref<char, Traits> > :
    public ZuStdStringTraits<boost::basic_string_ref<char, Traits> > { };
template <typename Traits>
struct ZuTraits<boost::basic_string_ref<wchar_t, Traits> > :
    public ZuStdWStringTraits<boost::basic_string_ref<wchar_t, Traits> > { };
template <typename T, size_t N>
struct ZuTraits<boost::array<T, N> > :
    public ZuStdArrayTraits<boost::array<T, N>, T, N> { };

namespace std {
  template <typename> class initializer_list;
}

template <typename Elem_>
struct ZuTraits<std::initializer_list<Elem_> > :
    public ZuGenericTraits<std::initializer_list<Elem_> > {
  enum { IsArray = 1 };
  typedef std::initializer_list<Elem_> T;
  typedef Elem_ Elem;
  inline static const Elem *data(const T &a) { return a.begin(); }
  inline static unsigned length(const T &a) { return a.size(); }
#if 0
  inline static T make(const Elem *data, unsigned length) {
    return T(data, length);
  }
#endif
};

template <typename L> struct ZuLambdaTraits :
  public ZuLambdaTraits<decltype(&L::operator())> { };
template <typename R, typename T, typename ...Args>
struct ZuLambdaTraits<R (T::*)(Args...) const> { enum { IsMutable = 0 }; };
template <typename R, typename T, typename ...Args>
struct ZuLambdaTraits<R (T::*)(Args...)> { enum { IsMutable = 1 }; };

template <typename L, typename T = void>
struct ZuIsMutable : public ZuIfT<ZuLambdaTraits<L>::IsMutable, T> { };
template <typename L, typename T = void>
struct ZuNotMutable : public ZuIfT<!ZuLambdaTraits<L>::IsMutable, T> { };

#endif /* ZuTraits_HPP */
