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
  // the passed lambda has signature Txn<T> *fn(void *), and
  // should be invoked by the callee, passing the address of the
  // buffer to fill with the message
  void sendNewOrder(Order *);
  template <typename L> void sendOrdered(Order *, L); 
  template <typename L> void sendReject(Order *, L);

  void sendModify(Order *);
  template <typename L> void sendModified(Order *, L);
  template <typename L> void sendModReject(Order *, L);

  void sendCancel(Order *);
  template <typename L> void sendCanceled(Order *, L);
  template <typename L> void sendCxlReject(Order *, L);

  template <typename L> void sendHold(Order *, L);
  template <typename L> void sendRelease(Order *, L);

  template <typename L> void sendFill(Order *, L);

  template <typename L> void sendClosed(Order *, L);
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
  inline static void requestIn(Event &event) {
    event.eventState = MxTEventState::Received;
  }
  inline static void requestOut(Event &event) {
    event.eventState = MxTEventState::Queued;
  }

  inline static void executionIn(Event &event) {
    event.eventState = MxTEventState::Received;
  }
  inline static void executionOut(Event &event) {
    event.eventState = MxTEventState::Sent;
  }

  inline void sendNewOrder(Order *order) {
    requestOut(order->orderTxn.template as<NewOrder>());
    app()->sendNewOrder(order);
  }
  inline void sendModify(Order *order) {
    requestOut(order->modifyTxn.template as<Modify>());
    app()->sendModify(order);
  }
  inline void sendCancel(Order *order) {
    requestOut(order->cancelTxn.template as<Cancel>());
    app()->sendCancel(order);
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
    requestIn(in.template as<NewOrder>());
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
    app()->sendHold(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Hold>(in.template reject<OrderHeld>());
      Hold &hold = out->template as<Hold>();
      hold.update(newOrder);
      executionOut(hold);
      return out;
    });
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T orderFiltered(
      Order *order, In &in) {
    order->orderTxn = in.template request<OrderFiltered>();
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Rejected;
    app()->sendReject(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Reject>(in.template reject<OrderFiltered>());
      Reject &reject = out->template as<Reject>();
      reject.update(newOrder);
      executionOut(reject);
      return out;
    });
  }

private:
  template <typename In> void applyOrdered(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.update(in.template as<Ordered>());
    if (ZuUnlikely(MxTOrderFlags::matchM(newOrder.flags))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::M);
      app()->sendModified(order, [&newOrder](void *ptr) {
	auto out = (Txn<Modified> *)ptr;
	Modified &modified = out->initModified(
	    (1U<<MxTEventFlags::Synthesized));
	modified.update(newOrder);
	executionOut(modified);
	return out;
      });
      return;
    }
    app()->sendOrdered(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Ordered>(in.template data<Ordered>());
      Ordered &ordered = out->template as<Ordered>();
      ordered.update(newOrder);
      executionOut(ordered);
      return out;
    });
  }
public:
  template <typename In> typename ZuIsBase<Txn_, In>::T ordered(
      Order *order, In &in) {
    executionIn(in.template as<Ordered>());
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
    Modify &modify = in.template as<Modify>();
    app()->sendModReject(order, [reason, &modify](void *ptr) {
      auto out = (Txn<ModReject> *)ptr;
      ModReject &modReject = out->initModReject(
	  (1U<<MxTEventFlags::Synthesized));
      modReject.rejReason = reason;
      modReject.update(modify);
      executionOut(modReject);
      return out;
    });
  }
  // synthetic cancel reject
  template <typename In> void cxlReject_(Order *order, In &in, int reason) {
    in.template as<Event>.eventState = MxTEventState::Rejected;
    Cancel &cancel = in.template as<Cancel>();
    app()->sendCxlReject(order, [reason, &cancel](void *ptr) {
      auto out = (Txn<CxlReject> *)ptr;
      CxlReject &cxlReject = out->initCxlReject(
	  (1U<<MxTEventFlags::Synthesized));
      cxlReject.rejReason = reason;
      cxlReject.update(cancel);
      executionOut(cxlReject);
      return out;
    });
  }

  template <typename In> void applyReject(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (!MxTEventState::matchXC(newOrder.eventState))
      newOrder.eventState = MxTEventState::Rejected;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    app()->sendReject(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Reject>(in.template data<Reject>());
      Reject &reject = out->template as<Reject>();
      reject.update(newOrder);
      executionOut(reject);
      return out;
    });
  }
