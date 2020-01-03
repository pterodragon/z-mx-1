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

// hash table configuration management & telemetry

#ifndef ZmHashMgr_HPP
#define ZmHashMgr_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuPrint.hpp>

#include <zlib/ZmFn.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmRBTree.hpp>

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

// display sequence:
//   id, addr, linear, bits, cBits, loadFactor, nodeSize,
//   count, effLoadFactor, resized
// derived display fields:
//   slots = 1<<bits
//   locks = 1<<cBits
struct ZmHashTelemetry {
  ZmIDString	id;		// primary key
  uintptr_t	addr;		// primary key
  double	loadFactor;	// (double)N / 16.0
  double	effLoadFactor;	// graphable (*)
  uint32_t	nodeSize;
  uint32_t	count;		// graphable (*)
  uint32_t	resized;	// dynamic
  uint8_t	bits;
  uint8_t	cBits;
  uint8_t	linear;
};

class ZmAPI ZmAnyHash_ : public ZmPolymorph {
public:
  virtual void telemetry(ZmHashTelemetry &) const { }
};
struct ZmAnyHash_PtrAccessor : public ZuAccessor<ZmAnyHash_, uintptr_t> {
  ZuInline static uintptr_t value(const ZmAnyHash_ &h) {
    return (uintptr_t)(void *)&h;
  }
};
struct ZmHashMgr_HeapID {
  inline static const char *id() { return "ZmHashMgr_"; }
};
typedef ZmRBTree<ZmAnyHash_,
	  ZmRBTreeNodeIsKey<true,
	    ZmRBTreeObject<ZmPolymorph,
	      ZmRBTreeHeapID<ZuNull,
		ZmRBTreeIndex<ZmAnyHash_PtrAccessor,
		  ZmRBTreeHeapID<ZmHashMgr_HeapID,
		    ZmRBTreeLock<ZmNoLock> > > > > > > ZmHashMgr_Tables;
typedef ZmHashMgr_Tables::Node ZmAnyHash;

class ZmHashMgr_;
class ZmAPI ZmHashMgr {
friend class ZmHashMgr_;
friend class ZmHashParams;
template <typename, class> friend class ZmHash; 
template <class, typename, class, unsigned> friend class ZmLHash_;

  template <class S> struct CSV_ {
    CSV_(S &stream) : m_stream(stream) {
      m_stream <<
	"id,addr,linear,bits,cBits,loadFactor,nodeSize,"
	"count,effLoadFactor,resized\n";
    }
    void print(ZmAnyHash *tbl) {
      ZmHashTelemetry data;
      tbl->telemetry(data);
      m_stream
	<< data.id << ','
	<< ZuBoxPtr(data.addr).hex() << ','
	<< (unsigned)data.linear << ','
	<< (unsigned)data.bits << ','
	<< (unsigned)data.cBits << ','
	<< ZuBoxed(data.loadFactor) << ','
	<< data.nodeSize << ','
	<< data.count << ','
	<< ZuBoxed(data.effLoadFactor) << ','
	<< data.resized << '\n';
    }
    S &stream() { return m_stream; }

  private:
    S	&m_stream;
  };

public:
  static void init(ZuString id, const ZmHashParams &params);

  static void all(ZmFn<ZmAnyHash *> fn);

  struct CSV;
friend struct CSV;
  struct CSV {
    template <typename S> ZuInline void print(S &s) const {
      ZmHashMgr::CSV_<S> csv(s);
      ZmHashMgr::all(
	  ZmFn<ZmAnyHash *>::Member<&ZmHashMgr::CSV_<S>::print>::fn(&csv));
    }
  };
  static CSV csv() { return CSV(); }

private:
  static ZmHashParams &params(ZuString id, ZmHashParams &in);

public:
  static void add(ZmAnyHash *);
  static void del(ZmAnyHash *);
};

template <> struct ZuPrint<ZmHashMgr::CSV> : public ZuPrintFn { };

inline const ZmHashParams &ZmHashParams::init(ZuString id)
{
  return ZmHashMgr::params(id, *this);
}

#endif /* ZmHashMgr_HPP */
