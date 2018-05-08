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

// hash table configuration

#ifndef ZvHashMgrCf_HPP
#define ZvHashMgrCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuStringN.hpp>

#include <ZmHeap.hpp>
#include <ZmHash.hpp>

#include <ZvCf.hpp>
#include <ZvCSV.hpp>

struct ZvHashMgrCf {
  typedef ZuStringN<ZmHeapIDSize> ID;
  struct Data {
    ID			id;
    ZuBox<unsigned>	bits;
    ZuBox<double>	loadFactor;
    ZuBox<unsigned>	cBits;
  };

  typedef ZvCSVColumn<ZvCSVColType::String, ID> IDCol;
  typedef ZvCSVColumn<ZvCSVColType::Int, ZuBox<unsigned> > UIntCol;
  typedef ZvCSVColumn<ZvCSVColType::Float, ZuBox<double> > DblCol;

  struct CSV : public ZvCSV {
  public:
    typedef ZuPOD<Data> POD;

    CSV() : m_pod(new POD()) {
      new (m_pod->ptr()) Data();
      add(new IDCol("id", offsetof(Data, id)));
      add(new UIntCol("bits", offsetof(Data, bits)));
      add(new DblCol("loadFactor", offsetof(Data, loadFactor)));
      add(new UIntCol("cBits", offsetof(Data, cBits)));
    }

    void alloc(ZuRef<ZuAnyPOD> &pod) { pod = m_pod; }

    template <typename File>
    void read(const File &file) {
      ZvCSV::read(file,
	  ZvCSVAllocFn::Member<&CSV::alloc>::fn(this),
	  ZvCSVReadFn::Member<&CSV::row>::fn(this));
    }

    void row(ZuAnyPOD *pod) {
      const Data *data = (const Data *)(pod->ptr());
      ZmHashMgr::init(data->id, ZmHashParams().
	  bits(data->bits).loadFactor(data->loadFactor).cBits(data->cBits));
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
      while (ZmRef<ZvCf> hashCf = i.subset(id))
	ZmHashMgr::init(id, ZvHashParams(hashCf));
    }
  }
};

#endif /* ZvHashMgrCf_HPP */