public:
  template <typename In> typename ZuIsBase<Txn_, In>::T reject(
      Order *order, In &in) {
    executionIn(in.template as<Reject>());
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
    requestIn(in.template as<Modify>());
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
	  app()->sendOrdered(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Ordered> *)ptr;
	    Ordered &ordered = out->initOrdered(
	      (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	    ordered.update(newOrder);
	    executionOut(ordered);
	    return out;
	  });
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	app()->sendModified(order, [&newOrder](void *ptr) {
	  auto out = (Txn<Modified> *)ptr;
	  Modified &modified = out->initModified(
	      (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	  modified.update(newOrder);
	  executionOut(modified);
	  return out;
	});
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  app()->sendModified(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Modified> *)ptr;
	    Modified &modified = out->initModified(
		(1U<<MxTEventFlags::Synthesized));
	    modified.update(newOrder);
	    executionOut(modified);
	    return out;
	  });
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
      app()->sendModified(order, [&newOrder, &modify](void *ptr) {
	auto out = (Txn<Modified> *)ptr;
	Modified &modified = out->initModified(
	    (1U<<MxTEventFlags::Synthesized) | (1U<<MxTEventFlags::Pending));
	modified.update(newOrder);
	modified.update(modify);
	executionOut(modified);
	return out;
      });
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
	  app()->sendOrdered(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Ordered> *)ptr;
	    Ordered &ordered = out->initOrdered(
	      (1U<<MxTEventFlags::Synthesized));
	    ordered.update(newOrder);
	    executionOut(ordered);
	    return out;
	  });
	  newOrder.update(in.template as<Modify>());
	  sendNewOrder(order);
	  return;
	}
	// new order is synthetic, following a previous modify-on-queue
	app()->sendModified(order, [&newOrder](void *ptr) {
	  auto out = (Txn<Modified> *)ptr;
	  Modified &modified = out->initModified(
	      (1U<<MxTEventFlags::Synthesized));
	  modified.update(newOrder);
	  executionOut(modified);
	  return out;
	});
	newOrder.update(in.template as<Modify>());
	if (newOrder.filled()) {
	  // modify-on-queue may have reduced qty so that order is now filled
	  app()->sendModified(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Modified> *)ptr;
	    Modified &modified = out->initModified(
		(1U<<MxTEventFlags::Synthesized));
	    modified.update(newOrder);
	    executionOut(modified);
	    return out;
	  });
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
      app()->sendModified(order, [&newOrder, &modify](void *ptr) {
	auto out = (Txn<Modified> *)ptr;
	Modified &modified = out->initModified(
	    (1U<<MxTEventFlags::Synthesized));
	modified.update(newOrder);
	modified.update(modify);
	executionOut(modified);
	return out;
      });
      modify.update(in.template as<Modify>());
      modify.eventState = MxTEventState::Deferred;
      if (MxTOrderFlags::matchC(newOrder.flags)) {
	// synthetic cancel/replace in process
	if (MxTEventState::matchQSP(cancel.eventState))
	  return; // cancel already sent
      } else
	newOrder.flags |= (1U<<MxTOrderFlags::C);
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
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
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
	else
	  sendCancel(order);
	order->modifyTxn = in.template request<ModHeld>();
	modify.eventState = MxTEventState::Held;
	app()->sendHold(order, [&in, &modify](void *ptr) {
	  auto out = new (ptr) Txn<Hold>(in.template reject<ModHeld>());
	  Hold &hold = out->template as<Hold>();
	  hold.update(modify);
	  executionOut(hold);
	  return out;
	});
	return;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Received;
	if (!MxTOrderFlags::matchM(newOrder.flags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.flags |= (1U<<MxTOrderFlags::M);
	  app()->sendOrdered(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Ordered> *)ptr;
	    Ordered &ordered = out->initOrdered(
	      (1U<<MxTEventFlags::Synthesized));
	    ordered.update(newOrder);
	    executionOut(ordered);
	    return out;
	  });
	} else {
	  // new order is synthetic, following a previous modify-on-queue
	  app()->sendModified(order, [&newOrder](void *ptr) {
	    auto out = (Txn<Modified> *)ptr;
	    Modified &modified = out->initModified(
		(1U<<MxTEventFlags::Synthesized));
	    modified.update(newOrder);
	    executionOut(modified);
	    return out;
	  });
	}
	newOrder.update(in.template request<ModHeld>());
	newOrder.eventState = MxTEventState::Held;
	app()->sendHold(order, [&in, &newOrder](void *ptr) {
	  auto out = new (ptr) Txn<Hold>(in.template reject<ModHeld>());
	  Hold &hold = out->template as<Hold>();
	  hold.update(newOrder);
	  executionOut(hold);
	  return out;
	});
	return;
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      app()->sendModified(order, [&newOrder, &modify](void *ptr) {
	auto out = (Txn<Modified> *)ptr;
	Modified &modified = out->initModified(
	    (1U<<MxTEventFlags::Synthesized));
	modified.update(newOrder);
	modified.update(modify);
	executionOut(modified);
	return out;
      });
      modify.update(in.template request<ModHeld>());
      modify.eventState = MxTEventState::Held;
      if (MxTOrderFlags::matchC(newOrder.flags)) {
	// synthetic cancel/replace in process
	if (MxTEventState::matchQSP(cancel.eventState))
	  goto cancelSent; // cancel already sent
      } else
	newOrder.flags |= (1U<<MxTOrderFlags::C);
      order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
      cancel.update(newOrder);
      sendCancel(order);
