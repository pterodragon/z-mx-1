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

// MxMD library internal API

#ifndef MxMDCore_HPP
#define MxMDCore_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <ZuPOD.hpp>

#include <ZmObject.hpp>
#include <ZmDRing.hpp>
#include <ZmRBTree.hpp>
#include <ZmRef.hpp>
#include <ZmThread.hpp>

#include <ZePlatform.hpp>

#include <ZvCf.hpp>
#include <ZvCmd.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMD.hpp>
#include <MxMDStream.hpp>

#include <MxMDBroadcast.hpp>

#include <MxMDRecord.hpp>
#include <MxMDReplay.hpp>

#include <MxMDPublisher.hpp>
#include <MxMDSubscriber.hpp>

#include <MxMDTelemetry.hpp>

class MxMDCore;

extern "C" {
  typedef void (*MxMDFeedPluginFn)(MxMDCore *md, ZvCf *cf);
};

class MxMDAPI MxMDCmd : public ZmPolymorph, public ZvCmdServer { };

class MxMDAPI MxMDCore : public MxMDLib, public MxEngineMgr {
friend class MxMDLib;

public:
  static unsigned vmajor();
  static unsigned vminor();

  typedef MxMultiplex Mx;

private:
  typedef ZmRBTree<ZmRef<Mx>,
	    ZmRBTreeIndex<Mx::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeBase<ZmObject> > > > > MxTbl;

  MxMDCore(ZmRef<MxTbl> mxTbl, Mx *mx);

  void init_(ZvCf *cf);

public:
  ~MxMDCore() { }

  ZuInline ZvCf *cf() { return m_cf.ptr(); }

  ZuInline Mx *mx() const { return m_mx; }
  ZuInline Mx *mx(const Mx::ID &id) const {
    MxTbl::Node *node = m_mxTbl->find(id);
    if (!node) return nullptr;
    return node->key();
  }
  template <typename L>
  ZuInline void allMx(L l) const {
    auto i = m_mxTbl->readIterator();
    while (MxTbl::Node *node = i.iterate()) l(node->key());
  }

  void start();
  void stop();
  void final();

  bool record(ZuString path);
  ZtString stopRecording();

  bool replay(ZuString path,
      MxDateTime begin = MxDateTime(),
      bool filter = true);
  ZtString stopReplaying();

  void startTimer(MxDateTime begin = MxDateTime());
  void stopTimer();

  void dumpTickSizes(ZuString path, MxID venue = MxID());
  void dumpInstruments(
      ZuString path, MxID venue = MxID(), MxID segment = MxID());
  void dumpOrderBooks(
      ZuString path, MxID venue = MxID(), MxID segment = MxID());

  void addCmd(ZuString name, ZuString syntax,
      ZvCmdFn fn, ZtString brief, ZtString usage);

  // for use by replay

  using Hdr = MxMDStream::Hdr;

  void pad(Hdr &);
  void apply(const Hdr &, bool filter);

private:
  void initCmds();

