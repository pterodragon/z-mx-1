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

// MxT order/execution database

#ifndef MxTOrderDB_HPP
#define MxTOrderDB_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxTLib_HPP
#include <MxTLib.hpp>
#endif

#include <Zdb.hpp>

#include <MxBase.hpp>

#include <MxTTypes.hpp>
#include <MxTOrder.hpp>

// CRTP - implementation must conform to the following interface:
#if 0
struct App : public MxTOrderDB<App> {
  struct Types : public MxTAppTypes<Types> { ... };

  using OrderPOD = MxTOrderDB<App>::OrderPOD;
  using ClosedPOD = MxTOrderDB<App>::ClosedPOD;

  void orderAdded(OrderPOD *);		// order recovered/replicated
  void closedAdded(ClosedPOD *);	// closed order recovered/replicated
};
#endif

template <typename App, typename Types_> class MxTOrderDB {
  typedef Types_ Types;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

public:
  using OrderData = typename Types::Order;		// open order
  using OrderDB = Zdb<OrderData>;
  using OrderPOD = ZdbPOD<OrderData>;

  using ClosedData =					// closed order
    typename Types::template Txn<typename Types::OrderFiltered>;
  using ClosedDB = Zdb<ClosedData>;
  using ClosedPOD = ZdbPOD<ClosedData>;

  void init(ZdbEnv *dbEnv, ZvCf *cf) {
    unsigned orderDBID = cf->getInt("orderDB", 0, 10000, true);
    unsigned closedDBID = cf->getInt("closedDB", 0, 10000, true);

    m_orderDB = new OrderDB(
	dbEnv, orderDBID, Types::DBVersion, ZdbCacheMode::FullCache,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new OrderPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->orderAdded(static_cast<OrderPOD *>(pod)); }},
	  ZdbDelFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->orderDeleted(static_cast<OrderPOD *>(pod)); }},
	  ZdbCopyFn{}});
    m_closedDB = new ClosedDB(
	dbEnv, closedDBID, Types::DBVersion, ZdbCacheMode::Normal,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new ClosedPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->closedAdded(static_cast<ClosedPOD *>(pod)); }},
	  ZdbDelFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->closedDeleted(static_cast<ClosedPOD *>(pod)); }},
	  ZdbCopyFn{}});
  }
  void final() {
    m_orderDB = nullptr;
    m_closedDB = nullptr;
  }

  ZuInline OrderDB *orderDB() const { return m_orderDB; }
  ZuInline ClosedDB *closedDB() const { return m_closedDB; }

  // FIXME - functions to move order from openDB to closedDB

private:
  ZmRef<OrderDB>	m_orderDB;
  ZmRef<ClosedDB>	m_closedDB;
};

#endif /* MxTOrderDB_HPP */
