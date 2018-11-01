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

#include <MxBase.hpp>

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

typedef ZuTuple<MxValue, MxValue, MxValue> MxMDTickSize_;
template <typename T> struct MxMDTickSize_Fn : public T {
  Mx_TupleField(MxValue, minPrice, 1);
  Mx_TupleField(MxValue, maxPrice, 2);
  Mx_TupleField(MxValue, tickSize, 3);
};
typedef ZuMixin<MxMDTickSize_, MxMDTickSize_Fn> MxMDTickSize;
struct MxMDTickSize_MinPxAccessor :
    public ZuAccessor<MxMDTickSize, MxFloat> {
  inline static MxValue value(const MxMDTickSize &t) { return t.minPrice(); }
};

#pragma pack(push, 4)

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
  uint16_t	pad_1 = 0;
  MxValue	strike;		// strike (null if not option)
  MxUInt	outstandingShares; // (null if not stock)
  MxValue	adv;		// average daily volume
};

struct MxMDLotSizes {
  MxValue	oddLotSize;
  MxValue	lotSize;
  MxValue	blockLotSize;

  ZuInline bool operator !() const { return !*lotSize; }
  ZuOpBool
};

struct MxMDL1Data {
  MxDateTime	stamp;
  MxNDP		pxNDP;			// price NDP
  MxNDP		qtyNDP;			// qty NDP
  MxEnum	status;			// MxTradingStatus
  MxEnum	tickDir;		// MxTickDir
  // Note: all px/qty are integers scaled by 10^ndp
  MxValue	base;			// aka adjusted previous day's close
  MxValue	open[MxMDNSessions];	// [0] is open of first session
  MxValue	close[MxMDNSessions];	// [0] is close of first session
  MxValue	last;
  MxValue	lastQty;
  MxValue	bid;			// best bid
  MxValue	bidQty;
  MxValue	ask;			// best ask
  MxValue	askQty;
  MxValue	high;
  MxValue	low;
  MxValue	accVol;
  MxValue	accVolQty;	// VWAP = accVol / accVolQty
  MxValue	match;		// auction - indicative match/IAP/equilibrium
  MxValue	matchQty;	// auction - indicative match volume
  MxValue	surplusQty;	// auction - surplus volume
  MxFlags	flags;
};

typedef MxString<12> MxMDFlagsStr;

#pragma pack(pop)

#endif /* MxMDTypes_HPP */
