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

  typedef typename TxnTypes::ExecTxn ExecTxn;
  typedef typename TxnTypes::AnyTxn AnyTxn;

  typedef typename TxnTypes::Order Order;

private:
  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

public:
  MxTOrderMgr() {
    { // ensure all static initializers are run once
      AnyTxn txn, request;

      txn.initNewOrder<0>();
      request = txn.template data<NewOrder>();
      txn.initOrderHeld(request, 0);
      txn.initOrderFiltered(request, 0);
      txn.initOrdered<0>();
      txn.initReject<0>();

      txn.initModify<0>();
      txn.initModSimulated<0>();
      request = txn.template data<Modify>();
      txn.initModHeld(request, 0);
      txn.initModFiltered(request, 0);
      txn.initModFilteredCxl(request, 0);
      txn.initModified<0>();
      txn.initModReject<0>();
      txn.initModRejectCxl<0>();

      txn.initCancel<0>();
      request = txn.template data<Cancel>();
      txn.initCxlFiltered(request, 0);
      txn.initCanceled<0>();
      txn.initCxlReject<0>();

      txn.initRelease<0>();
      txn.initDeny<0>();

      txn.initFill<0>();

      txn.initClosed<0>();

      txn.initOrderSent<0>();
      txn.initModifySent<0>();
      txn.initCancelSent<0>();
    }
  }

  // order state management

  // build ack message (Ordered/Modified/Canceled)
  template <typename Out>
  typename ZuIsBase<Txn_, Out>::T ack(Order *order, Out &out) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    auto &ack = order->ack();
    switch ((int)ack.eventType) {
      case MxTEventType::Ordered: {
	auto &ordered = out.initOrdered<0>(
	    ack.eventFlags | (1U<<MxTEventFlags::Tx), ack.eventLeg);
	if (newOrder.ack()) ordered.update(newOrder);
	if (modify.ack()) ordered.update(modify);
	ordered.eventState = MxTEventState::Sent;
	return true;
      }
      case MxTEventType::Modified: {
	auto &modified = out.initModified<0>(
	    ack.eventFlags | (1U<<MxTEventFlags::Tx), ack.eventLeg);
	if (newOrder.ack()) modified.update(newOrder);
	if (modify.ack()) modified.update(modify);
	modified.eventState = MxTEventState::Sent;
	return true;
      }
      case MxTEventType::Canceled: {
	auto &canceled = out.initCanceled<0>(
	    ack.eventFlags | (1U<<MxTEventFlags::Tx), ack.eventLeg);
	if (newOrder.ack()) canceled.update(newOrder);
	if (modify.ack()) canceled.update(modify);
	if (cancel.ack()) canceled.update(cancel);
	canceled.eventState = MxTEventState::Sent;
	return true;
      }
      default:
	return false;
    }
  }

private:
  template <int State>
  inline static void newOrderIn_(Order *order) {
    auto &newOrder = order->newOrder();
    newOrder.eventState = State;
    newOrder.rxtx<1, State == MxTEventState::Queued, 0>();
    // no need to set/clr other rx/tx bits since order is new
    // order->modify().rxtx<0, 0>();
    // order->cancel().rxtx<0, 0>();
    // order->ack().null();
    // order->exec().null();
  }
  template <int State>
  inline static void newOrderIn(Order *order) {
    auto &newOrder = order->newOrder();
    newOrder.eventState = State;
    newOrder.rxtx<1, State == MxTEventState::Queued, 0>();
    order->modify().rxtx<0, 0, 0>();
    order->cancel().rxtx<0, 0, 0>();
    order->ack().null();
    order->exec().null();
  }
  inline static void newOrderOut(Order *order) {
    auto &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Queued;
    newOrder.tx_set();
  }
  template <int State>
  inline static void modifyIn(Order *order) {
    order->newOrder().rxtx<0, 0, 0>();
    auto &modify = order->modify();
    modify.eventState = State;
    modify.rxtx<1, State == MxTEventState::Queued, 0>();
    order->cancel().rxtx<0, 0, 0>();
    order->ack().null();
    order->exec().null();
  }
  inline static void modifyOut(Order *order) {
    auto &modify = order->modify();
    modify.eventState = MxTEventState::Queued;
    modify.tx_set();
  }
  template <int State>
  inline static void cancelIn(Order *order) {
    order->newOrder().rxtx<0, 0, 0>();
    order->modify().rxtx<0, 0, 0>();
    auto &cancel = order->cancel();
    cancel.eventState = State;
    cancel.rxtx<1, State == MxTEventState::Queued, 0>();
    order->ack().null();
    order->exec().null();
  }
  inline static void cancelOut(Order *order) {
    auto &cancel = order->cancel();
    cancel.eventState = MxTEventState::Queued;
    cancel.tx_set();
  }

  template <int Type, int State, bool NewOrder, bool Modify, bool Cancel,
    bool Unsolicited, bool Synthesized, bool Pending>
  inline static void ackIn(Order *order, int leg = 0) {
    order->newOrder().rxtx<0, 0, NewOrder>();
    order->modify().rxtx<0, 0, Modify>();
    order->cancel().rxtx<0, 0, Cancel>();
    auto &ack = order->ack();
    ack.eventType = Type;
    ack.eventState = State;
    ack.eventFlags =
      (1U<<MxTEventFlags::Rx) |
      (((unsigned)(State == MxTEventState::Sent))<<MxTEventFlags::Tx) |
      (((unsigned)Unsolicited)<<MxTEventFlags::Unsolicited) |
      (((unsigned)Synthesized)<<MxTEventFlags::Synthesized) |
      (((unsigned)Pending)<<MxTEventFlags::Pending);
    ack.eventLeg = leg;
    order->exec().null();
  }
  template <int Type, int State, bool NewOrder, bool Modify, bool Cancel,
    bool Unsolicited, bool Synthesized, bool Pending>
  inline static void ackOut(Order *order, int leg = 0) {
    if constexpr (NewOrder) order->newOrder().ack_set();
    if constexpr (Modify) order->modify().ack_set();
    if constexpr (Cancel) order->cancel().ack_set();
    auto &ack = order->ack();
    ack.eventType = Type;
    ack.eventState = State;
    ack.eventFlags =
      (1U<<MxTEventFlags::Rx) |
      (((unsigned)(State == MxTEventState::Sent))<<MxTEventFlags::Tx) |
      (((unsigned)Unsolicited)<<MxTEventFlags::Unsolicited) |
      (((unsigned)Synthesized)<<MxTEventFlags::Synthesized) |
      (((unsigned)Pending)<<MxTEventFlags::Pending);
    ack.eventLeg = leg;
  }

  template <int State>
  inline static void execIn(Order *order) {
    order->newOrder().rxtx<0, 0, 0>();
    order->modify().rxtx<0, 0, 0>();
    order->cancel().rxtx<0, 0, 0>();
    order->ack().null();
    auto &exec = order->exec();
    exec.eventState = State;
    exec.rxtx<1, State == MxTEventState::Sent, 0>();
  }
  inline static void execOut(Order *order) {
    auto &exec = order->exec();
    exec.eventState = MxTEventState::Sent;
    exec.tx_set();
  }

