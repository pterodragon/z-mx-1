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

#include <ZmHashMgr.hpp>

#include <ZuStringN.hpp>

#include <ZmSingleton.hpp>

template <>
struct ZmCleanup<ZmHashMgr_> {
  enum { Level = ZmCleanupLevel::Library };
};

class ZmHashMgr_ : public ZmObject {
friend struct ZmSingletonCtor<ZmHashMgr_>;
friend class ZmHashMgr;

  struct HeapID {
    inline static const char *id() { return "ZmHashMgr_"; }
  };

  typedef ZmRBTree<ZmIDString,
	    ZmRBTreeVal<ZmHashParams,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeHeapID<HeapID,
		  ZmRBTreeLock<ZmNoLock> > > > > ID2Params;

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

  void add(ZmAnyHash *tbl) {
    ZmGuard<ZmPLock> guard(m_lock);
    m_tables.add(tbl);
    // deref, otherwise m_tables.add() prevents dtor from ever being called
    tbl->deref_();
  }

  void del(ZmAnyHash *tbl) {
    ZmGuard<ZmPLock> guard(m_lock);
    // double ref prevents m_tables.del() from recursing into dtor
    tbl->ref2_();
    m_tables.del(ZmAnyHash_PtrAccessor::value(*tbl));
    ZmAssert(tbl->deref_()); // check refCount is returned to 0
  }

  typedef ZmHashMgr_Tables Tables;

  void all(ZmFn<ZmAnyHash *> fn) {
    ZmRef<ZmAnyHash> tbl;
    {
      ZmGuard<ZmPLock> guard(m_lock);
      // for (;;) {
	tbl = m_tables.minimum();
	// if (!tbl || tbl->refCount() > 2) break;
	// m_tables.del(tbl);
      // }
    }
    while (tbl) {
      fn(tbl);
      {
	ZmGuard<ZmPLock> guard(m_lock);
	// for (;;) {
	  tbl = m_tables.readIterator<ZmRBTreeGreater>(
	      ZmAnyHash_PtrAccessor::value(*tbl)).iterate();
	  // if (!tbl || tbl->refCount() > 2) break;
	  // m_tables.del(tbl);
	// }
      }
    }
  }

  ZmPLock	m_lock;
  ID2Params	m_params;
  Tables	m_tables;
};

void ZmHashMgr::init(ZuString id, const ZmHashParams &params)
{
  ZmHashMgr_::instance()->init(id, params);
}

void ZmHashMgr::all(ZmFn<ZmAnyHash *> fn)
{
  ZmHashMgr_::instance()->all(fn);
}

ZmHashParams &ZmHashMgr::params(ZuString id, ZmHashParams &in)
{
  return ZmHashMgr_::instance()->params(id, in);
}

void ZmHashMgr::add(ZmAnyHash *tbl)
{
  ZmHashMgr_::instance()->add(tbl);
}

void ZmHashMgr::del(ZmAnyHash *tbl)
{
  ZmHashMgr_::instance()->del(tbl);
}
