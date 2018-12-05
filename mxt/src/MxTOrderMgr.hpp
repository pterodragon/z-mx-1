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

// MxT order state management

#ifndef MxTOrderMgr_HPP
#define MxTOrderMgr_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxTLib_HPP
#include <MxTLib.hpp>
#endif

#include <MxBase.hpp>

#include <MxTTypes.hpp>
#include <MxTOrder.hpp>

// application must instantiate MxTOrderMgr
// and supply the following interface:
#if 0
struct AppData : public MxTEventData<AppData> {
  typedef MxTEventData<MsgAppData> Base;

  // void NewOrder::update(const Ordered &);
  // void NewOrder::update(const Modify &);
  // void NewOrder::update(const Modified &);
  // void OrderLeg::update(const ModifyLeg &);
  // void OrderLeg::update(const ModifiedLeg &);
  // void Modify::update(const Modify &);
  // void Modify::update(const Modified &);
  // void ModifyLeg::update(const ModifyLeg &);
  // void ModifyLeg::update(const ModifiedLeg &);
  // void Modified::update(const Modify &);
  // void Modified::update(const NewOrder &);
  // void ModifiedLeg::update(const ModifyLeg &);
  // void ModifiedLeg::update(const OrderLeg &);
  // void Cancel::update(const Canceled &);
  // void Cancel::update(const NewOrder &);

  // extend data definitions
  struct NewOrder : public Base::NewOrder {
    // fields
    MxIDString	clOrdID;

    // update
    using Base::NewOrder::update;
    template <typename Update>
    inline typename ZuIs<NewOrder, Update>::T update(const Update &u) {
      Base::update(u);
      clOrdID.update(u.clOrdID);
    }

    // print
    template <typename S> inline void print(S &s) const {
      s << static_cast<const Base::NewOrder &>(*this) <<
	" clOrdID=" << clOrdID;
    }
  };
};
struct App : public MxTOrderMgr<App, AppData> {
  // log abnormal OSM transition
  template <typename Msg> void abnormal(Order *, const Msg &);

  // returns true if async. modify enabled for order
  bool asyncMod(Order *);
  // returns true if async. cancel enabled for order
  bool asyncCxl(Order *);

  // pre-process messages
  template <typename Event, typename Msg> void requestIn(Order *, Msg &);
  template <typename Event, typename Msg> void requestOut(Order *, Msg &);
  template <typename Event, typename Msg> void executionIn(Order *, Msg &);
  template <typename Event, typename Msg> void executionOut(Order *, Msg &);

  // send*() - send the corresponding message
  template <typename Msg> void sendNewOrder(Msg &);
  template <typename Msg> void sendOrdered(Msg &);
  template <typename Msg> void sendReject(Msg &);

  template <typename Msg> void sendModify(Msg &);
  template <typename Msg> void sendModified(Msg &);
  template <typename Msg> void sendModReject(Msg &);

  template <typename Msg> void sendCancel(Msg &);
  template <typename Msg> void sendCanceled(Msg &);
  template <typename Msg> void sendCxlReject(Msg &);

  template <typename Msg> void sendHold(Msg &);
  template <typename Msg> void sendRelease(Msg &);

  template <typename Msg> void sendFill(Msg &);

  template <typename Msg> void sendClosed(Msg &);
};
#endif

template <typename App, typename AppData>
class MxTOrderMgr : public MxTEventTypes<AppData> {
public:
  typedef MxTEventTypes<AppData> EventTypes;

  typedef typename EventTypes::Event Event;

  typedef typename EventTypes::OrderLeg OrderLeg;
  typedef typename EventTypes::ModifyLeg ModifyLeg;
  typedef typename EventTypes::CancelLeg CancelLeg;

  typedef typename EventTypes::AnyReject AnyReject;

  typedef typename EventTypes::NewOrder NewOrder;
  typedef typename EventTypes::OrderHeld OrderHeld;
  typedef typename EventTypes::OrderFiltered OrderFiltered;
  typedef typename EventTypes::Ordered Ordered;
  typedef typename EventTypes::Reject Reject;

  typedef typename EventTypes::Modify Modify;
  typedef typename EventTypes::ModSimulated ModSimulated;
  typedef typename EventTypes::ModHeld ModHeld;
  typedef typename EventTypes::ModFiltered ModFiltered;
  typedef typename EventTypes::ModFilteredCxl ModFilteredCxl;
  typedef typename EventTypes::Modified Modified;
  typedef typename EventTypes::ModReject ModReject;
  typedef typename EventTypes::ModRejectCxl ModRejectCxl;

  typedef typename EventTypes::Cancel Cancel;
  typedef typename EventTypes::CxlFiltered CxlFiltered;
  typedef typename EventTypes::Canceled Canceled;
  typedef typename EventTypes::CxlReject CxlReject;

  typedef typename EventTypes::Hold Hold;
  typedef typename EventTypes::Release Release;
  typedef typename EventTypes::Deny Deny;

  typedef typename EventTypes::Fill Fill;

  typedef typename EventTypes::Closed Closed;

  typedef typename EventTypes::OrderSent OrderSent;
  typedef typename EventTypes::ModifySent ModifySent;
  typedef typename EventTypes::CancelSent CancelSent;

  typedef typename EventTypes::Msg_ Msg_;
  template <typename Largest>
  using Msg = typename EventTypes::template Msg<Largest>;

