// -*- mode: c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
// vi: noet ts=8 sw=2

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

#ifndef ZiNetlinkMsg_HPP
#define ZiNetlinkMsg_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <stdint.h>

#include <zi_netlink.h>

#include <ZuAssert.hpp>

#include <ZtString.hpp>

#include <ZiPlatform.hpp>

class ZiConnection;

class ZiNetlinkHdr {
  ZiNetlinkHdr(const ZiNetlinkHdr &);
  ZiNetlinkHdr &operator =(const ZiNetlinkHdr &);

public:
  enum _ { PADDING = NLMSG_HDRLEN - sizeof(struct nlmsghdr) };

  inline uint32_t hdrSize() const { return NLMSG_HDRLEN; }
  inline uint32_t len() const { return m_n.nlmsg_len; }
  inline uint32_t dataLen() const { return m_n.nlmsg_len - NLMSG_HDRLEN; }

  inline uint16_t type() const { return m_n.nlmsg_type; }
  inline uint16_t flags() const { return m_n.nlmsg_flags; }
  inline uint32_t seq() const { return m_n.nlmsg_seq; }
  inline uint32_t pid() const { return m_n.nlmsg_pid; }

  inline struct nlmsghdr *hdr() { return &m_n; }

  template <typename S> inline void print(S &s) const {
    s << "ZiNetlinkHdr [[len = " << m_n.nlmsg_len
      << "] [type = " << m_n.nlmsg_type
      << "] [flags = " << m_n.nlmsg_flags
      << "] [seqNo = " << m_n.nlmsg_seq
      << "] [pid = " << m_n.nlmsg_pid
      << "] [hdrSize = " << hdrSize()
      << "] [dataLen = " << dataLen()
      << "] [size = " << hdrSize()
      << "]]";
  }

protected:
  inline ZiNetlinkHdr() { memset(&m_n, 0, sizeof(struct nlmsghdr)); }

  inline ZiNetlinkHdr(uint32_t len, uint16_t type,
      uint16_t flags, uint32_t seqNo, uint32_t portID) :
    m_n{(len + NLMSG_HDRLEN), type, flags, seqNo, portID} { }

private:
  struct nlmsghdr		m_n;
  char				m_pad[PADDING];
};
template <> struct ZuPrint<ZiNetlinkHdr> : public ZuPrintFn { };

class ZiGenericNetlinkHdr : public ZiNetlinkHdr {
  ZiGenericNetlinkHdr(const ZiGenericNetlinkHdr &);
  ZiGenericNetlinkHdr &operator =(const ZiGenericNetlinkHdr &);

public:
  enum _ { PADDING = GENL_HDRLEN - sizeof(struct genlmsghdr) };

  inline ZiGenericNetlinkHdr() { memset(&m_g, 0, sizeof(struct genlmsghdr)); }

  ZiGenericNetlinkHdr(uint32_t len, uint16_t type, uint16_t flags,
		      uint32_t seqNo, uint32_t portID, uint8_t cmd) :
    ZiNetlinkHdr(GENL_HDRLEN + len, type, flags, seqNo, portID) :
      m_g{cmd, ZiGenericNetlinkVersion, 0} { }

  ZiGenericNetlinkHdr(ZiConnection *connection, uint32_t seqNo, uint32_t len);

  inline uint8_t cmd() const { return m_g.cmd; }
  inline uint8_t version() const { return m_g.version; }
  inline uint32_t hdrSize() const { 
    return GENL_HDRLEN + ZiNetlinkHdr::hdrSize(); 
  }

  template <typename S> inline void print(S &s) const {
    s << "ZiGenericNetlinkHdr [" << static_cast<const ZiNetlinkHdr &>(*this)
      << " [cmd = " << m_g.cmd
      << "] [version = " << m_g.version
      << "] [reserved = " << m_g.reserved
      << "] [size = " << hdrSize()
      << "]]";
  }

private:
  struct genlmsghdr	m_g;
  char			m_pad[PADDING];
};
#define ZiGenericNetlinkHdr2Vec(x) (void *)&(x), x.hdrSize()
template <> struct ZuPrint<ZiGenericNetlinkHdr> : public ZuPrintFn { };

// Netlink Attributes

