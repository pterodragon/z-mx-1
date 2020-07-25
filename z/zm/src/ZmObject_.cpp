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

// object base class

#include <zlib/ZmObject.hpp>

#ifdef ZmObject_DEBUG
#include <zlib/ZuNew.hpp>

#include <zlib/ZmBackTrace.hpp>
#include <zlib/ZmRBTree.hpp>
#include <zlib/ZmPLock.hpp>

using DebugTree =
  ZmRBTree<const void *,
    ZmRBTreeVal<const ZmBackTrace *,
      ZmRBTreeUnique<true,
	ZmRBTreeLock<ZmPLock> > > >;

void ZmObject_Debug::debug() const
{
  if (!m_debug) {
    void *n = ::malloc(sizeof(DebugTree));
    new (n) DebugTree();
    if (m_debug.cmpXch(n, 0)) ::free(n);
  }
}

void ZmObject_Debug::dump(void *context, DumpFn fn) const
{
  if (!m_debug) return;
  auto i = ((DebugTree *)(void *)m_debug)->readIterator();
  DebugTree::NodeRef n;
  while (n = i.iterate()) (*fn)(context, n->key(), n->val());
}

void ZmObject_ref(const ZmObject_Debug *o, const void *referrer)
{
  ZmBackTrace *bt = new ZmBackTrace();
  bt->capture(1);
  ((DebugTree *)(void *)o->m_debug)->add(referrer, bt);
}

void ZmObject_deref(const ZmObject_Debug *o, const void *referrer)
{
  DebugTree::NodeRef n = ((DebugTree *)(void *)o->m_debug)->del(referrer);
  if (n) delete n->val();
}

#else

void ZmObject_ref(const ZmObject_Debug *, const void *) { }
void ZmObject_deref(const ZmObject_Debug *, const void *) { }

#endif