cancelSent:
      app()->sendHold(order, [&in, &modify](void *ptr) {
	auto out = new (ptr) Txn<Hold>(in.template reject<ModHeld>());
	Hold &hold = out->template as<Hold>();
	hold.update(modify);
	executionOut(hold);
	return out;
      });
      return;
    }
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    modReject_(order, in, rejReason);
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T modFiltered(
      Order *order, In &in) {
    app()->sendModReject(order, [&in](void *ptr) {
      auto out = new (ptr) Txn<ModReject>(in.template reject<ModFiltered>());
      ModReject &modReject = out->template as<ModReject>();
      modReject.update(in.template as<Modify>());
      executionOut(modReject);
      return out;
    });
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
      // inform client that order is canceled
      app()->sendCanceled(order, [&newOrder](void *ptr) {
	auto out = (Txn<Canceled> *)ptr;
	Canceled &canceled = out->initCanceled(
	    (1U<<MxTEventFlags::Synthesized));
	canceled.update(newOrder);
	executionOut(canceled);
	return out;
      });
      goto sendModReject;
    }
    newOrder.flags &= ~(1U<<MxTOrderFlags::C);
    order->cancelTxn.initCancel((1U<<MxTEventFlags::Synthesized));
    cancel.update(newOrder);
    if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	  !app()->asyncCxl(order)))
      cancel.eventState = MxTEventState::Deferred; // async. cancel disabled
    else
      sendCancel(order);
sendModReject:
    app()->sendModReject(order, [&in](void *ptr) {
      auto out = new (ptr) Txn<ModReject>(in.template reject<ModFiltered>());
      ModReject &modReject = out->template as<ModReject>();
      modReject.update(in.template as<Modify>());
      executionOut(modReject);
      return out;
    });
  }