  void l1(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void l2(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void l2_side(MxMDOBSide *, ZtString &);
  void instrument_(
      ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);

  void ticksizes(
      ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void instruments(
      ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void orderbooks(
      ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);

#if 0
  void tick(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void update(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);

  void feeds(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
  void venues(ZvCmdServerCxn *, ZvCf *, ZmRef<ZvCmdMsg>, ZmRef<ZvCmdMsg> &);
#endif

  void addVenueMapping_(ZuAnyPOD *);
  void addTickSize_(ZuAnyPOD *);
  void addInstrument_(ZuAnyPOD *);
  void addOrderBook_(ZuAnyPOD *);

  // Engine Management
  void addEngine(MxEngine *engine) {
    if (m_telemetry) m_telemetry->addEngine(engine);
  }
  void delEngine(MxEngine *) { }
  void engineState(MxEngine *, MxEnum, MxEnum) { }

  // Link Management
  void updateLink(MxAnyLink *) { }
  void delLink(MxAnyLink *) { }
  void linkState(MxAnyLink *link, MxEnum, MxEnum) { }

  // Pool Management
  void updateTxPool(MxAnyTxPool *) { }
  void delTxPool(MxAnyTxPool *) { }

  // Queue Management
  void addQueue(MxID id, bool tx, MxQueue *queue) {
    if (m_telemetry) m_telemetry->addQueue(id, tx, queue);
  }
  void delQueue(MxID id, bool tx) {
    if (m_telemetry) m_telemetry->delQueue(id, tx);
  }

public:
  // DB Management
  void addDBEnv(ZdbEnv *env) {
    if (m_telemetry) m_telemetry->addDBEnv(env);
  }

private:
  // Exception handling
  void exception(ZmRef<ZeEvent> e) { raise(ZuMv(e)); }

  // Traffic Logging (logThread)
  /* Example usage:
  app.log(id, MxTraffic([](const Msg *msg, ZmTime &stamp, ZuString &data) {
    stamp = msg->stamp();
    data = msg->buf();
  }, msg)); */
  void log(MxMsgID, MxTraffic) { }

public:
  inline MxMDBroadcast &broadcast() { return m_broadcast; }

  inline bool streaming() { return m_broadcast.active(); }

  template <typename Snapshot>
  bool snapshot(Snapshot &snapshot, MxID id, MxSeqNo seqNo) {
    bool ok = !(allVenues([&snapshot](MxMDVenue *venue) -> uintptr_t {
      return !MxMDStream::addVenue(
	  snapshot, venue->id(), venue->flags(), venue->orderIDScope()) ||
	venue->allTickSizeTbls(
	      [&snapshot, venue](MxMDTickSizeTbl *tbl) -> uintptr_t {
	  return !MxMDStream::addTickSizeTbl(
	      snapshot, venue->id(), tbl->id(), tbl->pxNDP()) ||
	    tbl->allTickSizes(
		[&snapshot, venue, tbl](const MxMDTickSize &ts) -> uintptr_t {
	    return !MxMDStream::addTickSize(snapshot,
		venue->id(), ts.minPrice(), ts.maxPrice(), ts.tickSize(),
		tbl->id(), tbl->pxNDP());
	  });
	});
    }) || allInstruments([&snapshot](MxMDInstrument *instrument) -> uintptr_t {
      return !MxMDStream::addInstrument(snapshot, instrument->shard()->id(),
	  MxDateTime(), instrument->key(), instrument->refData());
    }) || allOrderBooks([&snapshot](MxMDOrderBook *ob) -> uintptr_t {
      if (!MxMDStream::resetOB(snapshot, ob->shard()->id(),
	  MxDateTime(), ob->key())) return true;
      if (ob->legs() == 1) {
	return !MxMDStream::addOrderBook(snapshot, ob->shard()->id(),
	    MxDateTime(), ob->key(), ob->instrument()->key(),
	    ob->lotSizes(), ob->tickSizeTbl()->id(), ob->qtyNDP());
      } else {
	MxInstrKey instrumentKeys[MxMDNLegs];
	MxEnum sides[MxMDNLegs];
	MxRatio ratios[MxMDNLegs];
	for (unsigned i = 0, n = ob->legs(); i < n; i++) {
	  instrumentKeys[i] = ob->instrument(i)->key();
	  sides[i] = ob->side(i);
	  ratios[i] = ob->ratio(i);
	}
	return !MxMDStream::addCombination(snapshot, ob->shard()->id(),
	    MxDateTime(), ob->key(), ob->legs(), instrumentKeys,
	    ratios, ob->lotSizes(), ob->tickSizeTbl()->id(),
	    ob->pxNDP(), ob->qtyNDP(), sides);
      }
    }) || allVenues([&snapshot](MxMDVenue *venue) -> uintptr_t {
      return (venue->loaded() &&
	  !MxMDStream::refDataLoaded(snapshot, venue->id())) ||
	venue->allSegments([&snapshot, venue](
	      const MxMDSegment &segment) -> uintptr_t {
	  return !MxMDStream::tradingSession(snapshot, segment.stamp,
	      venue->id(), segment.id, segment.session);
	});
    }) || allOrderBooks([&snapshot](MxMDOrderBook *ob) -> uintptr_t {
      return !MxMDStream::l1(snapshot, ob->shard()->id(),
	  ob->key(), ob->l1Data()) ||
	!snapshotL2Side(snapshot, ob->bids()) ||
	!snapshotL2Side(snapshot, ob->asks());
    }) || !MxMDStream::endOfSnapshot(snapshot, id, seqNo, (uint8_t)true));
    MxMDStream::endOfSnapshot(m_broadcast, id, seqNo, ok);
    return ok;
  }

private:
  template <class Snapshot>
  static bool snapshotL2Side(Snapshot &snapshot, MxMDOBSide *side) {
    return !(!(!side->mktLevel() ||
	  snapshotL2PxLvl(snapshot, side->mktLevel())) ||
      side->allPxLevels([&snapshot](MxMDPxLevel *pxLevel) -> uintptr_t {
	return !snapshotL2PxLvl(snapshot, pxLevel);
      }));
  }
  template <class Snapshot>
  static bool snapshotL2PxLvl(Snapshot &snapshot, MxMDPxLevel *pxLevel) {
    unsigned orderCount = 0;
    if (pxLevel->allOrders(
	  [&snapshot, &orderCount](MxMDOrder *order) -> uintptr_t {
      ++orderCount;
      const MxMDOrderData &data = order->data();
      MxMDOrderBook *ob = order->orderBook();
      return !MxMDStream::addOrder(snapshot, ob->shard()->id(),
	  data.transactTime, ob->key(),
	  data.price, data.qty, data.rank, data.flags,
	  order->id(), ob->pxNDP(), ob->qtyNDP(), data.side);
    })) return false;
    if (orderCount) return true;
    const MxMDPxLvlData &data = pxLevel->data();
    MxMDOrderBook *ob = pxLevel->obSide()->orderBook();
    return MxMDStream::pxLevel(snapshot, ob->shard()->id(),
	data.transactTime, ob->key(), pxLevel->price(), data.qty,
	data.nOrders, data.flags, ob->pxNDP(), ob->qtyNDP(),
	pxLevel->side(), (uint8_t)false);
  }

private:
  ZmRef<MxMDVenue> venue_(MxID id, MxEnum orderIDScope, MxFlags flags);

private:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  void timer();

  Lock			m_stateLock; // prevents overlapping start() / stop()

  ZmRef<ZvCf>		m_cf;

  ZmRef<MxTbl>		m_mxTbl;
  Mx			*m_mx = 0;

  ZmRef<MxMDTelemetry>	m_telemetry;
  ZmRef<MxMDCmd>	m_cmd;

  MxMDBroadcast		m_broadcast;	// broadcasts updates

  ZmRef<MxMDRecord>	m_record;	// records to file
  ZmRef<MxMDReplay>	m_replay;	// replays from file

  ZmRef<MxMDPublisher>	m_publisher;	// publishes to network
  ZmRef<MxMDSubscriber>	m_subscriber;	// subscribes from network

  ZmRef<MxMDFeed>	m_localFeed;

  ZmScheduler::Timer	m_timer;
  Lock			m_timerLock;
    ZmTime		  m_timerNext;	// time of next timer event
};

ZuInline MxMDCore *MxMDRecord::core() const
  { return static_cast<MxMDCore *>(mgr()); }
ZuInline MxMDCore *MxMDReplay::core() const
  { return static_cast<MxMDCore *>(mgr()); }
ZuInline MxMDCore *MxMDPublisher::core() const
  { return static_cast<MxMDCore *>(mgr()); }
ZuInline MxMDCore *MxMDSubscriber::core() const
  { return static_cast<MxMDCore *>(mgr()); }

#endif /* MxMDCore_HPP */
