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
struct AppTypes : public MxTAppTypes<AppTypes> {
  typedef MxTAppTypes<AppTypes> Base;

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
struct App : public MxTOrderMgr<App, AppTypes> {
  // log abnormal OSM transition
  template <typename Txn> void abnormal(Order *, const Txn &);

  // returns true if async. modify enabled for order
  bool asyncMod(Order *);
  // returns true if async. cancel enabled for order
  bool asyncCxl(Order *);

  // send*() - send the corresponding message
  template <typename Txn> void sendNewOrder(Order *, Txn &);
  template <typename Txn> void sendOrdered(Order *, Txn &);
  template <typename Txn> void sendReject(Order *, Txn &);

  template <typename Txn> void sendModify(Order *, Txn &);
  template <typename Txn> void sendModified(Order *, Txn &);
  template <typename Txn> void sendModReject(Order *, Txn &);

  template <typename Txn> void sendCancel(Order *, Txn &);
  template <typename Txn> void sendCanceled(Order *, Txn &);
  template <typename Txn> void sendCxlReject(Order *, Txn &);

  template <typename Txn> void sendHold(Order *, Txn &);
  template <typename Txn> void sendRelease(Order *, Txn &);

  template <typename Txn> void sendFill(Order *, Txn &);

  template <typename Txn> void sendClosed(Order *, Txn &);
};
#endif


template <typename App, typename AppTypes>
class MxTOrderMgr : public MxTTxnTypes<AppTypes> {
public:
  typedef MxTTxnTypes<AppTypes> TxnTypes;

  MxTImport(TxnTypes);

  typedef typename TxnTypes::OrderHeld OrderHeld;
  typedef typename TxnTypes::OrderFiltered OrderFiltered;
  typedef typename TxnTypes::ModSimulated ModSimulated;
  typedef typename TxnTypes::ModRejectCxl ModRejectCxl;
  typedef typename TxnTypes::ModHeld ModHeld;
  typedef typename TxnTypes::ModFiltered ModFiltered;
  typedef typename TxnTypes::ModFilteredCxl ModFilteredCxl;
  typedef typename TxnTypes::CxlFiltered CxlFiltered;

  typedef typename TxnTypes::Txn_ Txn_;
  template <typename Largest>
  using Txn = typename TxnTypes::template Txn<Largest>;

  typedef typename TxnTypes::OrderTxn OrderTxn;
  typedef typename TxnTypes::ModifyTxn ModifyTxn;
  typedef typename TxnTypes::CancelTxn CancelTxn;

  typedef typename TxnTypes::AnyTxn AnyTxn;

  typedef typename TxnTypes::Order Order;

private:
  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

public:
  MxTOrderMgr() {
    { // ensure all static initializers are run once
      AnyTxn txn, request;

      txn.initNewOrder(0);
      request = txn.template data<NewOrder>();
      txn.initOrderHeld(request, 0);
      txn.initOrderFiltered(request, 0);
      txn.initOrdered(0);
      txn.initReject(0);

      txn.initModify(0);
      txn.initModSimulated(0);
      request = txn.template data<Modify>();
      txn.initModHeld(request, 0);
      txn.initModFiltered(request, 0);
      txn.initModFilteredCxl(request, 0);
      txn.initModified(0);
      txn.initModReject(0);
      txn.initModRejectCxl(0);

      txn.initCancel(0);
      request = txn.template data<Cancel>();
      txn.initCxlFiltered(request, 0);
      txn.initCanceled(0);
      txn.initCxlReject(0);

      txn.initRelease(0);
      txn.initDeny(0);

      txn.initFill(0);

      txn.initClosed(0);

      txn.initOrderSent(0);
      txn.initModifySent(0);
      txn.initCancelSent(0);
    }
  }

  // order state management

private:
  template <typename In> void requestIn(In &in) {
    Event &event = in.template as<Event>();
    event.eventState = MxTEventState::Received;
  }
  template <typename Out> void requestOut(Out &out) {
    Event &event = out.template as<Event>();
    event.eventState = MxTEventState::Queued;
  }

