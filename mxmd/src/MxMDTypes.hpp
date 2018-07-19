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

// MxMD library

#ifndef MxMDTypes_HPP
#define MxMDTypes_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#ifndef MxMDNLegs
#define MxMDNLegs 4	// up to MxNLegs legs per order
#endif
#ifndef MxMDNSessions
#define MxMDNSessions 3	// no market has >3 continuous trading sessions per day
#endif

struct MxMDSegment { // venue segment
  MxID		id;
  MxEnum	session;	// MxTradingSession
  MxDateTime	stamp;		// session start time stamp

  ZuInline bool operator !() const { return !id; }
  ZuOpBool
};

typedef ZuTuple<MxPx, MxPx, MxPx> MxMDTickSize_;
template <typename T> struct MxMDTickSize_Fn : public T {
  Mx_TupleField(MxFixed, minPrice, 1);
  Mx_TupleField(MxFixed, maxPrice, 2);
  Mx_TupleField(MxFixed, tickSize, 3);
};
typedef ZuMixin<MxMDTickSize_, MxMDTickSize_Fn> MxMDTickSize;
struct MxMDTickSize_MinPxAccessor :
    public ZuAccessor<MxMDTickSize, MxFloat> {
  inline static MxPx value(const MxMDTickSize &t) { return t.minPrice(); }
};

struct MxMDSecRefData {	// security reference data ("static data")
  MxBool	tradeable;	// usually true, is false e.g. for an index
  MxEnum	idSrc;		// symbol ID source
  MxEnum	altIDSrc;	// altSymbol ID source
  MxEnum	putCall;	// put/call (null if not option)
  MxSymString	symbol;		// symbol
  MxSymString	altSymbol;	// alternative symbol
  MxID		underVenue;	// underlying venue (null if no underlying)
  MxID		underSegment;	// underlying segment (can be null)
  MxIDString	underlying;	// underlying ID (null if no underlying)
  MxUInt	mat;		// maturity (null if not future/option)
  MxNDP		pxNDP;		// price NDP
  MxNDP		qtyNDP;		// qty NDP
  MxFixed	strike;		// strike (null if not option)
  MxUInt	outstandingShares; // (null if not stock)
  MxFixed	adv;		// average daily volume
};

struct MxMDLotSizes {
  MxFixed	oddLotSize;
  MxFixed	lotSize;
  MxFixed	blockLotSize;

  ZuInline bool operator !() const { return !*lotSize; }
  ZuOpBool
};

struct MxMDL1Data {
  MxDateTime	stamp;
  MxEnum	status;			// MxTradingStatus
  MxEnum	tickDir;		// MxTickDir
  // Note: all px/qty are integers scaled by 10^ndp
  MxFixed	base;			// aka adjusted previous day's close
  MxFixed	open[MxMDNSessions];	// [0] is open of first session
  MxFixed	close[MxMDNSessions];	// [0] is close of first session
  MxFixed	last;
  MxFixed	lastQty;
  MxFixed	bid;			// best bid
  MxFixed	bidQty;
  MxFixed	ask;			// best ask
  MxFixed	askQty;
  MxFixed	high;
  MxFixed	low;
  MxFloat	accVol;
  MxFixed	accVolQty;	// VWAP = accVol / accVolQty
  MxFixed	match;		// auction - indicative match/IAP/equilibrium
  MxFixed	matchQty;	// auction - indicative match volume
  MxFixed	surplusQty;	// auction - surplus volume
  MxFlags	flags;
};

#endif /* MxMDTypes_HPP */