private:
  // synthetic order ack
  void ordered_(Order *order) {
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.flags &= ~(1U<<MxTOrderFlags::M);
    app()->sendOrdered(order, [&newOrder](void *ptr) {
      auto out = (Txn<Ordered> *)ptr;
      Ordered &ordered = out->initOrdered(
	(1U<<MxTEventFlags::Synthesized));
      ordered.update(newOrder);
      executionOut(ordered);
      return out;
    });
  }

  // apply unsolicited Modified (aka Restated)
  template <typename In> void applyRestated(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    newOrder.update(in.template as<Modified>());
    app()->sendModified(order, [&newOrder](void *ptr) {
      auto out = (Txn<Modified> *)ptr;
      Modified &modified = out->initModified(
	  (1U<<MxTEventFlags::Unsolicited));
      modified.update(newOrder);
      executionOut(modified);
      return out;
    });
  }

public:
  template <typename In> typename ZuIsBase<Txn_, In>::T modified(
      Order *order, In &in) {
    Modified &modified = in.template as<Modified>();
    executionIn(modified);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
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
	app()->sendModified(order, [&in, &newOrder](void *ptr) {
	  auto out = new (ptr) Txn<Modified>(in.template data<Modified>());
	  Modified &modified = out->template as<Modified>();
	  modified.update(newOrder);
	  executionOut(modified);
	  return out;
	});
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
    executionIn(in.template as<ModReject>());
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      modify.eventState = MxTEventState::Rejected;
      app()->sendModReject(order, [&in](void *ptr) {
	auto out = new (ptr) Txn<ModReject>(in.template data<ModReject>());
	ModReject &modReject = out->template as<ModReject>();
	modReject.update(modify);
	executionOut(modReject);
	return out;
      });
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
    executionIn(in.template as<ModReject>());
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      newOrder.flags &= ~(1U<<MxTOrderFlags::C);
      modify.eventState = MxTEventState::Rejected;
      app()->sendModReject(order, [&in, &modify](void *ptr) {
	auto out = new (ptr) Txn<ModReject>(in.template data<ModReject>());
	ModReject &modReject = out->template as<ModReject>();
	modReject.update(modify);
	executionOut(modReject);
	return out;
      });
      // do not need to cancel if order already filled
      if (ZuLikely(!newOrder.filled())) {
	if (MxTEventState::matchQSP(cancel.eventState)) return;
	if (!MxTEventState::matchD(cancel.eventState)) {
	  order->cancelTxn.initCancel(
	    (1U<<MxTEventFlags::Synthesized));
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
    requestIn(in.template as<Cancel>());
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
      // inform client that order is canceled
      app()->sendCanceled(order, [&newOrder](void *ptr) {
	auto out = (Txn<Canceled> *)ptr;
	Canceled &canceled = out->initCanceled(
	    (1U<<MxTEventFlags::Synthesized));
	canceled.update(newOrder);
	executionOut(canceled);
	return out;
      });
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
    app()->sendCxlReject(order, [&in](void *ptr) {
      auto out = new (ptr) Txn<CxlReject>(in.template reject<CxlFiltered>());
      CxlReject &cxlReject = out->template as<CxlReject>();
      cxlReject.update(in.template as<Cancel>());
      executionOut(cxlReject);
      return out;
    });
  }

  template <typename In> typename ZuIsBase<Txn_, In>::T canceled(
      Order *order, In &in) {
    Canceled &canceled = in.template as<Canceled>();
    executionIn(canceled);
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
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
	app()->sendCanceled(order, [&in, &newOrder](void *ptr) {
	  auto out = new (ptr) Txn<Canceled>(in.template data<Canceled>());
	  Canceled &canceled = out->template as<Canceled>();
	  canceled.update(newOrder);
	  executionOut(canceled);
	  return out;
	});
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
      app()->sendCanceled(order, [&in, &newOrder](void *ptr) {
	auto out = new (ptr) Txn<Canceled>(in.template data<Canceled>());
	Canceled &canceled = out->template as<Canceled>();
	canceled.update(cancel);
	executionOut(canceled);
	return out;
      });
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
	app()->sendModified(order, [&newOrder](void *ptr) {
	  auto out = (Txn<Modified> *)ptr;
	  Modified &modified = out->initModified(
	      (1U<<MxTEventFlags::Synthesized));
	  modified.update(newOrder);
	  executionOut(modified);
	  return out;
	});
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
    executionIn(in.template as<CxlReject>());
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchDQS(cancel.eventState))) {
      // usual case
      cancel.eventState = MxTEventState::Rejected;
      if (MxTOrderFlags::matchC(newOrder.flags)) // simulated modify
	return;
      app()->sendCxlReject(order, [&in, &cancel](void *ptr) {
	auto out = new (ptr) Txn<CxlReject>(in.template data<CxlReject>());
	CxlReject &cxlReject = out->template as<CxlReject>();
	cxlReject.update(cancel);
	executionOut(cxlReject);
	return out;
      });
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
    app()->sendRelease(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Release>(in.template data<Release>());
      Release &release = out->template as<Release>();
      release.update(newOrder);
      executionOut(release);
      return out;
    });
    return true;
sendModRelease:
    app()->sendRelease(order, [&in, &modify](void *ptr) {
      auto out = new (ptr) Txn<Release>(in.template data<Release>());
      Release &release = out->template as<Release>();
      release.update(modify);
      executionOut(release);
      return out;
    });
    return true;
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
    app()->sendReject(order, [&in, &newOrder](void *ptr) {
      auto out = (Txn<Reject> *)ptr;
      Reject &reject = out->initReject((1U<<MxTEventFlags::Synthesized));
      memcpy(
	  &static_cast<AnyReject &>(reject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      reject.update(newOrder);
      executionOut(reject);
    });
sendModReject:
    app()->sendModReject(order, [&in, &modify](void *ptr) {
      auto out = (Txn<ModReject> *)ptr;
      ModReject &modReject = out->initModReject(
	  (1U<<MxTEventFlags::Synthesized));
      memcpy(
	  &static_cast<AnyReject &>(modReject),
	  &static_cast<const AnyReject &>(in.template as<Deny>()),
	  sizeof(AnyReject));
      modReject.update(modify);
      executionOut(modReject);
      return true;
    });
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
      app()->sendModified(order,
	  [eventFlags = modify.ackFlags, &newOrder](void *ptr) {
	auto out = (Txn<Modified> *)ptr;
	Modified &modified = out->initModified(eventFlags);
	modified.update(newOrder);
	executionOut(modified);
	return out;
      });
    }
    if (MxTEventState::matchP(cancel.eventState) &&
	!newOrder.pending(cancel)) {
      // cancel ack pending
      newOrder.eventState = MxTEventState::Closed;
      cancel.eventState = MxTEventState::Acknowledged;
      app()->sendCanceled(order,
	  [eventFlags = cancel.ackFlags, &cancel](void *ptr) {
	auto out = (Txn<Canceled> *)ptr;
	Canceled &canceled = out->initCanceled(eventFlags);
	canceled.update(cancel);
	executionOut(canceled);
	return out;
      });
    }
  }

public:
  template <typename In> typename ZuIsBase<Txn_, In>::T fill(
      Order *order, In &in) {
    Fill &fill = in.template as<Fill>();
    executionIn(fill);
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
    app()->sendFill(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Fill>(in.template data<Fill>());
      Fill &fill = out->template as<Fill>();
      fill.update(newOrder);
      executionOut(out);
      return out;
    });
    {
      // apply fill to order leg
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
    executionIn(in.template as<Closed>());
    NewOrder &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Closed;
    app()->sendClosed(order, [&in, &newOrder](void *ptr) {
      auto out = new (ptr) Txn<Closed>(in.template data<Closed>());
      Closed &closed = out->template as<Closed>();
      closed.update(newOrder);
      executionOut(out);
      return out;
    });
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