  template <typename In> void executionIn(In &in) {
    Event &event = in.template as<Event>();
    event.eventState = MxTEventState::Received;
  }
  template <typename Out> void executionOut(Out &out) {
    Event &event = out.template as<Event>();
    event.eventState = MxTEventState::Sent;
  }

  void sendNewOrder(Order *order) {
    requestOut(order->orderTxn);
    // Note: app should use leavesQty since this may be a simulated modify
    // (cancel/replace) of a partially filled order
    app()->sendNewOrder(order, order->orderTxn);
  }
  template <typename Out> inline void sendOrdered(Order *order, L l) {
    app()->sendOrdered(order, ZuMv(l));
  }
  template <typename Out> void sendReject(Order *order, Out &out) {
    executionOut(out);
    app()->sendReject(order, out);
  }
  template <typename Out> void sendHold(Order *order, Out &out) {
    executionOut(out);
    app()->sendHold(order, out);
  }
  void sendModify(Order *order) {
    requestOut(order->modifyTxn);
    app()->sendModify(order, order->modifyTxn);
  }
  template <typename Out> void sendModified(Order *order, Out &out) {
    executionOut(out);
    app()->sendModified(order, out);
  }
  template <typename Out> void sendModReject(Order *order, Out &out) {
    executionOut(out);
    app()->sendModReject(order, out);
  }
  void sendCancel(Order *order) {
    requestOut(order->cancelTxn);
    app()->sendCancel(order, order->cancelTxn);
  }
  template <typename Out> void sendCanceled(Order *order, Out &out) {
    executionOut(out);
    app()->sendCanceled(order, out);
  }
  template <typename Out> void sendCxlReject(Order *order, Out &out) {
    executionOut(out);
    app()->sendCxlReject(order, out);
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
  template <typename In> typename ZuIsBase<Txn_, In>::T newOrderIn(
      Order *order, In &in) {
    requestIn(in);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T newOrder(
      Order *order, In &in) {
    order->orderTxn = in.template data<NewOrder>();
    sendNewOrder(order);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T orderHeld(
      Order *order, In &in) {
    order->orderTxn = in.template request<OrderHeld>();
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Held;
    Txn<Hold> out(in.template reject<OrderHeld>());
    Hold &hold = out.template as<Hold>();
    hold.update(newOrder);
    sendHold(order, out);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T orderFiltered(
      Order *order, In &in) {
    order->orderTxn = in.template request<OrderFiltered>();
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Rejected;
    Txn<Reject> out(in.template reject<OrderFiltered>());
    Reject &reject = out.template as<Reject>();
    reject.update(newOrder);
    sendReject(order, out);
  }

private:
  template <typename In> void applyOrdered(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.update(in.template as<Ordered>());
    if (ZuUnlikely(MxTOrderFlags::matchM(newOrder.flags))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::M);
      Txn<Modified> out;
      Modified &modified = out.initModified((1U<<MxTEventFlags::Synthesized));
      executionIn(out);
      modified.update(newOrder);
      sendModified(order, out);
      return;
    }
    sendOrdered(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Ordered>(in.template data<Ordered>());
      Ordered &ordered = out->template as<Ordered>();
      ordered.update(newOrder);
      executionOut(*out);
      return out;
    });
  }
public:
  template <typename In> typename ZuIsBase<Txn_, In>::T ordered(
      Order *order, In &in) {
    executionIn(in);
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
    Txn<ModReject> out;
    ModReject &modReject = out.initModReject((1U<<MxTEventFlags::Synthesized));
    executionIn(out);
    modReject.rejReason = reason;
    modReject.update(in.template as<Modify>());
    sendModReject(order, out);
  }
  // synthetic cancel reject
  template <typename In> void cxlReject_(Order *order, In &in, int reason) {
    in.template as<Event>.eventState = MxTEventState::Rejected;
    Txn<CxlReject> out;
    CxlReject &cxlReject = out.initCxlReject((1U<<MxTEventFlags::Synthesized));
    executionIn(out);
    cxlReject.rejReason = reason;
    cxlReject.update(in.template as<Cancel>());
    sendCxlReject(order, out);
  }

  template <typename In> void applyReject(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (!MxTEventState::matchXC(newOrder.eventState))
      newOrder.eventState = MxTEventState::Rejected;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    Txn<Reject> out(in.template data<Reject>());
    Reject &reject = out.template as<Reject>();
    reject.update(newOrder);
    sendReject(order, out);
  }
public:
  template <typename In> typename ZuIsBase<Txn_, In>::T reject(
      Order *order, In &in) {
    executionIn(in);
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
	cxlReject_(order, order->cancelTxn, MxTRejReason::OrderClosed);
      else if (ZuUnlikely(MxTEventState::matchDQ(modify.eventState)))
	modReject_(order, order->modifyTxn, MxTRejReason::OrderClosed);
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
      modReject_(order, order->modifyTxn, MxTRejReason::OrderClosed);
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
  template <typename In> typename ZuIsBase<Txn_, In, MxEnum>::T modifyIn(
      Order *order, In &in) {
    requestIn(in);
    return filterModify(order, in);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T modify(
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
	order->modifyTxn = in.template data<Modify>();
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
	  Txn<Ordered> out;
	  Ordered &ordered = out.initOrdered(
	    (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	  executionIn(out);
	  ordered.update(newOrder);
	  sendOrdered(order, out);
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	Txn<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	executionIn(out);
	modified.update(newOrder);
	sendModified(order, out);
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  Txn<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn(out);
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
      Txn<Modified> out;
      Modified &modified = out.initModified(
	(1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
      executionIn(out);
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

  template <typename In> typename ZuIsBase<Txn_, In>::T modSimulated(
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
	order->modifyTxn = in.template data<Modify>();
	modify.eventState = MxTEventState::Deferred;
	order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn(order->cancelTxn);
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
	  Txn<Ordered> out;
	  Ordered &ordered = out.initOrdered((1U<<MxTEventFlags::Synthesized));
	  executionIn(out);
	  ordered.update(newOrder);
	  sendOrdered(order, out);
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	Txn<Modified> out;
	Modified &modified = out.initModified((1U<<MxTEventFlags::Synthesized));
	executionIn(out);
	modified.update(newOrder);
	sendModified(order, out);
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  Txn<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn(out);
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
      Txn<Modified> out;
      Modified &modified = out.initModified((1U<<MxTEventFlags::Synthesized));
      executionIn(out);
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
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn(order->cancelTxn);
      cancel.update(newOrder);
      sendCancel(order);
      return;
    }
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T modHeld(
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
	order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn(order->cancelTxn);
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
	else
	  sendCancel(order);
	order->modifyTxn = in.template request<ModHeld>();
	modify.eventState = MxTEventState::Held;
	Txn<Hold> out(in.template reject<ModHeld>());
	Hold &hold = out.template as<Hold>();
	hold.update(modify);
	sendHold(order, out);
	return;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Received;
	if (!MxTOrderFlags::matchM(newOrder.flags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.flags |= (1U<<MxTOrderFlags::M);
	  Txn<Ordered> out;
	  Ordered &ordered = out.initOrdered((1U<<MxTEventFlags::Synthesized));
	  executionIn(out);
	  ordered.update(newOrder);
	  sendOrdered(order, out);
	} else {
	  // new order is synthetic, following a previous modify-on-queue
	  Txn<Modified> out;
	  Modified &modified = out.initModified(
	    (1U<<MxTEventFlags::Synthesized));
	  executionIn(out);
	  modified.update(newOrder);
	  sendModified(order, out);
	}
	newOrder.update(in.template request<ModHeld>());
	newOrder.eventState = MxTEventState::Held;
	Txn<Hold> out(in.template reject<ModHeld>());
	Hold &hold = out.template as<Hold>();
	hold.update(newOrder);
	sendHold(order, out);
	return;
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      {
	Txn<Modified> out;
	Modified &modified = out.initModified(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn(out);
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
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn(order->cancelTxn);
      cancel.update(newOrder);
      sendCancel(order);
cancelSent:
      {
	Txn<Hold> out(in.template reject<ModHeld>());
	Hold &hold = out.template as<Hold>();
	hold.update(modify);
	sendHold(order, out);
      }
      return;
    }
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T modFiltered(
      Order *order, In &in) {
    Txn<ModReject> out(in.template reject<ModFiltered>());
    ModReject &modReject = out.template as<ModReject>();
    modReject.update(in.template as<Modify>());
    sendModReject(order, out);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T modFilteredCxl(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (MxTEventState::matchXC(newOrder.eventState) ||
	MxTEventState::matchDQSP(cancel.eventState)) {
      // order closed, or cancel already in progress; just reject modify
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      goto sendModReject;
    }
    if (MxTEventState::matchD(modify.eventState) ||
	(!MxTEventState::matchA(newOrder.eventState) &&
	 MxTEventState::matchQ(modify.eventState))) {
      // preceding modify in progress, but queued or deferred
      modify.eventState = MxTEventState::Deferred;
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn(order->cancelTxn);
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order)))
	cancel.eventState = MxTEventState::Deferred;
      else
	sendCancel(order);
      // reject preceding modify that was in progress
      modReject_(order, order->modifyTxn, MxTRejReason::BrokerReject);
      goto sendModReject;
    }
    if (MxTEventState::matchSP(modify.eventState)) {
      // preceding modify in progress, sent to market (or pending fill)
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn(order->cancelTxn);
      cancel.update(newOrder);
      if (ZuUnlikely(
	    (MxTEventState::matchS(newOrder.eventState) ||
	     MxTEventState::matchS(modify.eventState)) &&
	    !app()->asyncCxl(order)))
	cancel.eventState = MxTEventState::Deferred;
      else
	sendCancel(order);
      goto sendModReject;
    }
    if (MxTEventState::matchHQ(newOrder.eventState)) {
      // new order held/queued, modify-on-queue (now cancel-on-queue)
      newOrder.eventState = MxTEventState::Closed;
      {
	// inform client that order is canceled
	Txn<Canceled> out;
	Canceled &canceled = out.initCanceled((1U<<MxTEventFlags::Synthesized));
	executionIn(out);
	canceled.update(newOrder);
	sendCanceled(order, out);
      }
      goto sendModReject;
    }
    newOrder.flags &= ~(1U<<MxTOrderFlags::C);
    order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
    requestIn(order->cancelTxn);
    cancel.update(newOrder);
    if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	  !app()->asyncCxl(order)))
      cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
    else
      sendCancel(order);
sendModReject:
    Txn<ModReject> out(in.template reject<ModFiltered>());
    ModReject &modReject = out.template as<ModReject>();
    modReject.update(in.template as<Modify>());
    sendModReject(order, out);
  }

private:
  // synthetic order ack
  void ordered_(Order *order) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    Txn<Ordered> out;
    Ordered &ordered = out.initOrdered((1U<<MxTEventFlags::Synthesized));
    executionIn(out);
    ordered.update(newOrder);
    sendOrdered(order, out);
  }

  // apply unsolicited Modified (aka Restated)
  template <typename In> void applyRestated(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.update(in.template as<Modified>());
    Txn<Modified> out;
    Modified &modified = out.initModified((1U<<MxTEventFlags::Unsolicited));
    executionIn(out);
    modified.update(newOrder);
    sendModified(order, out);
  }

public:
  template <typename In> typename ZuIsBase<Txn_, In>::T modified(
      Order *order, In &in) {
    executionIn(in);
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
	Txn<Modified> out;
	Modified &outModified = out.initModified(modified.eventFlags);
	executionIn(out);
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

  template <typename In> typename ZuIsBase<Txn_, In>::T modReject(
      Order *order, In &in) {
    executionIn(in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      modify.eventState = MxTEventState::Rejected;
      Txn<ModReject> out(in.template data<ModReject>());
      ModReject &modReject = out.template as<ModReject>();
      modReject.update(modify);
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

  template <typename In> typename ZuIsBase<Txn_, In>::T modRejectCxl(
      Order *order, In &in) {
    executionIn(in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      modify.eventState = MxTEventState::Rejected;
      Txn<ModReject> out(in.template data<ModReject>());
      ModReject &modReject = out.template as<ModReject>();
      modReject.update(modify);
      sendModReject(order, out);
      // do not need to cancel if order already filled
      if (ZuLikely(!newOrder.filled())) {
	if (MxTEventState::matchQSP(cancel.eventState)) return;
	if (!MxTEventState::matchD(cancel.eventState)) {
	  order->cancelTxn.initCancel(
	    (1U<<MxTEventFlags::Synthesized));
	  requestIn(order->cancelTxn);
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
  template <typename In> typename ZuIsBase<Txn_, In, MxEnum>::T cancelIn(
      Order *order, In &in) {
    requestIn(in);
    return filterCancel(order, in);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T cancel(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHDQSPA(modify.eventState))) {
      // usual case
      if (ZuUnlikely(MxTEventState::matchHDQ(modify.eventState)))
	modReject_(order, order->modifyTxn, MxTRejReason::OrderClosed);
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      order->cancelTxn = in.template data<Cancel>();
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
	Txn<Canceled> out;
	Canceled &canceled = out.initCanceled(
	  (1U<<MxTEventFlags::Synthesized));
	executionIn(out);
	canceled.update(newOrder);
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

  template <typename In> typename ZuIsBase<Txn_, In>::T cxlFiltered(
      Order *order, In &in) {
    Txn<CxlReject> out(in.template reject<CxlFiltered>());
    CxlReject &cxlReject = out.template as<CxlReject>();
    cxlReject.update(in.template as<Cancel>());
    sendCxlReject(order, out);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T canceled(
      Order *order, In &in) {
    executionIn(in);
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
	modReject_(order, order->modifyTxn, MxTRejReason::OrderClosed);
      if (ZuUnlikely(!MxTEventState::matchDQS(cancel.eventState))) {
	// unsolicited
	in.template as<Event>().unsolicited_set();
	newOrder.eventState = MxTEventState::Closed;
	Txn<Canceled> out = in.template data<Canceled>();
	Canceled &canceled = out.template as<Canceled>();
	canceled.update(newOrder);
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
      Txn<Canceled> out = in.template data<Canceled>();
      Canceled &canceled = out.template as<Canceled>();
      canceled.update(cancel);
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
	Txn<Modified> out;
	Modified &modified = out.initModified((1U<<MxTEventFlags::Synthesized));
	executionIn(out);
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

  template <typename In> typename ZuIsBase<Txn_, In>::T cxlReject(
      Order *order, In &in) {
    executionIn(in);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchDQS(cancel.eventState))) {
      // usual case
      cancel.eventState = MxTEventState::Rejected;
      if (MxTOrderFlags::matchC(newOrder.flags)) // simulated modify
	return;
      Txn<CxlReject> out = in.template data<CxlReject>();
      CxlReject &cxlReject = out.template as<CxlReject>();
      cxlReject.update(cancel);
      sendCxlReject(order, out);
      return;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
  }

  template <typename In> typename ZuIsBase<Txn_, In, bool>::T release(
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
	  goto sendModRelease;
	}
	if (MxTEventState::matchDQ(cancel.eventState))
	  cancel.eventState = MxTEventState::Unset;
	newOrder.flags &= ~(1U<<MxTOrderFlags::C);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncMod(order))) {
	  // async. modify disabled
	  modify.eventState = MxTEventState::Deferred;
	  goto sendModRelease;
	}
	sendModify(order);
	goto sendModRelease;
      } else { // simulated modify
	modify.eventState = MxTEventState::Deferred;
	if (MxTEventState::matchDQSP(cancel.eventState)) goto sendRelease;
	order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
	requestIn(order->cancelTxn);
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order))) {
	  // async. cancel disabled
	  cancel.eventState = MxTEventState::Deferred;
	  goto sendModRelease;
	}
	sendCancel(order);
	goto sendModRelease;
      }
    }
    // abnormal
    app()->abnormal(order, in);
    return false;
sendRelease:
    {
      Txn<Release> out = in.template data<Release>();
      executionOut(out);
      Release &release = out.template as<Release>();
      release.update(newOrder);
      app()->sendRelease(out);
      return true;
    }
sendModRelease:
    {
      Txn<Release> out = in.template data<Release>();
      executionOut(out);
      Release &release = out.template as<Release>();
      release.update(modify);
      app()->sendRelease(out);
      return true;
    }
  }

  template <typename In> typename ZuIsBase<Txn_, In, bool>::T deny(
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
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      requestIn(order->cancelTxn);
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
      Txn<Reject> out;
      Reject &reject = out.initReject((1U<<MxTEventFlags::Synthesized));
      executionIn(out);
      memcpy(
	  &static_cast<AnyReject &>(reject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      reject.update(newOrder);
      sendReject(order, out);
      return true;
    }
sendModReject:
    {
      Txn<ModReject> out;
      ModReject &modReject = out.initModReject(
	  (1U<<MxTEventFlags::Synthesized));
      executionIn(out);
      memcpy(
	  &static_cast<AnyReject &>(modReject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      modReject.update(modify);
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
      Txn<Modified> out;
      Modified &modified = out.initModified(modify.ackFlags);
      executionIn(out);
      modified.update(newOrder);
      sendModified(order, out);
    }
    if (MxTEventState::matchP(cancel.eventState) &&
	!newOrder.pending(cancel)) {
      // cancel ack pending
      newOrder.eventState = MxTEventState::Closed;
      cancel.eventState = MxTEventState::Acknowledged;
      Txn<Canceled> out;
      Canceled &canceled = out.initCanceled(cancel.ackFlags);
      executionIn(out);
      canceled.update(cancel);
      sendCanceled(order, out);
    }
  }

public:
  template <typename In> typename ZuIsBase<Txn_, In>::T fill(
      Order *order, In &in) {
    executionIn(in);
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
    Txn<Fill> out = in.template data<Fill>();
    executionOut(out);
    Fill &fill = out.template as<Fill>();
    fill.update(newOrder);
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
  template <typename In> typename ZuIsBase<Txn_, In>::T closed(
      Order *order, In &in) {
    executionIn(in);
    NewOrder &newOrder = order->newOrder();
    Txn<Closed> out = in.template data<Closed>();
    executionOut(out);
    Closed &closed = out.template as<Closed>();
    closed.update(newOrder);
    app()->sendClosed(out);
    newOrder.eventState = MxTEventState::Closed;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Txn_, In, bool>::T orderSend(
      Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (ZuUnlikely(!MxTEventState::matchQ(newOrder.eventState))) return false;
    newOrder.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Txn_, In, bool>::T modifySend(
      Order *order, In &in) {
    Modify &modify = order->modify();
    if (ZuUnlikely(!MxTEventState::matchQ(modify.eventState))) return false;
    modify.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In> typename ZuIsBase<Txn_, In, bool>::T cancelSend(
      Order *order, In &in) {
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(!MxTEventState::matchQ(cancel.eventState))) return false;
    cancel.eventState = MxTEventState::Sent;
    return true;
  }
};

#endif /* MxTOrderMgr_HPP */
