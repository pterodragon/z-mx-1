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

// MxT Vocabulary Types

#ifndef MxTTypes_HPP
#define MxTTypes_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxTLib_HPP
#include <MxTLib.hpp>
#endif

#include <ZuPair.hpp>

#include <MxBase.hpp>

namespace MxTMktNoticeType {
  MxEnumValues(
    DFD,
    MktSuspend,
    MktRelease);
  MxEnumNames(
    "DFD",
    "MktSuspend",
    "MktRelease");
}

namespace MxOrdType {
  MxEnumValues(
      Market,
      Limit,
      Stop,
      StopLimit,
      Funari,		// limit to market on close (closing auction)
      MIT,		// market if touched
      Mkt2Limit,	// market during auction, unfilled becomes limit at AP
      Pegged,
      BestLimit,
      LIT,		// limit if touched
      BLIT);		// best limit if touched
  MxEnumNames(
      "Market", "Limit", "Stop", "StopLimit",
      "Funari", "MIT", "Mkt2Limit", "Pegged", "BestLimit", "LIT", "BLIT");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", Market,
      "2", Limit,
      "3", Stop,
      "4", StopLimit,
      "I", Funari,
      "J", MIT,
      "K", Mkt2Limit,
      "P", Pegged,
      "U", BestLimit,
      "W", LIT,
      "X", BLIT);

  inline bool isLimit(int ordType) {
    if (ZuUnlikely(ordType < 0 || ordType >= N)) return 0;
    static bool isLimit_[N] = { 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1 };
    return isLimit_[ordType];
  }

  inline bool isMkt(int ordType) { return !isLimit(ordType); }
}

namespace MxTimeInForce {
  MxEnumValues(Normal, IOC, FOK, AtOpen, AtClose, GTC, GTD);
  MxEnumNames(
      "Normal", "IOC", "FOK", "AtOpen", "AtClose", "GTC", "GTD");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "0", Normal,
      "1", GTC,
      "2", AtOpen,
      "3", IOC,
      "4", FOK,
      "6", GTD,
      "7", AtClose);
}

namespace MxPegType { // peg type
  MxEnumValues(
      Last, Mid, Open, Mkt, Primary, VWAP, TrailingStop, Limit, AltMid, Short);
  MxEnumNames(
      "Last", "Mid", "Open", "Mkt", "Primary", "VWAP",
      "TrailingStop", "Limit", "AltMid", "Short");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", Last,
      "2", Mid,
      "3", Open,
      "4", Mkt,
      "5", Primary,
      "7", VWAP,
      "8", TrailingStop,
      "9", Limit,
      "A", AltMid,	// aggressive mid pegging
      "S", Short);	// short-sell limit pegging (e.g. JPX/TSE)
}

namespace MxTQtyType {
  MxEnumValues(Unit, Lot);
  MxEnumNames("Unit", "Lot");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "0", Unit,
      "1", Lot);
}

namespace MxOrderCapacity {
  MxEnumValues(Agency, Principal, Mixed);
  MxEnumNames("Agency", "Principal", "Mixed");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "A", Agency,
      "P", Principal,
      "M", Mixed);
}

namespace MxCashMargin {
  MxEnumValues(Cash, MarginOpen, MarginClose);
  MxEnumNames("Cash", "MarginOpen", "MarginClose");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", Cash,
      "2", MarginOpen,
      "3", MarginClose);
}

namespace MxTFillLiquidity {
  MxEnumValues(Added, Removed, RoutedOut, Auction);
  MxEnumNames("Added", "Removed", "RoutedOut", "Auction");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", Added,
      "2", Removed,
      "3", RoutedOut,
      "4", Auction);
}

namespace MxTFillCapacity {
  MxEnumValues(Agent, CrossAsAgent, CrossAsPrincipal, Principal);
  MxEnumNames("Agent", "CrossAsAgent", "CrossAsPrincipal", "Principal");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "1", Agent,
      "2", CrossAsAgent,
      "3", CrossAsPrincipal,
      "4", Principal);
}

namespace MxTPosImpact {
  MxEnumValues(
      Open,	// order opens or extends a position
      Close);	// order closes or reduces a position
  MxEnumNames("Open", "Close");
}

