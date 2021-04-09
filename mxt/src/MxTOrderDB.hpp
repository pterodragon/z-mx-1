//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// MxT order/execution database

#ifndef MxTOrderDB_HPP
#define MxTOrderDB_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxTLib_HPP
#include <mxt/MxTLib.hpp>
#endif

#include <zlib/Zdb.hpp>

#include <mxbase/MxBase.hpp>

#include <mxt/MxTTypes.hpp>
#include <mxt/MxTOrder.hpp>

// CRTP - implementation must conform to the following interface:
#if 0
struct AppTypes : public MxTAppTypes<AppTypes> { ... };

struct App : public MxTOrderDB<App, AppTypes> {

  using OrderPOD = MxTOrderDB<App>::OrderPOD;
  using ClosedPOD = MxTOrderDB<App>::ClosedPOD;

  void orderAdded(OrderPOD *, int op);	// order recovered/replicated
  void closedAdded(ClosedPOD *, int op);// closed order recovered/replicated
};
#endif

template <typename App_, typename Types_>
  class MxTOrderDB {
public:
  using App = App_;
  using Types = Types_;

  const App *app() const { return static_cast<const App *>(this); }
  App *app() { return static_cast<App *>(this); }

  MxTImport(Types);

  using OrderData = Order;		// open order
  using OrderDB = Zdb<OrderData>;
  using OrderPOD = ZdbPOD<OrderData>;

  using ClosedData = ClosedOrder;
  using ClosedDB = Zdb<ClosedData>;
  using ClosedPOD = ZdbPOD<ClosedData>;

  void init(ZdbEnv *dbEnv, const ZvCf *cf) {
    m_orderDB = new OrderDB(
	dbEnv, "orderDB", Types::DBVersion, ZdbCacheMode::FullCache,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new OrderPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, int op, bool) {
	    app->orderAdded(static_cast<OrderPOD *>(pod), op); }},
	  app()->orderWriteFn()});
    m_closedDB = new ClosedDB(
	dbEnv, "closedDB", Types::DBVersion, ZdbCacheMode::Normal,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new ClosedPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, int op, bool) {
	    app->closedAdded(static_cast<ClosedPOD *>(pod), op); }},
	  app()->closedWriteFn()});
  }
  void final() {
    m_orderDB = nullptr;
    m_closedDB = nullptr;
  }

  OrderDB *orderDB() const { return m_orderDB; }
  ClosedDB *closedDB() const { return m_closedDB; }

  ZmRef<ClosedPOD> closeOrder(OrderPOD *pod) {
    auto order = pod->ptr();
    ZmRef<ClosedPOD> cpod = m_closedDB->push();
    auto closed = new (cpod->ptr()) ClosedOrder();
    closed->orderTxn = order->orderTxn.template data<NewOrder>();
    if (order->exec() && order->exec().eventType == MxTEventType::Reject)
      closed->closedTxn = order->execTxn.template data<Reject>();
    else if (order->exec() && order->exec().eventType == MxTEventType::Closed)
      closed->closedTxn = order->execTxn.template data<Closed>();
    else if (order->ack() && order->ack().eventType == MxTEventType::Canceled)
      closed->closedTxn = order->ackTxn.template data<Event>();
    closed->openRN = pod->rn();
    m_closedDB->put(cpod);
    return cpod;
  }

  using PurgeLock = ZmLock;
  using PurgeGuard = ZmGuard<PurgeLock>;

  ZtDate lastPurge() {
    PurgeGuard guard(m_purgeLock);
    return m_lastPurge;
  }
  void purged(OrderPOD *) { } // default
  void purge() {
    PurgeGuard guard(m_purgeLock);
    m_lastPurge.now();
    if (m_purgeClosedRN != ZdbNullRN)
      m_closedDB->purge(m_purgeClosedRN);
    if (m_purgeOrderRN != ZdbNullRN) {
      ZdbRN minRN = m_purgeClosedRN;
      if (minRN == ZdbNullRN) minRN = m_closedDB->minRN();
      for (ZdbRN rn = minRN; rn < m_purgeOrderRN; rn++) {
	if (ZmRef<ClosedPOD> cpod = m_closedDB->get_(rn)) {
	  auto closed = cpod->ptr();
	  if (ZmRef<OrderPOD> pod = m_orderDB->get_(closed->openRN)) {
	    app()->purged(pod);
	    m_orderDB->del(pod);
	  }
	}
      }
    }
    m_purgeClosedRN = m_purgeOrderRN;
    m_purgeOrderRN = m_closedDB->nextRN();
  }

private:
  ZmRef<OrderDB>	m_orderDB;
  ZmRef<ClosedDB>	m_closedDB;

  PurgeLock		m_purgeLock;
    ZtDate		  m_lastPurge;
    ZdbRN		  m_purgeOrderRN = ZdbNullRN;
    ZdbRN		  m_purgeClosedRN = ZdbNullRN;
};

#endif /* MxTOrderDB_HPP */
