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

// normalize char/signed char/unsigned char to char, leaving
// cv-qualifiers intact and all other types unchanged

#ifndef ZuNormChar_HPP
#define ZuNormChar_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

// element normalization
template <typename U> struct ZuNormChar { using T = U; };
template <>
struct ZuNormChar<signed char> { using T = char; };
template <>
struct ZuNormChar<unsigned char> { using T = char; };
template <>
struct ZuNormChar<const signed char> { using T = const char; };
template <>
struct ZuNormChar<const unsigned char> { using T = const char; };
template <>
struct ZuNormChar<volatile signed char> { using T = volatile char; };
template <>
struct ZuNormChar<volatile unsigned char> { using T = volatile char; };
template <>
struct ZuNormChar<const volatile signed char> {
  using T = const volatile char;
};
template <>
struct ZuNormChar<const volatile unsigned char> {
  using T = const volatile char;
};

#endif /* ZuNormChar_HPP */