public:
  template <typename In> struct Next { typedef void (*Fn)(Order *, In &); };

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  newOrder(Order *order, In &in) {
    order->orderTxn = in.template data<NewOrder>();
    newOrderIn_<MxTEventState::Queued>(order);
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  orderHeld(Order *order, In &in) {
    order->orderTxn = in.template request<OrderHeld>();
    newOrderIn_<MxTEventState::Held>(order);
    {
      order->execTxn = in.template reject<OrderHeld>();
      auto &hold = order->execTxn.template as<Hold>();
      hold.update(newOrder);
      execOut(order);
    }
    return nullptr;
  }

  // Note: filtered new orders, modifies and cancels occupy standalone
  // MxTOrder instances that are detached from the main OSM sequence
  // (for a filtered new order, the instance is the entire OSM sequence).
  // The origClOrdID remains re-usable for subsequent mod/cxl attempts
  // following a reject, while the clOrdID of the rejected request itself
  // cannot be re-used
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  orderFiltered(Order *order, In &in) {
    order->orderTxn = in.template request<OrderFiltered>();
    newOrderIn_<MxTEventState::Rejected>(order);
    {
      order->execTxn = in.template reject<OrderFiltered>();
      auto &reject = order->execTxn.template as<Reject>();
      reject.update(order->newOrder());
      execOut(order);
    }
    return nullptr;
  }

private:
  template <bool Unsolicited, typename In>
  void ordered_(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    newOrder.update(in.template as<Ordered>());
    unsigned leg = in.template data<Event>().eventLeg;
    if (ZuUnlikely(MxTEventFlags::matchM(newOrder.eventFlags))) {
      newOrder.modifyNew_clr();
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 0, 0, Unsolicited, 0, 0>(order, leg);
    } else
      ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	1, 0, 0, Unsolicited, 0, 0>(order, leg);
  }

public:
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  ordered(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  !MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchHQS(newOrder.eventState))) {
      // usual case - solicited
      ordered_<0>(order, in); // solicited
      if (ZuUnlikely(MxTEventState::matchD(cancel.eventState)))
	cancelOut(order);
      else if (ZuUnlikely(MxTEventState::matchD(modify.eventState)))
	modifyOut(order);
      return;
    }
    if (ZuUnlikely(MxTEventState::matchAC(newOrder.eventState))) {
      // unsolicited - order already acknowledged / closed - sink ack
      auto &ordered = in.template as<Ordered>();
      newOrder.update(ordered);
      ackIn<MxTEventType::Ordered, MxTEventState::Received,
	1, 0, 0, 1, 0, 0>(order, ordered.eventLeg); // OmcUsp
      return;
    }
    if (ZuLikely(
	  MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchS(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // solicited - synthetic cancel/replace in process
      ordered_<0>(order, in); // solicited
      if (ZuUnlikely(MxTEventState::matchDX(cancel.eventState)))
	cancelOut(order);
      return;
    }
    // abnormal
    app()->abnormal(order, in);
    ordered_<1>(order, in); // unsolicited
  }

private:
  // synthetic modify reject
  void modReject_(Order *order, int reason) {
    auto &modify = order->modify();
    modify.eventState = MxTEventState::Rejected;
    auto &modReject = order->execTxn.initModReject<1>();
    modReject.rejReason = reason;
    modReject.update(modify);
  }
  // synthetic cancel reject
  void cxlReject_(Order *order, int reason) {
    auto &cancel = order->cancel();
    cancel.eventState = MxTEventState::Rejected;
    auto &cxlReject = order->execTxn.initCxlReject<1>();
    cxlReject.rejReason = reason;
    cxlReject.update(cancel);
  }

public:
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  reject(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
      auto &newOrder = order->newOrder();
      if (!MxTEventState::matchXC(newOrder.eventState))
	newOrder.eventState = MxTEventState::Rejected;
      newOrder.modifyNew_clr();
      {
	order->execTxn = in.template data<Reject>();
	auto &reject = order->execTxn.template as<Reject>();
	reject.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchAX(newOrder.eventState)))
	  reject.unsolicited_set();
	execIn<MxTEventState::Sent>(order);
      }
      return nullptr;
    };
    if (ZuLikely(
	  !MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchHQSAX(newOrder.eventState))) {
      // usual case
      if (ZuUnlikely(MxTEventState::matchDQ(cancel.eventState))) {
	cxlReject_(order, MxTRejReason::OrderClosed);
	execIn<MxTEventState::Sent>(order);
	return next;
      } else if (ZuUnlikely(MxTEventState::matchDQ(modify.eventState))) {
	modReject_(order, MxTRejReason::OrderClosed);
	execIn<MxTEventState::Sent>(order);
	return next;
      } else {
	return next(order, in);
      }
    }
    if (ZuLikely(
	  MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // simulated modify in process
      if (ZuUnlikely(MxTEventState::matchDQ(cancel.eventState)))
	cancel.null();
      modReject_(order, MxTRejReason::OrderClosed);
      execIn<MxTEventState::Sent>(order);
      return next;
    }
    // abnormal
    in.template as<Reject>().unsolicited_set();
    app()->abnormal(order, in);
    return next(order, in);
  }

  MxEnum filterModify(Order *order) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState)))
      return MxTRejReason::OrderClosed;
    if (ZuUnlikely(MxTEventState::matchDQSP(cancel.eventState))) {
      if (!MxTEventFlags::matchC(newOrder.eventFlags))
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

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modify(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	order->modifyTxn = in.template data<Modify>();
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncMod(order)))
	  modifyIn<MxTEventState::Deferred>(order); // async. modify disabled
	else
	  modifyIn<MxTEventState::Queued>(order);
	// no need for modifyOut()
	return nullptr;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Acknowledged;
	if (!MxTEventFlags::matchM(newOrder.eventFlags)) {
	  // new order is original, not following a previous modify-on-queue
	  ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	    1, 0, 0, 0, 1, 1>(order); // OmcuSP
	  return [](Order *order, In &in) {
	    NewOrder &newOrder = order->newOrder();
	    newOrder.modifyNew_set();
	    newOrder.update(in.template as<Modify>());
	    newOrderIn<MxTEventState::Queued>(order);
	  };
	}
	// new order is synthetic, following a previous modify-on-queue
	ackIn<MxTEventType::Modified, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 1>(order); // OmcuSP
	return [](Order *order, In &in) {
	  NewOrder &newOrder = order->newOrder();
	  newOrder.update(in.template as<Modify>());
	  if (newOrder.filled()) {
	    // modify-on-queue may have reduced qty so that order is now filled
	    newOrder.modifyNew_clr();
	    ackIn<MxTEventType::Modified, MxTEventState::Sent,
	      1, 0, 0, 0, 1, 0>(order); // OmcuSp
	    return;
	  }
	  newOrderIn<MxTEventState::Queued>(order);
	};
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      // modify-on-queue
      modify.eventState = MxTEventState::Acknowledged;
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 1, 0, 0, 1, 1>(order); // OMcuSP
      return [](Order *order, In &in) {
	auto &newOrder = order->newOrder();
	auto &modify = order->modify();
	auto &cancel = order->cancel();
	modify.update(in.template as<Modify>());
	if (MxTEventFlags::matchC(newOrder.eventFlags)) {
	  // synthetic cancel/replace in process
	  if (MxTEventState::matchSP(cancel.eventState)) {
	    // cancel sent
	    modifyIn<MxTEventState::Deferred>(order);
	    return;
	  }
	  cancel.null();
	  newOrder.modifyCxl_clr();
	}
	modifyIn<MxTEventState::Queued>(order);
      };
    }
    // should never happen; somehow this point was reached despite
    // filterModify() previously returning OK
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    {
      auto &modReject = order->execTxn.initModReject<1>();
      modReject.rejReason = rejReason;
      modReject.update(in.template as<Modify>());
      execIn<MxTEventState::Sent>(order);
    }
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modSimulated(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	newOrder.modifyCxl_set();
	order->modifyTxn = in.template data<Modify>();
	modifyIn<MxTEventState::Deferred>(order);
	order->cancelTxn.initCancel<1>();
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred;
	else
	  cancelOut(order);
	return nullptr;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Acknowledged;
	if (!MxTEventFlags::matchM(newOrder.eventFlags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.modifyNew_set();
	  ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	    1, 0, 0, 0, 1, 1>(order); // OmcuSP
	  return [](Order *order, In &in) {
	    NewOrder &newOrder = order->newOrder();
	    newOrder.update(in.template as<Modify>());
	    newOrderIn<MxTEventState::Queued>(order);
	  };
	}
	// new order is modify, following a previous modify-on-queue
	ackIn<MxTEventType::Modified, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 1>(order); // OmcuSP
	return [](Order *order, In &in) {
	  auto &newOrder = order->newOrder();
	  newOrder.update(in.template as<Modify>());
	  if (newOrder.filled()) {
	    // modify-on-queue may have reduced qty so that order is now filled
	    newOrder.modifyNew_clr();
	    ackIn<MxTEventType::Modified, MxTEventState::Sent,
	      1, 0, 0, 0, 1, 0>(order); // OmcuSp
	    return;
	  }
	  newOrderIn<MxTEventState::Queued>(order);
	};
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      // modify held/deferred/queued, modify-on-queue
      modify.eventState = MxTEventState::Acknowledged;
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 1, 0, 0, 1, 1>(order); // OMcuSP
      return [](Order *order, In &in) {
	auto &newOrder = order->newOrder();
	auto &modify = order->modify();
	auto &cancel = order->cancel();
	modify.update(in.template as<Modify>());
	modify.eventState = MxTEventState::Deferred;
	if (MxTEventFlags::matchC(newOrder.eventFlags)) {
	  // simulated modify in process
	  if (MxTEventState::matchQSP(cancel.eventState))
	    return; // cancel already sent
	} else
	  newOrder.modifyCxl_set();
	order->cancelTxn.initCancel<1>();
	cancel.update(newOrder);
	cancelIn<MxTEventState::Queued>(order);
      };
    }
    // should never happen; somehow this point was reached despite
    // filterModify() previously returning OK
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    {
      auto &modReject = order->execTxn.initModReject<1>();
      modReject.rejReason = rejReason;
      modReject.update(in.template as<Modify>());
      execIn<MxTEventState::Sent>(order);
    }
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modHeld(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchUAX(modify.eventState) &&
	  MxTEventState::matchUAX(cancel.eventState))) {
      if (ZuLikely(
	    MxTEventState::matchSA(newOrder.eventState))) {
	// usual case
	newOrder.modifyCxl_set();
	order->modifyTxn = in.template request<ModHeld>();
	modifyIn<MxTEventState::Held>(order);
	{
	  order->execTxn = in.template reject<ModHeld>();
	  auto &hold = order->execTxn.template as<Hold>();
	  hold.update(newOrder);
	  hold.update(modify);
	  execOut(hold);
	}
	order->cancelTxn.initCancel<1>();
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred;
	else
	  cancelOut(order);
	return nullptr;
      }
      if (MxTEventState::matchHQ(newOrder.eventState)) {
	// new order held/queued, modify-on-queue
	newOrder.eventState = MxTEventState::Acknowledged;
	if (!MxTEventFlags::matchM(newOrder.eventFlags)) {
	  // new order is original, not following a previous modify-on-queue
	  newOrder.modifyNew_set();
	  ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	    1, 0, 0, 0, 1, 1>(order); // OmcuSP
	} else {
	  // new order is modify, following a previous modify-on-queue
	  ackIn<MxTEventType::Modified, MxTEventState::Sent,
	    1, 0, 0, 0, 1, 1>(order); // OmcuSP
	}
	return [](Order *order, In &in) {
	  NewOrder &newOrder = order->newOrder();
	  newOrder.update(in.template request<ModHeld>());
	  newOrderIn<MxTEventState::Held>(order);
	  {
	    order->execTxn = in.template reject<ModHeld>();
	    auto &hold = order->execTxn.template as<Hold>();
	    hold.update(newOrder);
	    execOut(hold);
	  }
	};
      }
    }
    if (ZuLikely(
	  MxTEventState::matchA(newOrder.eventState) &&
	  MxTEventState::matchHDQ(modify.eventState))) {
      // modify held/deferred/queued, modify-on-queue
      modify.eventState = MxTEventState::Acknowledged;
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 1, 0, 0, 1, 1>(order); // OMcuSP
      return [](Order *order, In &in) {
	auto &newOrder = order->newOrder();
	auto &modify = order->modify();
	auto &cancel = order->cancel();
	modify.update(in.template request<ModHeld>());
	modifyIn<MxTEventState::Held>(order);
	if (!MxTEventFlags::matchC(newOrder.eventFlags))
	  newOrder.modifyCxl_set();
	if (!MxTEventState::matchQSP(cancel.eventState)) {
	  order->cancelTxn.initCancel<1>();
	  cancel.update(newOrder);
	  cancelOut(order);
	}
	{
	  order->execTxn = in.template reject<ModHeld>();
	  auto &hold = order->execTxn.template as<Hold>();
	  hold.update(modify);
	  execOut(hold);
	}
      };
    }
    // should never happen; somehow this point was reached despite
    // filterModify() previously returning OK
    MxEnum rejReason = filterModify(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    {
      auto &modReject = order->execTxn.initModReject<1>();
      modReject.rejReason = rejReason;
      modReject.update(in.template as<Modify>());
      execIn<MxTEventState::Sent>(order);
    }
    return nullptr;
  }

  // Note: caller must pass a MxTOrder instance that is detached from the
  // main OSM sequence so that any pending modify can be safely overwritten
  // with the filtered request, since the original will remain intact in the
  // main sequence
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modFiltered(Order *order, In &in) {
    order->modifyTxn = in.template request<Modify>();
    {
      order->execTxn = in.template reject<ModFiltered>();
      modifyIn<MxTEventState::Rejected>(order);
      auto &modReject = order->execTxn.template as<ModReject>();
      modReject.update(order->modify());
      execOut(order);
    }
    return nullptr;
  }

  // Note: see modFiltered() comment
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modFilteredCxl(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
      return modFiltered(order, in);
    };
    if (MxTEventState::matchXC(newOrder.eventState) ||
	MxTEventState::matchDQSP(cancel.eventState)) {
      // order closed, or cancel already in progress; just reject modify
      newOrder.modifyCxl_clr();
      return next(order, in);
    }
    if (MxTEventState::matchD(modify.eventState) ||
	(!MxTEventState::matchA(newOrder.eventState) &&
	 MxTEventState::matchQ(modify.eventState))) {
      // preceding modify in progress, but queued or deferred
      newOrder.modifyCxl_clr();
      order->cancelTxn.initCancel<1>();
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order)))
	cancelIn<MxTEventState::Deferred>(order);
      else
	cancelIn<MxTEventState::Queued>(order);
      // reject preceding modify that was in progress
      modReject_(order, MxTRejReason::BrokerReject);
      execOut(order);
      return next;
    }
    if (MxTEventState::matchSP(modify.eventState)) {
      // preceding modify in progress, sent to market (or pending fill)
      newOrder.modifyCxl_clr();
      order->cancelTxn.initCancel<1>();
      cancel.update(newOrder);
      if (ZuUnlikely(
	    (MxTEventState::matchS(newOrder.eventState) ||
	     MxTEventState::matchS(modify.eventState)) &&
	    !app()->asyncCxl(order)))
	cancelIn<MxTEventState::Deferred>(order);
      else
	cancelIn<MxTEventState::Queued>(order);
      return next;
    }
    if (MxTEventState::matchHQ(newOrder.eventState)) {
      // new order held/queued, modify-on-queue (now cancel-on-queue)
      newOrder.eventState = MxTEventState::Closed;
      // inform client that order is canceled
      ackIn<MxTEventType::Canceled, MxTEventState::Sent,
	1, 0, 0, 0, 1, 0>(order); // OmcuSp
      return next;
    }
    newOrder.modifyCxl_clr();
    order->cancelTxn.initCancel<1>();
    cancel.update(newOrder);
    if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	  !app()->asyncCxl(order)))
      cancelIn<MxTEventState::Deferred>(order);
    else
      cancelIn<MxTEventState::Queued>(order);
    return next;
  }