namespace MxTRejReason {
  MxEnumValues(
    UnknownOrder,		// unknown order
    DuplicateOrder,		// duplicate order
    BadList,			// bad list order information
    ModifyPending,		// modify pending (in response to modify)
    CancelPending,		// cancel pending (in response to cancel)
    OrderClosed,		// order closed (in response to modify/cancel)
    PriceNotRoundTick,		// price not round tick
    PriceOutOfRange,		// price out of range
    QtyNotRoundLot,		// qty not round lot
    QtyOutOfRange,		// qty out of range
    BadSide,			// bad side
    BadOrderType,		// bad order type
    BadTimeInForce,		// bad time in force
    BadLocate,			// bad locate
    BadOrderCapacity,		// bad order capacity
    BadCashMargin,		// bad cash margin
    BadExpireTime,		// bad expire time
    BadInstrument,		// bad instrument
    BadMarket,			// bad market (destination)
    BadQtyType,			// bad qty type
    BadNumberOfLegs,		// bad number of legs
    BadMinimumQty,		// bad minimum qty
    BadMaximumFloor,		// bad maximum floor
    BadPegType,			// bad peg type
    BadPegOffset,		// bad peg offset
    BadPegPrice,		// bad peg price
    TriggerPxNotRoundTick,	// trigger price not round tick
    TriggerPxOutOfRange,	// trigger price out of range
    BadCrossType,		// bad cross type
    BadBookingType,		// bad booking type
    BadContraBroker,		// bad contra broker
    BadClient,			// bad client
    BadAccount,			// bad account
    BadInvestorID,		// bad investor ID
    BrokerReject,		// broker-specific reject
    MarketReject,		// market-specific reject
    OSM);			// order state management
  enum { OK = Invalid };	// OK == Invalid == -1
  MxEnumNames(
    "UnknownOrder",
    "DuplicateOrder",
    "ModifyPending",
    "CancelPending",
    "OrderClosed",
    "PriceNotRoundTick",
    "PriceOutOfRange",
    "QtyNotRoundLot",
    "QtyOutOfRange",
    "BadSide",
    "BadOrderType",
    "BadTimeInForce",
    "BadLocate",
    "BadOrderCapacity",
    "BadCashMargin",
    "BadExpireTime",
    "BadInstrument",
    "BadMarket",
    "BadQtyType",
    "BadNumberOfLegs",
    "BadMinimumQty",
    "BadMaximumFloor",
    "BadPegType",
    "BadPegOffset",
    "BadPegPrice",
    "TriggerPxNotRoundTick",
    "TriggerPxOutOfRange",
    "BadCrossType",
    "BadBookingType",
    "BadContraBroker",
    "BadClient",
    "BadAccount",
    "BadInvestorID",
    "BrokerReject",
    "MarketReject",
    "OSM");
}

namespace MxTCrossType {
  MxEnumValues(
      OnBook,	// crossed on a listed book (on-exchange)
      SI,	// by a systematic internalizer (dark pool / crossing engine)
      Broker,	// by the broker's trading desk (not using an SI)
      MTF,	// by a multilateral trading facility
      OTC,	// buying broker crossed with selling broker "over the counter"
      Direct);	// buyer directly crossed with seller
  MxEnumNames("OnBook", "SI", "Broker", "MTF", "OTC", "Direct");
}

namespace MxBookingType {
  MxEnumValues(Normal, CFD, Swap);
  MxEnumNames("Normal", "CFD", "Swap");
  MxEnumMapAlias(Map, CSVMap);
  MxEnumMap(FixMap,
      "0", Normal,
      "1", CFD,
      "2", Swap);
}

struct MxTTimeFmt {
  struct Null { template <typename S> inline void print(S &) const { } };
  inline static ZtDateFmt::FIX<9, Null> &fmt() {
    thread_local ZtDateFmt::FIX<9, Null> fmt_;
    return fmt_;
  }
};
template <> struct ZuPrint<MxTTimeFmt::Null> : public ZuPrintFn { };

struct MxTEventIDFmt {
  template <typename S> inline void print(S &s) const {
    if (!*id) return;
    s << date.fmt(ZuFmt::Right<8>()) << ':' << id.hex(ZuFmt::Right<8>());
  }
  MxUInt64	id;
  MxUInt	date;
};
template <> struct ZuPrint<MxTEventIDFmt> : public ZuPrintFn { };

struct MxTBoolFmt {
  template <typename S> inline void print(S &s) const {
    if (!*v) return;
    s << (v ? '1' : '0');
  }
  MxBool	v;
};
template <> struct ZuPrint<MxTBoolFmt> : public ZuPrintFn { };

#endif /* MxTTypes_HPP */
