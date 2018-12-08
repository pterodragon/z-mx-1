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

// MxT Order State Management - Transactions, Events and Order State

// A transaction (*Txn) is a POD union (buffer) containing an event,
// with enough space for one of a number of different event types; it can
// be stored/retrieved/sent/rcvd as-is
//
// An event is a specific type that encapsulates an update to order state
//
// An order state is an order transaction together with (at most one)
// pending modify transaction and (at most one) pending cancel transaction

#ifndef MxTOrder_HPP
#define MxTOrder_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxTLib_HPP
#include <MxTLib.hpp>
#endif

#include <new>

#include <ZuLargest.hpp>

#include <MxBase.hpp>

#include <MxTTypes.hpp>

#ifndef MxT_NLegs
#define MxT_NLegs 1
#endif

namespace MxTEventType {
  MxEnumValues(
      NewOrder,			// new order accepted, queued to market
      OrderHeld,		// new order held (for trigger or risk review)
      OrderFiltered,		// new order filtered (locally rejected)
      Ordered,			// order acknowledged
      Reject,			// order rejected
      Modify,			// modify accepted, queued to market
      ModSimulated,		// modify accepted, simulated as cancel/replace
      ModHeld,			// modify held, cancel original order
      ModFiltered,		// modify filtered, original order left open
      ModFilteredCxl,		// modify filtered, cancel original order
      Modified,			// modify acknowledged
      ModReject,		// modify rejected, original order left open
      ModRejectCxl,		// modify rejected, cancel original order
      Cancel,			// cancel
      CxlFiltered,		// cancel filtered
      Canceled,			// cancel acknowledged
      CxlReject,		// cancel rejected
      Release,			// release from being held
      Deny,			// deny (reject by broker) following held
      Fill,			// order filled (partially or fully)
      Closed,			// order closed (expired)
      OrderSent,		// new order sent to market
      ModifySent,		// modify sent to market
      CancelSent		// cancel sent to market
  );
  MxEnumNames(
      "NewOrder", "OrderHeld", "OrderFiltered", "Ordered", "Reject",
      "Modify", "ModSimulated", "ModHeld", "ModFiltered",
      "ModFilteredCxl", "Modified", "ModReject", "ModRejectCxl",
      "Cancel", "CxlFiltered", "Canceled", "CxlReject",
      "Release", "Deny",
      "Fill", "Closed",
      "OrderSent", "ModifySent", "CancelSent");
}

namespace MxTOrderFlags { // order flags
  MxEnumValues(
      C, ModifyCxl = 0,
      M, ModifyNew = 1);
  MxEnumNames("ModifyCxl", "ModifyNew");
  MxEnumFlags(Flags,
      "C", ModifyCxl, "M", ModifyNew);

  inline bool matchC(const MxFlags &v) { return v & (1U<<C); }
  inline bool matchM(const MxFlags &v) { return v & (1U<<M); }
  inline bool matchCM(const MxFlags &v)
    { return (v & ((1U<<C) | (1U<<M))) == ((1U<<C) | (1U<<M)); }
}

// Note: Pending causes Pending New/Modify instead of Ordered/Modified
// when processing Modify-on-Queue
namespace MxTEventFlags { // event flags
  MxEnumValues(
      Unsolicited,	// unsolicited modified / canceled from market
      Synthesized,	// synthesized - not received from market
      Pending,		// synthesized and pending ordered / modified
      RiskFiltered,	// risk filtered
      RiskHeld,		// risk held
      Stopped		// stopped (held by stop)
  );
  MxEnumNames(
      "Unsolicited", "Synthesized", "Pending",
      "RiskFiltered", "RiskHeld", "Stopped");
  MxEnumFlags(Flags,
      "Unsolicited", Unsolicited,
      "Synthesized", Synthesized,
      "Pending", Pending,
      "RiskFiltered", RiskFiltered,
      "RiskHeld", RiskHeld,
      "Stopped", Stopped);
}

namespace MxTEventState { // event state
  MxEnumValues(
      U, Unset = 0,
      R, Received = 1,
      H, Held = 2,
      D, Deferred = 3, // deferred awaiting ack of pending order or modify
      Q, Queued = 4,
      S, Sent = 5,
      P, PendingFill = 6, // ack before fill
      A, Acknowledged = 7,
      X, Rejected = 8,
      C, Closed = 9);
  MxEnumNames(
      "Unset", "Received", "Held", "Deferred", "Queued", "Sent",
      "PendingFill", "Acknowledged", "Rejected", "Closed");
  MxEnumMap(CSVMap,
      "Unset", Unset,
      "Received", Received,
      "Held", Held,
      "Deferred", Deferred,
      "Queued", Queued,
      "Sent", Sent,
      "PendingFill", PendingFill,
      "Acknowledged", Acknowledged,
      "Rejected", Rejected,
      "Closed", Closed);

