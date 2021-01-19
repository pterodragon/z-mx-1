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

// null type

#ifndef ZuNull_HPP
#define ZuNull_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuTraits.hpp>

class ZuNull { };

template <> struct ZuTraits<ZuNull> : public ZuBaseTraits<ZuNull> {
  enum { IsPOD = 1 };
};

template <typename T> struct ZuCmp;
template <> struct ZuCmp<ZuNull> {
  ZuInline static constexpr int cmp(ZuNull n1, ZuNull n2) { return 0; }
  ZuInline static constexpr bool less(ZuNull n1, ZuNull n2) { return false; }
  ZuInline static constexpr bool equals(ZuNull n1, ZuNull n2) { return true; }
  ZuInline static constexpr bool null(ZuNull n) { return true; }
  ZuInline static const ZuNull &null() { static const ZuNull _; return _; }
};

#endif /* ZuNull_HPP */
