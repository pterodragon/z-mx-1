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

// hash table configuration management & statistics

#ifndef ZmHashMgr_HPP
#define ZmHashMgr_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuPrint.hpp>

#include <ZmFn.hpp>

class ZmHashParams {
public:
  inline ZmHashParams() : m_bits(8), m_loadFactor(1.0), m_cBits(5) { }
  inline ZmHashParams(ZuString id) :
      m_bits(8), m_loadFactor(1.0), m_cBits(5) {
    init(id);
  }

  const ZmHashParams &init(ZuString id);

  inline ZmHashParams &bits(unsigned v) { m_bits = v; return *this; }
  inline ZmHashParams &loadFactor(double v) { m_loadFactor = v; return *this; }
  inline ZmHashParams &cBits(unsigned v) { m_cBits = v; return *this; }

  ZuInline unsigned bits() const { return m_bits; }
  ZuInline double loadFactor() const { return m_loadFactor; }
  ZuInline unsigned cBits() const { return m_cBits; }

private:
  unsigned	m_bits;
  double	m_loadFactor;
  unsigned	m_cBits;
};

struct ZmHashStats {
  const char	*id;
  bool		linear;
  unsigned	nodeSize;
  unsigned	bits;
  unsigned	cBits;
  double	loadFactor;
  unsigned	count;
  double	effLoadFactor;
  unsigned	resized;
};

class ZmHashMgr_;
class ZmAPI ZmHashMgr {
friend class ZmHashMgr_;
friend class ZmHashParams;
template <typename, class> friend class ZmHash; 
template <typename, class> friend class ZmLHash; 

  template <class S> struct CSV_ {
    CSV_(S &stream) : m_stream(stream) {
      m_stream <<
	"id,linear,nodeSize,bits,cBits,"
	"loadFactor,count,effLoadFactor,resized\n";
    }
    void print(const ZmHashStats &stats) {
      m_stream <<
	stats.id << ',' <<
	(stats.linear ? 'Y' : 'N') << ',' <<
	stats.nodeSize << ',' <<
	stats.bits << ',' <<
	stats.cBits << ',' <<
	stats.loadFactor << ',' <<
	stats.count << ',' <<
	stats.effLoadFactor << ',' <<
	stats.resized << '\n';
    }
    S &stream() { return m_stream; }

  private:
    S	&m_stream;
  };

public:
  static void init(ZuString id, const ZmHashParams &params);

  typedef ZmFn<const ZmHashStats &> StatsFn;

  static void stats(StatsFn fn);

  struct CSV;
friend struct CSV;
  struct CSV {
    template <typename S> ZuInline void print(S &s) const {
      ZmHashMgr::CSV_<S> csv(s);
      ZmHashMgr::stats(StatsFn::Member<&ZmHashMgr::CSV_<S>::print>::fn(&csv));
    }
  };
  static CSV csv() { return CSV(); }

private:
  static ZmHashParams &params(ZuString id, ZmHashParams &in);

  typedef ZmFn<ZmHashStats &> ReportFn;
  static void add(void *ptr, ReportFn fn);
  static void del(void *ptr);
};

template <> struct ZuPrint<ZmHashMgr::CSV> : public ZuPrintFn { };

inline const ZmHashParams &ZmHashParams::init(ZuString id)
{
  return ZmHashMgr::params(id, *this);
}

#endif /* ZmHashMgr_HPP */