private:
  template <bool Unsolicited, typename In>
  void modified_(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    if constexpr (Unsolicited) {
      order->modifyTxn.initModify<1>();
      modify.update(newOrder);
    }
    modify.update(in.template as<Modified>());
    if (newOrder.pending(modify)) {
      // modify ack is pending a fill - defer it and resume in fill()
      modify.eventState = MxTEventState::PendingFill;
      ackIn<MxTEventType::Modified, MxTEventState::PendingFill,
	1, 1, 0, Unsolicited, 0, 0>(order); // OMcUsp
      modify.ackFlags = order->ack().eventFlags;
    } else {
      // modify ack can be processed immediately
      modify.eventState = MxTEventState::Acknowledged;
      newOrder.update(modify);
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 0, 0, Unsolicited, 0, 0>(order); // OmcUsp
    }
    if (MxTEventState::matchD(cancel.eventState)) // deferred cancel
      cancelOut(order);
  }

public:
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modified(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (MxTEventState::matchSA(newOrder.eventState) &&
	MxTEventState::matchDQS(modify.eventState)) {
      // usual case
      auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
	modified_<0>(order, in); // solicited
	return nullptr;
      };
      // acknowledge any sent new order
      if (MxTEventState::matchS(newOrder.eventState)) {
	newOrder.eventState = MxTEventState::Acknowledged;
	ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 0>(order); // OmcuSp
	return next;
      }
      return next(order, in);
    }
    // unsolicited (i.e. unilaterally restated by broker/market)
    {
      auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
	modified_<1>(order, in); // unsolicited
	return nullptr;
      };
      // acknowledge any pending new order (unless simulating modify)
      if (!MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchHQS(newOrder.eventState)) {
	newOrder.eventState = MxTEventState::Acknowledged;
	ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 0>(order); // OmcuSp
	return next;
      }
      if (MxTEventState::matchAC(newOrder.eventState))
	return next(order, in); // usual case
    }
    if (MxTEventFlags::matchC(newOrder.eventFlags) &&
	MxTEventState::matchS(newOrder.eventState) &&
	MxTEventState::matchHD(modify.eventState) &&
	MxTEventState::matchDQSPX(cancel.eventState)) {
      // unacknowledged order and simulated modify in process
      newOrder.eventState = MxTEventState::Acknowledged;
      ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	1, 0, 0, 0, 1, 0>(order); // OmcuSp
      return [](Order *order, In &in) -> typename Next<In>::Fn {
	modified_<1>(order, in);
	// send any cancel needed for cancel/replace
	// deferred cancels are taken care of by modified_()
	if (MxTEventState::matchX(cancel.eventState))
	  cancelOut(order);
	return nullptr;
      };
    }
    // abnormal
    app()->abnormal(order, in);
    newOrder.update(in.template as<Modified>());
    ackIn<MxTEventType::Modified, MxTEventState::Sent,
      1, 0, 0, 1, 0, 0>(order); // OmcUsp
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modReject(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    order->execTxn = in.template data<ModReject>();
    auto &modReject = order->execTxn.template as<ModReject>();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      modReject.update(modify);
      modify.eventState = MxTEventState::Rejected;
      execIn<MxTEventState::Sent>(order);
      if (MxTEventState::matchD(cancel.eventState))
	cancelOut(order);
      return nullptr;
    }
    if (MxTEventState::matchXC(newOrder.eventState) &&
	MxTEventState::matchSP(modify.eventState)) {
      modReject.update(modify);
      modify.eventState = MxTEventState::Rejected;
      execIn<MxTEventState::Received>(order);
      return nullptr;
    }
    app()->abnormal(order, in);
    execIn<MxTEventState::Received>(order);
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  modRejectCxl(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    order->execTxn = in.template data<ModReject>();
    auto &modReject = order->execTxn.template as<ModReject>();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchDQSP(modify.eventState))) {
      newOrder.modifyCxl_clr();
      modReject.update(modify);
      execIn<MxTEventState::Sent>(order);
      // do not need to cancel if order already filled
      if (ZuLikely(!newOrder.filled())) {
	if (MxTEventState::matchQSP(cancel.eventState)) return nullptr;
	if (!MxTEventState::matchD(cancel.eventState)) {
	  order->cancelTxn.initCancel<1>();
	  cancel.update(newOrder);
	}
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order)))
	  cancel.eventState = MxTEventState::Deferred;
	else
	  cancelOut(order);
      }
      return nullptr;
    }
    if (MxTEventState::matchXC(newOrder.eventState) &&
	MxTEventState::matchSP(modify.eventState)) {
      modReject.update(modify);
      modify.eventState = MxTEventState::Rejected;
      execIn<MxTEventState::Received>(order);
      return nullptr;
    }
    app()->abnormal(order, in);
    execIn<MxTEventState::Received>(order);
  }

  MxEnum filterCancel(Order *order) {
    NewOrder &newOrder = order->newOrder();
    Modify &modify = order->modify();
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState)))
      return MxTRejReason::OrderClosed;
    if (ZuUnlikely(MxTEventState::matchDQSP(cancel.eventState))) {
      if (!MxTOrderFlags::matchC(newOrder.flags))
	return MxTRejReason::CancelPending;
      else {
	newOrder.modifyCxl_clr();
	return MxTRejReason::CancelPending;
      }
    }
    return MxTRejReason::OK;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  cancel(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHDQSPA(modify.eventState))) {
      // usual case
      newOrder.modifyCxl_clr();
      order->cancelTxn = in.template data<Cancel>();
      cancel.update(newOrder);
      if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	    !app()->asyncCxl(order)))
	cancelIn<MxTEventState::Deferred>(order); // async. cancel disabled
      else
	cancelIn<MxTEventState::Queued>(order);
      // synthetically reject any held/deferred/queued modify
      if (ZuUnlikely(MxTEventState::matchHDQ(modify.eventState))) {
	modReject_(order, MxTRejReason::OrderClosed);
	execOut(order);
      }
    }
    // close any held/queued new order
    if (MxTEventState::matchHQ(newOrder.eventState)) {
      newOrder.modifyNew_clr();
      newOrder.eventState = MxTEventState::Closed;
      // inform client that order is canceled
      ackIn<MxTEventType::Canceled, MxTEventState::Sent,
	1, 0, 0, 0, 1, 0>(order); // OmcuSp
      return;
    }
    // should never happen; somehow this point was reached despite
    // filterCancel() previously returning OK when app called cancelIn()
    MxEnum rejReason = filterCancel(order, in);
    if (ZuUnlikely(rejReason == MxTRejReason::OK)) // should never happen
      rejReason = MxTRejReason::OSM;
    cxlReject_(order, in, rejReason);
    execIn<MxTEventState::Sent>(order);
  }

  // Note: see modFiltered() comment
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  cxlFiltered(Order *order, In &in) {
    order->cancelTxn = in.template request<Cancel>();
    cancelIn<MxTEventState::Rejected>(order);
    {
      order->execTxn = in.template reject<CxlFiltered>();
      auto &cxlReject = order->execTxn.template as<CxlReject>();
      cxlReject.update(order->cancel());
      execOut(order);
    }
    return nullptr;
  }

