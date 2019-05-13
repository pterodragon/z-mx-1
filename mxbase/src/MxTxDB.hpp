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

// MxT transmission database

#ifndef MxTxDB_HPP
#define MxTxDB_HPP

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
struct App : public MxTxDB<App> {
  void txAdded(TxPOD *);		// transmission recovered/replicated

  template <typename ...Args>
  void logError(Args &&... args);	// log error
};
#endif

template <typename App_> class MxTxDB {
public:
  using App = App_;

  ZuInline const App *app() const { return static_cast<const App *>(this); }
  ZuInline App *app() { return static_cast<App *>(this); }

  enum { DBVersion = 0 }; // increment when schema changes

  struct TxData {
    MxMsgID	msgID;
    ZdbRN	msgRN = ZdbNullRN;
    int32_t	msgType = -1;
  };

  using TxDB = Zdb<TxData>;
  using TxPOD = ZdbPOD<TxData>;

  void init(ZdbEnv *dbEnv, ZvCf *cf) {
    m_txDB = new TxDB(
	dbEnv, "txDB", DBVersion, ZdbCacheMode::Normal,
	ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) { pod = new TxPOD(db); },
	  ZdbAddFn{app(), [](App *app, ZdbAnyPOD *pod, bool) {
	    app->txAdded(static_cast<TxPOD *>(pod)); }},
	  ZdbDelFn{}, ZdbCopyFn{}});
  }
  void final() {
    m_txDB = nullptr;
  }

  ZuInline TxDB *txDB() const { return m_txDB; }

  // l(MxQMsg *msg, ZdbRN &rn, int32_t &type) -> void
  template <typename Link, typename L>
  void txStore(Link *link, MxQMsg *msg, L l) {
    auto &txPOD = link->txPOD;
    if (ZuUnlikely(!txPOD))
      txPOD = m_txDB->push();
    else
      txPOD = m_txDB->update(txPOD);
    if (ZuUnlikely(!txPOD)) { app()->logError("txDB update failed"); return; }
    auto &txData = txPOD->data();
    txData.msgID = msg->id;
    l(msg, txData.msgRN, txData.msgType);
    if (ZuUnlikely(txPOD->rn() == txPOD->prevRN()))
      m_txDB->put(txPOD);
    else
      m_txDB->putUpdate(txPOD, false);
  }

  // l(ZdbRN rn, int32_t type) -> ZmRef<MxQMsg>
  template <typename Link, typename L>
  ZmRef<MxQMsg> txRetrieve(Link *link, MxSeqNo seqNo, L l) {
    auto txPOD = link->txPOD;
    while (txPOD) {
      auto &txData = txPOD->data();
      if (ZuUnlikely(txData.msgID.seqNo < seqNo)) return nullptr;
      ZmRef<MxQMsg> msg = l(txData.msgRN, txData.msgType);
      if (!msg) return nullptr;
      msg->load(txData.msgID);
      if (ZuUnlikely(txData.msgID.seqNo == seqNo)) return msg;
      link->txQueue()->enqueue(msg);
      ZdbRN prevRN = txPOD->prevRN();
      if (txPOD->rn() == prevRN) return nullptr;
      txPOD = m_txDB->get(prevRN);
    }
    return nullptr;
  }

private:
  ZmRef<TxDB>	m_txDB;
};

#endif /* MxTxDB_HPP */