  typedef typename EventTypes::OrderMsg OrderMsg;
  typedef typename EventTypes::ModifyMsg ModifyMsg;
  typedef typename EventTypes::CancelMsg CancelMsg;

  typedef typename EventTypes::AnyMsg AnyMsg;

  typedef typename EventTypes::Order Order;

private:
  inline const App *app() const { return static_cast<const App *>(this); }
  inline App *app() { return static_cast<App *>(this); }

public:
  MxTOrderMgr() {
    { // ensure all static initializers are run once
      AnyMsg msg, request;

      msg.initNewOrder(0);
      request = msg.template data<NewOrder>();
      msg.initOrderHeld(request, 0);
      msg.initOrderFiltered(request, 0);
      msg.initOrdered(0);
      msg.initReject(0);

      msg.initModify(0);
      msg.initModSimulated(0);
      request = msg.template data<Modify>();
      msg.initModHeld(request, 0);
      msg.initModFiltered(request, 0);
      msg.initModFilteredCxl(request, 0);
      msg.initModified(0);
      msg.initModReject(0);
      msg.initModRejectCxl(0);

      msg.initCancel(0);
      request = msg.template data<Cancel>();
      msg.initCxlFiltered(request, 0);
      msg.initCanceled(0);
      msg.initCxlReject(0);

      msg.initRelease(0);
      msg.initDeny(0);

      msg.initFill(0);

      msg.initClosed(0);

      msg.initOrderSent(0);
      msg.initModifySent(0);
      msg.initCancelSent(0);
    }
  }

  // order state management

private:
  template <typename Event, typename In>
  void requestIn(Order *order, In &in) {
    Event &event = in.template as<Event>();
    event.eventState = MxTEventState::Received;
    app()->template requestIn<Event>(order, in);
  }
  template <typename Event, typename Out>
  void requestOut(Order *order, Out &out) {
    Event &event = out.template as<Event>();
    event.eventState = MxTEventState::Queued;
    app()->template requestOut<Event>(order, out);
  }

  template <typename Event, typename In>
  void executionIn(Order *order, In &in) {
    Event &event = in.template as<Event>();
    event.eventState = MxTEventState::Received;
    app()->template executionIn<Event>(order, in);
  }
  template <typename Event, typename Out>
  void executionOut(Order *order, Out &out) {
    Event &event = out.template as<Event>();
    event.eventState = MxTEventState::Sent;
    app()->template executionOut<Event>(order, out);
  }

  void sendNewOrder(Order *order) {
    requestOut<NewOrder, OrderMsg>(order, order->orderMsg);
    // Note: app should use leavesQty since this may be a simulated modify
    // (cancel/replace) of a partially filled order
    app()->sendNewOrder(order->orderMsg);
  }
  template <typename Out> void sendOrdered(Order *order, Out &out) {
    executionOut<Ordered, Out>(order, out);
    app()->sendOrdered(out);
  }
  template <typename Out> void sendReject(Order *order, Out &out) {
    executionOut<Reject, Out>(order, out);
    app()->sendReject(out);
  }
  template <typename Out> void sendHold(Order *order, Out &out) {
    executionOut<Hold, Out>(order, out);
    app()->sendHold(out);
  }
  void sendModify(Order *order) {
    requestOut<Modify, ModifyMsg>(order, order->modifyMsg);
    app()->sendModify(order->modifyMsg);
  }
  template <typename Out> void sendModified(Order *order, Out &out) {
    executionOut<Modified, Out>(order, out);
    app()->sendModified(out);
  }
  template <typename Out> void sendModReject(Order *order, Out &out) {
    executionOut<ModReject, Out>(order, out);
    app()->sendModReject(out);
  }
  void sendCancel(Order *order) {
    requestOut<Cancel, CancelMsg>(order, order->cancelMsg);
    app()->sendCancel(order->cancelMsg);
  }
  template <typename Out> void sendCanceled(Order *order, Out &out) {
    executionOut<Canceled, Out>(order, out);
    app()->sendCanceled(out);
  }
  template <typename Out> void sendCxlReject(Order *order, Out &out) {
    executionOut<CxlReject, Out>(order, out);
    app()->sendCxlReject(out);
  }

public:
  // for requests, need pre- and post-exposures
  // new order - no pre, post is actual qty
  // modify - pre- is current qty (including pending modifies),
  //   post is including applied modify
  //
  // first rollback pre, then risk-check post
  // if post is rejected (filtered), then post is recalculated following *{Held,Filtered} and applied
  // if post is accepted, post is recalculated following NewOrder, etc. and applied
  // for cancels and ERs, pre- and post- are just calculated before/after
  // corresponding  processing (rollback pre, rollforward post)