/* from netlink.h:
 *
 *  <------- NLA_HDRLEN ------> <-- NLA_ALIGN(payload)-->
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 * |        Header       | Pad |     Payload       | Pad |
 * |   (struct nlattr)   | ing |                   | ing |
 * +---------------------+- - -+- - - - - - - - - -+- - -+
 *  <-------------- nlattr->nla_len -------------->
 * NLA_HDRLEN				== hdrLen()
 * nlattr->nla_len			== len()
 * NLA_ALIGN(payload) - NLA_HDRLEN	== dataLen()
 * NLA_HDRLEN + NLA_ALIGN(payload)	== size()
 */
class ZiNetlinkAttr {
  ZiNetlinkAttr(const ZiNetlinkAttr &);
  ZiNetlinkAttr &operator =(const ZiNetlinkAttr &);

public:
  enum _ { PADDING = 0 }; // NLA_HDRLEN - sizeof(struct nlattr) };

  ZiNetlinkAttr() { memset(this, 0, sizeof(ZiNetlinkAttr)); }

  inline uint16_t hdrLen() const { return NLA_HDRLEN; }
  inline uint16_t len() const { return m_na.nla_len; }
  inline uint16_t dataLen() const { return len() - hdrLen(); }
  inline uint16_t size() const { return hdrLen() + NLA_ALIGN(dataLen()); }
  inline uint16_t type() const { return m_na.nla_type; }

  inline char *data_() { return ((char *)&m_na); }
  inline char *data() { return ((char *)&m_na) + hdrLen(); }
  inline const char *data() const { return ((const char *)&m_na) + hdrLen(); }
  inline ZiNetlinkAttr *next() {
    return (ZiNetlinkAttr *)((char *)&m_na + size());
  }

  template <typename S> inline void print(S &s) const {
    s << "ZiNetlinkAttr [[len = " << m_na.nla_len
      << "] [type = " << m_na.nla_type
      << "] [hdrLen = " << hdrLen()
      << "] [dataLen = " << dataLen()
      << "] [size = " << size()
      << "]]";
  }
		
protected:	   
  inline ZiNetlinkAttr(uint16_t type, uint16_t len) :
    m_na{NLA_HDRLEN + PADDING + len, type} { }

private:
  struct nlattr		m_na;
  char			m_pad[PADDING];
};
template <> struct ZuPrint<ZiNetlinkHdr> : public ZuPrintFn { };

class ZiNetlinkFamilyName : public ZiNetlinkAttr {
  enum _ { PADDING = NLA_ALIGN(GENL_NAMSIZ) - GENL_NAMSIZ };

public:
  inline ZiNetlinkFamilyName(ZuString s) :
    ZiNetlinkAttr(CTRL_ATTR_FAMILY_NAME,
	s.length() >= GENL_NAMSIZ ? GENL_NAMSIZ : (s.length() + 1)) {
    unsigned len = s.length() >= GENL_NAMSIZ ? GENL_NAMSIZ : (s.length() + 1);
    memcpy(m_familyName, s.data(), len - 1);
    m_familyName[len] = 0;
  }

  template <typename S> inline void print(S &s) const {
    s << "ZiNetlinkFamilyName [" << static_cast<const ZiNetlinkAttr &>(*this)
      << " [familyName = " << m_familyName << "]]";
  }

private:
  char m_familyName[GENL_NAMSIZ];
  char m_pad[PADDING];
};
template <> struct ZuPrint<ZiNetlinkFamilyName> : public ZuPrintFn { };

class ZiNetlinkDataAttr : public ZiNetlinkAttr {
public:
  ZiNetlinkDataAttr(int len) : ZiNetlinkAttr(ZiGNLAttr_Data, len) { }
};
#define ZiNelinkDataAttr2Vec(x) (void *)x.data_(), x.hdrLen()

template <typename T, uint16_t AttrType>
class ZiNetlinkAttr_ : public ZiNetlinkAttr {
  enum _ { PADDING = NLA_ALIGN(sizeof(T)) };

public:
  ZiNetlinkAttr_(const T &v) : ZiNetlinkAttr(AttrType, sizeof(T)) {
    m_data = v;
  }

  inline const T &data() const { return m_data; }

private:
  T	m_data;
  char	m_pad[PADDING];
};

typedef ZiNetlinkAttr_<uint16_t, ZiGNLAttr_PCI> ZiNetlinkOpCodeAttr;

#endif /* ZiNetlinkMsg_HPP */