  // convenient short-hand pattern matches
  inline bool matchU(const MxEnum &v) { return v == U; }
  inline bool matchUAX(const MxEnum &v)
    { return v == U || v == A || v == X; }
  inline bool matchR(const MxEnum &v) { return v == R; }
  inline bool matchH(const MxEnum &v) { return v == H; }
  inline bool matchHD(const MxEnum &v) { return v == H || v == D; }
  inline bool matchHDQ(const MxEnum &v)
    { return v == H || v == D || v == Q; }
  inline bool matchHQ(const MxEnum &v) { return v == H || v == Q; }
  inline bool matchHQS(const MxEnum &v)
    { return v == H || v == Q || v == S; }
  inline bool matchHQSA(const MxEnum &v)
    { return v == H || v == Q || v == S || v == A; }
  inline bool matchHDQSP(const MxEnum &v) { return v >= H && v <= P; }
  inline bool matchHDQSPA(const MxEnum &v) { return v >= H && v <= A; }
  inline bool matchHQSAX(const MxEnum &v)
    { return v == H || v == Q || v == S || v == A || v == X; }
  inline bool matchD(const MxEnum &v) { return v == D; }
  inline bool matchDQ(const MxEnum &v) { return v == D || v == Q; }
  inline bool matchDQS(const MxEnum &v) { return v >= D && v <= S; }
  inline bool matchDQSP(const MxEnum &v) { return v >= D && v <= P; }
  inline bool matchDQSPA(const MxEnum &v) { return v >= D && v <= A; }
  inline bool matchDQSPX(const MxEnum &v)
    { return (v >= D && v <= P) || v == X; }
  inline bool matchDSP(const MxEnum &v)
    { return v == D || v == S || v == P; }
  inline bool matchDX(const MxEnum &v) { return v == D || v == X; }
  inline bool matchQ(const MxEnum &v) { return v == Q; }
  inline bool matchQS(const MxEnum &v) { return v == Q || v == S; }
  inline bool matchQSP(const MxEnum &v)
    { return v == Q || v == S || v == P; }
  inline bool matchQX(const MxEnum &v) { return v == Q || v == X; }
  inline bool matchS(const MxEnum &v) { return v == S; }
  inline bool matchSP(const MxEnum &v) { return v == S || v == P; }
  inline bool matchSPX(const MxEnum &v)
    { return v == S || v == P || v == X; }
  inline bool matchSA(const MxEnum &v) { return v == S || v == A; }
  inline bool matchP(const MxEnum &v) { return v == P; }
  inline bool matchA(const MxEnum &v) { return v == A; }
  inline bool matchAC(const MxEnum &v) { return v == A || v == C; }
  inline bool matchACX(const MxEnum &v)
    { return v == A || v == C || v == X; }
  inline bool matchAX(const MxEnum &v) { return v == A || v == X; }
  inline bool matchX(const MxEnum &v) { return v == X; }
  inline bool matchXC(const MxEnum &v) { return v == X || v == C; }
}

#pragma pack(push, 4)