  // for each leg, there is an instrument - i.e. an asset pair
  // for reach asset, there is a long and short exposure (single value)
  //
  // instrument buy
  // -> base asset long exposure increases by qty
  // -> quote asset short exposure increases by px*qty
  //
  // instrument sell
  // -> base asset short exposure increases by qty
  // -> quote asset long exposure increases by px*qty
  //
  // in order to be ok, notional balances have to be >= short exposures
  // notional balance == actual/loaned balance for cash trading
  // notional balance is amplified by margining for margin trading
  //
  // risk:
  // NewOrder pre = order->newOrder;
  // if (MxTEventState::matchHDQSP(order->modify().eventState))
  //   pre.expose(order->modify());
  // Portfolio pf = ... // actual portfolio for RCG
  // ShadowPortfolio spf = pf // copy positions for instruments in legs
  // spf -= pre; // cancel current exposure
  // NewOrder post = pre;
  // post.expose(modify);
  // spf += post; // apply new exposure to shadow portfolio
  // if (spf.breach()) { // if spf is now in breach, reject
  //   ...Filtered(...) // OSM
  //   ...Held(...) // OSM
  // } else
  //   ...(...) // OSM
  // // recalculate post based on OSM outcome
  // post = order->newOrder;
  // if (MxTEventState::matchHDQSP(order->modify().eventState))
  //   post.expose(order->modify());
  // { guard pf; pf -= pre, pf += post; }
  // if (pf.alert()) { ... alerting; }

