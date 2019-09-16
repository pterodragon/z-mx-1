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
#include <ZuIfT.hpp>

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

  void zero() { memset(data, 0, Bytes); }
  void fill() { memset(data, 0xff, Bytes); }

  ZuInline void set(unsigned i) {
    data[i>>Shift] |= ((uint64_t)1)<<(i & Mask);
  }
  ZuInline void clr(unsigned i) {
    data[i>>Shift] &= ~(((uint64_t)1)<<(i & Mask));
  }
  ZuInline bool val(unsigned i) const {
    return data[i>>Shift] & ((uint64_t)1)<<(i & Mask);
  }

  template <unsigned I> struct Index { enum { OK = I < Words }; };
  template <typename Fn, unsigned I>
  ZuInline static typename ZuIfT<Index<I>::OK>::T
  opFn(uint64_t *v1) {
    Fn::fn(v1[I]);
    opFn<Fn, I + 1>(v1);
  }
  template <typename Fn, unsigned I>
  ZuInline static typename ZuIfT<Index<I>::OK>::T
  opFn(uint64_t *v1, const uint64_t *v2) {
    Fn::fn(v1[I], v2[I]);
    opFn<Fn, I + 1>(v1, v2);
  }
  template <typename, unsigned I>
  ZuInline static typename ZuIfT<!Index<I>::OK>::T
  opFn(uint64_t *) { }
  template <typename, unsigned I>
  ZuInline static typename ZuIfT<!Index<I>::OK>::T
  opFn(uint64_t *, const uint64_t *) { }

  struct Not {
    ZuInline static void fn(uint64_t &v1) { v1 = ~v1; }
  };
  struct Or {
    ZuInline static void fn(uint64_t &v1, const uint64_t &v2) { v1 |= v2; }
  };
  struct And {
    ZuInline static void fn(uint64_t &v1, const uint64_t &v2) { v1 &= v2; }
  };
  struct Xor {
    ZuInline static void fn(uint64_t &v1, const uint64_t &v2) { v1 ^= v2; }
  };

  inline void flip() { opFn<Not, 0>(data); }

  inline ZuBitmap &operator |=(const ZuBitmap &b) {
    opFn<Or, 0>(data, b.data);
    return *this;
  }
  inline ZuBitmap &operator &=(const ZuBitmap &b) {
    opFn<And, 0>(data, b.data);
    return *this;
  }
  inline ZuBitmap &operator ^=(const ZuBitmap &b) {
    opFn<Xor, 0>(data, b.data);
    return *this;
  }

  uint64_t	data[Words];
};

#endif /* ZuBitmap_HPP */
