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

// cppcodec C++ wrapper - Hex (uppercase) encode/decode

#ifndef ZtlsHex_HPP
#define ZtlsHex_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZtlsLib.hpp>

#include <cppcodec/hex_upper.hpp>

namespace Ztls {
namespace Hex {

// both encode and decode return:
// +ve - bytecount written
// -ve - bytes that would have been written (destination buffer is too small)

// does not null-terminate dst
ZuInline unsigned enclen(unsigned slen) { return slen<<1; }
ZuInline int encode(char *dst, unsigned dlen, const void *src, unsigned slen) {
  using hex = cppcodec::hex_upper;
  return hex::encode(dst, dlen, (const uint8_t *)src, slen);
}

// does not null-terminate dst
ZuInline unsigned declen(unsigned slen) { return (slen + 1)>>1; }
ZuInline int decode(void *dst, unsigned dlen, const char *src, unsigned slen) {
  using hex = cppcodec::hex_upper;
  return hex::decode((uint8_t *)dst, dlen, src, slen);
}

}
}

#endif /* ZtlsHex_HPP */
