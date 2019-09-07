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

// mbedtls C++ wrapper - Base 64 encode/decode

#ifndef ZtlsBase64_HPP
#define ZtlsBase64_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZtlsLib.hpp>

#include <mbedtls/md.h>

namespace Ztls {
namespace Base64 {

// both encode and decode return:
// +ve - bytecount written
// -ve - bytes that would have been written (destination buffer is too small)

// null-terminates dst if successful
ZuInline int encode(char *dst, unsigned dlen, const void *src, unsigned slen) {
  size_t olen;
  int i = mbedtls_base64_encode(
      (unsigned char *)dst, dlen, &olen, (const unsigned char *)src, slen);
  if (i) return -(int)olen;
  return (int)olen;
}

// does not null-terminate dst
ZuInline int decode(void *dst, unsigned dlen, const void *src, unsigned slen) {
  size_t olen;
  int i = mbedtls_base64_decode(
      (unsigned char *)dst, dlen, &olen, (const unsigned char *)src, slen);
  if (i) return -(int)olen;
  return (int)olen;
}

}
}

#endif /* ZtlsBase64_HPP */