template <typename AppTypes> struct MxTEventTypes {

  struct Event : public ZuPrintable {
    MxEnum		eventType;	// EventType
    MxEnum		eventState;	// EventState
    MxUInt8		eventFlags = 0;	// EventFlags
    MxUInt8		eventLeg = 0;

#define Event_Flag(Bit, Fn) \
    inline bool Fn() const { \
      return eventFlags & (1U<<MxTEventFlags::Bit); \
    } \
    inline void Fn##_set() { eventFlags |=  (1U<<MxTEventFlags::Bit); } \
    inline void Fn##_clr() { eventFlags &= ~(1U<<MxTEventFlags::Bit); }

    Event_Flag(Unsolicited, unsolicited)
    Event_Flag(Synthesized, synthesized)
    Event_Flag(Pending, pending)
    Event_Flag(RiskFiltered, riskFiltered)
    Event_Flag(RiskHeld, riskHeld)
    Event_Flag(Stopped, stopped)

    template <typename Update>
    inline void update(const Update &u) { }

    template <typename S> inline void print(S &s) const {
      s << "eventType=" << MxTEventType::name(eventType)
	<< " eventState=" << MxTEventState::name(eventState)
	<< " eventLeg=" << eventLeg
	<< " eventFlags=";
      MxTEventFlags::Flags::instance()->print(s, eventFlags);
    }
  };

  struct OrderLeg;
  struct ModifyLeg;
  struct ModifyLeg_;
  struct CancelLeg;
  struct CancelLeg_;
  struct Order;
  template <typename Leg> struct Modify_;
  template <typename Leg> struct Cancel_;

  typedef ZuBox0(uint8_t) NLegs;

  struct Legs_ { };
  template <typename Leg> struct Legs : public Legs_ {
    Leg		legs[MxT_NLegs];
#if MxT_NLegs > 1
    NLegs	nLegs_;
    inline unsigned nLegs() const { return nLegs_; }
#else
    inline unsigned nLegs() const { return 1; }
#endif

    template <typename Update>
    inline typename ZuIs<Legs_, Update>::T update(const Update &u) {
      unsigned n = nLegs();
      for (unsigned i = 0; i < n; i++) legs[i].update(u.legs[i]);
    }
    template <typename Update>
    inline typename ZuIsNot<Legs_, Update>::T update(const Update &u) { }

    // calculates worst-case exposure due to potential modification/update
    template <typename Update>
    inline typename ZuIs<Legs_, Update>::T expose(const Update &u) {
      unsigned n = nLegs();
      for (unsigned i = 0; i < n; i++) legs[i].expose(u.legs[i]);
    }

    // returns true if update u is pending on fills
    template <typename Update>
    inline typename ZuIs<Legs_, Update, bool>::T pending(const Update &u) {
      unsigned n = nLegs();
      for (unsigned i = 0; i < n; i++)
	if (legs[i].cumQty < u.legs[i].cumQty) return true;
      return false;
    }

    // returns true if order has been fully filled
    inline bool filled() {
      unsigned n = nLegs();
      for (unsigned i = 0; i < n; i++)
	if (legs[i].cumQty < legs[i].orderQty) return false;
      return true;
    }

    template <typename S> inline void print(S &s) const {
      s << "nLegs=" << nLegs() << " legs=[";
      bool first = true;
      unsigned n = nLegs();
      for (unsigned i = 0; i < n; i++) {
	if (ZuUnlikely(first)) first = false; else s << ',';
	s << ZuBoxed(i) << "={";
	legs[i].print(s);
	s << '}';
      }
      s << ']';
    }
  };

  // tags ack (Ordered/Modified/Canceled leg)
  struct AckLeg__ { };

  // holds a cumulative quantity for a single leg
  struct CanceledLeg_ {
    MxValue		cumQty;
    MxNDP		qtyNDP;
    uint8_t		pad_0[3];

    // only update cumQty from an acknowledgment
    template <typename Update>
    inline typename ZuIfT<
	ZuConversion<CanceledLeg_, Update>::Is &&
	ZuConversion<AckLeg__, Update>::Is>::T update(const Update &u) {
      cumQty.update(u.cumQty, MxValueReset);
    }
    template <typename Update>
    inline typename ZuIfT<
	!ZuConversion<CanceledLeg_, Update>::Is ||
	!ZuConversion<AckLeg__, Update>::Is>::T update(const Update &u) { }

    template <typename S> inline void print(S &s) const {
      s << "qtyNDP=" << qtyNDP << " cumQty=" << MxValNDP{cumQty, qtyNDP};
    }
  };

  struct CancelLeg_ : public CanceledLeg_ {
    MxValue		orderQty;

    using CanceledLeg_::update;
    template <typename Update>
    inline typename ZuIs<CancelLeg_, Update>::T update(const Update &u) {
      CanceledLeg_::update(u);
      orderQty.update(u.orderQty, MxValueReset);
    }

    template <typename Update> inline void expose(const Update &u) {
      if (orderQty < u.orderQty) orderQty = u.orderQty;
    }

    template <typename S> inline void print(S &s) const {
      CanceledLeg_::print(s);
      s << " orderQty=" << MxValNDP{orderQty, this->qtyNDP};
    }
  };
  struct CancelLeg : public CancelLeg_ { };

  template <typename Leg> struct Cancel_ : public Legs<Leg> {
    MxFlags		ackFlags;	// pending ack - EventFlags

    template <typename S> inline void print(S &s) const {
      Legs<Leg>::print(s);
      s << " ackFlags=";
      MxTEventFlags::Flags::instance()->print(s, ackFlags);
    }
  };

  // requests must include the data for the corresponding ack and reject
  struct Cancel :
      public AppTypes::Event, public Cancel_<typename AppTypes::CancelLeg> {
    enum { EventType = MxTEventType::Cancel };

    template <typename Update> inline void update(const Update &u) {
      AppTypes::Event::update(u);
      Cancel_<typename AppTypes::CancelLeg>::update(u);
    }
    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Cancel_<typename AppTypes::CancelLeg>::print(s);
    }
  };

  struct ModifyLeg_ : public CancelLeg_ {
    MxValue		px;		// always set; ref. px for mkt orders
    MxEnum		side;		// MxSide
    MxEnum		ordType;	// MxOrdType
    MxNDP		pxNDP;
    uint8_t		pad_0[1];

    using CancelLeg_::update;
    template <typename Update>
    inline typename ZuIs<ModifyLeg_, Update>::T update(const Update &u) {
      CancelLeg_::update(u);
      if (*u.side &&
	  (u.side == MxSide::Sell ||
	   u.side == MxSide::SellShort ||
	   u.side == MxSide::SellShortExempt) &&
	  (side == MxSide::Sell ||
	   side == MxSide::SellShort ||
	   side == MxSide::SellShortExempt))
	side = u.side;
      ordType.update(u.ordType);
      if (!*u.px) {
	if (ZuUnlikely(*u.pxNDP)) {
	  px = MxValNDP{px, pxNDP}.adjust(u.pxNDP);
	  pxNDP = u.pxNDP;
	}
      } else {
	px = u.px;
	pxNDP.update(u.pxNDP);
      }
    }

    template <typename Update> inline void expose(const Update &u) {
      CancelLeg_::expose(u);
      // shorts are riskier than sells, but other side changes are invalid
      if ((u.side == MxSide::SellShort ||
	   u.side == MxSide::SellShortExempt) &&
	  (side == MxSide::Sell ||
	   side == MxSide::SellShort ||
	   side == MxSide::SellShortExempt))
	side = u.side;
      // ordType changes do not impact exposure
      // adjust price to most exposed, depending on side
      if (side == MxSide::Buy) {
	if (px < u.px) px = u.px;
      } else {
	if (px > u.px) px = u.px;
      }
    }

    template <typename S> inline void print(S &s) const {
      s << "side=" << MxSide::name(side)
	<< " ordType=" << MxOrdType::name(ordType)
	<< " pxNDP=" << pxNDP
	<< " px=" << MxValNDP{px, pxNDP}
	<< ' ';
	CancelLeg_::print(s);
    }
  };
  struct ModifyLeg : public ModifyLeg_ { };

  struct Modify__ { };
  template <typename Leg>
  struct Modify_ : public Modify__, public Cancel_<Leg> {
    MxEnum		timeInForce;	// MxTimeInForce
    uint8_t		pad_0[3];

    using Cancel_<Leg>::update;
    template <typename Update>
    inline typename ZuIs<Modify__, Update>::T update(const Update &u) {
      Cancel_<Leg>::update(u); // takes care of legs
      timeInForce.update(u.timeInForce);
    }

    template <typename S> inline void print(S &s) const {
      Cancel_<Leg>::print(s);
      s << " timeInForce=" << MxTimeInForce::name(timeInForce);
    }
  };

  // requests must include the data for the corresponding ack and reject
  struct Modify :
      public AppTypes::Event, public Modify_<typename AppTypes::ModifyLeg> {
    enum { EventType = MxTEventType::Modify };

    template <typename Update> inline void update(const Update &u) {
      AppTypes::Event::update(u);
      Modify_<typename AppTypes::ModifyLeg>::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Modify_<typename AppTypes::ModifyLeg>::print(s);
    }
  };

  struct Ordered : public AppTypes::Event {
    enum { EventType = MxTEventType::Ordered };

    template <typename Update> inline void update(const Update &u) {
      AppTypes::Event::update(u);
    }
  };

  struct ModifiedLeg : public AckLeg__, public ModifyLeg_ { };

  struct Modified : public AppTypes::Event,
      public Modify_<typename AppTypes::ModifiedLeg> {
    enum { EventType = MxTEventType::Modified };

    template <typename Update> inline void update(const Update &u) {
      AppTypes::Event::update(u);
      Modify_<typename AppTypes::ModifiedLeg>::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Modify_<typename AppTypes::ModifiedLeg>::print(s);
    }
  };

  struct CanceledLeg : public AckLeg__, public CanceledLeg_ { };

  struct Canceled : public AppTypes::Event,
      public Cancel_<typename AppTypes::CanceledLeg> {
    enum { EventType = MxTEventType::Canceled };

    template <typename Update> inline void update(const Update &u) {
      Event::update(u);
      Cancel_<typename AppTypes::CanceledLeg>::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Cancel_<typename AppTypes::CanceledLeg>::print(s);
    }
  };

  struct OrderLeg_ : public ModifyLeg_ {
    MxValue		leavesQty;
    MxValue		grossTradeAmt;

    template <typename Update> inline void update(const Update &u) {
      ModifyLeg_::update(u);
      updateLeavesQty();
    }

    template <typename Update> inline void expose(const Update &u) {
      ModifyLeg_::expose(u);
      updateLeavesQty();
    }

    inline void updateLeavesQty() {
      leavesQty = this->orderQty > this->cumQty ?
	(this->orderQty - this->cumQty) : (MxValue)0;
    }

    template <typename S> inline void print(S &s) const {
      ModifyLeg_::print(s);
      s << " leavesQty=" << MxValNDP{leavesQty, this->qtyNDP};
    }
  };

  struct OrderLeg : public OrderLeg_ { };
  template <typename Leg> struct Order_ : public Modify_<Leg> {
    MxFlags		flags;		// MxTOrderFlags

    template <typename S> inline void print(S &s) const {
      Modify_<Leg>::print(s);
      s << " flags=";
      MxTOrderFlags::Flags::instance()->print(s, flags);
    }
  };

  // requests must include the data for the corresponding ack and reject
  struct NewOrder :
      public AppTypes::Event, public Order_<typename AppTypes::OrderLeg> {
    enum { EventType = MxTEventType::NewOrder };

    inline NewOrder &operator =(const NewOrder &v) {
      if (this != &v) memcpy((void *)this, &v, sizeof(NewOrder));
      return *this;
    }

    template <typename Update> inline void update(const Update &u) {
      Event::update(u);
      Order_<typename AppTypes::OrderLeg>::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Order_<typename AppTypes::OrderLeg>::print(s);
    }
  };

  // eventLeg will be set
  struct Fill : public AppTypes::Event {
    enum { EventType = MxTEventType::Fill };

    MxValue		lastPx;
    MxValue		lastQty;
    MxNDP		pxNDP;
    MxNDP		qtyNDP;
    uint8_t		pad_0[2];

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << " lastPx=" << MxValNDP{lastPx, pxNDP}
	<< " lastQty=" << MxValNDP{lastQty, qtyNDP};
    }
  };

  struct Closed : public AppTypes::Event {
    enum { EventType = MxTEventType::Closed };
  };

  struct AnySent : public AppTypes::Event { };

  struct OrderSent : public AppTypes::AnySent {
    enum { EventType = MxTEventType::OrderSent };
  };
  struct ModifySent : public AppTypes::AnySent {
    enum { EventType = MxTEventType::ModifySent };
  };
  struct CancelSent : public AppTypes::AnySent {
    enum { EventType = MxTEventType::CancelSent };
  };

  // generic reject data for new order / modify / cancel
  struct AnyReject : public AppTypes::Event {
    MxInt		rejCode;	// source-specific numerical code
    MxEnum		rejReason;	// RejReason
    uint8_t		pad_0[3];
    MxTxtString		rejText;	// reject text

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << " rejReason=" << MxTRejReason::name(rejReason)
	<< " rejCode=" << rejCode
	<< " rejText=" << rejText;
    }
  };

  // used for mkt-initiated rejects
  struct Reject : public AppTypes::AnyReject {
    enum { EventType = MxTEventType::Reject };
  };

  // used for mkt-initiated modify rejects
  struct ModReject : public AppTypes::AnyReject {
    enum { EventType = MxTEventType::ModReject };
  };

  // used for mkt-initiated cancel rejects
  struct CxlReject : public AppTypes::AnyReject {
    enum { EventType = MxTEventType::CxlReject };
  };

  // used for broker-initiated hold of order/modify
  struct Hold : public AppTypes::AnyReject { };

  // used for broker-initiated release of a held order/modify
  struct Release : public Event {
    enum { EventType = MxTEventType::Release };
  };

  // used for broker-initiated reject of a held modify (implies cancel)
  struct Deny : public AppTypes::AnyReject {
    enum { EventType = MxTEventType::Deny };
  };
};

