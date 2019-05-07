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
// An open order is an order transaction together with (at most one)
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
      Closed			// order closed (expired)
  );
  MxEnumNames(
      "NewOrder", "OrderHeld", "OrderFiltered", "Ordered", "Reject",
      "Modify", "ModSimulated", "ModHeld", "ModFiltered",
      "ModFilteredCxl", "Modified", "ModReject", "ModRejectCxl",
      "Cancel", "CxlFiltered", "Canceled", "CxlReject",
      "Release", "Deny",
      "Fill", "Closed");
}

// Note: Pending causes Pending New/Modify instead of Ordered/Modified
// when processing modify-on-queue; Ack is overloaded to specify fill target
namespace MxTEventFlags { // event flags
  MxEnumValues(
      Rx,		// received (cleared before each txn)
      Tx,		// transmitted (cleared before each txn)
      Ack,		// OMC - acknowledged - cleared before each txn)
      C, ModifyCxl = C,	// synthetic cancel/replace in progress
      M, ModifyNew = M,	// new order/ack following modify-on-queue
      Unsolicited,	// unsolicited modified/canceled from market
      Synthesized,	// synthesized - not received from market
      Pending		// synthesized and pending ordered/modified
  );
  MxEnumNames(
      "Rx", "Tx", "Ack",
      "ModifyCxl", "ModifyNew",
      "Unsolicited", "Synthesized", "Pending");
  MxEnumFlags(Flags,
      "Rx", Rx, "Tx", Tx, "Ack", Ack,
      "ModifyCxl", ModifyCxl, "ModifyNew", ModifyNew,
      "Unsolicited", Unsolicited,
      "Synthesized", Synthesized,
      "Pending", Pending);

  inline bool matchC(const MxFlags &v) { return v & (1U<<C); }
  inline bool matchM(const MxFlags &v) { return v & (1U<<M); }
  inline bool matchCM(const MxFlags &v)
    { return (v & ((1U<<C) | (1U<<M))) == ((1U<<C) | (1U<<M)); }
}

