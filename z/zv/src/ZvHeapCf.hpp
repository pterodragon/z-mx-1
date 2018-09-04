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

// heap configuration

#ifndef ZvHeapCf_HPP
#define ZvHeapCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuStringN.hpp>

#include <ZmHeap.hpp>
#include <ZuPOD.hpp>

#include <ZvCf.hpp>
#include <ZvCSV.hpp>

struct ZvHeapMgrCf {
  typedef ZuStringN<ZmHeapIDSize> ID;
  struct Data {
    ID			id;
    ZuBox<unsigned>	alignment;
    ZuBox<unsigned>	partition;
    ZuBox<uint64_t>	cacheSize;
    ZmBitmap		cpuset;
  };

  typedef ZvCSVColumn<ZvCSVColType::String, ID> IDCol;
  typedef ZvCSVColumn<ZvCSVColType::Int, ZuBox<unsigned> > UIntCol;
  typedef ZvCSVColumn<ZvCSVColType::Int, ZuBox<uint64_t> > UInt64Col;
  class BitmapCol : public ZvCSVColumn<ZvCSVColType::Func, ZmBitmap> {
    typedef ZvCSVColumn<ZvCSVColType::Func, ZmBitmap> Base;
    typedef Base::ParseFn ParseFn;
    typedef Base::PlaceFn PlaceFn;
  public:
    template <typename ID>
    inline BitmapCol(const ID &id, int offset) :
	Base(id, offset,
	     ParseFn::Ptr<&BitmapCol::parse_>::fn(),
	     PlaceFn::Ptr<&BitmapCol::place_>::fn()) { }
    static void parse_(ZmBitmap *i, ZuString b) { *i = ZmBitmap(b); }
    static void place_(ZtArray<char> &b, const ZmBitmap *i) { b << *i; }
  };

  struct CSV : public ZvCSV {
  public:
    typedef ZuPOD<Data> POD;

    CSV() : m_pod(new POD()) {
      new (m_pod->ptr()) Data();
      add(new IDCol("id", offsetof(Data, id)));
      add(new UIntCol("alignment", offsetof(Data, alignment)));
      add(new UIntCol("partition", offsetof(Data, partition)));
      add(new UInt64Col("cacheSize", offsetof(Data, cacheSize)));
      add(new BitmapCol("cpuset", offsetof(Data, cpuset)));
    }

    template <typename File>
    void read(const File &file) {
      ZvCSV::readFile(file,
	  ZvCSVAllocFn::Member<&CSV::alloc>::fn(this),
	  ZvCSVReadFn::Member<&CSV::row>::fn(this));
    }

    void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

    void row(ZuAnyPOD *pod) {
      const Data *data = (const Data *)(pod->ptr());
      ZmHeapMgr::init(data->id, data->partition,
	  ZmHeapConfig{data->alignment, data->cacheSize, data->cpuset});
    }

  private:
    ZuRef<POD>	m_pod;
  };

  inline static void init(ZvCf *cf) {
    if (!cf) return;

    if (ZuString file = cf->get("file")) CSV().read(file);

    {
      ZvCf::Iterator i(cf);
      ZuString id;
      while (ZmRef<ZvCf> heapCf = i.subset(id)) {
	ZuString partition;
	ZvCf::Iterator j(heapCf);
	while (ZmRef<ZvCf> partitionCf = j.subset(partition))
	  ZmHeapMgr::init(
	      id, ZuBox<unsigned>(partition),
	      ZmHeapConfig{
		(unsigned)partitionCf->getInt(
		    "alignment", 0, ZmPlatform::CacheLineSize, false, 0),
		(uint64_t)partitionCf->getInt64(
		    "cacheSize", 0, 2<<24, false, 0),
		partitionCf->get("cpuset")});
      }
    }
  }
};

#endif /* ZvHeapCf_HPP */
