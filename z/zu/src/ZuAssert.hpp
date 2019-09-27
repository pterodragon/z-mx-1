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

// compile-time assertion

#ifndef ZuAssert_HPP
#define ZuAssert_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

template <bool> struct ZuAssertion_FAILED;
template <> struct ZuAssertion_FAILED<true> { };

template <int N> struct ZuAssert_TEST { };

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
#define ZuAssert_Unused_Typedef __attribute__((unused))
#else
#define ZuAssert_Unused_Typedef
#endif

// need to indirect via two macros to ensure expansion of __LINE__
#define ZuAssert_Typedef_(p, l) p##l
#define ZuAssert_Typedef(p, l) ZuAssert_Typedef_(p, l)

#define ZuAssert(x) \
	typedef ZuAssert_TEST<sizeof(ZuAssertion_FAILED<(bool)(x)>)> \
	ZuAssert_Typedef(ZuAssert_, __LINE__) ZuAssert_Unused_Typedef

// compile time C assert
#define ZuCAssert(x) switch (0) { case 0: case (x): ; }

#endif /* ZuAssert_HPP */