private:
  template <bool Unsolicited, typename In>
  void canceled_(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &cancel = order->cancel();
    if constexpr (Unsolicited) {
      order->cancelTxn.initCancel<1>();
      cancel.update(newOrder);
    }
    cancel.update(in.template as<Canceled>());
    if (newOrder.pending(cancel)) {
      cancel.eventState = MxTEventState::PendingFill;
      ackIn<MxTEventType::Canceled, MxTEventState::PendingFill,
	0, 0, 1, Unsolicited, 0, 0>(order); // omCUsp
      cancel.ackFlags = order->ack().eventFlags;
    } else {
      cancel.eventState = MxTEventState::Acknowledged;
      newOrder.eventState = MxTEventState::Closed;
      newOrder.update(cancel);
      ackIn<MxTEventType::Canceled, MxTEventState::Sent,
	1, 0, 0, Unsolicited, 0, 0>(order); // OmcUsp
    }
  }

  template <bool Unsolicited, typename In>
  void modCanceled_(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &cancel = order->cancel();
    if constexpr (Unsolicited) {
      order->cancelTxn.initCancel<1>();
      cancel.update(newOrder);
    }
    cancel.update(in.template as<Canceled>());
    if (newOrder.pending(cancel)) {
      cancel.eventState = MxTEventState::PendingFill;
      ackIn<MxTEventType::Modified, MxTEventState::PendingFill,
	0, 0, 1, 0, 0, 0>(order); // omCusp
      cancel.ackFlags = order->ack().eventFlags;
    } else {
      newOrder.update(modify);
      if (MxTEventState::matchH(modify.eventState)) {
	newOrder.eventState = MxTEventState::Held;
	modify.null();
      }
      newOrder.mc<1, 0>();
      cancel.eventState = MxTEventState::Acknowledged;
      ackIn<MxTEventType::Modified, MxTEventState::Sent,
	1, 0, 0, 0, 0, 0>(order); // Omcusp
    }
  }

