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

// hwloc bitmap

#ifndef ZmBitmap_HPP
#define ZmBitmap_HPP

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4800)
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <hwloc.h>

#include <zlib/ZuPrint.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuConversion.hpp>

class ZmBitmap;

template <> struct ZuTraits<ZmBitmap> : public ZuGenericTraits<ZmBitmap> {
  enum { IsComparable = 1 };
};

class ZmBitmap {
public:
  ZuInline ZmBitmap() : m_map(0) { }
  ZuInline ZmBitmap(const ZmBitmap &b) :
    m_map(b.m_map ? hwloc_bitmap_dup(b.m_map) : (hwloc_bitmap_t)0) { }
  ZuInline ZmBitmap(ZmBitmap &&b) : m_map(b.m_map) { b.m_map = 0; }
  ZuInline ZmBitmap &operator =(const ZmBitmap &b) {
    if (this == &b) return *this;
    if (!b.m_map) {
      if (m_map) hwloc_bitmap_free(m_map);
      m_map = 0;
      return *this;
    }
    if (!m_map) m_map = hwloc_bitmap_alloc();
    hwloc_bitmap_copy(m_map, b.m_map);
    return *this;
  }
  ZuInline ZmBitmap &operator =(ZmBitmap &&b) {
    m_map = b.m_map;
    b.m_map = 0;
    return *this;
  }
  inline ~ZmBitmap() { if (m_map) hwloc_bitmap_free(m_map); }

private:
  ZuInline void lazy() const {
    if (ZuUnlikely(!m_map))
      const_cast<ZmBitmap *>(this)->m_map = hwloc_bitmap_alloc();
  }

public:
  inline bool operator ==(const ZmBitmap &b) const {
    if (this == &b || m_map == b.m_map) return true;
    if (!m_map || !b.m_map) return false;
    return hwloc_bitmap_isequal(m_map, b.m_map);
  }

  inline bool operator !=(const ZmBitmap &b) { return !operator ==(b); }
  
  inline int cmp(const ZmBitmap &b) const {
    if (this == &b || m_map == b.m_map) return 0;
    if (!m_map) return -1;
    if (!b.m_map) return 1;
    return hwloc_bitmap_compare(m_map, b.m_map);
  }

  typedef ZuPair<unsigned, unsigned> Range;

  inline ZmBitmap &set(unsigned i) {
    lazy();
    hwloc_bitmap_set(m_map, i);
    return *this;
  }
  inline ZmBitmap &clr(unsigned i) {
    lazy();
    hwloc_bitmap_clr(m_map, i);
    return *this;
  }
  template <typename T>
  inline typename ZuSame<Range, T, ZmBitmap &>::T set(const T &v) {
    lazy();
    hwloc_bitmap_set_range(m_map, v.p1(), v.p2());
    return *this;
  }
  template <typename T>
  inline typename ZuSame<Range, T, ZmBitmap &>::T clr(const T &v) {
    lazy();
    hwloc_bitmap_clr_range(m_map, v.p1(), v.p2());
    return *this;
  }

  inline bool operator &&(unsigned i) const {
    if (!m_map) return false;
    return hwloc_bitmap_isset(m_map, i);
  }
  inline bool operator &&(const ZmBitmap &b) const {
    if (!m_map) return !b.m_map;
    return hwloc_bitmap_isincluded(b.m_map, m_map);
  }
  inline bool operator ||(const ZmBitmap &b) const {
    if (!m_map) return false;
    return hwloc_bitmap_intersects(b.m_map, m_map);
  }

  inline ZmBitmap operator |(const ZmBitmap &b) const {
    lazy(); b.lazy();
    ZmBitmap r;
    hwloc_bitmap_or(r.m_map, m_map, b.m_map);
    return r;
  }
  inline ZmBitmap operator &(const ZmBitmap &b) const {
    lazy(); b.lazy();
    ZmBitmap r;
    hwloc_bitmap_and(r.m_map, m_map, b.m_map);
    return r;
  }
  inline ZmBitmap operator ^(const ZmBitmap &b) const {
    lazy(); b.lazy();
    ZmBitmap r;
    hwloc_bitmap_xor(r.m_map, m_map, b.m_map);
    return r;
  }
  inline ZmBitmap operator ~() const {
    lazy();
    ZmBitmap r;
    hwloc_bitmap_not(r.m_map, m_map);
    return r;
  }

  inline ZmBitmap &operator |=(const ZmBitmap &b) {
    lazy(); b.lazy();
    hwloc_bitmap_or(m_map, m_map, b.m_map);
    return *this;
  }
  inline ZmBitmap &operator &=(const ZmBitmap &b) {
    lazy(); b.lazy();
    hwloc_bitmap_and(m_map, m_map, b.m_map);
    return *this;
  }
  inline ZmBitmap &operator ^=(const ZmBitmap &b) {
    lazy(); b.lazy();
    hwloc_bitmap_xor(m_map, m_map, b.m_map);
    return *this;
  }

