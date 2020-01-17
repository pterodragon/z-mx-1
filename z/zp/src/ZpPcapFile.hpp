// -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
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

// ZpPcap - pcap interface

#ifndef ZpPcapFile_HPP
#define ZpPcapFile_HPP

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4251 4800)
#endif

#include <zlib/ZuAssert.hpp>

#include <zlib/ZmTime.hpp>

#include <zlib/ZpLib.hpp>

#define ZPCAP_VULONG(major, minor) (((major)<<16) | (minor))
#define ZPCAP_VMAJOR(n) (((u_int32_t)n)>>16)
#define ZPCAP_VMINOR(n) (((u_int32_t)n) & 0xffff)

#define ZPCAP_VERSION ZPCAP_VULONG(2,4) // manually maintained

// Header for .pcap files version 2.4
// (http://wiki.wireshark.org/Development/LibpcapFileFormat)
class ZpPcapFileHeader {
  u_int32_t m_magic_number;	// magic number
  u_int16_t m_version_major;	// major version number
  u_int16_t m_version_minor;	// minor version number
  int32_t  m_thiszone;		// GMT to local correction
  u_int32_t m_sigfigs;		// accuracy of timestamps
  u_int32_t m_snaplen;		// max length of captured packets, in octets
  u_int32_t m_network;		// data link type

  enum { MagicNumber = 0xa1b2c3d4 };

public:

  // more types here: www.tcpdump.org/linktypes.html
  enum { LinkLayerTypeNull = 0, LinkLayerTypeEthernet = 1 };

  ZpPcapFileHeader(int layer = LinkLayerTypeEthernet, int snapLen = 65535,
		   int zone = 0) :
    m_magic_number(MagicNumber), m_version_major(ZPCAP_VMAJOR(ZPCAP_VERSION)),
    m_version_minor(ZPCAP_VMINOR(ZPCAP_VERSION)), m_thiszone(zone),
    m_sigfigs(0), m_snaplen(snapLen), m_network((u_int32_t)layer) { }

  u_int32_t magicNumber() const { return m_magic_number; }
  u_int32_t version() const {
    return ZPCAP_VULONG(m_version_major, m_version_minor);
  }

  int timeZone() const { return m_thiszone; }
  int snapLen() const { return m_snaplen; }
  int linkLayerType() const { return m_network; }
};
ZuAssert(sizeof(ZpPcapFileHeader) == (6 * 4));

class ZpPcapRecordHeader {
  u_int32_t m_ts_sec;         // timestamp seconds
  u_int32_t m_ts_usec;        // timestamp microseconds
  u_int32_t m_incl_len;       // number of octets of packet saved in file
  u_int32_t m_orig_len;       // actual length of packet

public:
  ZpPcapRecordHeader(u_int64_t nsec, int len, int origLen) :
    m_ts_sec(nsec / 1000000000), m_ts_usec((nsec % 1000000000) / 1000),
    m_incl_len(len), m_orig_len(origLen) { }

  ZpPcapRecordHeader(ZmTime ts, int len, int origLen) :
    m_ts_sec((u_int32_t)ts.sec()), m_ts_usec(ts.nsec() / 1000),
    m_incl_len(len), m_orig_len(origLen) { }

  int sec() const { return m_ts_sec; }
  int usec() const { return m_ts_usec; }
  int len() const { return m_incl_len; }
  int origLen() const { return m_orig_len; }
};

ZuAssert(sizeof(ZpPcapRecordHeader) == (4 * 4));

#endif /* ZpPcapFile_HPP */