  // app calls newOrderIn followed by one of newOrder order{Held,Filtered}
  template <typename In> typename ZuIsBase<Msg_, In>::T newOrderIn(
      Order *order, In &in) {
    requestIn<NewOrder>(order, in);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T newOrder(
      Order *order, In &in) {
    order->orderMsg = in.template data<NewOrder>();
    sendNewOrder(order);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T orderHeld(
      Order *order, In &in) {
    order->orderMsg = in.template request<OrderHeld>();
    order->newOrder().eventState = MxTEventState::Held;
    Msg<Hold> out(in.template reject<OrderHeld>());
    sendHold(order, out);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T orderFiltered(
      Order *order, In &in) {
    order->orderMsg = in.template request<OrderFiltered>();
    order->newOrder().eventState = MxTEventState::Rejected;
    Msg<Reject> out(in.template reject<OrderFiltered>());
    sendReject(order, out);
  }

private:
  template <typename In> void applyOrdered(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.update(in.template as<Ordered>());
    if (ZuUnlikely(MxTOrderFlags::matchM(newOrder.flags))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::M);
      Msg<Modified> out;
      Modified &modified = out.initModified(
	(1U<<MxTEventFlags::Synthesized));
      executionIn<Modified, Msg<Modified> >(order, out);
      modified.update(newOrder);
      sendModified(order, out);
      return;
    }
    Msg<Ordered> out(in.template data<Ordered>());
    sendOrdered(order, out);
  }
public:
  template <typename In> typename ZuIsBase<Msg_, In>::T ordered(
      Order *order, In &in) {
    executionIn<Ordered, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  !MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchHQS(newOrder.eventState))) {
      // usual case - solicited
      applyOrdered(order, in);
      if (ZuUnlikely(MxTEventState::matchD(cancel.eventState)))
	sendCancel(order);
      else if (ZuUnlikely(MxTEventState::matchD(modify.eventState)))
	sendModify(order);
      return;
    }
    if (ZuUnlikely(MxTEventState::matchAC(newOrder.eventState))) {
      // unsolicited - order already acknowledged / closed
      in.template as<Event>().unsolicited_set();
      return;
    }
    if (ZuLikely(
	  MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchS(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // solicited - synthetic cancel/replace in process
      applyOrdered(order, in);
      if (ZuUnlikely(MxTEventState::matchDX(cancel.eventState)))
	sendCancel(order);
      return;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
    applyOrdered(order, in);
  }

private:
  // synthetic modify reject
  template <typename In> void modReject_(Order *order, In &in, int reason) {
    in.template as<Event>.eventState = MxTEventState::Rejected;
    Msg<ModReject> out;
    ModReject &modReject = out.initModReject(
	(1U<<MxTEventFlags::Synthesized));
    executionIn<ModReject, Msg<ModReject> >(order, out);
    modReject.rejReason = reason;
    sendModReject(order, out);
  }
  // synthetic cancel reject
  template <typename In> void cxlReject_(Order *order, In &in, int reason) {
    in.template as<Event>.eventState = MxTEventState::Rejected;
    Msg<CxlReject> out;
    CxlReject &cxlReject = out.initCxlReject(
	(1U<<MxTEventFlags::Synthesized));
    executionIn<CxlReject, Msg<CxlReject> >(order, out);
    cxlReject.rejReason = reason;
    sendCxlReject(order, out);
  }

  template <typename In> void applyReject(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (!MxTEventState::matchXC(newOrder.eventState))
      newOrder.eventState = MxTEventState::Rejected;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    Msg<Reject> out(in.template data<Reject>());
    sendReject(order, out);
  }
public:
  template <typename In> typename ZuIsBase<Msg_, In>::T reject(
      Order *order, In &in) {
    executionIn<Reject, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  !MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchHQSAX(newOrder.eventState))) {
      // usual case
      if (ZuUnlikely(MxTEventState::matchAX(newOrder.eventState)))
	in.template as<Event>().unsolicited_set();
      if (ZuUnlikely(MxTEventState::matchDQ(cancel.eventState)))
	cxlReject_(order, order->cancelMsg, MxTRejReason::OrderClosed);
      else if (ZuUnlikely(MxTEventState::matchDQ(modify.eventState)))
	modReject_(order, order->modifyMsg, MxTRejReason::OrderClosed);
      applyReject(order, in);
      return;
    }
    if (ZuLikely(
	  MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // synthetic cancel/replace in process
      if (ZuUnlikely(MxTEventState::matchDQ(cancel.eventState)))
	cancel.eventState = MxTEventState::Unset;
      modReject_(order, order->modifyMsg, MxTRejReason::OrderClosed);
      applyReject(order, in);
      return;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
    applyReject(order, in);
  }

private:
  template <typename In> MxEnum filterModify(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState)))
      return MxTRejReason::OrderClosed;
    if (ZuUnlikely(MxTEventState::matchDQSP(cancel.eventState))) {
      if (!MxTOrderFlags::matchC(newOrder.flags))
	return MxTRejReason::CancelPending;
      else
	return MxTRejReason::ModifyPending;
    }
    if (ZuUnlikely(MxTEventState::matchDSP(modify.eventState)))
      return MxTRejReason::ModifyPending;
    if (ZuUnlikely(
	  !MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchQ(modify.eventState)))
      return MxTRejReason::ModifyPending;
    return MxTRejReason::OK;
  }

public:
  // app calls modifyIn followed by one of
  // modify mod{Simulated,Held,Filtered,FilteredCxl}
  // returns MxTRejReason::OK, or reason if modify is rejected due to OSM
  template <typename In> typename ZuIsBase<Msg_, In, MxEnum>::T modifyIn(
      Order *order, In &in) {
    requestIn<Modify, In>(order, in);
    return filterModify(order, in);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modify(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	order->modifyMsg = in.template data<Modify>();
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncMod(order))) {
	  // async. modify disabled
	  modify.eventState = MxTEventState::Deferred;
	  return;
	}
	sendModify(order);
	return;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Received;
	if (!MxTOrderFlags::matchM(newOrder.flags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.flags |= (1U<<MxTOrderFlags::M);
	  Msg<Ordered> out;
	  out.initOrdered(
	    (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	  executionIn<Ordered, Msg<Ordered> >(order, out);
	  sendOrdered(order, out);
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	Msg<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	executionIn<Modified, Msg<Modified> >(order, out);
	modified.update(newOrder);
	sendModified(order, out);
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  Msg<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn<Modified, Msg<Modified> >(order, out);
	  modified.update(newOrder);
	  sendModified(order, out);
	  newOrder.flags &= ~(1U<<MxTOrderFlags::M);
	  newOrder.eventState = MxTEventState::Acknowledged;
	  return;
	}
	sendNewOrder(order);
	return;
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      // modify-on-queue
      Msg<Modified> out;
      Modified &modified = out.initModified(
	(1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
      executionIn<Modified, Msg<Modified> >(order, out);
      modified.update(newOrder);
      modified.update(modify);
      sendModified(order, out);
      modify.update(in.template as<Modify>());
      modify.eventState = MxTEventState::Deferred;
      if (MxTOrderFlags::matchC(newOrder.flags)) {
	// synthetic cancel/replace in process
	if (MxTEventState::matchSP(cancel.eventState)) {
	  // cancel sent
	  modify.eventState = MxTEventState::Deferred;
	  return;
	}
	cancel.eventState = MxTEventState::Unset;
	newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      }
      sendModify(order);
      return;
    }
    // should never happen; somehow this point was reached despite
    // filterModify() previously returning OK when app called modifyIn()
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modSimulated(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	newOrder.flags |= (1U<<MxTOrderFlags::C);
	order->modifyMsg = in.template data<Modify>();
	modify.eventState = MxTEventState::Deferred;
	order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order))) {
	  // async. cancel disabled
	  cancel.eventState = MxTEventState::Deferred;
	  return;
	}
	sendCancel(order);
	return;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Received;
	if (!MxTOrderFlags::matchM(newOrder.flags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.flags |= (1U<<MxTOrderFlags::M);
	  Msg<Ordered> out;
	  out.initOrdered((1U<<MxTEventFlags::Synthesized));
	  executionIn<Ordered, Msg<Ordered> >(order, out);
	  sendOrdered(order, out);
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	Msg<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn<Modified, Msg<Modified> >(order, out);
	modified.update(newOrder);
	sendModified(order, out);
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  Msg<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn<Modified, Msg<Modified> >(order, out);
	  modified.update(newOrder);
	  sendModified(order, out);
	  newOrder.flags &= ~(1U<<MxTOrderFlags::M);
	  newOrder.eventState = MxTEventState::Acknowledged;
	  return;
	}
	sendNewOrder(order);
	return;
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      // modify-on-queue
      Msg<Modified> out;
      Modified &modified = out.initModified(
	(1U<<MxTEventFlags::Synthesized));
      executionIn<Modified, Msg<Modified> >(order, out);
      modified.update(newOrder);
      modified.update(modify);
      sendModified(order, out);
      modify.update(in.template as<Modify>());
      modify.eventState = MxTEventState::Deferred;
      if (MxTOrderFlags::matchC(newOrder.flags)) {
	// synthetic cancel/replace in process
	if (MxTEventState::matchQSP(cancel.eventState))
	  return; // cancel already sent
      } else
	newOrder.flags |= (1U<<MxTOrderFlags::C);
      order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
      cancel.update(newOrder);
      sendCancel(order);
      return;
    }
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modHeld(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	newOrder.flags |= (1U<<MxTOrderFlags::C);
	order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
	else
	  sendCancel(order);
	order->modifyMsg = in.template request<ModHeld>();
	modify.eventState = MxTEventState::Held;
	Msg<Hold> out(in.template reject<ModHeld>());
	sendHold(order, out);
	return;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Received;
	if (!MxTOrderFlags::matchM(newOrder.flags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.flags |= (1U<<MxTOrderFlags::M);
	  Msg<Ordered> out;
	  out.initOrdered((1U<<MxTEventFlags::Synthesized));
	  executionIn<Ordered, Msg<Ordered> >(order, out);
	  sendOrdered(order, out);
	} else {
	  // new order is synthetic, following a previous modify-on-queue
	  Msg<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn<Modified, Msg<Modified> >(order, out);
	  modified.update(newOrder);
	  sendModified(order, out);
	}
	newOrder.update(in.template request<ModHeld>());
	newOrder.eventState = MxTEventState::Held;
	Msg<Hold> out(in.template reject<ModHeld>());
	sendHold(order, out);
	return;
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      {
	Msg<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn<Modified, Msg<Modified> >(order, out);
	modified.update(newOrder);
	modified.update(modify);
	sendModified(order, out);
      }
      modify.update(in.template request<ModHeld>());
      modify.eventState = MxTEventState::Held;
      if (MxTOrderFlags::matchC(newOrder.flags)) {
	// synthetic cancel/replace in process
	if (MxTEventState::matchQSP(cancel.eventState))
	  goto cancelSent; // cancel already sent
      } else
	newOrder.flags |= (1U<<MxTOrderFlags::C);
      order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
      cancel.update(newOrder);
      sendCancel(order);
cancelSent:
      {
	Msg<Hold> out(in.template reject<ModHeld>());
	sendHold(order, out);
      }
      return;
    }
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modFiltered(
      Order *order, In &in) {
    Msg<ModReject> out(in.template reject<ModFiltered>());
    sendModReject(order, out);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modFilteredCxl(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (MxTEventState::matchXC(newOrder.eventState) ||
	MxTEventState::matchDQSP(cancel.eventState)) {
      // order closed, or cancel already in progress; just reject modify
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      Msg<ModReject> out(in.template reject<ModFiltered>());
      sendModReject(order, out);
      return;
    }
    if (MxTEventState::matchD(modify.eventState) ||
	(!MxTEventState::matchA(newOrder.eventState) &&
	 MxTEventState::matchQ(modify.eventState))) {
      // preceding modify in progress, but queued or deferred
      modify.eventState = MxTEventState::Deferred;
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order)))
	cancel.eventState = MxTEventState::Deferred;
      else
	sendCancel(order);
      {
	// reject latest modify
	Msg<ModReject> out(in.template reject<ModFiltered>());
	sendModReject(order, out);
      }
      {
	// reject preceding modify that was in progress
	Msg<ModReject> out(order->modifyMsg.template data<Modify>());
	modify.eventState = MxTEventState::Rejected;
	sendModReject(order, out);
      }
      return;
    }
    if (MxTEventState::matchSP(modify.eventState)) {
      // preceding modify in progress, sent to market (or pending fill)
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
      cancel.update(newOrder);
      if (ZuUnlikely(
	    (MxTEventState::matchS(newOrder.eventState) ||
	     MxTEventState::matchS(modify.eventState)) &&
	    !app()->asyncCxl(order)))
	cancel.eventState = MxTEventState::Deferred;
      else
	sendCancel(order);
      {
	// reject latest modify
	Msg<ModReject> out(in.template reject<ModFiltered>());
	sendModReject(order, out);
      }
      return;
    }
    if (MxTEventState::matchHQ(newOrder.eventState)) {
      // new order held/queued, modify-on-queue (now cancel-on-queue)
      newOrder.eventState = MxTEventState::Closed;
      {
	// inform client that order is canceled
	Msg<Canceled> out;
	Canceled &canceled = out.initCanceled(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn<Canceled, Msg<Canceled> >(order, out);
	sendCanceled(order, out);
      }
      {
	// reject modify
	Msg<ModReject> out(in.template reject<ModFiltered>());
	sendModReject(order, out);
      }
      return;
    }
    newOrder.flags &= ~(1U<<MxTOrderFlags::C);
    order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
    requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
    cancel.update(newOrder);
    if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	  !app()->asyncCxl(order)))
      cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
    else
      sendCancel(order);
    {
      // reject modify
      Msg<ModReject> out(in.template reject<ModFiltered>());
      sendModReject(order, out);
    }
  }

private:
  // synthetic order ack
  void ordered_(Order *order) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    Msg<Ordered> out;
    out.initOrdered((1U<<MxTEventFlags::Synthesized));
    executionIn<Ordered, Msg<Ordered> >(order, out);
    sendOrdered(order, out);
  }

  // apply unsolicited Modified (aka Restated)
  template <typename In> void applyRestated(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.update(in.template as<Modified>());
    Msg<Modified> out;
    Modified &modified = out.initModified((1U<<MxTEventFlags::Unsolicited));
    modified.update(newOrder);
    sendModified(order, out);
  }

