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

// simple bitmap type

#ifndef ZuBitmap_HPP
#define ZuBitmap_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuInt.hpp>

template <unsigned Bits_> class ZuBitmap {
public:
  enum { Bits = Bits_ };
  enum { Bytes = (Bits>>3) };
  enum { Shift = 6 };
  enum { Mask = ((1<<Shift) - 1) };
  enum { Words = (Bits>>Shift) };

  ZuBitmap() { zero(); }
  ZuBitmap(const ZuBitmap &b) { memcpy(data, b.data, Bytes); }
  ZuBitmap &operator =(const ZuBitmap &b) {
    if (ZuLikely(this != &b)) memcpy(data, b.data, Bytes);
    return *this;
  }
  ZuBitmap(ZuBitmap &&b) = default;
  ZuBitmap &operator =(ZuBitmap &&b) = default;

  void zero() { memset(0, data, Bytes); }
  void fill() { memset(0xff, data, Bytes); }

  ZuInline void set(unsigned i) {
    data[i>>Shift] |= ((uint64_t)1)<<(i & Mask);
  }
  ZuInline void clr(unsigned i) {
    data[i>>Shift] &= ~(((uint64_t)1)<<(i & Mask));
  }
  ZuInline bool val(unsigned i) const {
    return data[i>>Shift] & ((uint64_t)1)<<(i & Mask);
  }

  uint64_t	data[Words];
};

#endif /* ZuBitmap_HPP */