  inline ZmBitmap &zero() {
    lazy();
    hwloc_bitmap_zero(m_map);
    return *this;
  }
  inline ZmBitmap &fill() {
    lazy();
    hwloc_bitmap_fill(m_map);
    return *this;
  }

  inline bool operator !() const {
    return !m_map || hwloc_bitmap_iszero(m_map);
  }

  inline bool full() const {
    return m_map ? hwloc_bitmap_isfull(m_map) : false;
  }

  inline int first() const {
    return !m_map ? -1 : hwloc_bitmap_first(m_map);
  }
  inline int next(int i) const {
    return !m_map ? -1 : hwloc_bitmap_next(m_map, i);
  }
  inline int last() const {
    return !m_map ? -1 : hwloc_bitmap_last(m_map);
  }
  inline int count() const {
    return !m_map ? 0 : hwloc_bitmap_weight(m_map);
  }

  class Iterator {
  public:
    inline Iterator(const ZmBitmap &b) :
	m_b(b), m_i(b.count() < 0 ? -1 : b.first()) { }
    inline int iterate() {
      int i = m_i;
      if (i >= 0) m_i = m_b.next(i);
      return i;
    }
  private:
    const ZmBitmap	&m_b;
    int			m_i;
  };
  inline Iterator iterator() const { return Iterator(*this); }

  // hwloc_bitmap_t is a pointer
  inline operator hwloc_bitmap_t() {
    lazy();
    return m_map;
  }

  inline ZmBitmap(uint64_t v) :
      m_map(hwloc_bitmap_alloc()) { hwloc_bitmap_from_ulong(m_map, v); }
  inline uint64_t uint64() const {
    if (ZuLikely(!m_map)) return 0;
    return hwloc_bitmap_to_ulong(m_map);
  }
  inline ZmBitmap(uint128_t v) :
      m_map(hwloc_bitmap_alloc()) {
    hwloc_bitmap_from_ith_ulong(m_map, 0, (uint64_t)v);
    hwloc_bitmap_from_ith_ulong(m_map, 1, (uint64_t)(v >> 64U));
  }
  inline uint128_t uint128() const {
    if (ZuLikely(!m_map)) return 0;
    return (uint128_t)hwloc_bitmap_to_ith_ulong(m_map, 0) |
      ((uint128_t)hwloc_bitmap_to_ith_ulong(m_map, 1) << 64U);
  }
  template <typename S>
  inline ZmBitmap(const S &s, typename ZuIsCharString<S>::T *_ = 0) :
      m_map(hwloc_bitmap_alloc()) { scan(s); }
  template <typename S>
  inline typename ZuIsCharString<S, ZmBitmap &>::T operator =(const S &s) {
    if (m_map) hwloc_bitmap_zero(m_map);
    scan(s);
    return *this;
  }
  inline unsigned scan(ZuString s) {
    lazy();
    const char *data = s.data();
    unsigned length = s.length(), offset = 0;
    if (!length) return 0;
    ZuBox<int> begin, end;
    int j;
    while (offset < length) {
      if (data[offset] == ',') { ++offset; continue; }
      if ((j = begin.scan(data + offset, length - offset)) <= 0) break;
      offset += j;
      if (offset < length && data[offset] == '-') {
	if ((j = end.scan(data + offset + 1, length - offset - 1)) > 0)
	  offset += j + 1;
	else {
	  end = -1;
	  ++offset;
	}
      } else
	end = begin;
      hwloc_bitmap_set_range(m_map, begin, end);
    }
    return offset;
  }
  template <typename S> inline void print(S &s) const {
    if (!m_map || hwloc_bitmap_iszero(m_map)) return;
    ZmBitmap tmp = *this;
    ZuBox<int> begin = hwloc_bitmap_first(m_map);
    bool first = true;
    while (begin >= 0) {
      if (!first)
	s << ',';
      else
	first = false;
      ZuBox<int> end = begin, next;
      hwloc_bitmap_set_range(tmp.m_map, 0, begin);
      if (hwloc_bitmap_isfull(tmp.m_map)) { s << begin << '-'; return; }
      while ((next = hwloc_bitmap_next(m_map, end)) == end + 1) end = next;
      if (end == begin)
	s << begin;
      else
	s << begin << '-' << end;
      begin = next;
    }
  }

private:
  hwloc_bitmap_t	m_map;
};

// generic printing
template <> struct ZuPrint<ZmBitmap> : public ZuPrintFn { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmBitmap_HPP */
