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

// fast UTF-8/16/32 conversion

#ifndef ZuUTF_HPP
#define ZuUTF_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuString.hpp>

struct ZuUTF8 {
  using Elem = uint8_t;

  static unsigned in(const uint8_t *s, unsigned n, uint32_t &u_) {
    if (ZuUnlikely(n < 1)) return 0;
    uint8_t c = *s;
    if (ZuLikely(c < 0x80)) {
      u_ = c;
      return 1;
    }
    if (ZuUnlikely(n < 2)) return 0;
    uint32_t u = c;
    if (ZuLikely((c>>5) == 0x6)) {
      c = *++s;
      u_ = ((u<<6) & 0x7ff) + (c & 0x3f);
      return 2;
    }
    if (ZuUnlikely(n < 3)) return 0;
    if (ZuLikely((c>>4) == 0xe)) {
      c = *++s;
      u = ((u<<12) & 0xffff) + ((((uint32_t)c)<<6) & 0xfff);
      c = *++s;
      u_ = u + (c & 0x3f);
      return 3;
    }
    if (ZuUnlikely(n < 4)) return 0;
    if (ZuLikely((c>>3U) == 0x1e)) {
      c = *++s;
      u = ((u<<18) & 0x1fffff) + ((((uint32_t)c)<<12) & 0x3ffff);
      c = *++s;
      u += (((uint32_t)c)<<6) & 0xfff;
      c = *++s;
      u_ = u + (c & 0x3f);
      return 4;
    }
    return 0;
  }

  static unsigned out(uint32_t u) {
    if (ZuLikely(u < 0x80)) return 1;
    if (ZuLikely(u < 0x800)) return 2;
    if (ZuLikely(u < 0x10000)) return 3;
    return 4;
  }

  static unsigned out(uint8_t *s, unsigned n, uint32_t u) {
    if (ZuUnlikely(n < 1)) return 0;
    if (ZuLikely(u < 0x80)) {
      *s = u;
      return 1;
    }
    if (ZuUnlikely(n < 2)) return 0;
    if (ZuLikely(u < 0x800)) {
      *s++ = (u>>6) | 0xc0;
      *s = (u & 0x3f) | 0x80;
      return 2;
    }
    if (ZuUnlikely(n < 3)) return 0;
    if (ZuLikely(u < 0x10000)) {
      *s++ = (u>>12) | 0xe0;
      *s++ = ((u>>6) & 0x3f) | 0x80;
      *s = (u & 0x3f) | 0x80;
      return 3;
    }
    if (ZuUnlikely(n < 4)) return 0;
    *s++ = (u>>18) | 0xf0;
    *s++ = ((u>>12) & 0x3f) | 0x80;
    *s++ = ((u>>6) & 0x3f) | 0x80;
    *s = (u & 0x3f) | 0x80;
    return 4;
  }
};

struct ZuUTF16 {
  using Elem = uint16_t;

  static unsigned in(const uint16_t *s, unsigned n, uint32_t &u_) {
    uint16_t c = *s;
    if (ZuUnlikely(n < 1)) return 0;
    if (ZuLikely(c < 0xd800 || c >= 0xc000)) {
      u_ = c;
      return 1;
    }
    if (ZuUnlikely(n < 2 || c >= 0xdc00)) return 0;
    uint32_t u = c;
    c = *++s;
    if (ZuUnlikely(c < 0xdc00 || c >= 0xc000)) return 0;
    u_ = (((u - 0xd800)<<10) | 0x10000) + (((uint32_t)c) - 0xdc00);
    return 2;
  }

  static unsigned out(uint32_t u) {
    if (ZuLikely(u < 0xd800 || (u >= 0xc000 && u < 0x10000))) return 1;
    return 2;
  }

  static unsigned out(uint16_t *s, unsigned n, uint32_t u) {
    if (ZuUnlikely(n < 1)) return 0;
    if (ZuLikely(u < 0xd800 || (u >= 0xc000 && u < 0x10000))) {
      *s = u;
      return 1;
    }
    if (ZuUnlikely(n < 2)) return 0;
    *s++ = ((u & 0xffff)>>10) + 0xd800;
    *s = (u & 0x3ff) + 0xdc00;
    return 2;
  }
};

struct ZuUTF32 {
  using Elem = uint32_t;

  static unsigned in(const uint32_t *s, unsigned n, uint32_t &u) {
    if (ZuUnlikely(n < 1)) return 0;
    u = *s;
    return 1;
  }

  constexpr static unsigned out(uint32_t) { return 1; }

  static unsigned out(uint32_t *s, unsigned n, uint32_t u) {
    if (ZuUnlikely(n < 1)) return 0;
    *s = u;
    return 1;
  }
};

template <unsigned> struct ZuUTF_;
template <> struct ZuUTF_<1> { using T = ZuUTF8; };
template <> struct ZuUTF_<2> { using T = ZuUTF16; };
template <> struct ZuUTF_<4> { using T = ZuUTF32; };

template <typename OutChar, typename InChar> struct ZuUTF {
  using OutUTF = typename ZuUTF_<sizeof(OutChar)>::T;
  using OutElem = typename OutUTF::Elem;
  using InUTF = typename ZuUTF_<sizeof(InChar)>::T;
  using InElem = typename InUTF::Elem;

  static unsigned len(ZuArray<const InChar> s_) {
    const InElem *s = (const InElem *)s_.data();
    unsigned n = s_.length();
    uint32_t u;
    unsigned l = 0;
    for (unsigned i = n; i; ) {
      unsigned j = InUTF::in(s, i, u);
      if (ZuUnlikely(!j)) break;
      s += j, i -= j;
      l += OutUTF::out(u);
    }
    return l;
  }

  static unsigned cvt(ZuArray<OutChar> o_, ZuArray<const InChar> s_) {
    OutElem *o = (OutElem *)o_.data();
    unsigned l = o_.length();
    const InElem *s = (const InElem *)s_.data();
    unsigned n = s_.length();
    uint32_t u;
    for (unsigned i = n; i; ) {
      unsigned j = InUTF::in(s, i, u);
      if (ZuUnlikely(!j)) break;
      s += j, i -= j;
      j = OutUTF::out(o, l, u);
      if (!j) break;
      o += j, l -= j;
    }
    return o_.length() - l;
  }
};

#endif /* ZuUTF_HPP */