public:
  template <typename In> typename ZuIsBase<Msg_, In>::T modified(
      Order *order, In &in) {
    executionIn<Modified, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    Modified &modified = in.template as<Modified>();
    if (MxTEventState::matchSA(newOrder.eventState) &&
	MxTEventState::matchDQS(modify.eventState)) {
      // usual case
      if (MxTEventState::matchS(newOrder.eventState))
	ordered_(order); // unacknowledged order needs synthetic ack
      modify.update(modified); // update with result
      if (newOrder.pending(modify)) {
	modify.eventState = MxTEventState::PendingFill;
	modify.ackFlags = modified.eventFlags;
      } else {
	modify.eventState = MxTEventState::Acknowledged;
	newOrder.update(modify);
	Msg<Modified> out;
	Modified &outModified = out.initModified(modified.eventFlags);
	outModified.update(newOrder);
	sendModified(order, out);
      }
      if (MxTEventState::matchD(cancel.eventState))
	sendCancel(order);
      return;
    }
    // unsolicited (i.e. unilaterally restated by broker/market)
    in.template as<Event>().unsolicited_set();
    if (MxTEventState::matchAC(newOrder.eventState)) {
      // usual case
      applyRestated(order, in);
      return;
    }
    if (!MxTOrderFlags::matchC(newOrder.flags) &&
	MxTEventState::matchHQS(newOrder.eventState)) {
      // unacknowledged order
      ordered_(order);
      applyRestated(order, in);
      if (ZuUnlikely(MxTEventState::matchD(cancel.eventState)))
	sendCancel(order);
      else if (ZuUnlikely(MxTEventState::matchD(modify.eventState)))
	sendModify(order);
      return;
    }
    if (MxTOrderFlags::matchC(newOrder.flags) &&
	MxTEventState::matchS(newOrder.eventState) &&
	MxTEventState::matchHD(modify.eventState) &&
	MxTEventState::matchDQSPX(cancel.eventState)) {
      // unacknowledged order with held or deferred modify
      ordered_(order);
      applyRestated(order, in);
      if (ZuUnlikely(MxTEventState::matchDX(cancel.eventState)))
	sendCancel(order);
      return;
    }
    // abnormal
    app()->abnormal(order, in);
    applyRestated(order, in);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modReject(
      Order *order, In &in) {
    executionIn<ModReject, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      modify.eventState = MxTEventState::Rejected;
      Msg<ModReject> out(in.template data<ModReject>());
      sendModReject(order, out);
      if (MxTEventState::matchD(cancel.eventState))
	sendCancel(order);
      return;
    }
    if (MxTEventState::matchXC(newOrder.eventState) &&
	MxTEventState::matchSP(modify.eventState)) {
      modify.eventState = MxTEventState::Rejected;
      return;
    }
    app()->abnormal(order, in);
    modify.eventState = MxTEventState::Rejected;
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T modRejectCxl(
      Order *order, In &in) {
    executionIn<ModReject, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      modify.eventState = MxTEventState::Rejected;
      Msg<ModReject> out(in.template data<ModReject>());
      sendModReject(order, out);
      // do not need to cancel if order already filled
      if (ZuLikely(!newOrder.filled())) {
	if (MxTEventState::matchQSP(cancel.eventState)) return;
	if (!MxTEventState::matchD(cancel.eventState)) {
	  order->cancelMsg.initCancel(
	    (1U<<MxTEventFlags::Synthesized));
	  requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
	  cancel.update(newOrder);
	}
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred;
	else
	  sendCancel(order);
      }
      return;
    }
    if (MxTEventState::matchXC(newOrder.eventState) &&
	MxTEventState::matchSP(modify.eventState)) {
      modify.eventState = MxTEventState::Rejected;
      return;
    }
    app()->abnormal(order, in);
    modify.eventState = MxTEventState::Rejected;
  }

