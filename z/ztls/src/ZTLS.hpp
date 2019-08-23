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

// ZTLS - mbedtls C++ wrapper

#ifndef ZTLS_HPP
#define ZTLS_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZTLSLib.hpp>

#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl_cache.h>
#include <mbedtls/ssl_ticket.h>

#include <ZuString.hpp>

namespace ZTLS {

class Conf {
public:
  inline Conf() {
    mbedtls_entropy_init(&m_entropy);
    mbedtls_ctr_drbg_init(&m_ctr_drbg);
    mbedtls_x509_crt_init(&m_cacert);
    mbedtls_ssl_config_init(&m_conf);
  }
  inline ~Conf() {
    mbedtls_ssl_config_free(&m_conf);
    mbedtls_x509_crt_free(&m_cacert);
    mbedtls_ctr_drbg_free(&m_ctr_drbg);
    mbedtls_entropy_free(&m_entropy);
  }

  inline mbedtls_ssl_config *conf() { return &m_conf; }

protected:
  bool init() {
    mbedtls_ssl_conf_dbg(&m_conf, my_debug, stdout); // FIXME
    if (mbedtls_ctr_drbg_seed(
	  &m_ctr_drbg, mbedtls_entropy_func, &m_entropy, 0, 0)) return false;
    mbedtls_ssl_conf_rng(&m_conf, mbedtls_ctr_drbg_random, &m_ctr_drbg );
    mbedtls_ssl_conf_renegotiation(&m_conf, MBEDTLS_SSL_RENEGOTIATION_ENABLED);
    return true;
  }

  // Arch - /etc/ssl/cert.pem
  // Ubuntu - /etc/ssl/certs
  inline bool loadCA(ZuString path) {
    if (ZiFile::isdir(path))
      ret = mbedtls_x509_crt_parse_path(&m_cacert, path);
    else
      ret = mbedtls_x509_crt_parse_file(&m_cacert, path);
    if (ret < 0) return false;
    mbedtls_ssl_conf_ca_chain(&m_conf, &m_cacert, 0);
  }

protected:
  mbedtls_entropy_context	m_entropy;
  mbedtls_ctr_drbg_context	m_ctr_drbg;
  mbedtls_x509_crt		m_cacert;
  mbedtls_ssl_config		m_conf;
};

class SSL {
public:
  inline SSL(Conf *conf, ZiConnection *cxn) {
    mbedtls_ssl_init(&m_ssl);
    mbedtls_ssl_setup(&m_ssl, conf->conf());
  // FIXME - client side - need to set hostname to verify server
    if( ( ret = mbedtls_ssl_set_hostname( &ssl, opt.server_name ) ) != 0 )
  }
  inline ~SSL() {
    mbedtls_ssl_free(&m_ssl);
  }

  // FIXME - handshake,read, write, etc.

protected:
  mbedtls_ssl_context	m_ssl;
  ZiConnection		*m_cxn; // FIXME
};

// FIXME Cl - equivalent to CliLink
// FIXME CliSSL - equivalent to CliLink - includes connect, etc.
// FIXME SrvSSL - equivalent to SrvLink

class Client : public Conf {
public:
  inline bool init(ZuString caPath, const char **alpn) {
    mbedtls_ssl_config_defaults(&m_conf,
	MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_STREAM,
	MBEDTLS_SSL_PRESET_DEFAULT);

    Conf::init();

    mbedtls_ssl_conf_session_tickets(&m_conf,
	MBEDTLS_SSL_SESSION_TICKETS_ENABLED);

    mbedtls_ssl_conf_authmode(&m_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    if (!loadCA(caPath)) return false;
    if (list) mbedtls_ssl_conf_alpn_protocols(&m_conf, list);

    return true;
  }

  // FIXME - mkLink(host, port, etc.) -> CliSSL etc.
};

class Server : public Conf {
public:
  inline Server() {
    mbedtls_x509_crt_init(&m_cert);
    mbedtls_pk_init(&m_key);
    mbedtls_ssl_cache_init(&m_cache);
    mbedtls_ssl_ticket_init(&m_ticket_ctx);
  }
  inline ~Server() {
    mbedtls_ssl_ticket_free(&m_ticket_ctx);
    mbedtls_ssl_cache_free(&m_cache);
    mbedtls_pk_free(&m_key);
    mbedtls_x509_crt_free(&m_cert);
  }

  inline bool init(ZuString caPath, const char **alpn,
      ZuString certPath, ZuString keyPath) {
    mbedtls_ssl_config_defaults(&m_conf,
	MBEDTLS_SSL_IS_SERVER,
	MBEDTLS_SSL_TRANSPORT_STREAM,
	MBEDTLS_SSL_PRESET_DEFAULT);

    Conf::init();

    mbedtls_ssl_cache_set_max_entries(&m_cache, opt.cache_max); // FIXME
    mbedtls_ssl_cache_set_timeout(&m_cache, opt.cache_timeout); // FIXME
    mbedtls_ssl_conf_session_cache(&m_conf, &m_cache,
	mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);

    if (mbedtls_ssl_ticket_setup(&m_ticket_ctx,
	  mbedtls_ctr_drbg_random, &ctr_drbg,
	  MBEDTLS_CIPHER_AES_256_GCM,
	  opt.ticket_timeout)) return false;
    mbedtls_ssl_conf_session_tickets_cb(&m_conf,
	mbedtls_ssl_ticket_write,
	mbedtls_ssl_ticket_parse,
	&m_ticket_ctx);

    mbedtls_ssl_conf_authmode(&m_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    if (!loadCA(caPath)) return false;
    if (list) mbedtls_ssl_conf_alpn_protocols(&m_conf, list);

    if (mbedtls_x509_crt_parse_file(&m_cert, certPath)) return false;
    if (mbedtls_pk_parse_keyfile(&m_pkey, keyPath, "")) return false;
    if (mbedtls_ssl_conf_own_cert(&m_conf, &m_cert, &m_key)) return false;

    return true;
  }

  // FIXME - listen(), accept() -> SSL etc.
  // FIXME - host, port, etc.
 
protected:
  mbedtls_x509_crt		m_cert;
  mbedtls_pk_context		m_key;
  mbedtls_ssl_cache_context	m_cache;
  mbedtls_ssl_ticket_context	m_ticket_ctx;
};

}

#endif /* ZTLS_HPP */