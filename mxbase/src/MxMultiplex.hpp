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

// Mx Multiplex

#ifndef MxMultiplex_HPP
#define MxMultiplex_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <ZuPolymorph.hpp>

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>
#include <ZvMultiplexCf.hpp>

#include <MxBase.hpp>

class MxMultiplex : public ZuPolymorph, public ZiMultiplex {
public:
  typedef ZmScheduler::ID ID;

  struct IDAccessor : public ZuAccessor<MxMultiplex *, ID> {
    ZuInline static ID value(const MxMultiplex *mx) {
      return mx->params().id();
    }
  };

  template <typename ID_>
  inline MxMultiplex(const ID_ &id) :
      ZiMxParams(
	  ZiMxParams().scheduler([&](auto &s) { s.id(id); })) { }
  template <typename ID_>
  inline MxMultiplex(const ID_ &id, ZvCf *cf) :
    ZiMultiplex(ZvMxParams(cf,
	  ZiMxParams().scheduler([&](auto &s) { s.id(id); }))) { }
};

#endif /* MxMultiplex_HPP */
