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

namespace ZvSchedulerPriorities {
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
}

struct ZvSchedParams : public ZmSchedParams {
  inline ZvSchedParams() { }

  using ZmSchedParams::ZmSchedParams;
#if 0
  inline ZvSchedParams(ZmSchedParams &&p) : ZmSchedParams(ZuMv(p)) { }
  inline ZvSchedParams &operator =(ZmSchedParams &&p) {
    ZmSchedParams::operator =(ZuMv(p));
    return *this;
  }
#endif

  inline ZvSchedParams(ZvCf *cf) { init(cf); }
  inline ZvSchedParams(ZvCf *cf, ZmSchedParams &&deflt) :
    ZmSchedParams(ZuMv(deflt)) { init(cf); }

  inline void init(ZvCf *cf) {
    if (!cf) return;

    static unsigned ncpu = ZmPlatform::getncpu();

    if (ZuString s = cf->get("id")) id(s);
    nThreads(cf->getInt("nThreads", 1, 1024, false, nThreads()));
    stackSize(cf->getInt("stackSize", 16384, 2<<20, false, stackSize()));
    priority(cf->getEnum<ZvSchedulerPriorities::Map>(
	  "priority", false, ZmThreadPriority::Normal));
    partition(cf->getInt("partition", 0, ncpu - 1, false, 0));
    if (ZuString s = cf->get("quantum")) quantum((double)ZuBox<double>(s));
    queueSize(cf->getInt("queueSize", 8192, (1U<<30U), false, queueSize()));
    ll(cf->getInt("ll", 0, 1, false, ll()));
    spin(cf->getInt("spin", 0, INT_MAX, false, spin()));
    timeout(cf->getInt("timeout", 0, 3600, false, timeout()));
    if (ZmRef<ZvCf> threadsCf = cf->subset("threads", false)) {
      ZvCf::Iterator i(threadsCf);
      ZuString id;
      while (ZmRef<ZvCf> threadCf = i.subset(id)) {
	ZuBox<unsigned> tid = id;
	if (id != ZuStringN<4>{tid})
	  throw ZtString() << "bad thread ID \"" << id << '"';
	ZmSchedParams::Thread &thread = this->thread(tid);
	thread.isolated(cf->getInt("isolated", 0, 1, false, thread.isolated()));
	if (ZuString s = cf->get("name")) thread.name(s);
	thread.stackSize(
	    cf->getInt("stackSize", 0, INT_MAX, false, thread.stackSize()));
	if (ZuString s = cf->get("priority")) {
	  if (s == "RealTime") thread.priority(ZmThreadPriority::RealTime);
	  else if (s == "High") thread.priority(ZmThreadPriority::High);
	  else if (s == "Normal") thread.priority(ZmThreadPriority::Normal);
	  else if (s == "Low") thread.priority(ZmThreadPriority::Low);
	  else throw ZtString() << "bad thread priority \"" << s << '"';
	}
	thread.partition(
	    cf->getInt("partition", 0, INT_MAX, false, thread.partition()));
	if (ZuString s = cf->get("cpuset")) thread.cpuset(s);
	thread.detached(
	    cf->getInt("detached", 0, 1, false, thread.detached()));
      }
    }
  }
};

#endif /* ZvSchedulerCf_HPP */
