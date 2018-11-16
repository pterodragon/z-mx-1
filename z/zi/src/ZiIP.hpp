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

// IP address

#ifndef ZiIP_HPP
#define ZiIP_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <ZiLib.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuTraits.hpp>
#include <ZuString.hpp>
#include <ZuPrint.hpp>

#include <ZePlatform.hpp>

#include <ZiPlatform.hpp>

class ZiAPI ZiIP : public in_addr {
public:
  typedef ZiPlatform::Hostname Hostname;

  enum Result {
    OK		= Zi::OK,
    IOError	= Zi::IOError
  };

  ZuInline ZiIP() { s_addr = 0; }

  ZuInline ZiIP(const ZiIP &a) { s_addr = a.s_addr; }
  ZuInline ZiIP &operator =(const ZiIP &a) {
    s_addr = a.s_addr;
    return *this;
  }

  ZuInline explicit ZiIP(struct in_addr ia) { s_addr = ia.s_addr; }
  ZuInline ZiIP &operator =(struct in_addr ia) {
    s_addr = ia.s_addr;
    return *this;
  }

  template <typename S>
  inline ZiIP(S &&s, typename ZuIsString<S>::T *_ = 0) {
    if (!s) { s_addr = 0; return; }
    ZeError e;
    if (resolve(ZuFwd<S>(s), &e) != OK) throw e;
  }
  template <typename S>
  inline typename ZuIsString<S, ZiIP &>::T &operator =(S &&s) {
    if (!s) { s_addr = 0; return *this; }
    ZeError e;
    if (resolve(ZuFwd<S>(s), &e) != OK) throw e;
    return *this;
  }

  ZuInline bool equals(const ZiIP &a) const { return s_addr == a.s_addr; }
  ZuInline int cmp(const ZiIP &a) const {
    return ZuCmp<uint32_t>::cmp(s_addr, a.s_addr);
  }
  ZuInline bool operator ==(const ZiIP &a) const { return s_addr == a.s_addr; }
  ZuInline bool operator !=(const ZiIP &a) const { return s_addr != a.s_addr; }

  ZuInline bool operator !() const { return !s_addr; }
  ZuOpBool

  ZuInline uint32_t hash() const { return *(uint32_t *)&s_addr; }

  template <typename S> void print(S &s) const {
    uint32_t addr = (uint32_t)ntohl(s_addr);
    s <<
      ZuBoxed((((uint32_t)addr)>>24) & 0xff) << '.' <<
      ZuBoxed((((uint32_t)addr)>>16) & 0xff) << '.' <<
      ZuBoxed((((uint32_t)addr)>>8 ) & 0xff) << '.' <<
      ZuBoxed((((uint32_t)addr)    ) & 0xff);
  }

  ZuInline bool multicast() const {
    unsigned i = (((uint32_t)ntohl(s_addr))>>24) & 0xff;
     return i >= 224 && i < 240;
  }

private:
  int resolve_(ZuString, ZeError *e = 0);
#ifdef _WIN32
  int resolve_(ZuWString, ZeError *e = 0);
#endif
public:
  template <typename S>
  inline typename ZuIsString<S, int>::T resolve(S &&s, ZeError *e = 0) {
    return resolve_(ZuFwd<S>(s), e);
  }
  Hostname name(ZeError *e = 0);
};

template <> struct ZuTraits<ZiIP> : public ZuGenericTraits<ZiIP> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};

// generic printing
template <> struct ZuPrint<ZiIP> : public ZuPrintFn { };

class ZiSockAddr {
public:
  ZuInline ZiSockAddr() { null(); }
  inline ZiSockAddr(ZiIP ip, uint16_t port) { init(ip, port); }

  ZuInline void null() { m_sin.sin_family = AF_UNSPEC; }
  inline void init(ZiIP ip, uint16_t port) {
    m_sin.sin_family = AF_INET;
    m_sin.sin_port = htons(port);
    m_sin.sin_addr = ip;
    memset(&m_sin.sin_zero, 0, sizeof(m_sin.sin_zero));
  }

  ZuInline ZiIP ip() const { return ZiIP(m_sin.sin_addr); }
  ZuInline uint16_t port() const { return ntohs(m_sin.sin_port); }

  ZuInline struct sockaddr *sa() { return (struct sockaddr *)&m_sin; }
  ZuInline int len() const { return sizeof(struct sockaddr_in); }

  ZuInline bool operator !() const { return m_sin.sin_family == AF_UNSPEC; }
  ZuOpBool

  struct sockaddr_in	m_sin;
};

#endif /* ZiIP_HPP */
