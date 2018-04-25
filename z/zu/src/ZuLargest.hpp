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

// compile-time determination of maximum sizeof()
//
// ZuLargest<T1, T2>::T evaluates to whichever of T1 or T2 is largest

#ifndef ZuLargest_HPP
#define ZuLargest_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuIf.hpp>
#include <ZuNull.hpp>

template <typename T1, typename T2 = ZuNull, typename T3 = ZuNull,
	  typename T4 = ZuNull, typename T5 = ZuNull, typename T6 = ZuNull,
	  typename T7 = ZuNull, typename T8 = ZuNull, typename T9 = ZuNull,
	  typename T10 = ZuNull, typename T11 = ZuNull, typename T12 = ZuNull,
	  typename T13 = ZuNull, typename T14 = ZuNull, typename T15 = ZuNull,
	  typename T16 = ZuNull, typename T17 = ZuNull, typename T18 = ZuNull,
	  typename T19 = ZuNull, typename T20 = ZuNull, typename T21 = ZuNull,
	  typename T22 = ZuNull, typename T23 = ZuNull, typename T24 = ZuNull,
	  typename T25 = ZuNull, typename T26 = ZuNull, typename T27 = ZuNull,
	  typename T28 = ZuNull, typename T29 = ZuNull, typename T30 = ZuNull>
struct ZuLargest {
  typedef typename ZuLargest<T1, T2>::T L;
  typedef typename ZuLargest<L, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
			     T14, T15, T16, T17, T18, T19, T20, T21, T22, T23,
			     T24, T25, T26, T27, T28, T29, T30, ZuNull>::T T;
};

template <typename T1, typename T2>
struct ZuLargest<T1, T2, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull,
		 ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull,
		 ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull,
		 ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull, ZuNull,
		 ZuNull> {
  typedef typename ZuIf<T1, T2, (sizeof(T1) > sizeof(T2))>::T T;
};

#endif /* ZuLargest_HPP */
