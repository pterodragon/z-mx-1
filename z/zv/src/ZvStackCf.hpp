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

// ZmStack configuration

#ifndef ZvStackCf_HPP
#define ZvStackCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZmStack.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvCSV.hpp>

struct ZvStackParams : public ZmStackParams {
  inline ZvStackParams() : ZmStackParams() { }

  inline ZvStackParams(const ZmStackParams &p) : ZmStackParams(p) { }
  inline ZvStackParams &operator =(const ZmStackParams &p) {
    ZmStackParams::operator =(p);
    return *this;
  }

  inline ZvStackParams(ZvCf *cf) : ZmStackParams() { init(cf); }
  inline ZvStackParams(ZvCf *cf, const ZmStackParams &deflt) :
      ZmStackParams(deflt) { init(cf); }

  inline void init(ZvCf *cf) {
    ZmStackParams::operator =(ZmStackParams());

    if (!cf) return;

    initial(cf->getInt("initial", 2, 28, false, initial()));
    increment(cf->getInt("increment", 0, 12, false, increment()));
    maxFrag(cf->getDbl("maxFrag", 1, 256, false, maxFrag()));
  }
};

#endif /* ZvStackCf_HPP */
