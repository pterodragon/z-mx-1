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

// iconv wrapper

#ifndef ZtIconv_HPP
#define ZtIconv_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <stdlib.h>	// for size_t

#include <iconv.h>

#include <zlib/ZuIf.hpp>

#include <zlib/ZmLock.hpp>

// specialize ZtIconvFn for any output buffer type
template <class Out> struct ZtIconvFn {
  enum { N = sizeof(typename ZuTraits<Out>::Elem) };
  // set length of buffer in bytes, retaining/copying previous data if resized
  // returning actual length obtained (which can be less than requested)
  ZuInline static size_t length(Out &buf, size_t n) {
    buf.length((n + N - 1) / N);
    return buf.length() * N;
  }
  // return modifiable pointer to buffer data
  ZuInline static char *data(Out &buf) { return (char *)buf.data(); }
};

class ZtIconv {
  ZtIconv();
  ZtIconv(const ZtIconv &);
  ZtIconv &operator =(const ZtIconv &);		// prevent mis-use

  // different iconv libraries have inconsistent types for iconv()'s inbuf
  // parameter - insulate ourselves accordingly
  class IconvTraits {
  private:
    typedef char	Small;
    struct		Big { char _[2]; };
    static Small	test(size_t (*)(
	  iconv_t, const char **, size_t *, char **, size_t *));
    static Big	test(...);
  public:
    enum { Const = sizeof(test(&iconv)) == sizeof(Small) };
  };
  typedef typename ZuIf<const char **, char **, IconvTraits::Const>::T InBuf;

public:
  inline ZtIconv(const char *tocode, const char *fromcode) :
    m_cd(iconv_open(tocode, fromcode)) { }
  ~ZtIconv() {
    if (m_cd != (iconv_t)-1) iconv_close(m_cd);
  }

  template <class Out, typename In>
  typename ZuIfT<ZuTraits<Out>::IsString && ZuTraits<In>::IsString, int>::T
  convert(Out &out, const In &in) {
    if (ZuUnlikely(m_cd == (iconv_t)-1)) return -1;
    const char *inBuf = (const char *)ZuTraits<In>::data(in);
    size_t inSize =
      ZuTraits<In>::length(in) * sizeof(typename ZuTraits<In>::Elem);
    if (ZuUnlikely(!inSize)) {
      ZtIconvFn<Out>::length(out, 0);
      return 0;
    }
    size_t outSize = inSize;
    outSize = ZtIconvFn<Out>::length(out, outSize);
    size_t inLen = inSize;
    size_t outLen = outSize;
    char *outBuf = ZtIconvFn<Out>::data(out);
  resized:
    size_t n = iconv(m_cd, (InBuf)&inBuf, &inLen, &outBuf, &outLen);
    if (n == (size_t)-1 && errno == E2BIG) {
      if (ZuUnlikely(inLen >= inSize)) inLen = 0;
      double ratio = (double)(outSize - outLen) / (double)(inSize - inLen);
      if (ZuUnlikely(ratio < 1.0)) ratio = 1.0;
      size_t newOutSize = (size_t)(ratio * 1.1 * (double)inSize);
      size_t minOutSize = (size_t)((double)outSize * 1.1);
      if (ZuUnlikely(newOutSize < minOutSize)) newOutSize = minOutSize;
      newOutSize = ZtIconvFn<Out>::length(out, newOutSize);
      if (ZuUnlikely(newOutSize <= minOutSize)) {
	outLen += (newOutSize - outSize);
	outSize = newOutSize;
	goto ret;
      }
      outBuf = ZtIconvFn<Out>::data(out) + (outSize - outLen);
      outLen += (newOutSize - outSize);
      outSize = newOutSize;
      goto resized;
    }
  ret:
    iconv(m_cd, 0, 0, 0, 0);
    ZtIconvFn<Out>::length(out, outSize - outLen);
    return outSize - outLen;
  }

private:
  iconv_t	m_cd;
};

#endif /* ZtIconv_HPP */
