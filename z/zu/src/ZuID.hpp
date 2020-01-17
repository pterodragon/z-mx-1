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

// Union of a 64bit integer and a human-readable string
// (8-byte left-aligned zero-padded)

#ifndef ZuID_HPP
#define ZuID_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuString.hpp>

// An ZuID is a union of a 64bit unsigned integer with an 8-byte
// zero-padded string; this permits short human-readable string identifiers
// to be compared and hashed very rapidly using integer 64bit operations
// without needing a map between names and numbers

// Note: the string will not be null-terminated if all 8 bytes are in use

// the implementation assumes the architecture supports misaligned 64bit loads

class ZuID {
public:
  ZuInline ZuID() : m_val(0) { }

  ZuInline ZuID(const ZuID &b) : m_val(b.m_val) { }
  ZuInline ZuID &operator =(const ZuID &b) { m_val = b.m_val; return *this; }

  template <typename S>
  ZuInline ZuID(S &&s, typename ZuIsString<S>::T *_ = 0) {
    init(ZuFwd<S>(s));
  }
  template <typename S>
  ZuInline typename ZuIsString<S, ZuID &>::T operator =(S &&s) {
    init(ZuFwd<S>(s));
    return *this;
  }

  template <typename V> struct IsUInt64 {
    enum { OK = !ZuTraits<V>::IsString &&
      ZuConversion<V, uint64_t>::Exists };
  };
  template <typename V, typename R = void>
  struct MatchUInt64 : public ZuIfT<IsUInt64<V>::OK, R> { };

  template <typename V>
  ZuInline ZuID(V v, typename MatchUInt64<V>::T *_ = 0) : m_val(v) { }
  template <typename V>
  ZuInline typename MatchUInt64<V, ZuID &>::T operator =(V v) {
    m_val = v;
    return *this;
  }

  ZuInline void init(ZuString s) {
    if (ZuLikely(s.length() == 8)) {
      const uint64_t *ZuMayAlias(ptr) = (const uint64_t *)s.data();
      m_val = *ptr; // potentially misaligned load
      return;
    }
    m_val = 0;
    unsigned n = s.length();
    if (ZuUnlikely(!n)) return;
    if (ZuUnlikely(n > 8)) n = 8;
    memcpy(&m_val, s.data(), n);
  }

  ZuInline char *data() {
    char *ZuMayAlias(ptr) = (char *)&m_val;
    return ptr;
  }
  ZuInline const char *data() const {
    const char *ZuMayAlias(ptr) = (const char *)&m_val;
    return ptr;
  }
  ZuInline unsigned length() const {
    if (!m_val) return 0U;
#ifdef Zu_BIGENDIAN
    return (71U - __builtin_clzl(m_val))>>3U;
#else
    return (71U - __builtin_ctzl(m_val))>>3U;
#endif
  }

  ZuInline operator ZuString() const { return ZuString(data(), length()); }
  ZuInline ZuString string() const { return ZuString(data(), length()); }

  template <typename S> ZuInline void print(S &s) const { s << string(); }

  ZuInline operator uint64_t() const { return m_val; }

  ZuInline bool equals(uint64_t v) const { return m_val == v; }
  ZuInline int cmp(uint64_t v) const {
    return m_val < v ? -1 : m_val > v ? 1 : 0;
  }

  ZuInline bool operator ==(uint64_t v) const { return m_val == v; }
  ZuInline bool operator !=(uint64_t v) const { return m_val != v; }
  ZuInline bool operator >(uint64_t v) const { return m_val > v; }
  ZuInline bool operator >=(uint64_t v) const { return m_val >= v; }
  ZuInline bool operator <(uint64_t v) const { return m_val < v; }
  ZuInline bool operator <=(uint64_t v) const { return m_val <= v; }

  ZuInline bool operator !() const { return !m_val; }

  ZuInline bool operator *() const { return m_val; }

  ZuInline void null() { m_val = 0; }

  ZuInline ZuID &update(const ZuID &id) {
    if (*id) m_val = id.m_val;
    return *this;
  }

  ZuInline uint32_t hash() const { return ZuHash<uint64_t>::hash(m_val); }

private:
  uint64_t	m_val;
};
template <> struct ZuTraits<ZuID> : public ZuTraits<uint64_t> {
  typedef ZuID T;
  enum { IsPrimitive = 0, IsComparable = 1, IsHashable = 1 };
};
template <> struct ZuCmp<ZuID> : public ZuCmp0<uint64_t> {
  ZuInline static const ZuID &null() { static const ZuID v; return v; }
  ZuInline static bool null(uint64_t id) { return !id; }
};
template <> struct ZuPrint<ZuID> : public ZuPrintFn { };

#endif /* ZuID_HPP */
