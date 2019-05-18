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

// MxT reception database

#ifndef MxRxDB_HPP
#define MxRxDB_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <Zdb.hpp>

#include <MxBase.hpp>

// CRTP - implementation must conform to the following interface:
#if 0
struct App : public MxRxDB<App> {
  void rxAdded(RxPOD *);		// reception recovered/replicated

  template <typename ...Args>
  void logError(Args &&... args);	// log error
};
#endif

template <typename App_> class MxRxDB {
public:
  using App = App_;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  enum { DBVersion = 0 }; // increment when schema changes

  struct RxData {
    MxMsgID	msgID;
  };

  using RxDB = Zdb<RxData>;
  using RxPOD = ZdbPOD<RxData>;

  void init(ZdbEnv *dbEnv, ZvCf *cf) {
    m_rxDB = new RxDB(
	dbEnv, "rxDB", DBVersion, ZdbCacheMode::Normal,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new RxPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->rxAdded(static_cast<RxPOD *>(pod)); }},
	  ZdbDelFn{}, ZdbCopyFn{}});
  }
  void final() {
    m_rxDB = nullptr;
  }

  ZuInline RxDB *rxDB() const { return m_rxDB; }

  template <typename Link>
  void rxStore(Link *link, const MxMsgID &msgID) {
    auto &rxPOD = link->rxPOD;
    if (ZuUnlikely(!rxPOD))
      rxPOD = m_rxDB->push();
    else
      rxPOD = m_rxDB->update(rxPOD);
    if (ZuUnlikely(!rxPOD)) { app()->logError("rxDB update failed"); return; }
    auto &rxData = rxPOD->data();
    rxData.msgID = msgID;
    if (ZuUnlikely(rxPOD->rn() == rxPOD->prevRN()))
      m_rxDB->put(rxPOD);
    else
      m_rxDB->putUpdate(rxPOD);
  }

private:
  ZmRef<RxDB>	m_rxDB;
};

#endif /* MxRxDB_HPP */
