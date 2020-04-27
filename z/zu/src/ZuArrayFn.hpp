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

// fast generic array operations

// ZuArrayFn<T> provides a library of fast array operations used
// by many other templates and classes

#ifndef ZuArrayFn_HPP
#define ZuArrayFn_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <stdlib.h>
#include <string.h>

#include <zlib/ZuNew.hpp>
#include <zlib/ZuAssert.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuConversion.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// #pragma warning(disable:4800)
#endif

// array initialization / destruction / move / copy operations

template <typename T, class Cmp, bool IsPrimitive> class ZuArrayFn_ItemOps_;

template <typename T, class Cmp> class ZuArrayFn_ItemOps_<T, Cmp, false> {
public:
  ZuInline static void initItem(void *dst) { new (dst) T(Cmp::null()); }
  template <typename P>
  ZuInline static void initItem(void *dst, P &&p) {
    new (dst) T(ZuFwd<P>(p));
  }
  ZuInline static void destroyItem(T *dst) { (*dst).~T(); }
};

template <typename T, class Cmp> class ZuArrayFn_ItemOps_<T, Cmp, true> {
public:
  ZuInline static void initItem(void *dst) { *(T *)dst = Cmp::null(); }
  template <typename P>
  ZuInline static void initItem(void *dst, P &&p) {
    *(T *)dst = ZuFwd<P>(p);
  }
  ZuInline static void destroyItem(T *dst) { }
};

template <typename T, class Cmp> class ZuArrayFn_ItemOps :
    public ZuArrayFn_ItemOps_<T, Cmp, ZuTraits<T>::IsPrimitive> {
  typedef ZuArrayFn_ItemOps_<T, Cmp, ZuTraits<T>::IsPrimitive> Base;
public:
  static void initItems(T *dst, unsigned length) {
    if (ZuLikely(length))
      do { Base::initItem((void *)dst++); } while (--length > 0);
  }
};

template <typename T, class Cmp, bool IsPOD>
class ZuArrayFn_Ops : public ZuArrayFn_ItemOps<T, Cmp> {
public:
  static void destroyItems(T *dst, unsigned length) {
    if (ZuLikely(length))
      do { (*dst++).~T(); } while (--length > 0);
  }

  template <typename S>
  static void copyItems(T *dst, const S *src, unsigned length) {
    if (ZuUnlikely(!length || (void *)dst == (void *)src)) return;
    do { new ((void *)dst++) T(*src++); } while (--length > 0);
  }

  static void moveItems(T *dst, T *src, unsigned length) {
    if (ZuUnlikely(!length || dst == src)) return;
    if (src > dst || length < (unsigned)(dst - src)) {
      do {
	new ((void *)dst++) T(ZuMv(*src)); (*src++).~T();
      } while (--length > 0);
    } else {
      dst += length;
      src += length;
      do {
	new ((void *)--dst) T(ZuMv(*--src)); (*src).~T();
      } while (--length > 0);
    }
  }
};

template <typename T, class Cmp>
class ZuArrayFn_Ops<T, Cmp, true> : public ZuArrayFn_ItemOps<T, Cmp> {
public:
  ZuInline static void destroyItems(T *dst, unsigned length) { }
  template <typename S>
  static typename ZuNotSame<T, S>::T copyItems(
      T *dst, const S *src, unsigned length) {
    if (ZuUnlikely(!length || (void *)dst == (void *)src)) return;
    do { new ((void *)dst++) T(*src++); } while (--length > 0);
  }
  template <typename S>
  ZuInline static typename ZuSame<T, S>::T copyItems(
      T *dst, const S *src, unsigned length) {
    memcpy((void *)dst, src, length * sizeof(T));
  }
  ZuInline static void moveItems(T *dst, const T *src, unsigned length) {
    memmove((void *)dst, src, length * sizeof(T));
  }
};

// array comparison

template <typename T, class Cmp, class DefltCmp, bool IsIntegral, unsigned Size>
struct ZuArrayFn_Cmp {
  static int cmp(const T *dst, const T *src, unsigned length) {
    while (ZuLikely(length--))
      if (int i = Cmp::cmp(*dst++, *src++)) return i;
    return 0;
  }

  static bool equals(const T *dst, const T *src, unsigned length) {
    while (ZuLikely(length--))
      if (!Cmp::equals(*dst++, *src++)) return false;
    return true;
  }
};

template <typename T, class Cmp> struct ZuArrayFn_Cmp<T, Cmp, Cmp, true, 1> {
  ZuInline static int cmp(const T *dst, const T *src, unsigned length) {
    return memcmp(dst, src, length * sizeof(T));
  }
  ZuInline static bool equals(const T *dst, const T *src, unsigned length) {
    return !memcmp(dst, src, length * sizeof(T));
  }
};

template <typename T> struct ZuArrayFn_Hash {
  static uint32_t hash(const T *data, unsigned length) {
    ZuHash_FNV::Value v = ZuHash_FNV::initial_();
    if (ZuLikely(length))
      do {
	v = ZuHash_FNV::hash_(v, ZuHash<T>::hash(*data++));
      } while (--length > 0);
    return (uint32_t)v;
  }
};
template <typename T> struct ZuArrayFn_StringHash {
  ZuInline static uint32_t hash(const T *data, unsigned length) {
    return ZuStringHash<T>::hash(data, length);
  }
};
template <>
struct ZuArrayFn_Hash<char> : public ZuArrayFn_StringHash<char> { };
template <>
struct ZuArrayFn_Hash<wchar_t> : public ZuArrayFn_StringHash<wchar_t> { };

// main template

template <typename T, class Cmp = ZuCmp<T> > class ZuArrayFn :
  public ZuArrayFn_Ops<T, Cmp, ZuTraits<T>::IsPOD>,
  public ZuArrayFn_Cmp<T, Cmp, ZuCmp<T>, ZuTraits<T>::IsIntegral, sizeof(T)>,
  public ZuArrayFn_Hash<T> { };

template <typename T> class ZuArrayFn_Null {
public:
  ZuInline static void initItem(T *dst) { }
  template <typename P> ZuInline static void initItem(T *dst, P &&p) { }
  ZuInline static void destroyItem(T *dst) { }
  ZuInline static void initItems(T *dst, unsigned length) { }
  ZuInline static void destroyItems(T *dst, unsigned length) { }
  template <typename S>
  ZuInline static void copyItems(T *dst, const S *src, unsigned length) { }
  ZuInline static void moveItems(T *dst, const T *src, unsigned length) { }
  ZuInline static int cmp(const T *dst, const T *src, unsigned length)
    { return 0; }
  ZuInline static bool equals(const T *dst, const T *src, unsigned length)
    { return true; }
  ZuInline static uint32_t hash(const T *data, unsigned length)
    { return 0; }
};

template <typename Cmp>
class ZuArrayFn<ZuNull, Cmp> : public ZuArrayFn_Null<ZuNull> { };
template <typename Cmp>
class ZuArrayFn<void, Cmp> : public ZuArrayFn_Null<void> { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuArrayFn_HPP */