#pragma pack(pop)

template <typename AppTypes> struct MxTTxnTypes : public AppTypes {
  // types that can be extended by the app
  typedef typename AppTypes::Event Event;

  typedef typename AppTypes::OrderLeg OrderLeg;
  typedef typename AppTypes::ModifyLeg ModifyLeg;
  typedef typename AppTypes::CancelLeg CancelLeg;

  typedef typename AppTypes::AnyReject AnyReject;

  typedef typename AppTypes::NewOrder NewOrder;
  typedef typename AppTypes::Ordered Ordered;
  typedef typename AppTypes::Reject Reject;

  typedef typename AppTypes::Modify Modify;
  typedef typename AppTypes::Modified Modified;
  typedef typename AppTypes::ModReject ModReject;

  typedef typename AppTypes::Cancel Cancel;
  typedef typename AppTypes::Canceled Canceled;
  typedef typename AppTypes::CxlReject CxlReject;

  typedef typename AppTypes::Hold Hold;
  typedef typename AppTypes::Release Release;
  typedef typename AppTypes::Deny Deny;

  typedef typename AppTypes::Fill Fill;
  typedef typename AppTypes::Closed Closed;

  typedef typename AppTypes::AnySent AnySent;

  typedef typename AppTypes::OrderSent OrderSent;
  typedef typename AppTypes::ModifySent ModifySent;
  typedef typename AppTypes::CancelSent CancelSent;

  // additional typedefs
  struct ModSimulated : public Modify {
    enum { EventType = MxTEventType::ModSimulated };
  };

  struct ModRejectCxl : public ModReject {
    enum { EventType = MxTEventType::ModRejectCxl };
  };

  struct Filtered__ { };
  template <typename Request_, typename Reject_, int EventType_>
  struct Filtered_ : public Filtered__, public Request_ {
    enum { EventType = EventType_ };
    typedef Request_ Request;
    typedef Reject_ Reject;

    Reject	reject;

    template <typename S> inline void print(S &s) const {
      Request::print(s);
      s << ' ';
      reject.print(s);
    }
  };

  typedef Filtered_<NewOrder,
	  Hold, MxTEventType::OrderHeld> OrderHeld;
  typedef Filtered_<NewOrder,
	  Reject, MxTEventType::OrderFiltered> OrderFiltered;
  typedef Filtered_<Modify,
	  Hold, MxTEventType::ModHeld> ModHeld;
  typedef Filtered_<Modify,
	  ModReject, MxTEventType::ModFiltered> ModFiltered;
  typedef Filtered_<Modify,
	  ModReject, MxTEventType::ModFilteredCxl> ModFilteredCxl;
  typedef Filtered_<Cancel,
	  CxlReject, MxTEventType::CxlFiltered> CxlFiltered;

  struct Buf_ : public ZuPrintable { };

  // buffer containing Event-derived types discriminated by eventType
  template <typename Largest> struct Buf : public Buf_ {
    enum { Size =
      (sizeof(Largest) + sizeof(uintptr_t) - 1) / sizeof(uintptr_t) };

    ZuInline Buf() { as<Event>().eventState = MxTEventState::Unset; }

    ZuInline const void *ptr() const { return &data[0]; }
    ZuInline void *ptr() { return &data[0]; }

    template <typename T> ZuInline const T &as() const {
      const T *ZuMayAlias(ptr) = (const T *)&data[0];
      return *ptr;
    }
    template <typename T> ZuInline T &as() {
      T *ZuMayAlias(ptr) = (T *)&data[0];
      return *ptr;
    }

    ZuInline const MxEnum &type() const { return as<Event>().eventType; }
    ZuInline const MxFlags &flags() const { return as<Event>().eventFlags; }

    ZuInline bool operator !() const {
      return as<Event>().eventState == MxTEventState::Unset;
    }
    ZuOpBool;

    inline size_t size() {
      if (!*this) return 0;
      switch ((int)this->type()) {
	case MxTEventType::NewOrder: return sizeof(NewOrder);
	case MxTEventType::OrderHeld: return sizeof(OrderHeld);
	case MxTEventType::OrderFiltered: return sizeof(OrderFiltered);
	case MxTEventType::Ordered: return sizeof(Ordered);
	case MxTEventType::Reject: return sizeof(Reject);

	case MxTEventType::Modify: return sizeof(Modify);
	case MxTEventType::ModSimulated: return sizeof(ModSimulated);
	case MxTEventType::ModHeld: return sizeof(ModHeld);
	case MxTEventType::ModFiltered: return sizeof(ModFiltered);
	case MxTEventType::ModFilteredCxl: return sizeof(ModFilteredCxl);
	case MxTEventType::Modified: return sizeof(Modified);
	case MxTEventType::ModReject: return sizeof(ModReject);
	case MxTEventType::ModRejectCxl: return sizeof(ModRejectCxl);

	case MxTEventType::Cancel: return sizeof(Cancel);
	case MxTEventType::CxlFiltered: return sizeof(CxlFiltered);
	case MxTEventType::Canceled: return sizeof(Canceled);
	case MxTEventType::CxlReject: return sizeof(CxlReject);

	case MxTEventType::Release: return sizeof(Release);
	case MxTEventType::Deny: return sizeof(Deny);

	case MxTEventType::Fill: return sizeof(Fill);

	case MxTEventType::Closed: return sizeof(Closed);

	case MxTEventType::OrderSent: return sizeof(OrderSent);
	case MxTEventType::ModifySent: return sizeof(ModifySent);
	case MxTEventType::CancelSent: return sizeof(CancelSent);

	default: return sizeof(Event);
      }
    }

    template <typename S> inline void print(S &s) const {
      if (!*this) return;
      switch ((int)this->type()) {
	default: break;
	case MxTEventType::NewOrder: s << as<NewOrder>(); break;
	case MxTEventType::OrderHeld: s << as<OrderHeld>(); break;
	case MxTEventType::OrderFiltered: s << as<OrderFiltered>(); break;
	case MxTEventType::Ordered: s << as<Ordered>(); break;
	case MxTEventType::Reject: s << as<Reject>(); break;

	case MxTEventType::Modify: s << as<Modify>(); break;
	case MxTEventType::ModSimulated: s << as<ModSimulated>(); break;
	case MxTEventType::ModHeld: s << as<ModHeld>(); break;
	case MxTEventType::ModFiltered: s << as<ModFiltered>(); break;
	case MxTEventType::ModFilteredCxl: s << as<ModFilteredCxl>(); break;
	case MxTEventType::Modified: s << as<Modified>(); break;
	case MxTEventType::ModReject: s << as<ModReject>(); break;
	case MxTEventType::ModRejectCxl: s << as<ModRejectCxl>(); break;

	case MxTEventType::Cancel: s << as<Cancel>(); break;
	case MxTEventType::Canceled: s << as<Canceled>(); break;
	case MxTEventType::CxlReject: s << as<CxlReject>(); break;

	case MxTEventType::Release: s << as<Release>(); break;
	case MxTEventType::Deny: s << as<Deny>(); break;

	case MxTEventType::Fill: s << as<Fill>(); break;

	case MxTEventType::Closed: s << as<Closed>(); break;

	case MxTEventType::OrderSent: s << as<OrderSent>(); break;
	case MxTEventType::ModifySent: s << as<ModifySent>(); break;
	case MxTEventType::CancelSent: s << as<CancelSent>(); break;
      }
    }

  private:
    uintptr_t		data[Size];
  };

  struct Txn_ {
  protected:
    struct Data__ { };
    struct Request__ { };
    struct Reject__ { };
  };
  template <typename Largest>
  class Txn : public Buf<Largest>, public Txn_ {
    typedef typename Txn_::Data__ Data__;
    typedef typename Txn_::Request__ Request__;
    typedef typename Txn_::Reject__ Reject__;

  public:
    inline Txn() { }

    // Txn m1, m2;
    // m1 = m2.data<NewOrder>(); // copy m2 (containing NewOrder) to m1
    template <typename Txn_, typename T_> struct Data : public Data__ {
      typedef Txn_ Txn;
      typedef T_ T;
      inline Data(const Txn &txn_) : txn(txn_) { }
      const Txn	&txn;
    };
    template <typename T> inline Data<Txn, T> data() const {
      return Data<Txn, T>(*this);
    }
  private:
    template <typename Data_> inline void initData(const Data_ &data) {
      memcpy((void *)this, &data.txn, sizeof(typename Data_::T));
    }
  public:
    template <typename Data_> inline Txn(const Data_ &data,
	typename ZuIfT<ZuConversion<Data__, Data_>::Base &&
	  sizeof(typename Data_::T) <= sizeof(Largest)>::T *_ = 0) {
      initData(data);
    }
    template <typename Data_> typename ZuIfT<
	ZuConversion<Data__, Data_>::Base &&
	  sizeof(typename Data_::T) <= sizeof(Largest),
	Txn &>::T operator =(const Data_ &data) {
      initData(data);
      return *this;
    }

    // Txn m1, m2;
    // m1 = m2.request<OrderFiltered>(); // assign request part of m2 to m1
    template <typename Txn_, typename T_> struct Request_ : public Request__ {
      typedef Txn_ Txn;
      typedef T_ T;
      inline Request_(const Txn &txn_) : txn(txn_) { }
      const Txn	&txn;
    };
    template <typename T>
    inline ZuIsBase<Filtered__, T, Request_<Txn, T> > request() const {
      return Request_<Txn, T>(*this);
    }
  private:
    template <typename Request_>
    inline void initRequest_(const Request_ &request) {
      memcpy((void *)this, &request.txn,
	  sizeof(typename Request_::T::Request_));
    }
  public:
    template <typename Request_>
    inline Txn(const Request_ &request,
	typename ZuIfT<ZuConversion<Request__, Request_>::Base &&
	  sizeof(typename Request_::T::Request_) <=
	    sizeof(Largest)>::T *_ = 0) {
      initRequest_(request);
    }
    template <typename Request_> typename ZuIfT<
	ZuConversion<Request__, Request_>::Base &&
	  sizeof(typename Request_::T::Request_) <= sizeof(Largest),
	Txn &>::T operator =(const Request_ &request) {
      initRequest_(request);
      return *this;
    }

    // Txn<...> m1, m2;
    // m1 = m2.reject<OrderFiltered>(); // assign reject part of m2 to m1
    template <typename Txn_, typename T_> struct Reject_ : public Reject__ {
      typedef Txn_ Txn;
      typedef T_ T;
      inline Reject_(const Txn &txn_) : txn(txn_) { }
      const Txn	&txn;
    };
    template <typename T>
    inline ZuIsBase<Filtered__, T, Reject_<Txn, T> > reject() const {
      return Reject_<Txn, T>(*this);
    }
  private:
    template <typename Reject_>
    inline void initReject_(const Reject_ &reject) {
      memcpy((void *)this, &reject.txn.reject,
	  sizeof(typename Reject_::T::Reject_));
    }
  public:
    template <typename Reject_>
    inline Txn(const Reject_ &reject,
	typename ZuIfT<ZuConversion<Reject__, Reject_>::Base &&
	  sizeof(typename Reject_::T::Reject_) <= sizeof(Largest)>::T *_ = 0) {
      initReject_(reject);
    }
    template <typename Reject_> inline typename ZuIfT<
	ZuConversion<Reject__, Reject_>::Base && 
	  sizeof(typename Reject_::T::Reject_) <= sizeof(Largest),
	Txn &>::T operator =(const Reject_ &reject) {
      initReject_(reject);
      return *this;
    }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#define Txn_Init(Type) \
    inline Type &init##Type(MxUInt8 eventFlags_) { \
      static Type blank; \
      { \
	Event &event = this->template as<Event>(); \
	memcpy(&event, &blank, sizeof(Type)); \
	event.eventType = Type::EventType; \
	event.eventFlags = eventFlags_; \
      } \
      return this->template as<Type>(); \
    }

#define Txn_InitFiltered(Type) \
    template <typename Txn> \
    inline Type &init##Type(const Txn &txn, MxFlags eventFlags_) { \
      memcpy((void *)this, &txn, sizeof(typename Type::Request)); \
      static typename Type::Reject blank; \
      { \
	typename Type::Reject &reject = this->template as<Type>().reject; \
	memcpy(&reject, &blank, sizeof(typename Type::Reject)); \
	reject.eventFlags = eventFlags_ | (1U<<MxTEventFlags::Synthesized); \
      } \
      return this->template as<Type>(); \
    }

    Txn_Init(NewOrder)
    Txn_InitFiltered(OrderHeld)
    Txn_InitFiltered(OrderFiltered)
    Txn_Init(Ordered)
    Txn_Init(Reject)

    Txn_Init(Modify)
    Txn_Init(ModSimulated)
    Txn_InitFiltered(ModHeld)
    Txn_InitFiltered(ModFiltered)
    Txn_InitFiltered(ModFilteredCxl)
    Txn_Init(Modified)
    Txn_Init(ModReject)
    Txn_Init(ModRejectCxl)

    Txn_Init(Cancel)
    Txn_InitFiltered(CxlFiltered)
    Txn_Init(Canceled)
    Txn_Init(CxlReject)

    Txn_Init(Release)
    Txn_Init(Deny)

    Txn_Init(Fill)

    Txn_Init(Closed)

    Txn_Init(OrderSent)
    Txn_Init(ModifySent)
    Txn_Init(CancelSent)
  };

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

  // OrderTxn can contain a new order
  typedef Txn<NewOrder> OrderTxn;

  // ModifyTxn can contain a modify
  typedef Txn<Modify> ModifyTxn;

  // CancelTxn can contain a cancel
  typedef Txn<Cancel> CancelTxn;

  // ExecTxn can contain a execution(notice)/ack/reject
  typedef typename ZuLargest<
    Ordered, Reject,
    Modified, ModReject,
    Canceled, CxlReject,
    Release, Deny,
    Fill, Closed>::T Exec_Largest;
  typedef Txn<Exec_Largest> ExecTxn;

  // UpdateTxn can contain any update to an open order
  typedef typename ZuLargest<
    Modify,
    Cancel,
    Exec_Largest>::T Update_Largest;
  typedef Txn<Update_Largest> UpdateTxn;

  // AnyTxn can contain any trading message
  typedef typename ZuLargest<
    NewOrder,
    Update_Largest,
    OrderFiltered,
    ModFiltered,
    CxlFiltered>::T Txn_Largest;
  typedef Txn<Txn_Largest> AnyTxn;

  // Order - open order state including pending modify/cancel
  struct Order : public ZuPrintable {
    inline Order() { }

    OrderTxn		orderTxn;	// new order
    ModifyTxn		modifyTxn;	// (pending) modify
    CancelTxn		cancelTxn;	// (pending) cancel

    inline NewOrder &newOrder() {
      return orderTxn.template as<NewOrder>();
    }
    inline const NewOrder &newOrder() const {
      return orderTxn.template as<NewOrder>();
    }
    inline Modify &modify() {
      return modifyTxn.template as<Modify>();
    }
    inline const Modify &modify() const {
      return modifyTxn.template as<Modify>();
    }
    inline Cancel &cancel() {
      return cancelTxn.template as<Cancel>();
    }
    inline const Cancel &cancel() const {
      return cancelTxn.template as<Cancel>();
    }

    template <typename S> inline void print(S &s) const {
      s << "orderTxn={" << orderTxn
	<< "} modifyTxn={" << modifyTxn
	<< "} cancelTxn={" << cancelTxn << '}';
    }
  };
};

#endif /* MxTOrder_HPP */
