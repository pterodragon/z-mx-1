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

// balance (deposited assets, loaned assets, traded/confirmed assets)
struct MxBalance {
  MxValue	deposited;	// deposited
  MxValue	loanAvail;	// loan available
  MxValue	loaned;		// loan used (should be <= loanAvail)
  MxValue	confirmed;	// confirmed / settled
  MxValue	traded;		// traded / realized
};

// exposure (open orders, unexpired derivatives, margined positions)
struct MxExpSide {
  MxValue	open;		// total qty of open/working orders
  MxValue	futures;	// total qty of futures (on this underlying)
  MxValue	options;	// total qty of options (on this underlying)
  MxValue	margin;		// total margin (= open for cash trading)
};

// can be aggregated across instruments/venues
struct MxExpAsset {
  MxExposure	longExp;
  MxExposure	shortExp;
};

// exposure for an instrument - i.e. an asset pair
struct MxExposure {
  MxExpAsset	base;
  MxExpAsset	quote;
};

#endif /* MxPosition_HPP */
