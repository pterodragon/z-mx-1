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

#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmThread.hpp>
#include <ZuPOD.hpp>

#include <ZePlatform.hpp>

#include <ZvCf.hpp>
#include <ZvRingCf.hpp>
#include <ZvThreadCf.hpp>
#include <ZvCmd.hpp>

#include <MxMultiplex.hpp>
#include <MxEngine.hpp>

#include <MxMD.hpp>
#include <MxMDCmd.hpp>
#include <MxMDStream.hpp>

#include <MxMDRing.hpp>
#include <MxMDThread.hpp>

#include <MxMDRecord.hpp>
#include <MxMDReplay.hpp>

#include <MxMDPublisher.hpp>
#include <MxMDSubscriber.hpp>

class MxMDCore;

extern "C" {
  typedef void (*MxMDFeedPluginFn)(MxMDCore *md, ZvCf *cf);
};

class MxMDAPI MxMDCore :
  public ZmPolymorph, public MxMDLib, public MxMDCmd,
  public ZmThreadMgr, public MxEngineMgr {
friend class MxMDLib;
friend class MxMDCmd;

public:
  static unsigned vmajor();
  static unsigned vminor();

  typedef MxMultiplex Mx;

private:
  class CmdServer : public ZvCmdServer {
  friend class MxMDLib;
  friend class MxMDCmd;
  friend class MxMDCore;

    typedef ZmRBTree<ZtString,
	      ZmRBTreeVal<ZuTuple<CmdFn, ZtString, ZtString>,
		ZmRBTreeLock<ZmNoLock> > > Cmds;

    void rcvd(ZvCmdLine *, const ZvInvocation &, ZvAnswer &);

    void help(const CmdArgs &args, ZtArray<char> &result);

    CmdServer(MxMDCore *md);
  public:
    virtual ~CmdServer() { }

  private:
    void init(ZvCf *cf);
    void final();

    typedef ZuBoxFmt<ZuBox<unsigned>, ZuFmt::Right<6> > TimeFmt;
    inline TimeFmt timeFmt(MxDateTime t) {
      return ZuBox<unsigned>((t + m_tzOffset).hhmmss()).fmt(ZuFmt::Right<6>());
    }

    MxMDCore	*m_md;
    ZmRef<ZvCf>	m_syntax;
    Cmds	m_cmds;
    int		m_tzOffset = 0;
  };

  MxMDCore(ZmRef<Mx> mx);

  void init_(ZvCf *cf);

public:
  virtual ~MxMDCore() { }

  ZuInline Mx *mx() { return m_mx.ptr(); }
  ZuInline ZvCf *cf() { return m_cf.ptr(); }

  void start();
  void stop();
  void final();

  void record(ZuString path);
  void stopRecording();

  void publish();
  void stopPublish();

  void subscribe();
  void stopSubscribe();

  void stopStreaming();

  void replay(
    ZuString path,
    MxDateTime begin = MxDateTime(),
    bool filter = true);
  void stopReplaying();

  void startTimer(MxDateTime begin = MxDateTime());
  void stopTimer();

  void dumpTickSizes(ZuString path, MxID venue = MxID());
  void dumpSecurities(
      ZuString path, MxID venue = MxID(), MxID segment = MxID());
  void dumpOrderBooks(
      ZuString path, MxID venue = MxID(), MxID segment = MxID());

private:
  typedef MxMDStream::Frame Frame;
  typedef MxMDStream::Msg Msg;

  void pad(Frame *);
  void apply(const Frame *);

  void startCmdServer();
  void initCmds();

  void l1(const CmdArgs &, ZtArray<char> &);
  void l2(const CmdArgs &, ZtArray<char> &);
  void l2_side(MxMDOBSide *, ZtArray<char> &);
  void security_(const CmdArgs &, ZtArray<char> &);

  void ticksizes(const CmdArgs &, ZtArray<char> &);
  void securities(const CmdArgs &, ZtArray<char> &);
  void orderbooks(const CmdArgs &, ZtArray<char> &);

  void recordCmd(const CmdArgs &, ZtArray<char> &);
  // void subscribeCmd(const CmdArgs &, ZtArray<char> &);
  void replayCmd(const CmdArgs &, ZtArray<char> &);

#if 0
  void tick(const CmdArgs &, ZtArray<char> &);
  void update(const CmdArgs &, ZtArray<char> &);

  void feeds(const CmdArgs &, ZtArray<char> &);
  void venues(const CmdArgs &, ZtArray<char> &);
#endif

  void addVenueMapping_(ZuAnyPOD *);
  void addTickSize_(ZuAnyPOD *);
  void addSecurity_(ZuAnyPOD *);
  void addOrderBook_(ZuAnyPOD *);

  // Engine Management
  void addEngine(MxEngine *) { }
  void delEngine(MxEngine *) { }
  void engineState(MxEngine *, MxEnum) { }

  // Link Management
  void updateLink(MxAnyLink *) { }
  void delLink(MxAnyLink *) { }
  void linkState(MxAnyLink *, MxEnum, ZuString txt) { }

  // Pool Management
  void updateTxPool(MxAnyTxPool *) { }
  void delTxPool(MxAnyTxPool *) { }

  // Queue Management
  void addQueue(MxID id, bool tx, MxQueue *) { }
  void delQueue(MxID id, bool tx) { }

  // Exception handling
  void exception(ZmRef<ZeEvent> e) { raise(ZuMv(e)); }

  // Traffic Logging (logThread)
  /* Example usage:
  app.log(id, MxTraffic([](const Msg *msg, ZmTime &stamp, ZuString &data) {
    stamp = msg->stamp();
    data = msg->buf();
  }, msg)); */
  void log(MxMsgID, MxTraffic) { }

  struct ThreadID {
    enum {
      Recorder = 0,
      Snapper
    };
  };

  void threadName(ZmThreadName &, unsigned id);

  inline MxMDRing &broadcast() { return m_broadcast; }

  inline bool streaming() { return m_broadcast.active(); }

  template <typename Snapshot>
  bool snapshot(Snapshot &snapshot) {
    MxSeqNo seqNo = m_broadcast.seqNo();
    return !(allVenues([&snapshot](MxMDVenue *venue) -> uintptr_t {
      return venue->allTickSizeTbls(
	    [&snapshot, venue](MxMDTickSizeTbl *tbl) -> uintptr_t {
	return !MxMDStream::addTickSizeTbl(snapshot, venue->id(), tbl->id()) ||
	  tbl->allTickSizes(
	      [&snapshot, venue, tbl](const MxMDTickSize &ts) -> uintptr_t {
	  return !MxMDStream::addTickSize(snapshot,
	      venue->id(), tbl->id(), tbl->pxNDP(),
	      ts.minPrice(), ts.maxPrice(), ts.tickSize());
	});
      });
    }) || allSecurities([&snapshot](MxMDSecurity *security) -> uintptr_t {
      return !MxMDStream::addSecurity(snapshot, MxDateTime(),
	  security->key(), security->shard()->id(),
	  security->refData());
    }) || allOrderBooks([&snapshot](MxMDOrderBook *ob) -> uintptr_t {
      if (ob->legs() == 1) {
	return !MxMDStream::addOrderBook(snapshot, MxDateTime(),
	    ob->key(), ob->security()->key(), ob->tickSizeTbl()->id(),
	    ob->qtyNDP(), ob->lotSizes());
      } else {
	MxSecKey securityKeys[MxMDNLegs];
	MxEnum sides[MxMDNLegs];
	MxRatio ratios[MxMDNLegs];
	for (unsigned i = 0, n = ob->legs(); i < n; i++) {
	  securityKeys[i] = ob->security(i)->key();
	  sides[i] = ob->side(i);
	  ratios[i] = ob->ratio(i);
	}
	return !MxMDStream::addCombination(snapshot, MxDateTime(),
	    ob->key(), ob->pxNDP(), ob->qtyNDP(),
	    ob->legs(), securityKeys, sides, ratios,
	    ob->tickSizeTbl()->id(), ob->lotSizes());
      }
    }) || allVenues([&snapshot](MxMDVenue *venue) -> uintptr_t {
      return venue->allSegments([&snapshot, venue](
	    const MxMDSegment &segment) -> uintptr_t {
	return !MxMDStream::tradingSession(snapshot, segment.stamp,
	    venue->id(), segment.id, segment.session);
      });
    }) || allOrderBooks([&snapshot](MxMDOrderBook *ob) -> uintptr_t {
      return !MxMDStream::l1(snapshot, ob->key(), ob->l1Data()) ||
	!snapshotL2Side(snapshot, ob->bids()) ||
	!snapshotL2Side(snapshot, ob->asks());
    }) || !MxMDStream::endOfSnapshot(snapshot, seqNo));
  }
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
      return !MxMDStream::addOrder(snapshot, data.transactTime,
	  ob->key(), order->id(), data.side, ob->pxNDP(), ob->qtyNDP(),
	  data.rank, data.price, data.qty, data.flags);
    })) return false;
    if (orderCount) return true;
    const MxMDPxLvlData &data = pxLevel->data();
    MxMDOrderBook *ob = pxLevel->obSide()->orderBook();
    return MxMDStream::pxLevel(snapshot, data.transactTime,
	ob->key(), pxLevel->side(), false, ob->pxNDP(), ob->qtyNDP(),
	pxLevel->price(), data.qty, data.nOrders, data.flags);
  }

  // FIXME - refactor so void *push(size) and push2() are run-time ZmFn
  // associated with each snapshot, i.e. snapshot list is of ZmFn pairs,
  // not of ZiRingParams;
  // - remove dedicated thread; move to using thread specified in
  // "snapper" section of cf, per recorder/subscriber/publisher;
  // move to MxMDSnapper
  // FIXME - rework to use MxMDSnapper, not own built-in snapshot thread;
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  ZmRef<MxMDVenue> venue(MxID id);

  void timer();

  Lock			m_stateLock; // prevents overlapping start() / stop()

  ZmRef<Mx>		m_mx;
  ZmRef<ZvCf>		m_cf;
  ZmRef<CmdServer>	m_cmd;

  ZtString		m_recordCfPath;
  ZtString		m_replayCfPath;

  MxMDThread		m_recorderThread;
  MxMDThread		m_snapperThread;

  MxMDRing		m_broadcast;	// broadcasts updates

  MxMDRecord		m_record;	// records to file
  MxMDReplay		m_replay;	// replays from file

  MxMDPublisher		m_publisher;	// publishes to network
  MxMDSubscriber	m_subscriber;	// subscribes to network

  ZmRef<MxMDFeed>	m_localFeed;

  ZmScheduler::Timer	m_timer;
};

#endif /* MxMDCore_HPP */
