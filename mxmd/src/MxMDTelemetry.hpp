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

// MxMD telemetry

#ifndef MxMDTelemetry_HPP
#define MxMDTelemetry_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <mxmd/MxMDLib.hpp>
#endif

#include <zlib/ZmRBTree.hpp>

#include <mxbase/MxBase.hpp>
#include <mxbase/MxQueue.hpp>
#include <mxbase/MxTelemetry.hpp>

class MxEngine;
class MxAnyLink;
class MxMDCore;

class MxMDAPI MxMDTelemetry : public ZmPolymorph, public MxTelemetry::Server {
  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

public:
  MxMDTelemetry() : m_time{ZmTime::Now} { }

  void init(MxMDCore *core, ZvCf *cf);
  void final();

  void run(MxTelemetry::Server::Cxn *);

  void addEngine(MxEngine *);
  void addQueue(unsigned type, MxID, QueueFn);
  void delQueue(unsigned type, MxID);
  void addDBEnv(ZdbEnv *);

private:
  typedef ZmRBTree<MxID,
	    ZmRBTreeVal<ZmRef<MxEngine>,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > > Engines;

  typedef ZmRBTree<ZuPair<MxID, bool>,
	    ZmRBTreeVal<ZmRef<MxQueue>,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > > Queues;

  MxMDCore	*m_core = 0;
  Lock		m_lock;
    Engines	  m_engines;
    Queues	  m_queues;
    ZuRef<ZdbEnv> m_dbEnv = nullptr;
  ZmTime	m_time;
};

#endif /* MxMDTelemetry_HPP */
