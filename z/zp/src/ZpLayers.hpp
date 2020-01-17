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

// ZpLayer - OSI layer type layouts

#ifndef ZpLayer_HPP
#define ZpLayer_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZiIP.hpp>
#include <zlib/ZpExtract.hpp>

struct ZpLayer {
  struct Protocol { enum { TCP = 0x06 }; };
};

class ZpLayerIP {
  struct Data {
    u_int8_t	m_vhl;		/* header length, version */
    u_int8_t	m_tos;		/* type of service */
    u_int16_t	m_len;		/* total length */
    u_int16_t	m_id;		/* identification */
    u_int16_t	m_off;		/* fragment offset field */
    u_int8_t	m_ttl;		/* time to live */
    u_int8_t	m_proto;	/* protocol */
    u_int16_t	m_checksum;	/* checksum */
    in_addr     m_src;          /* source and dest address */
    in_addr     m_dst;
  };

public:
  static int version(const uint8_t *ip) {
    return (((const Data *)ip)->m_vhl & 0xf0) >> 4;
  }

  static int len(const uint8_t *ip) {
    return ZpExtract_16BITS(&((const Data *)ip)->m_len);
  }
  
  // in number of bytes
  static int headLen(const uint8_t *ip) {
    return (((const Data *)ip)->m_vhl & 0x0f) * 4;
  }

  static int transport(const uint8_t *ip) {
    return ((const Data *)ip)->m_proto;
  }

  static ZiIP src(const uint8_t *ip) {
    return ZiIP(((const Data *)ip)->m_src);
  }

  static ZiIP dst(const uint8_t *ip) {
    return ZiIP(((const Data *)ip)->m_dst);
  }
};

class ZpLayerTCP {
  struct Data {
    u_int16_t	m_sport;		/* source port */
    u_int16_t	m_dport;		/* destination port */
    u_int32_t	m_seq;			/* sequence number */
    u_int32_t	m_ack;			/* acknowledgement number */
    u_int8_t	m_offx2;		/* data offset, rsvd */
    u_int8_t	m_flags;
    u_int16_t	m_win;			/* window */
    u_int16_t	m_sum;			/* checksum */
    u_int16_t	m_urp;			/* urgent pointer */
  };

public:
  struct CtrlFlags { enum  { FIN = 0, SYN, RST, PSH, ACK, URG, ECE, CWR  }; };

  // in number of bytes
  static int offset(const uint8_t *tcp) {
    return ((((const Data *)tcp)->m_offx2 & 0xf0) >> 4) * 4;
  }

  static int seq(const uint8_t *tcp) {
    return ZpExtract_32BITS(&((const Data *)tcp)->m_seq);
  }

  static int sport(const uint8_t *tcp) {
    return ZpExtract_16BITS(&((const Data *)tcp)->m_sport);
  }

  static int dport(const uint8_t *tcp) {
    return ZpExtract_16BITS(&((const Data *)tcp)->m_dport);
  }
  int flags(const uint8_t *tcp) {
    return ((const Data *)tcp)->m_flags; }
};

#endif /* ZpLayer_HPP */
