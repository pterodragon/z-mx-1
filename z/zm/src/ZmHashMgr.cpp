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

// hash table

#include <ZmHash.hpp>

#include <ZuStringN.hpp>

#include <ZmRBTree.hpp>
#include <ZmSingleton.hpp>

template <>
struct ZmCleanup<ZmHashMgr_> {
  enum { Level = ZmCleanupLevel::Library };
};

typedef ZuStringN<ZmHeapIDSize> IDString;

class ZmHashMgr_ : public ZmObject {
friend struct ZmSingletonCtor<ZmHashMgr_>;
friend class ZmHashMgr;

  struct HeapID {
    inline static const char *id() { return "ZmHashMgr_"; }
  };

  typedef ZmHashMgr::StatsFn StatsFn;
  typedef ZmHashMgr::ReportFn ReportFn;

  typedef ZmRBTree<IDString,
	    ZmRBTreeVal<ZmHashParams,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > ID2Params;
  typedef ZmRBTree<void *,
	    ZmRBTreeVal<ReportFn,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > Ptr2ReportFn;

  ZmHashMgr_() { }

  static ZmHashMgr_ *instance() {
    return ZmSingleton<ZmHashMgr_>::instance();
  }

  void init(ZuString id, const ZmHashParams &params) {
    ZmGuard<ZmPLock> guard(m_lock);
    if (ID2Params::Node *node = m_params.find(id))
      node->val() = params;
    else
      m_params.add(id, params);
  }
  ZmHashParams &params(ZuString id, ZmHashParams &in) {
    {
      ZmGuard<ZmPLock> guard(m_lock);
      if (ID2Params::Node *node = m_params.find(id))
	in = node->val();
    }
    return in;
  }

  void add(void *ptr, ReportFn fn) {
    ZmGuard<ZmPLock> guard(m_lock);
    m_tables.del(ptr);
    m_tables.add(ptr, fn);
  }
  void del(void *ptr) {
    ZmGuard<ZmPLock> guard(m_lock);
    m_tables.del(ptr);
  }

  void stats(StatsFn fn) {
    ZmHashStats stats;
    ZmGuard<ZmPLock> guard(m_lock);
    auto i = m_tables.readIterator();
    while (ReportFn rfn = i.iterateVal()) { rfn(stats); fn(stats); }
  }

  ZmPLock	m_lock;
  ID2Params	m_params;
  Ptr2ReportFn	m_tables;
};

void ZmHashMgr::init(ZuString id, const ZmHashParams &params)
{
  ZmHashMgr_::instance()->init(id, params);
}

void ZmHashMgr::stats(StatsFn fn)
{
  ZmHashMgr_::instance()->stats(fn);
}

ZmHashParams &ZmHashMgr::params(ZuString id, ZmHashParams &in)
{
  return ZmHashMgr_::instance()->params(id, in);
}

void ZmHashMgr::add(void *ptr, ReportFn fn)
{
  ZmHashMgr_::instance()->add(ptr, fn);
}

void ZmHashMgr::del(void *ptr)
{
  ZmHashMgr_::instance()->del(ptr);
}
