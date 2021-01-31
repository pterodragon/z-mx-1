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

// generic printing, with support for output to non-Z strings / streams

#ifndef ZuPrint_HPP
#define ZuPrint_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>

struct ZuPrintable { };

struct ZuPrintCannot {
  enum { OK = 0, String = 0, Delegate = 0, Buffer = 0 };
};

struct ZuPrintFn {
  enum { OK = 1, String = 0, Delegate = 1, Buffer = 0 };
  template <typename S, typename T>
  ZuInline static void print(S &s, const T &v) { v.print(s); }
};

ZuPrintCannot ZuPrintType(...);

template <typename U, bool = ZuConversion<ZuPrintable, U>::Base>
struct ZuDefaultPrint {
  using T = decltype(ZuPrintType(ZuDeclVal<U *>()));
};
template <typename U>
struct ZuDefaultPrint<U, true> { using T = ZuPrintFn; };

template <typename T>
struct ZuPrint : public ZuDefaultPrint<T>::T { };

struct ZuPrintString {
  enum { OK = 1, String = 1, Delegate = 0, Buffer = 0 };
};
struct ZuPrintDelegate {
  enum { OK = 1, String = 0, Delegate = 1, Buffer = 0 };
};
struct ZuPrintBuffer {
  enum { OK = 1, String = 0, Delegate = 0, Buffer = 1 };
};

struct ZuPrintNull {
  enum { OK = 1, String = 0, Delegate = 1, Buffer = 0 };
  template <typename S, typename T>
  ZuInline static void print(S &, const T &) { }
};

template <typename T> struct ZuPrint<const T> : public ZuPrint<T> { };
template <typename T> struct ZuPrint<volatile T> : public ZuPrint<T> { };
template <typename T>
struct ZuPrint<const volatile T> : public ZuPrint<T> { };
template <typename T> struct ZuPrint<T &> : public ZuPrint<T> { };
template <typename T> struct ZuPrint<const T &> : public ZuPrint<T> { };
template <typename T> struct ZuPrint<volatile T &> : public ZuPrint<T> { };
template <typename T>
struct ZuPrint<const volatile T &> : public ZuPrint<T> { };
template <typename T>
struct ZuPrint<T &&> : public ZuPrint<T> { };

template <typename S> struct ZuStdStream;

template <typename S> struct ZuStdStream__ {
  enum { Is = 1 };
  template <typename P> ZuInline static typename ZuIfT<ZuPrint<P>::String>::T
      print(S &s, const P &p) {
    const typename ZuTraits<P>::Elem *ptr = ZuTraits<P>::data(p);
    if (ZuLikely(ptr))
      ZuStdStream<S>::append(s, ptr, ZuTraits<P>::length(p));
  }
  template <typename P> ZuInline static typename ZuIfT<ZuPrint<P>::Delegate>::T
      print(S &s, const P &p) {
    ZuPrint<P>::print(s, p);
  }
  template <typename P> static typename ZuIfT<ZuPrint<P>::Buffer>::T
      print(S &s, const P &p) {
    unsigned len = ZuPrint<P>::length(p);
    char *buf = ZuAlloca(buf, len);
    if (ZuLikely(buf))
      ZuStdStream<S>::append(s, buf, ZuPrint<P>::print(buf, len, p));
  }
};

#include <iostream>

#include <zlib/ZuStdString.hpp>

template <typename, bool> struct ZuStdStream_ { enum { Is = 0 }; };
template <typename S> struct ZuStdStream_<S, true> : public ZuStdStream__<S> {
  static S &append(S &s, const char *data, unsigned length) {
    if (ZuLikely(data)) s.write(data, (size_t)length);
    return s;
  }
};
template <typename S> struct ZuStdStream :
  public ZuStdStream_<S, ZuConversion<std::ios_base, S>::Base> { };

template <typename T, typename A>
struct ZuStdStream<std::basic_string<char, T, A> > :
    public ZuStdStream__<std::basic_string<char, T, A> > {
  using S = std::basic_string<char, T, A>;
  static S &append(S &s, const char *data, unsigned length) {
    if (ZuLikely(data)) s.append(data, (size_t)length);
    return s;
  }
};

template <typename S, typename P, bool>
struct ZuStdStreamable_ { enum { OK = 0 }; };
template <typename S, typename P>
struct ZuStdStreamable_<S, P, true> { enum { OK = ZuPrint<P>::OK }; };
template <typename S, typename P>
struct ZuStdStreamable : public ZuStdStreamable_<S, P, ZuStdStream<S>::Is> { };

template <typename S, typename P>
inline typename ZuIfT<ZuStdStreamable<S, P>::OK, S &>::T
operator <<(S &s, const P &p) { ZuStdStream<S>::print(s, p); return s; }

template <typename S, typename P>
inline typename ZuIfT<ZuStdStreamable<S, P>::OK, S &>::T
operator +=(S &s, const P &p) { ZuStdStream<S>::print(s, p); return s; }

#endif /* ZuPrint_HPP */
