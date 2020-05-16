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

// concrete generic sequence number (uint64)

#ifndef ZvSeqNo_HPP
#define ZvSeqNo_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuBox.hpp>

struct ZvSeqNoCmp {
  ZuInline static int cmp(uint64_t v1, uint64_t v2) {
    int64_t d = v1 - v2; // handles wraparound ok
    return d < 0 ? -1 : d > 0 ? 1 : 0;
  }
  ZuInline static bool equals(uint64_t v1, uint64_t v2) { return v1 == v2; }
  ZuInline static bool null(uint64_t v) { return !v; }
  ZuInline static uint64_t null() { return 0; }
};
using ZvSeqNo = ZuBox<uint64_t, ZvSeqNoCmp>;

#endif /* ZvSeqNo_HPP */
