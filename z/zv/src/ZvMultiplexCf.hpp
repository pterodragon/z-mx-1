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

// socket I/O multiplexer configuration

#ifndef ZvMultiplexCf_HPP
#define ZvMultiplexCf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>
#include <ZvSchedulerCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

struct ZvMxParams : public ZiMxParams {
  inline ZvMxParams() : ZiMxParams() { }

  inline ZvMxParams(const ZiMxParams &p) :
    ZiMxParams(p) { }
  inline ZvMxParams &operator =(const ZiMxParams &p) {
    ZiMxParams::operator =(p);
    return *this;
  }

  inline ZvMxParams(ZvCf *cf) { init(cf); }

  inline ZvMxParams(ZvCf *cf, const ZiMxParams &deflt) :
    ZiMxParams(deflt) { init(cf); }

  inline ZvSchedParams &scheduler() {
    return static_cast<ZvSchedParams &>(
	ZiMxParams::scheduler());
  }
  inline const ZvSchedParams &scheduler() const {
    return static_cast<const ZvSchedParams &>(
	ZiMxParams::scheduler());
  }

  inline void init(ZvCf *cf) {
    if (!cf) return;

    scheduler().init(cf);
    if (const auto &name = cf->get("rxThread")) rxThread(scheduler().tid(name));
    if (const auto &name = cf->get("txThread")) txThread(scheduler().tid(name));
#ifdef ZiMultiplex_EPoll
    epollMaxFDs(cf->getInt("epollMaxFDs", 1, 100000, false, epollMaxFDs()));
    epollQuantum(cf->getInt("epollQuantum", 1, 1024, false, epollQuantum()));
#endif
    rxBufSize(cf->getInt("rcvBufSize", 0, INT_MAX, false, rxBufSize()));
    txBufSize(cf->getInt("sndBufSize", 0, INT_MAX, false, txBufSize()));
#ifdef ZiMultiplex_DEBUG
    trace(cf->getInt("trace", 0, 1, false, trace()));
    debug(cf->getInt("debug", 0, 1, false, debug()));
    frag(cf->getInt("frag", 0, 1, false, frag()));
    yield(cf->getInt("yield", 0, 1, false, yield()));
#endif
  }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvMultiplexCf_HPP */