public:
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  canceled(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(
	  !MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchHQSA(newOrder.eventState))) {
      // usual case
      auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
	auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
	  auto &cancel = order->cancel();
	  if (ZuLikely(MxTEventState::matchDQS(cancel.eventState)))
	    canceled_<0>(order, in); // solicited
	  else
	    canceled_<1>(order, in); // unsolicited
	  return nullptr;
	};
	// reject any pending modify
	auto &modify = order->modify();
	if (MxTEventState::matchDQ(modify.eventState)) {
	  modReject_(order, MxTRejReason::OrderClosed);
	  execIn<MxTEventState::Sent>(order);
	  return next;
	}
	return next(order, in);
      };
      // acknowledge any pending new order
      auto &newOrder = order->newOrder();
      if (MxTEventState::matchHQS(newOrder.eventState)) {
	newOrder.eventState = MxTEventState::Acknowledged;
	ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 0>(order); // OmcuSp
	return next;
      }
      return next(order, in);
    }
    if (ZuUnlikely(MxTEventState::matchXC(newOrder.eventState))) {
      // late - order already closed
      if (MxTEventState::matchDQS(cancel.eventState)) {
	// solicited
	cancel.eventState = MxTEventState::Acknowledged;
	ackIn<MxTEventType::Canceled, MxTEventState::Received,
	  1, 0, 0, 0, 0, 0>(order, ordered.eventLeg); // Omcusp
      } else {
	// unsolicited
	ackIn<MxTEventType::Canceled, MxTEventState::Received,
	  1, 0, 0, 1, 0, 0>(order, ordered.eventLeg); // OmcUsp
      }
      return nullptr;
    }
    if (ZuLikely(
	  MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchSA(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState))) {
      // simulated modify held or deferred
      auto next = [](Order *order, In &in) -> typename Next<In>::Fn {
	if (MxTEventState::matchDQS(cancel.eventState))
	  modCanceled_<0>(order, in); // solicited
	else
	  modCanceled_<1>(order, in); // unsolicited
	if (newOrder.filled()) {
	  // modify may have reduced qty so that order is now filled
	  newOrder.modifyNew_clr();
	  newOrder.eventState = MxTEventState::Acknowledged;
	} else
	  newOrderOut(order);
	return nullptr;
      };
      // acknowledge any pending new order
      if (MxTEventState::matchS(newOrder.eventState)) {
	newOrder.eventState = MxTEventState::Acknowledged;
	ackIn<MxTEventType::Ordered, MxTEventState::Sent,
	  1, 0, 0, 0, 1, 0>(order); // OmcuSp
	return next;
      }
      return next(order, in);
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
    if (!MxTEventState::matchXC(newOrder.eventState))
      newOrder.eventState = MxTEventState::Closed;
    ackIn<MxTEventType::Canceled, MxTEventState::Received,
      1, 0, 0, 1, 0, 0>(order); // OmcUsp
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  cxlReject(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    order->execTxn = in.template data<CxlReject>();
    auto &cxlReject = order->execTxn.template as<CxlReject>();
    if (ZuLikely(MxTEventState::matchDQS(cancel.eventState))) {
      // usual case
      cxlReject.update(cancel);
      cancel.eventState = MxTEventState::Rejected;
      if (MxTEventFlags::matchC(newOrder.eventFlags))
	execIn<MxTEventState::Received>(order); // simulated modify
      else
	execIn<MxTEventState::Sent>(order);
      return nullptr;
    }
    // abnormal
    in.template as<Event>().unsolicited_set();
    app()->abnormal(order, in);
    execIn<MxTEventState::Received>(order);
    return nullptr;
  }

  template <typename In>
  typename ZuIsBase<Txn_, In, bool>::T release(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchH(newOrder.eventState))) {
      // release new order
      {
	order->execTxn = in.template data<Release>();
	auto &release = order->execTxn.template as<Release>();
	release.update(newOrder);
	execIn<MxTEventState::Sent>(order);
      }
      newOrderOut(order);
      return nullptr;
    }
    if (ZuLikely(MxTEventState::matchH(modify.eventState))) {
      order->execTxn = in.template data<Release>();
      execIn<MxTEventState::Sent>(order);
      auto &release = order->execTxn.template as<Release>();
      release.update(newOrder);
      release.update(modify);
      if (ZuLikely(!MxTEventFlags::matchC(newOrder.eventFlags))) {
	// release modify
	if (MxTEventState::matchSP(cancel.eventState)) {
	  modify.eventState = MxTEventState::Deferred;
	  return nullptr;
	}
	if (MxTEventState::matchDQX(cancel.eventState))
	  cancel.null();
	newOrder.modifyCxl_clr();
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncMod(order))) {
	  // async. modify disabled
	  modify.eventState = MxTEventState::Deferred;
	  return nullptr;
	}
	modifyOut(order);
	return nullptr;
      } else {
	// release simulated modify
	modify.eventState = MxTEventState::Deferred;
	if (!MxTEventState::matchDQSP(cancel.eventState)) {
	  order->cancelTxn.initCancel<1>();
	  cancel.update(newOrder);
	  if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
		!app()->asyncCxl(order))) {
	    // async. cancel disabled
	    cancel.eventState = MxTEventState::Deferred;
	  } else
	    cancelOut(order);
	}
	return nullptr;
      }
    }
    // abnormal
    app()->abnormal(order, in);
    {
      order->execTxn = in.template data<Release>();
      execIn<MxTEventState::Received>(order);
    }
    return nullptr;
  }

  // deny in -> reject/modReject out
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  deny(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    if (ZuLikely(MxTEventState::matchH(newOrder.eventState))) {
      // reject new order
      newOrder.eventState = MxTEventState::Rejected;
      newOrder.modifyNew_clr();
      {
	auto &reject = order->execTxn.initReject<1>();
	reject.update(in.template as<Deny>());
	reject.update(newOrder);
	execIn<MxTEventState::Sent>(order);
      }
      return nullptr;
    }
    if (ZuLikely(MxTEventState::matchH(modify.eventState))) {
      // reject modify
      newOrder.modifyCxl_clr();
      modify.eventState = MxTEventState::Rejected;
      {
	auto &modReject = order->execTxn.initModReject<1>();
	modReject.update(in.template as<Deny>());
	modReject.update(modify);
	execIn<MxTEventState::Sent>(order);
      }
      if (!MxTEventState::matchDQSP(cancel.eventState)) {
	order->cancelTxn.initCancel<1>();
	cancel.update(newOrder);
	if (ZuUnlikely(MxTEventState::matchS(newOrder.eventState) &&
	      !app()->asyncCxl(order))) {
	  // async. cancel disabled
	  cancel.eventState = MxTEventState::Deferred;
	} else
	  cancelOut(order);
      }
      return nullptr;
    }
    // abnormal
    app()->abnormal(order, in);
    {
      order->execTxn = in.template data<Deny>();
      execIn<MxTEventState::Received>(order);
    }
    return nullptr;
  }