private:
  template <typename In> MxEnum filterCancel(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState)))
      return MxTRejReason::OrderClosed;
    if (ZuUnlikely(MxTEventState::matchDQSP(cancel.eventState))) {
      if (!MxTOrderFlags::matchC(newOrder.flags))
	return MxTRejReason::CancelPending;
      else
	return MxTRejReason::ModifyPending;
    }
    return MxTRejReason::OK;
  }

public:
  // app calls cancelIn followed by one of cancel cxlFiltered
  // returns MxTRejReason::OK, or reason if cancel is rejected due to OSM
  template <typename In> typename ZuIsBase<Msg_, In, MxEnum>::T cancelIn(
      Order *order, In &in) {
    requestIn<Cancel, In>(order, in);
    return filterCancel(order, in);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T cancel(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHDQSPA(modify.eventState))) {
      // usual case
      if (ZuUnlikely(MxTEventState::matchHDQ(modify.eventState)))
	modReject_(order, order->modifyMsg, MxTRejReason::OrderClosed);
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelMsg = in.template data<Cancel>();
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order))) {
	// async. cancel disabled
	cancel.eventState = MxTEventState::Deferred;
	return;
      }
      sendCancel(order);
    }
    if (MxTEventState::matchHQ(newOrder.eventState)) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::M);
      newOrder.eventState = MxTEventState::Closed;
      {
	// inform client that order is canceled
	Msg<Canceled> out;
	Canceled &canceled = out.initCanceled(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn<Canceled, Msg<Canceled> >(order, out);
	sendCanceled(order, out);
      }
      return;
    }
    // should never happen; somehow this point was reached despite
    // filterCancel() previously returning OK when app called cancelIn()
    MxEnum rejReason = filterCancel(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    cxlReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T cxlFiltered(
      Order *order, In &in) {
    Msg<CxlReject> out(in.template reject<CxlFiltered>());
    sendCxlReject(order, out);
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T canceled(
      Order *order, In &in) {
    executionIn<Canceled, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    Canceled &canceled = in.template as<Canceled>();
    if (ZuLikely(
	  !MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchHQSA(newOrder.eventState))) {
      // usual case
      if (MxTEventState::matchHQS(newOrder.eventState))
	ordered_(order);
      if (MxTEventState::matchDQ(modify.eventState))
	modReject_(order, order->modifyMsg, MxTRejReason::OrderClosed);
      if (ZuUnlikely(!MxTEventState::matchDQS(cancel.eventState))) {
	// unsolicited
	in.template as<Event>().unsolicited_set();
	newOrder.eventState = MxTEventState::Closed;
	Msg<Canceled> out = in.template data<Canceled>();
	sendCanceled(order, out);
	return;
      }
      cancel.update(canceled);
      if (newOrder.pending(cancel)) {
	// pending fill
	cancel.eventState = MxTEventState::PendingFill;
	cancel.ackFlags = canceled.eventFlags;
	return;
      }
      cancel.eventState = MxTEventState::Acknowledged;
      newOrder.eventState = MxTEventState::Closed;
      Msg<Canceled> out = in.template data<Canceled>();
      sendCanceled(order, out);
      return;
    }
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState))) {
      // late - order already closed
      if (MxTEventState::matchDQS(cancel.eventState))
	cancel.eventState = MxTEventState::Acknowledged;
      return;
    }
    if (ZuLikely(
	  MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState))) {
      // modify held or deferred (simulated by cancel/replace)
      if (MxTEventState::matchS(newOrder.eventState))
	ordered_(order);
      if (MxTEventState::matchDQS(cancel.eventState)) {
	cancel.update(canceled);
	if (newOrder.pending(cancel)) {
	  // pending fill
	  cancel.eventState = MxTEventState::PendingFill;
	  cancel.ackFlags = canceled.eventFlags;
	  return;
	}
	cancel.eventState = MxTEventState::Acknowledged;
      }
      newOrder.flags =
	(newOrder.flags & ~(1U<<MxTOrderFlags::C)) | (1U<<MxTOrderFlags::M);
      newOrder.update(modify);
      if (MxTEventState::matchH(modify.eventState)) {
	modify.eventState = MxTEventState::Unset;
	newOrder.eventState = MxTEventState::Held;
	return;
      }
      modify.eventState = MxTEventState::Unset;
      if (newOrder.filled()) {
	// modify may have reduced qty so that order is now filled
	Msg<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn<Modified, Msg<Modified> >(order, out);
	modified.update(newOrder);
	sendModified(order, out);
	newOrder.flags &= ~(1U<<MxTOrderFlags::M);
	newOrder.eventState = MxTEventState::Acknowledged;
	return;
      }
      sendNewOrder(order);
      return;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
    if (!MxTEventState::matchXC(newOrder.eventState))
      newOrder.eventState = MxTEventState::Closed;
  }

  template <typename In> typename ZuIsBase<Msg_, In>::T cxlReject(
      Order *order, In &in) {
    executionIn<CxlReject, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchDQS(cancel.eventState))) {
      // usual case
      cancel.eventState = MxTEventState::Rejected;
      if (MxTOrderFlags::matchC(newOrder.flags)) // simulated modify
	return;
      Msg<CxlReject> out = in.template data<CxlReject>();
      sendCxlReject(order, out);
      return;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
  }

  template <typename In> typename ZuIsBase<Msg_, In, bool>::T release(
      Order *order, In &in) {
    in.template as<Event>().eventState = MxTEventState::Received;
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchH(newOrder.eventState))) {
      sendNewOrder(order);
      goto sendRelease;
    }
    if (ZuLikely(MxTEventState::matchH(modify.eventState))) {
      if (ZuLikely(modify.eventType == MxTEventType::Modify)) {
	if (MxTEventState::matchSP(cancel.eventState)) {
	  modify.eventState = MxTEventState::Deferred;
	  goto sendRelease;
	}
	if (MxTEventState::matchDQ(cancel.eventState))
	  cancel.eventState = MxTEventState::Unset;
	newOrder.flags &= ~(1U<<MxTOrderFlags::C);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncMod(order))) {
	  // async. modify disabled
	  modify.eventState = MxTEventState::Deferred;
	  goto sendRelease;
	}
	sendModify(order);
	goto sendRelease;
      } else {
	modify.eventState = MxTEventState::Deferred;
	if (MxTEventState::matchDQSP(cancel.eventState)) goto sendRelease;
	order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order))) {
	  // async. cancel disabled
	  cancel.eventState = MxTEventState::Deferred;
	  goto sendRelease;
	}
	sendCancel(order);
	goto sendRelease;
      }
    }
    // abnormal
    app()->abnormal(order, in);
    return false;
