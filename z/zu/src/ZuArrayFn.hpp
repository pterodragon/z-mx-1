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

// fast generic array operations

// ZuArrayFn<T> provides a library of fast array operations used
// by many other templates and classes

#ifndef ZuArrayFn_HPP
#define ZuArrayFn_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <stdlib.h>
#include <string.h>

#include <ZuNew.hpp>
#include <ZuAssert.hpp>
#include <ZuTraits.hpp>
#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuConversion.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// #pragma warning(disable:4800)
#endif

// array initialization / destruction / move / copy operations

template <typename T, class Cmp, bool IsPrimitive> class ZuArrayFn_ItemOps_;

template <typename T, class Cmp> class ZuArrayFn_ItemOps_<T, Cmp, false> {
public:
  inline static void initItem(void *dst) { new (dst) T(Cmp::null()); }
  template <typename P>
  inline static void initItem(void *dst, P &&p) {
    new (dst) T(ZuFwd<P>(p));
  }
  inline static void destroyItem(T *dst) { (*dst).~T(); }
};

template <typename T, class Cmp> class ZuArrayFn_ItemOps_<T, Cmp, true> {
public:
  inline static void initItem(void *dst) { *(T *)dst = Cmp::null(); }
  template <typename P>
  inline static void initItem(void *dst, P &&p) {
    *(T *)dst = ZuFwd<P>(p);
  }
  inline static void destroyItem(T *dst) { }
};

template <typename T, class Cmp> class ZuArrayFn_ItemOps :
    public ZuArrayFn_ItemOps_<T, Cmp, ZuTraits<T>::IsPrimitive> {
  typedef ZuArrayFn_ItemOps_<T, Cmp, ZuTraits<T>::IsPrimitive> Base;
public:
  inline static void initItems(T *dst, unsigned length) {
    if (ZuLikely(length))
      do { Base::initItem((void *)dst++); } while (--length > 0);
  }
};

template <typename T, class Cmp, bool IsPOD>
class ZuArrayFn_Ops : public ZuArrayFn_ItemOps<T, Cmp> {
public:
  inline static void destroyItems(T *dst, unsigned length) {
    if (ZuLikely(length))
      do { (*dst++).~T(); } while (--length > 0);
  }

  template <typename S>
  inline static void copyItems(T *dst, const S *src, unsigned length) {
    if (ZuUnlikely(!length || (void *)dst == (void *)src)) return;
    do { new ((void *)dst++) T(*src++); } while (--length > 0);
  }

  inline static void moveItems(T *dst, T *src, unsigned length) {
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
  inline static void destroyItems(T *dst, unsigned length) { }
  template <typename S>
  inline static typename ZuNotSame<T, S>::T copyItem(T *dst, const S &src) {
    new ((void *)dst) T(src);
  }
  template <typename S>
  inline static typename ZuNotSame<T, S>::T copyItems(
      T *dst, const S *src, unsigned length) {
    if (ZuUnlikely(!length || (void *)dst == (void *)src)) return;
    do { new ((void *)dst++) T(*src++); } while (--length > 0);
  }
  template <typename S>
  inline static typename ZuSame<T, S>::T copyItem(T *dst, const S &src) {
    memcpy((void *)dst, &src, sizeof(T));
  }
  template <typename S>
  inline static typename ZuSame<T, S>::T copyItems(
      T *dst, const S *src, unsigned length) {
    memcpy((void *)dst, src, length * sizeof(T));
  }
  inline static void moveItem(T *dst, T *src) {
    memmove((void *)dst, src, sizeof(T));
  }
  inline static void moveItems(T *dst, const T *src, unsigned length) {
    memmove((void *)dst, src, length * sizeof(T));
  }
};

// array comparison

template <typename T, class Cmp, class DefltCmp, bool IsIntegral, unsigned Size>
struct ZuArrayFn_Cmp {
  inline static int cmp(const T *dst, const T *src, unsigned length) {
    if (ZuUnlikely(!length)) return 0;
    do {
      if (int i = Cmp::cmp(*dst++, *src++)) return i;
    } while (--length > 0);
    return 0;
  }

  inline static bool equals(const T *dst, const T *src, unsigned length) {
    if (ZuUnlikely(!length)) return true;
    do {
      if (!Cmp::equals(*dst++, *src++)) return false;
    } while (--length > 0);
    return true;
  }
};

template <typename T, class Cmp> struct ZuArrayFn_Cmp<T, Cmp, Cmp, true, 1> {
  inline static int cmp(const T *dst, const T *src, unsigned length) {
    return memcmp(dst, src, length * sizeof(T));
  }
  inline static bool equals(const T *dst, const T *src, unsigned length) {
    return !memcmp(dst, src, length * sizeof(T));
  }
};

template <typename T> struct ZuArrayFn_Hash {
  inline static uint32_t hash(const T *data, unsigned length) {
    ZuHash_FNV::Value v = ZuHash_FNV::initial_();
    if (ZuLikely(length))
      do {
	v = ZuHash_FNV::hash_(v, ZuHash<T>::hash(*data++));
      } while (--length > 0);
    return (uint32_t)v;
  }
};
template <typename T> struct ZuArrayFn_StringHash {
  inline static uint32_t hash(const T *data, unsigned length) {
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
  inline static void initItem(T *dst) { }
  template <typename P> inline static void initItem(T *dst, P &&p) { }
  inline static void destroyItem(T *dst) { }
  inline static void initItems(T *dst, unsigned length) { }
  inline static void destroyItems(T *dst, unsigned length) { }
  template <typename S>
  inline static void copyItem(T *dst, const S &src) { }
  template <typename S>
  inline static void copyItems(T *dst, const S *src, unsigned length) { }
  inline static void moveItem(T *dst, T *src) { }
  inline static void moveItems(T *dst, const T *src, unsigned length) { }
  inline static int cmp(const T *dst, const T *src, unsigned length)
    { return 0; }
  inline static bool equals(const T *dst, const T *src, unsigned length)
    { return true; }
  inline static uint32_t hash(const T *data, unsigned length)
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
