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

// scheduler configuration

#ifndef ZvSchedulerCf_HPP
#define ZvSchedulerCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZmScheduler.hpp>

#include <ZvCf.hpp>

struct ZvSchedulerPriorities {
  ZtEnumValues(
      RealTime = ZmThreadPriority::RealTime,
      High = ZmThreadPriority::High,
      Normal = ZmThreadPriority::Normal,
      Low = ZmThreadPriority::Low);
  ZtEnumMap(Map,
      "RealTime", RealTime,
      "High", High,
      "Normal", Normal,
      "Low", Low);
};

struct ZvSchedulerParams : public ZmSchedulerParams {
  inline ZvSchedulerParams() { }

  inline ZvSchedulerParams(const ZmSchedulerParams &p) :
    ZmSchedulerParams(p) { }
  inline ZvSchedulerParams &operator =(const ZmSchedulerParams &p) {
    ZmSchedulerParams::operator =(p);
    return *this;
  }

  inline ZvSchedulerParams(ZvCf *cf) { init(cf); }
  inline ZvSchedulerParams(ZvCf *cf,
      const ZmSchedulerParams &deflt) :
    ZmSchedulerParams(deflt) { init(cf); }

  inline void init(ZvCf *cf) {
    if (!cf) return;

    static unsigned ncpu = ZmPlatform::getncpu();

    id(cf->get("id"));
    nThreads(cf->getInt("nThreads", 1, 1024, false, nThreads()));
    stackSize(cf->getInt("stackSize", 16384, 2<<20, false, stackSize()));
    priority(cf->getEnum<ZvSchedulerPriorities::Map>(
	  "priority", false, ZmThreadPriority::Normal));
    partition(cf->getInt("partition", 0, ncpu - 1, false, 0));
    if (ZtZString s = cf->get("affinity")) affinity(s);
    if (ZtZString s = cf->get("isolation")) isolation(s);
    if (ZtZString s = cf->get("quantum")) quantum((double)ZuBox<double>(s));
    queueSize(cf->getInt("queueSize", 8192, (1U<<30U), false, queueSize()));
    ll(cf->getInt("ll", 0, 1, false, ll()));
    spin(cf->getInt("spin", 0, INT_MAX, false, spin()));
    timeout(cf->getInt("timeout", 0, 3600, false, timeout()));
  }
};

#endif /* ZvSchedulerCf_HPP */