sendRelease:
    Msg<Release> out = in.template data<Release>();
    executionOut<Release, Msg<Release> >(order, out);
    app()->sendRelease(out);
    return true;
  }

  template <typename In> typename ZuIsBase<Msg_, In, bool>::T deny(
      Order *order, In &in) {
    in.template as<Event>().eventState = MxTEventState::Received;
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchH(newOrder.eventState))) {
      newOrder.eventState = MxTEventState::Rejected;
      if (newOrder.flags & (1U<<MxTOrderFlags::M)) {
	newOrder.flags &= ~(1U<<MxTOrderFlags::M);
	goto sendModReject;
      }
      goto sendReject;
    }
    if (ZuLikely(MxTEventState::matchH(modify.eventState))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      modify.eventState = MxTEventState::Rejected;
      if (MxTEventState::matchDQSP(cancel.eventState))
	goto sendModReject;
      order->cancelMsg.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn<Cancel, CancelMsg>(order, order->cancelMsg);
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order))) {
	// async. cancel disabled
	cancel.eventState = MxTEventState::Deferred;
	goto sendModReject;
      }
      sendCancel(order);
      goto sendModReject;
    }
    // abnormal
    app()->abnormal(order, in);
    return false;
sendReject:
    {
      Msg<Reject> out;
      Reject &reject = out.initReject((1U<<MxTEventFlags::Synthesized));
      executionIn<Reject, Msg<Reject> >(order, out);
      memcpy(
	  &static_cast<AnyReject &>(reject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      sendReject(order, out);
      return true;
    }
sendModReject:
    {
      Msg<ModReject> out;
      ModReject &reject = out.initModReject(
	(1U<<MxTEventFlags::Synthesized));
      executionIn<ModReject, Msg<ModReject> >(order, out);
      memcpy(
	  &static_cast<AnyReject &>(reject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      sendModReject(order, out);
      return true;
    }
  }

private:
  void pendingFill(Order *order) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (MxTEventState::matchP(modify.eventState) &&
	!newOrder.pending(modify)) {
      // modify ack pending
      modify.eventState = MxTEventState::Acknowledged;
      newOrder.update(modify);
      Msg<Modified> out;
      Modified &modified = out.initModified(modify.ackFlags);
      modified.update(newOrder);
      sendModified(order, out);
    }
    if (MxTEventState::matchP(cancel.eventState) &&
	!newOrder.pending(cancel)) {
      // cancel ack pending
      newOrder.eventState = MxTEventState::Closed;
      cancel.eventState = MxTEventState::Acknowledged;
      Msg<Canceled> out;
      Canceled &canceled = out.initCanceled(cancel.ackFlags);
      executionIn<Canceled, Msg<Canceled> >(order, out);
      sendCanceled(order, out);
    }
  }

public:
  template <typename In> typename ZuIsBase<Msg_, In>::T fill(
      Order *order, In &in) {
    executionIn<Fill, In>(order, in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchAC(newOrder.eventState)))
      // order already acknowledged / closed
      goto sendFill;
    if (ZuLikely(
	  !MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchHQS(newOrder.eventState))) {
      // order unacknowledged, possible pending cancel or modify
      applyOrdered(order, in);
      if (ZuUnlikely(MxTEventState::matchD(cancel.eventState)))
	sendCancel(order);
      else if (ZuUnlikely(MxTEventState::matchD(modify.eventState)))
	sendModify(order);
      goto sendFill;
    }
    if (ZuLikely(
	  MxTOrderFlags::matchC(newOrder.flags) &&
	  MxTEventState::matchS(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // order unacknowledged, synthetic cancel/replace in process
      applyOrdered(order, in);
      if (ZuUnlikely(MxTEventState::matchDX(cancel.eventState)))
	sendCancel(order);
      goto sendFill;
    }
    // always process fills, regardless of order status
sendFill:
    Msg<Fill> out = in.template data<Fill>();
    executionOut<Fill, Msg<Fill> >(order, out);
    app()->sendFill(out);
    {
      // apply fill to order leg
      Fill &fill = in.template as<Fill>();
#if _NLegs > 1
      unsigned leg_ = !*fill.eventLeg ? 0U : (unsigned)fill.eventLeg;
      if (leg_ >= _NLegs) {
	// fill has been forwarded to client, but cannot be applied to order
	// due to invalid leg
	app()->abnormal(order, in);
	return;
      }
      OrderLeg &leg = newOrder.legs[leg_];
#else
      OrderLeg &leg = newOrder.legs[0];
#endif
      leg.cumQty += fill.lastQty;
      leg.updateLeavesQty();
      leg.grossTradeAmt += (fill.lastQty * fill.lastPx);
    }
    pendingFill(order);
  }

  // closes the order
  template <typename In> typename ZuIsBase<Msg_, In>::T closed(
      Order *order, In &in) {
    executionIn<Closed, In>(order, in);
    Msg<Closed> out = in.template data<Closed>();
    executionOut<Closed, Msg<Closed> >(order, out);
    app()->sendClosed(out);
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Closed;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Msg_, In, bool>::T orderSend(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (ZuUnlikely(!MxTEventState::matchQ(newOrder.eventState))) return false;
    newOrder.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Msg_, In, bool>::T modifySend(
      Order *order, In &in) {
    Modify &modify = order->modify();
    if (ZuUnlikely(!MxTEventState::matchQ(modify.eventState))) return false;
    modify.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Msg_, In, bool>::T cancelSend(
      Order *order, In &in) {
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(!MxTEventState::matchQ(cancel.eventState))) return false;
    cancel.eventState = MxTEventState::Sent;
    return true;
  }
};

#endif /* MxTOrderMgr_HPP */
