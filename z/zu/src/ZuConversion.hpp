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

// type convertibility & inheritance
//
// all types are de-referenced before comparison
//
// Same, Is and Base ignore cv qualifiers
//
// ZuConversion<T1, T2>::Exists	- T1 can be converted into T2
// ZuConversion<T1, T2>::Same	- T1 is same type as T2
// ZuConversion<T1, T2>::Is	- T1 is the same or a base class of T2
// ZuConversion<T1, T2>::Base	- T1 is a base class of T2

#ifndef ZuConversion_HPP
#define ZuConversion_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuIfT.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244 4800)
#endif

template <typename T1, typename T2> struct ZuConversion__ {
private:
  typedef char	Small;
  class		Big { char _[2]; };
  static Small	ZuConversion_test(const T2 &_); // named due to VS2010 bug
  static Big	ZuConversion_test(...);
  static T1	&ZuConversion_mkT1();		// reference due to VS2010 bug

public:
  ZuConversion__(); // keep gcc quiet
  enum {
    _ = sizeof(T1) == sizeof(T2), // ensure both types are complete
    Exists = sizeof(ZuConversion_test(ZuConversion_mkT1())) == sizeof(Small),
    Same = 0
  };
};

#define ZuConversionFriend ZuConversion__

template <typename T> struct ZuConversion__<T, T>
  { enum { Exists = 1, Same = 1, Base = 0, Is = 1 }; };

template <typename T_> struct ZuConversion_Array
  { typedef T_ T; };
template <typename T_> struct ZuConversion_Array<T_ []>
  { typedef T_ *const T; };
template <typename T_> struct ZuConversion_Array<const T_ []>
  { typedef const T_ *const T; };
template <typename T_> struct ZuConversion_Array<volatile T_ []>
  { typedef volatile T_ *const T; };
template <typename T_> struct ZuConversion_Array<const volatile T_ []>
  { typedef const volatile T_ *const T; };
template <typename T_, int N> struct ZuConversion_Array<T_ [N]>
  { typedef T_ *const T; };
template <typename T_, int N> struct ZuConversion_Array<const T_ [N]>
  { typedef const T_ *const T; };
template <typename T_, int N> struct ZuConversion_Array<volatile T_ [N]>
  { typedef volatile T_ *const T; };
template <typename T_, int N> struct ZuConversion_Array<const volatile T_ [N]>
  { typedef const volatile T_ *const T; };

template <typename T1, typename T2> struct ZuConversion_ {
  typedef typename ZuConversion_Array<T1>::T U1;
  typedef typename ZuConversion_Array<T2>::T U2;
  enum {
    Exists = ZuConversion__<U1, U2>::Exists,
    Same = ZuConversion__<const volatile U1, const volatile U2>::Same,
    Is = ZuConversion__<const volatile U2 *, const volatile U1 *>::Exists &&
      !ZuConversion__<const volatile U1 *, const volatile void *>::Same
  };
};

template <> struct ZuConversion_<void, void>
  { enum { Exists = 1, Same = 1, Base = 0, Is = 1 }; };
template <typename T> struct ZuConversion_<void, T>
  { enum { Exists = 0, Same = 0, Base = 0, Is = 0 }; };
template <typename T> struct ZuConversion_<T, void>
  { enum { Exists = 0, Same = 0, Base = 0, Is = 0 }; };

template <typename T_> struct ZuConversion_Void
  { typedef T_ T; };
template <> struct ZuConversion_Void<const void>
  { typedef void T; };
template <> struct ZuConversion_Void<volatile void>
  { typedef void T; };
template <> struct ZuConversion_Void<const volatile void>
  { typedef void T; };

template <typename T1, typename T2> class ZuConversion {
  typedef typename ZuConversion_Void<typename ZuDeref<T1>::T>::T U1;
  typedef typename ZuConversion_Void<typename ZuDeref<T2>::T>::T U2;
public:
  enum {
    Exists = ZuConversion_<U1, U2>::Exists,
    Same = ZuConversion_<U1, U2>::Same,
    Is = ZuConversion_<U1, U2>::Is,
    Base = ZuConversion_<U1, U2>::Is && !ZuConversion_<U1, U2>::Same
  };
};

// SFINAE techniques...
template <typename T1, typename T2, typename T = void>
struct ZuSame : public ZuIfT<ZuConversion<T1, T2>::Same, T> { };
template <typename T1, typename T2, typename T = void>
struct ZuNotSame : public ZuIfT<!ZuConversion<T1, T2>::Same, T> { };
template <typename T1, typename T2, typename T = void> struct ZuConvertible :
    public ZuIfT<ZuConversion<T1, T2>::Exists, T> { };
template <typename T1, typename T2, typename T = void> struct ZuNotConvertible :
    public ZuIfT<!ZuConversion<T1, T2>::Exists, T> { };
template <typename T1, typename T2, typename T = void>
struct ZuIsBase : public ZuIfT<ZuConversion<T1, T2>::Base, T> { };
template <typename T1, typename T2, typename T = void>
struct ZuNotBase : public ZuIfT<!ZuConversion<T1, T2>::Base, T> { };
template <typename T1, typename T2, typename T = void>
struct ZuIs : public ZuIfT<ZuConversion<T1, T2>::Is, T> { };
template <typename T1, typename T2, typename T = void>
struct ZuIsNot : public ZuIfT<!ZuConversion<T1, T2>::Is, T> { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuConversion_HPP */
