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

// mbedtls C++ wrapper - HMAC message digest

#ifndef ZtlsHMAC_HPP
#define ZtlsHMAC_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZtlsLib.hpp>

#include <mbedtls/md.h>

namespace Ztls {

class HMAC {
public:
  // mbedtls_md_type_t, e.g. MBEDTLS_MD_SHA256
  HMAC(mbedtls_md_type_t type) {
    mbedtls_md_init(&m_ctx);
    mbedtls_md_setup(&m_ctx, mbedtls_md_info_from_type(type), 1);
  }
  ~HMAC() {
    mbedtls_md_free(&m_ctx);
  }

  ZuInline void start(ZuArray<uint8_t> a) {
    mbedtls_md_hmac_starts(&m_ctx, (const unsigned char *)a.data(), a.length());
  }
  ZuInline void start(const void *key, unsigned length) {
    mbedtls_md_hmac_starts(&m_ctx, (const unsigned char *)key, length);
  }

  ZuInline void update(ZuArray<uint8_t> a) {
    mbedtls_md_hmac_update(&m_ctx, (const unsigned char *)a.data(), a.length());
  }
  ZuInline void update(const void *data, unsigned length) {
    mbedtls_md_hmac_update(&m_ctx, (const unsigned char *)data, length);
  }

  // MBEDTLS_MD_MAX_SIZE is max size of output buffer (i.e. 64 for SHA512)
  ZuInline void finish(void *output) {
    mbedtls_md_hmac_finish(&m_ctx, (unsigned char *)output);
  }

  ZuInline void reset() {
    mbedtls_md_hmac_reset(&m_ctx);
  }

private:
  mbedtls_md_context_t	m_ctx;
};

}

#endif /* ZtlsHMAC_HPP */
