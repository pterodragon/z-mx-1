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

// time-based one time password

#ifndef ZtlsTOTP_HPP
#define ZtlsTOTP_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZtlsLib.hpp>

#include <stdlib.h>

#include <ZuByteSwap.hpp>

#include <ZmTime.hpp>

#include <ZtlsHMAC.hpp>

namespace Ztls {
namespace TOTP {

// Google Authenticator
inline unsigned google(const void *data, unsigned len, int timeOffset = 0) {
  ZuBigEndian<uint64_t> t = (ZmTimeNow().sec() / 30) + timeOffset;
  HMAC hmac(MBEDTLS_MD_SHA1);
  uint8_t sha1[20];
  hmac.start(data, len);
  hmac.update(&t, 8);
  hmac.finish(sha1);
  ZuBigEndian<uint32_t> code;
  memcpy(&code, sha1 + (sha1[19] & 0xf), 4);
  return code % (uint32_t)1000000;
}

}
}

#endif /* ZtlsTOTP_HPP */
