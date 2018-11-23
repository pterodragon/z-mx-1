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

// MxBase position keeping

#ifndef MxPosition_HPP
#define MxPosition_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <MxBase.hpp>

struct MxBalance {
  MxValue	deposited;	// deposited
  MxValue	confirmed;	// confirmed / settled
  MxValue	traded;		// traded / realized
};

struct MxAssetBal {
  MxBalance	longBal;	// long sells reduce longBal
  MxBalance	shortBal;	// short sold balance
  MxValue	shortAvail;	// short availability (>= shortBal.tradedQty)
};

struct MxExposure {
  MxValue	open;		// total qty of open/working orders
  MxValue	futures;	// total qty of futures (on this underlying)
  MxValue	options;	// total qty of options (on this underlying)
  MxValue	margin;		// total margin (= limit + mkt for cash trading)
};

// can be aggregated across pairs/venues
struct MxAssetPos : public MxAssetBal {
  MxExposure	longExp;
  MxExposure	shortExp;
};

struct MxPosition {
  MxAssetPos	basePos;
  MxAssetPos	quotePos;
};

#endif /* MxPosition_HPP */
