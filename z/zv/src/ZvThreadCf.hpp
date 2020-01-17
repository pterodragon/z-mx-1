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

// thread configuration

#ifndef ZvThreadCf_HPP
#define ZvThreadCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZmThread.hpp>

#include <zlib/ZtEnum.hpp>

#include <zlib/ZvCf.hpp>

namespace ZvThreadPriorities {
  ZtEnumValues(
      RealTime = ZmThreadPriority::RealTime,
      High = ZmThreadPriority::High,
      Normal = ZmThreadPriority::Normal,
      Low = ZmThreadPriority::Low);
  ZtEnumMap(Map,
      "RealTime", RealTime, "High", High, "Normal", Normal, "Low", Low);
}

struct ZvThreadParams : public ZmThreadParams {
  ZvThreadParams() { }

  ZvThreadParams(const ZmThreadParams &p) : ZmThreadParams(p) { }
  ZvThreadParams &operator =(const ZmThreadParams &p) {
    ZmThreadParams::operator =(p);
    return *this;
  }

  ZvThreadParams(ZvCf *cf) { init(cf); }
  ZvThreadParams(ZvCf *cf, const ZmThreadParams &deflt) :
      ZmThreadParams(deflt) { init(cf); }

  void init(ZvCf *cf) {
    ZmThreadParams::operator =(ZmThreadParams());

    if (!cf) return;

    static unsigned ncpu = ZmPlatform::getncpu();
    stackSize(cf->getInt("stackSize", 16384, 2<<20, false, stackSize()));
    priority(cf->getEnum<ZvThreadPriorities::Map>(
	  "priority", false, ZmThreadPriority::Normal));
    partition(cf->getInt("partition", 0, ncpu - 1, false, 0));
    cpuset(cf->get("cpuset"));
  }
};

#endif /* ZvThreadCf_HPP */
