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

// ring buffer configuration

#ifndef ZvRingCf_HPP
#define ZvRingCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZiRing.hpp>

#include <ZvCf.hpp>

struct ZvRingParams : public ZiRingParams {
  inline ZvRingParams() { }

  inline ZvRingParams(const ZiRingParams &p) : ZiRingParams(p) { }
  inline ZvRingParams &operator =(const ZiRingParams &p) {
    ZiRingParams::operator =(p);
    return *this;
  }

  inline ZvRingParams(ZvCf *cf) { init(cf); }
  inline ZvRingParams(ZvCf *cf, const ZiRingParams &deflt) :
      ZiRingParams(deflt) { init(cf); }

  inline void init(ZvCf *cf) {
    if (!cf) return;
    name(cf->get("name", true));
    size(cf->getInt("size", 8192, (1U<<30U), false, 131072));
    ll(cf->getInt("ll", 0, 1, false, 0));
    spin(cf->getInt("spin", 0, INT_MAX, false, 1000));
    timeout(cf->getInt("timeout", 0, 3600, false, 1));
    killWait(cf->getInt("killWait", 0, 3600, false, 1));
    coredump(cf->getInt("coredump", 0, 1, false, 0));
  }
};

#endif /* ZvRingCf_HPP */
