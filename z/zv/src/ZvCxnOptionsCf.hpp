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

// socket connection options configuration

#ifndef ZvCxnOptionsCf_HPP
#define ZvCxnOptionsCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

class ZvAPI ZvInvalidMulticastIP : public ZvError {
public:
  inline ZvInvalidMulticastIP(ZuString s) : m_addr(s) { }
#if 0
  inline ZvInvalidMulticastIP(const ZvInvalidMulticastIP &e) :
    m_addr(e.m_addr) { }
  inline ZvInvalidMulticastIP &operator =(const ZvInvalidMulticastIP &e) {
    if (this != &e) m_addr = e.m_addr;
    return *this;
  }
  inline ZvInvalidMulticastIP(ZvInvalidMulticastIP &&e) :
    m_addr(ZuMv(e.m_addr)) { }
  inline ZvInvalidMulticastIP &operator =(ZvInvalidMulticastIP &&e) {
    m_addr = ZuMv(e.m_addr);
    return *this;
  }
#endif

  void print_(ZmStream &s) const {
    s << "invalid multicast IP \"" << m_addr << '"';
  }

private:
  ZtString	m_addr;
};

struct ZvCxnOptions : public ZiCxnOptions {
  inline ZvCxnOptions() : ZiCxnOptions() { }

  inline ZvCxnOptions(const ZiCxnOptions &p) : ZiCxnOptions(p) { }
  inline ZvCxnOptions &operator =(const ZiCxnOptions &p) {
    ZiCxnOptions::operator =(p);
    return *this;
  }

  inline ZvCxnOptions(ZvCf *cf) : ZiCxnOptions() { init(cf); }

  inline ZvCxnOptions(ZvCf *cf, const ZiCxnOptions &deflt) :
      ZiCxnOptions(deflt) { init(cf); }

  inline void init(ZvCf *cf) {
    if (!cf) return;

    flags(cf->getFlags<ZiCxnOptions::Flags>("options", false, 0));

    // multicastInterface is the IP address of the interface used for sending
    // multicastTTL is the TTL (number of hops) used for sending
    // multicastGroups are the groups that are subscribed to for receiving
    // - each group is "addr interface", where addr is the multicast IP
    //   address of the group being subscribed to, and interface is the
    //   IP address of the interface used to receive the messages; interface
    //   can be 0.0.0.0 to subscribe on all interfaces
    // Example: multicastGroups { 239.193.2.51 192.168.1.99 }
    if (multicast()) {
      if (ZuString s = cf->get("multicastInterface", false)) mif(s);
      ttl(cf->getInt("multicastTTL", 0, INT_MAX, false, ttl()));
      if (ZmRef<ZvCf> groups = cf->subset("multicastGroups", false)) {
	ZvCf::Iterator i(groups);
	ZuString addr_, mif_;
	while (mif_ = i.get(addr_)) {
	  ZiIP addr(addr_), mif(mif_);
	  if (!addr || !addr.multicast()) throw ZvInvalidMulticastIP(addr_);
	  mreq(ZiMReq(addr, mif));
	}
      }
    }
    if (netlink())
      familyName(cf->get("familyName", true));
#if 0
    if (netmap())
      if (ZuString s = cf->get("netmapInterface", true)) nif(s);
#endif
  }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvCxnOptionsCf_HPP */