private:
  template <typename In>
  void fillOrdered_(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    newOrder.eventState = MxTEventState::Acknowledged;
    unsigned leg = in.template data<Event>().eventLeg;
    if (ZuUnlikely(MxTEventFlags::matchM(newOrder.eventFlags))) {
      newOrder.modifyNew_clr();
      ackOut<MxTEventType::Modified, MxTEventState::Sent,
	1, 0, 0, 1, 0, 0>(order, leg);
    } else
      ackOut<MxTEventType::Ordered, MxTEventState::Sent,
	1, 0, 0, 1, 0, 0>(order, leg);
  }

public:
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  fill(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    auto &modify = order->modify();
    auto &cancel = order->cancel();
    // always process fills, regardless of order status
    order->execTxn = in.template data<Fill>();
    auto &fill = order->execTxn.template as<Fill>();
    Fill &fill = out->template as<Fill>();
    execIn<MxTEventState::Sent>(order);
    if (ZuLikely(
	  MxTEventState::matchAC(newOrder.eventState)))
      // order already acknowledged / closed
      goto applyFill;
    if (ZuLikely(
	  !MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchHQS(newOrder.eventState))) {
      // order unacknowledged, possible pending cancel or modify
      fillOrdered_<0>(order, in); // solicited
      if (ZuUnlikely(MxTEventState::matchD(cancel.eventState)))
	cancelOut(order);
      else if (ZuUnlikely(MxTEventState::matchD(modify.eventState)))
	modifyOut(order);
      goto applyFill;
    }
    if (ZuLikely(
	  MxTEventFlags::matchC(newOrder.eventFlags) &&
	  MxTEventState::matchS(newOrder.eventState) &&
	  MxTEventState::matchHD(modify.eventState) &&
	  MxTEventState::matchDQSPX(cancel.eventState))) {
      // order unacknowledged, simulated modify held or deferred
      fillOrdered_<0>(order, in); // solicited
      if (ZuUnlikely(MxTEventState::matchDX(cancel.eventState)))
	cancelOut(order);
      goto applyFill;
    }
applyFill:
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
    if (MxTEventState::matchP(modify.eventState) &&
	!newOrder.pending(modify)) {
      // modify ack pending
      modify.eventState = MxTEventState::Acknowledged;
      newOrder.update(modify);
      ackOut<MxTEventType::Modified, MxTEventState::Sent,
	1, 0, 0, 0, 0, 0>(order); // Omcusp
    }
    if (MxTEventState::matchP(cancel.eventState) &&
	!newOrder.pending(cancel)) {
      // cancel ack pending - (simulated modify cancels are never deferred)
      cancel.eventState = MxTEventState::Acknowledged;
      newOrder.eventState = MxTEventState::Closed;
      newOrder.update(cancel);
      ackOut<MxTEventType::Canceled, MxTEventState::Sent,
	1, 0, 0, 0, 0, 0>(order); // Omcusp
    }
    fill.update(newOrder);
    return nullptr;
  }

  // closes the order
  template <typename In>
  typename ZuIsBase<Txn_, In, typename Next<In>::Fn>::T
  closed(Order *order, In &in) {
    auto &newOrder = order->newOrder();
    order->execTxn = in.template data<Closed>();
    auto &closed = order->execTxn.template as<Closed>();
    newOrder.eventState = MxTEventState::Closed;
    newOrder.update(closed);
    closed.update(newOrder);
    execIn<MxTEventState::Sent>(order);
    return nullptr;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In>
  typename ZuIsBase<Txn_, In, bool>::T orderSend(Order *order, In &in) {
    NewOrder &newOrder = order->newOrder();
    if (ZuUnlikely(!MxTEventState::matchQ(newOrder.eventState))) return false;
    newOrder.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In>
  typename ZuIsBase<Txn_, In, bool>::T modifySend(Order *order, In &in) {
    Modify &modify = order->modify();
    if (ZuUnlikely(!MxTEventState::matchQ(modify.eventState))) return false;
    modify.eventState = MxTEventState::Sent;
    return true;
  }

  // returns true if transmission should proceed, false if aborted
  template <typename In>
  typename ZuIsBase<Txn_, In, bool>::T cancelSend(Order *order, In &in) {
    Cancel &cancel = order->cancel();
    if (ZuUnlikely(!MxTEventState::matchQ(cancel.eventState))) return false;
    cancel.eventState = MxTEventState::Sent;
    return true;
  }
};

#endif /* MxTOrderMgr_HPP */