namespace MxTEventState { // event state
  MxEnumValues(
      U, Unset = U,
      R, Received = R,
      H, Held = H,
      D, Deferred = D, // deferred awaiting ack of pending order or modify
      Q, Queued = Q,
      S, Sent = S,
      P, PendingFill = P, // ack before fill
      A, Acknowledged = A,
      X, Rejected = X,
      C, Closed = C);
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
  inline bool matchHDQS(const MxEnum &v) { return v >= H && v <= S; }
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
  inline bool matchDQX(const MxEnum &v) { return v == D || v == Q || v == X; }
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

template <typename AppTypes> struct MxTAppTypes {

#pragma pack(push, 1)
  struct Event : public ZuPrintable {
    MxEnum	eventType = MxTEventType::Invalid;	// MxTEventType
    MxEnum	eventState = MxTEventState::Unset;	// MxTEventState
    MxUInt8	eventFlags = 0;				// MxTEventFlags
    MxUInt8	eventLeg = 0;

    inline void null() {
      eventType = MxTEventType::Invalid;
      eventState = MxTEventState::Unset;
      eventFlags = 0;
      eventLeg = 0;
    }

    ZuInline bool operator !() const {
      return eventState == MxTEventState::Unset;
    }
    ZuOpBool;

    template <bool Rx, bool Tx, bool Ack>
    inline void rxtx() {
      eventFlags = (eventFlags &
	~((1U<<MxTEventFlags::Rx) |
	  (1U<<MxTEventFlags::Tx) |
	  (1U<<MxTEventFlags::Ack))) |
	(((unsigned)Rx)<<MxTEventFlags::Rx) |
	(((unsigned)Tx)<<MxTEventFlags::Tx) |
	(((unsigned)Ack)<<MxTEventFlags::Ack);
    }
    template <bool M, bool C>
    inline void mc() {
      eventFlags = (eventFlags &
	~((1U<<MxTEventFlags::M) |
	  (1U<<MxTEventFlags::C))) |
	(((unsigned)M)<<MxTEventFlags::M) |
	(((unsigned)C)<<MxTEventFlags::C);
    }

#define Event_Flag(Bit, Fn) \
    inline bool Fn() const { \
      return eventFlags & (1U<<MxTEventFlags::Bit); \
    } \
    inline void Fn##_set() { eventFlags |=  (1U<<MxTEventFlags::Bit); } \
    inline void Fn##_clr() { eventFlags &= ~(1U<<MxTEventFlags::Bit); }

    Event_Flag(Rx, rx)
    Event_Flag(Tx, tx)
    Event_Flag(Ack, ack)
    Event_Flag(ModifyNew, modifyNew)
    Event_Flag(ModifyCxl, modifyCxl)
    Event_Flag(Unsolicited, unsolicited)
    Event_Flag(Synthesized, synthesized)
    Event_Flag(Pending, pending)

    template <typename Update>
    inline void update(const Update &u) { }

    template <typename S> inline void print(S &s) const {
      if (!*this) return;
      s << "eventType=" << MxTEventType::name(eventType)
	<< " eventState=" << MxTEventState::name(eventState)
	<< " eventLeg=" << eventLeg
	<< " eventFlags=";
      MxTEventFlags::Flags::instance()->print(s, eventFlags);
    }
  };
#pragma pack(pop)

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
    inline typename ZuIs<Legs_, Update, bool>::T pendingFill(const Update &u) {
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

    template <typename Update>
    inline void update(const Update &) { }

    template <typename S> inline void print(S &s) const {
      if (*cumQty)
	s << "qtyNDP=" << qtyNDP
	  << " cumQty=" << MxValNDP{cumQty, qtyNDP};
    }
  };

  struct CancelLeg_ : public CanceledLeg_ {
    MxValue		orderQty;

    template <typename Update>
    inline typename ZuIs<CancelLeg_, Update>::T update(const Update &u) {
      CanceledLeg_::update(u);
      orderQty.update(u.orderQty);
    }
    template <typename Update>
    inline typename ZuIsNot<CancelLeg_, Update>::T update(const Update &u) {
      CanceledLeg_::update(u);
    }

    template <typename Update> inline void expose(const Update &u) {
      if (orderQty < u.orderQty) orderQty = u.orderQty;
    }

    template <typename S> inline void print(S &s) const {
      CanceledLeg_::print(s);
      if (*orderQty)
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
    MxValue		cumValue = 0;	// FIX GrossTradeAmt
    MxEnum		side;		// MxSide
    MxEnum		ordType;	// MxOrdType
    MxNDP		pxNDP;
    uint8_t		pad_0[1];

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
      px.update(u.px);
    }
    template <typename Update>
    inline typename ZuIsNot<ModifyLeg_, Update>::T update(const Update &u) {
      CancelLeg_::update(u);
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
	<< " ordType=" << MxOrdType::name(ordType);
      if (*px)
	s << " pxNDP=" << pxNDP
	  << " px=" << MxValNDP{px, pxNDP};
      s << " cumValue=" << MxValNDP{cumValue, pxNDP} << ' ';
      CancelLeg_::print(s);
    }
  };
  struct ModifyLeg : public ModifyLeg_ { };

  struct Modify__ { };
  template <typename Leg>
  struct Modify_ : public Modify__, public Cancel_<Leg> {
    MxEnum		timeInForce;	// MxTimeInForce
    uint8_t		pad_0[3];

    template <typename Update>
    inline typename ZuIs<Modify__, Update>::T update(const Update &u) {
      Cancel_<Leg>::update(u); // takes care of legs
      timeInForce.update(u.timeInForce);
    }
    template <typename Update>
    inline typename ZuIsNot<Modify__, Update>::T update(const Update &u) {
      Cancel_<Leg>::update(u);
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
      AppTypes::Event::update(u);
      Cancel_<typename AppTypes::CanceledLeg>::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << ' ';
      Cancel_<typename AppTypes::CanceledLeg>::print(s);
    }
  };

  struct OrderLeg_ : public ModifyLeg_ {
    MxValue		leavesQty = 0;

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

    template <typename S> inline void print(S &s) const {
      Modify_<Leg>::print(s);
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
      AppTypes::Event::update(u);
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
      s << " pxNDP=" << pxNDP
	<< " qtyNDP=" << qtyNDP
	<< " lastPx=" << MxValNDP{lastPx, pxNDP}
	<< " lastQty=" << MxValNDP{lastQty, qtyNDP};
    }
  };

  struct Closed : public AppTypes::Event {
    enum { EventType = MxTEventType::Closed };
  };

  // generic reject data for new order / modify / cancel
  struct AnyReject : public AppTypes::Event {
    MxInt		rejCode = 0;	// source-specific numerical code
    MxEnum		rejReason;	// MxTRejReason

    template <typename Update>
    inline typename ZuIs<AnyReject, Update>::T update(const Update &u) {
      AppTypes::Event::update(u);
      rejCode = u.rejCode;
      rejReason = u.rejReason;
    }
    template <typename Update>
    inline typename ZuIsNot<AnyReject, Update>::T update(const Update &u) {
      AppTypes::Event::update(u);
    }

    template <typename S> inline void print(S &s) const {
      AppTypes::Event::print(s);
      s << " rejReason=" << MxTRejReason::name(rejReason)
	<< " rejCode=" << rejCode;
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

// types that can be extended by the app
#define MxTImport_(Scope) \
  using Event = typename Scope::Event; \
 \
  using OrderLeg = typename Scope::OrderLeg; \
  using ModifyLeg = typename Scope::ModifyLeg; \
  using CancelLeg = typename Scope::CancelLeg; \
 \
  using AnyReject = typename Scope::AnyReject; \
 \
  using NewOrder = typename Scope::NewOrder; \
  using Ordered = typename Scope::Ordered; \
  using Reject = typename Scope::Reject; \
 \
  using Modify = typename Scope::Modify; \
  using Modified = typename Scope::Modified; \
  using ModReject = typename Scope::ModReject; \
 \
  using Cancel = typename Scope::Cancel; \
  using Canceled = typename Scope::Canceled; \
  using CxlReject = typename Scope::CxlReject; \
 \
  using Hold = typename Scope::Hold; \
  using Release = typename Scope::Release; \
  using Deny = typename Scope::Deny; \
 \
  using Fill = typename Scope::Fill; \
 \
  using Closed = typename Scope::Closed

template <typename AppTypes> struct MxTTxnTypes : public AppTypes {
  MxTImport_(AppTypes);

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

    ZuInline Event &event() { return as<Event>(); }
    ZuInline const Event &event() const { return as<Event>(); }

    ZuInline const MxEnum &type() const { return as<Event>().eventType; }
    ZuInline const MxFlags &flags() const { return as<Event>().eventFlags; }

    ZuInline bool operator !() const { return !as<Event>(); }
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

    // Txn<...> m1, m2;
    // m1 = m2.request<OrderFiltered>(); // assign request part of m2 to m1
    template <typename Txn_, typename T_> struct Request_ : public Request__ {
      typedef Txn_ Txn;
      typedef T_ T;
      inline Request_(const Txn &txn_) : txn(txn_) { }
      const Txn	&txn;
    };
    template <typename T>
    inline typename ZuIsBase<Filtered__, T, Request_<Txn, T> >::T
    request() const {
      return Request_<Txn, T>(*this);
    }
  private:
    template <typename Request>
    inline void initRequest_(const Request &request) {
      memcpy((void *)this, &request.txn,
	  sizeof(typename Request::T::Request));
    }
  public:
    template <typename Request>
    inline Txn(const Request &request,
	typename ZuIfT<ZuConversion<Request__, Request>::Base &&
	  sizeof(typename Request::T::Request) <=
	    sizeof(Largest)>::T *_ = 0) {
      initRequest_(request);
    }
    template <typename Request> typename ZuIfT<
	ZuConversion<Request__, Request>::Base &&
	  sizeof(typename Request::T::Request) <= sizeof(Largest),
	Txn &>::T operator =(const Request &request) {
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
    inline typename ZuIsBase<Filtered__, T, Reject_<Txn, T> >::T
    reject() const {
      return Reject_<Txn, T>(*this);
    }
  private:
    template <typename Reject>
    inline void initReject_(const Reject &reject) {
      memcpy((void *)this, &reject.txn.template as<typename Reject::T>().reject,
	  sizeof(typename Reject::T::Reject));
    }
  public:
    template <typename Reject>
    inline Txn(const Reject &reject,
	typename ZuIfT<ZuConversion<Reject__, Reject>::Base &&
	  sizeof(typename Reject::T::Reject) <= sizeof(Largest)>::T *_ = 0) {
      initReject_(reject);
    }
    template <typename Reject> inline typename ZuIfT<
	ZuConversion<Reject__, Reject>::Base && 
	  sizeof(typename Reject::T::Reject) <= sizeof(Largest),
	Txn &>::T operator =(const Reject &reject) {
      initReject_(reject);
      return *this;
    }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#define Txn_Init(Type) \
    template <bool Synthesized = false> \
    inline Type &init##Type(unsigned flags = 0U, unsigned leg = 0) { \
      static Type blank; \
      { \
	Event &event = this->template as<Event>(); \
	memcpy(&event, &blank, sizeof(Type)); \
	event.eventType = Type::EventType; \
	event.eventState = MxTEventState::Received; \
	event.eventFlags = flags | \
	  ((unsigned)Synthesized)<<MxTEventFlags::Synthesized; \
	event.eventLeg = leg; \
      } \
      return this->template as<Type>(); \
    }

    Txn_Init(NewOrder)
    Txn_Init(OrderHeld)
    Txn_Init(OrderFiltered)
    Txn_Init(Ordered)
    Txn_Init(Reject)

    Txn_Init(Modify)
    Txn_Init(ModSimulated)
    Txn_Init(ModHeld)
    Txn_Init(ModFiltered)
    Txn_Init(ModFilteredCxl)
    Txn_Init(Modified)
    Txn_Init(ModReject)
    Txn_Init(ModRejectCxl)

    Txn_Init(Cancel)
    Txn_Init(CxlFiltered)
    Txn_Init(Canceled)
    Txn_Init(CxlReject)

    Txn_Init(Release)
    Txn_Init(Deny)

    Txn_Init(Fill)

    Txn_Init(Closed)
  };

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

  typedef Txn<NewOrder> OrderTxn;	// order / order ack
  typedef Txn<Modify> ModifyTxn;	// modify / modify ack
  typedef Txn<Cancel> CancelTxn;	// cancel / cancel ack

  typedef Txn<Event> AckTxn;		// ack event header

  // ExecTxn can contain a reject/execution(notice) (acks update OMC)
  typedef typename ZuLargest<
    Reject, ModReject, CxlReject,
    Release, Deny,
    Fill, Closed>::T Exec_Largest;
  typedef Txn<Exec_Largest> ExecTxn;

  // AnyTxn can contain any request or event
  typedef typename ZuLargest<
    NewOrder, Modify, Cancel,
    Ordered, Modified, Canceled,
    OrderHeld, ModHeld,
    OrderFiltered, ModFiltered, CxlFiltered,
    Exec_Largest>::T Any_Largest;
  typedef Txn<Any_Largest> AnyTxn;

  // Order - open order state including pending modify/cancel
  struct Order : public ZuPrintable {
    inline Order() { }

    // following each state transition, outgoing messages are
    // transmitted/processed in the following sequence:
    // order/modify/cancel, exec, ack; multiple transitions
    // are used to advance any pending acks
    OrderTxn		orderTxn;	// new order
    ModifyTxn		modifyTxn;	// (pending) modify
    CancelTxn		cancelTxn;	// (pending) cancel
    AckTxn		ackTxn;		// last ack of above OMC
    ExecTxn		execTxn;	// last execution

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
    inline Event &ack() {
      return ackTxn.template as<Event>();
    }
    inline const Event &ack() const {
      return ackTxn.template as<Event>();
    }
    inline Event &exec() {
      return execTxn.template as<Event>();
    }
    inline const Event &exec() const {
      return execTxn.template as<Event>();
    }

    template <typename S> inline void print(S &s) const {
      s << "orderTxn={" << orderTxn
	<< "} modifyTxn={" << modifyTxn
	<< "} cancelTxn={" << cancelTxn
	<< "} ackTxn={" << ack()
	<< "} execTxn={" << execTxn
	<< '}';
    }
  };
};

#define MxTImport(Scope) \
  MxTImport_(Scope); \
 \
  using OrderHeld = typename Scope::OrderHeld; \
  using OrderFiltered = typename Scope::OrderFiltered; \
  using ModSimulated = typename Scope::ModSimulated; \
  using ModRejectCxl = typename Scope::ModRejectCxl; \
  using ModHeld = typename Scope::ModHeld; \
  using ModFiltered = typename Scope::ModFiltered; \
  using ModFilteredCxl = typename Scope::ModFilteredCxl; \
  using CxlFiltered = typename Scope::CxlFiltered; \
 \
  using Txn_ = typename Scope::Txn_; \
  template <typename T> using Txn = typename Scope::template Txn<T>; \
 \
  using OrderTxn = typename Scope::OrderTxn; \
  using ModifyTxn = typename Scope::ModifyTxn; \
  using CancelTxn = typename Scope::CancelTxn; \
 \
  using ExecTxn = typename Scope::ExecTxn; \
  using AnyTxn = typename Scope::AnyTxn; \
 \
  using Order = typename Scope::Order

#endif /* MxTOrder_HPP */
