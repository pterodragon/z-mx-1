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

  template <typename ...Args> ZuInline void run(Args &&... args) const
    { m_shard->run(ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void invoke(Args &&... args) const
    { m_shard->invoke(ZuFwd<Args>(args)...); }

private:
  Shard		*m_shard;
};

#endif /* ZmShard_HPP */
