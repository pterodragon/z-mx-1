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

// shard

#ifndef ZmShard_HPP
#define ZmShard_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

template <class Mgr_> class ZmShard {
public:
  typedef Mgr_ Mgr;

  ZuInline ZmShard(Mgr *mgr, unsigned tid) : m_mgr(mgr), m_tid(tid) { }

  ZuInline Mgr *mgr() const { return m_mgr; }
  ZuInline unsigned tid() const { return m_tid; }

  template <typename ...Args> ZuInline void run(Args &&... args) const
    { m_mgr->run(m_tid, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void invoke(Args &&... args) const
    { m_mgr->invoke(m_tid, ZuFwd<Args>(args)...); }

private:
  Mgr		*m_mgr;
  unsigned	m_tid;
};

template <class Shard_> class ZmSharded {
public:
  typedef Shard_ Shard;

  ZuInline ZmSharded(Shard *shard) : m_shard(shard) { }

  ZuInline Shard *shard() const { return m_shard; }

private:
  Shard		*m_shard;
};

template <typename T> class ZmHandle {
public:
  typedef typename T::Shard Shard;

  ZuInline ZmHandle() { }
  ZuInline ZmHandle(Shard *shard) : m_shard(shard) { }
  ZuInline ZmHandle(T *o) : m_shard(o->shard()), m_object(o) { }

  ZuInline ZmHandle(const ZmHandle &h) :
    m_shard(h.m_shard), m_object(h.m_object) { }
  ZuInline ZmHandle(ZmHandle &&h) :
    m_shard(h.m_shard), m_object(ZuMv(h.m_object)) { }
  ZuInline ZmHandle &operator =(const ZmHandle &h) {
    if (ZuUnlikely(this == &h)) return *this;
    m_shard = h.m_shard;
    m_object = h.m_object;
    return *this;
  }
  ZuInline ZmHandle &operator =(ZmHandle &&h) {
    m_shard = h.m_shard;
    m_object = ZuMv(h.m_object);
    return *this;
  }

  ZuInline operator bool() const { return m_object; }

  ZuInline int shardID() const { return m_shard ? (int)m_shard->id() : -1; }

  ZuInline ZmRef<T> &object(Shard *shard) {
    if (ZuLikely(shard == m_shard)) return object;
    static ZmRef<T> null;
    return null;
  }

  template <typename L>
  ZuInline void invokeMv(L l) {
    m_shard->invoke(
	[l = ZuMv(l), shard = m_shard, o = ZuMv(m_object)]() mutable {
      l(shard, ZuMv(o));
    });
  }
  template <typename L>
  ZuInline typename ZuNotMutable<L>::T invoke(L l) const {
    m_shard->invoke(
	[l = ZuMv(l), shard = m_shard, o = m_object.ptr()]() {
      l(shard, o);
    });
  }
  template <typename L>
  ZuInline typename ZuIsMutable<L>::T invoke(L l) const {
    m_shard->invoke(
	[l = ZuMv(l), shard = m_shard, o = m_object.ptr()]() mutable {
      l(shard, o);
    });
  }

private:
  Shard		*m_shard = 0;
  ZmRef<T>	m_object;
};

#endif /* ZmShard_HPP */
