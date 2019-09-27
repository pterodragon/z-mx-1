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

// Zu network byte ordering

// deprecated - use ZuByteSwap for future code

#ifndef ZuNetwork_HPP
#define ZuNetwork_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuAssert.hpp>
#include <zlib/ZuByteSwap.hpp>

#if Zu_BIGENDIAN
template <typename T, int N> struct ZuTwiddleFn {
  ZuAssert(sizeof(T) == N);
  inline static T fn(const T &i) { return i; }
};
#define ZuHtons(x) x
#define ZuHtonl(x) x
#define ZuHtonll(x) x
#else
template <typename T, typename I> struct ZuTwiddleFn_ {
  inline static T fn(const T &i) {
    const I *ZuMayAlias(i_) = (const I *)(const void *)&i;
    I r = ZuByteSwap<I>::bswap(*i_);
    T *ZuMayAlias(r_) = (T *)(void *)&r;
    return *r_;
  }
};
template <typename T> struct ZuTwiddleFn_<T, T> {
  inline static T fn(const T &i) {
    return ZuByteSwap<T>::bswap(i);
  }
};
template <typename T, int N>
struct ZuTwiddleFn : public ZuTwiddleFn_<T, typename ZuByteSwap<T>::I> {
  ZuAssert(sizeof(T) == N);
};
#define ZuHtons(x) Zu_bswap16(x)
#define ZuHtonl(x) Zu_bswap32(x)
#define ZuHtonll(x) Zu_bswap64(x)
#endif
template <typename T, int N>
struct ZuUntwiddleFn : public ZuTwiddleFn<T, N> { };

#endif /* ZuNetwork_HPP */
