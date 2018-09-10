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

#include <MxMDMsg.hpp>

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

inline auto MxMDFileDiag(ZuString path, ZeError e) {
  return [path = MxTxtString(path), e](const ZeEvent &, ZmStream &s) {
      s << "MxMD \"" << path << "\": " << e; };
}

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
  void apply(Frame *);

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
  void subscribeCmd(const CmdArgs &, ZtArray<char> &);
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

  // Traffic Logging (logThread)
  /* Example usage:
  app.log(id, MxTraffic([](const Msg *msg, ZmTime &stamp, ZuString &data) {
    stamp = msg->stamp();
    data = msg->buf();
  }, msg)); */
  void log(MxMsgID, MxTraffic) { }

  struct ThreadID {
    enum {
      RecReceiver = 0,
      RecSnapper,
      Snapper
    };
  };

  void threadName(ZmThreadName &, unsigned id);

  template <class Lock> class Channel {
  public:
    typedef ZmGuard<Lock> Guard;
    typedef MxMDStream::Ring Ring;

    inline Channel(MxMDCore *core) : m_core(core) {
      m_config.name("RMD").size(131072); // 131072 is ~100mics at 1Gbit/s
    }
    inline Channel(MxMDCore *core, const ZiRingParams &config) :
      m_core(core), m_config(config) { }

    inline ~Channel() { close_(); }

    inline void init(ZvCf *cf) { m_config.init(cf); }

    inline const ZiRingParams &config() const { return m_config; }

    bool open() {
      Guard guard(m_lock);
      ++m_openCount;
      if (m_ring) return true;
      m_ring = new Ring(m_config);
      ZeError e;
      if (m_ring->open(Ring::Create | Ring::Read | Ring::Write, &e) < 0) {
	m_openCount = 0;
	m_ring = 0;
	guard.unlock();
	m_core->raise(ZeEVENT(Error,
	  ([name = MxTxtString(m_config.name()), e](
	    const ZeEvent &, ZmStream &s) {
	      s << '"' << name << "\": "
	      "failed to open IPC shared memory ring buffer: " << e;
	    })));
	return false;
      }
      return true;
    }

    void close() {
      Guard guard(m_lock);
      if (!m_openCount) return;
      if (!--m_openCount) close_();
    }

  private:
    void close_() {
      if (ZuLikely(m_ring)) {
	m_ring->close();
	m_ring = 0;
      }
    }

  public:
    inline void eof() {
      Guard guard(m_lock);
      if (ZuUnlikely(!m_ring)) return;
      m_openCount = 0;
      m_ring->eof();
      m_ring->close();
      m_ring = 0;
    }

    inline bool active() { return m_openCount; }

    Frame *push(unsigned size) {
      m_lock.lock();
      if (ZuUnlikely(!m_ring)) { m_lock.unlock(); return 0; }
    retry:
      if (void *ptr = m_ring->push(size)) return (MxMDStream::Frame *)ptr;
      int i = m_ring->writeStatus();
      if (ZuUnlikely(i == Zi::EndOfFile)) { // should never happen
	m_ring->eof(false);
	goto retry;
      }
      m_openCount = 0;
      m_ring->eof();
      m_ring->close();
      m_ring = 0;
      m_lock.unlock();
      if (i != Zi::NotReady)
	m_core->raise(ZeEVENT(Error,
	  ([name = MxTxtString(m_config.name())](
	      const ZeEvent &, ZmStream &s) {
	    s << '"' << name << "\": "
	    "IPC shared memory ring buffer overflow";
	  })));
      return 0;
    }

    inline void push2() {
      if (ZuLikely(m_ring)) m_ring->push2();
      m_lock.unlock();
    }

    inline int writeStatus() {
      Guard guard(m_lock);
      if (ZuUnlikely(!m_ring)) return Zi::NotReady;
      return m_ring->writeStatus();
    }

    void reqDetach(int id = -1) {
      if (id < 0) {
	Guard guard(m_lock);
	if (!m_ring) return;
	id = m_ring->id();
      }
      MxMDStream::detach(*this, id);
    }

    // caller must ensure channel is open during below calls
    inline int attach() { return m_ring->attach(); }
    inline int detach() { return m_ring->detach(); }
    inline const Frame *shift() { return m_ring->shift(); }
    inline void shift2() { m_ring->shift2(); }
    inline int readStatus() { return m_ring->readStatus(); }
    inline int id() { return m_ring->id(); }

  protected:
    MxMDCore			*m_core;
    ZvRingParams		m_config;
    Lock			m_lock;
    unsigned			  m_openCount = 0;
    ZmRef<MxMDStream::Ring>	  m_ring;
  };

  typedef Channel<ZmPLock> Broadcast;
  typedef Channel<ZmNoLock> Unicast;

  inline Broadcast &broadcast() { return m_broadcast; }

  inline bool streaming() { return m_broadcast.active(); }

  template <typename Snapshot>
  bool snapshot(Snapshot &snapshot) {
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
    }) || !MxMDStream::endOfSnapshot(snapshot));
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

  class Snapper;
friend class Snapper;
  class Snapper {
  public:
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;

    inline Snapper(MxMDCore *core) : m_core(core) { }

    void snap(const ZiRingParams &ringParams) {
      Guard guard(m_lock);
      m_snapshots.push(ringParams);
      if (!m_thread)
	m_thread = ZmThread(m_core, ThreadID::Snapper,
	    ZmFn<>::Member<&Snapper::run>::fn(this),
	    m_core->m_snapperParams);
    }

    void stop() {
      Guard guard(m_lock);
      if (!!m_thread) {
	m_snapshots.push(ZiRingParams(""));
	m_thread.join();
	m_thread = ZmThread();
	m_snapshots.clean();
      }
    }

  private:
    typedef ZmList<ZiRingParams,
	      ZmListLock<ZmPLock> > Snapshots;

    void run() {
      for (;;) {
	m_sem.wait();
	ZiRingParams ringParams = m_snapshots.shift();
	if (!ringParams.name()) return;
	Unicast snapshot(m_core, ringParams);
	if (!snapshot.open()) continue;
	m_core->snapshot(snapshot);
	snapshot.eof();
	snapshot.close();
      }
    }

  private:
    MxMDCore		*m_core;
    ZmSemaphore		m_sem;
    Snapshots		m_snapshots;
    Lock		m_lock;
    ZmThread		  m_thread;
  };

  class Recorder;
friend class Recorder;
  class Recorder : public MxEngineApp, public MxEngine {
  public:
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;
    typedef MxMDStream::Msg Msg;

    // Rx (called from engine's rx thread)
    MxEngineApp::ProcessFn processFn() {
      return static_cast<MxEngineApp::ProcessFn>(
	  [](MxEngineApp *app, MxAnyLink *, MxQMsg *msg) {
	static_cast<Recorder *>(app)->writeMsg(msg);
      });
    }

    // Tx (called from engine's tx thread)
    void sent(MxAnyLink *, MxQMsg *) { }
    void aborted(MxAnyLink *, MxQMsg *) { }
    void archive(MxAnyLink *, MxQMsg *) { }
    ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

    class Link : public MxLink<Link> {
    public:
      Link(MxID id) : MxLink<Link>{id} { }

      void init(MxEngine *engine) { MxLink<Link>::init(engine); }

      ZmTime reconnectInterval(unsigned reconnects) { return ZmTime{1}; }
      ZmTime reRequestInterval() { return ZmTime{1}; }

      // Rx
      void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
      void reRequest(const MxQueue::Gap &now) { }

      // Tx
      bool send_(MxQMsg *msg, bool more) { return true; }
      bool resend_(MxQMsg *msg, bool more){ return true; }

      bool sendGap_(const MxQueue::Gap &gap, bool more){ return true; }
      bool resendGap_(const MxQueue::Gap &gap, bool more){ return true; }

      MxID id() const { return MxAnyTx::id(); }

      void update(ZvCf *cf) { }
      void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo) { }

      void connect() { MxAnyLink::connected(); }
      void disconnect() { MxAnyLink::disconnected(); }
    };

  private:
    struct SnapQueue_HeapID {
      inline static const char *id() { return "MxMD.SnapQueue"; }
    };
  public:
    typedef MxQueueRx<Link> Rx;
    typedef ZmList<ZmRef<Msg>,
	      ZmListObject<ZuNull,
		ZmListLock<ZmNoLock,
		  ZmListHeapID<SnapQueue_HeapID> > > > SnapQueue;

    inline Recorder(MxMDCore *core) : m_core(core) { }

    void init() {
      ZmRef<ZvCf> cf = m_core->cf()->subset("recorder", false, true);
      MxEngine::init(m_core, this, m_core->mx(), cf);
      m_link = MxEngine::updateLink<Link>("recorder", nullptr);
      m_link->init(this);
    }

    inline ~Recorder() { m_file.close(); }

    inline ZiFile::Path path() const {
      ReadGuard guard(m_ioLock);
      return m_path;
    }

    inline bool running() const {
      ReadGuard guard(m_threadLock);
      return m_snapThread || m_recvRunning;
    }

    template <typename Path> bool start(Path &&path) {
      if (!fileOpen(ZuFwd<Path>(path))) return false;
      if (!m_core->broadcast().open()) return false;
      MxEngine::start();
      recvStart();
      snapStart();
      return true;
    }

    void stop() {
      snapStop();
      recvStop();
      MxEngine::stop();
      m_core->broadcast().close();
      fileClose();
    }

    template <typename Path> bool fileOpen(const Path &path) {
      Guard guard(m_ioLock);
      if (!!m_file && m_path == path) return true;
      if (!!m_file) { m_file.close(); m_path = ""; }
      try {
	ZeError e;
	if (m_file.open(path,
	      ZiFile::WriteOnly | ZiFile::Append | ZiFile::Create,
	      0666, &e) != Zi::OK)
	  throw ZeEVENT(Error, MxMDFileDiag(path, e));
	if (!m_file.offset()) {
	  using namespace MxMDStream;
	  FileHdr hdr("RMD", MxMDCore::vmajor(), MxMDCore::vminor());
	  if (m_file.write(&hdr, sizeof(FileHdr), &e) != Zi::OK)
	    throw ZeEVENT(Error, MxMDFileDiag(path, e));
	}
	m_path = path;
      } catch (ZeEvent *e) {
	m_file.close();
	guard.unlock();
	m_core->raise(e);
	return false;
      }
      return true;
    }
    void fileClose() {
      Guard guard(m_ioLock);
      m_file.close();
    }

    void recvStart() {
      {
	Guard guard(m_threadLock);
	if (m_recvRunning) return;
	m_recvRunning = 1;
      }
      m_link->rxInvoke([](Rx *rx) {
	static_cast<Recorder *>(
	    static_cast<Link *>(rx)->engine())->recv(rx);
      });
      m_attachSem.wait();
    }
    void recvStop() {
      Guard guard(m_threadLock);
      if (!m_recvRunning) return;
      using namespace MxMDStream;
      m_core->broadcast().reqDetach();
      m_detachSem.wait();
      m_recvRunning = 0;
    }

    void snapStart() {
      Guard guard(m_threadLock);
      if (!!m_snapThread) return;
      m_snapThread = ZmThread(m_core, ThreadID::RecSnapper,
	  ZmFn<>::Member<&Recorder::snap>::fn(this),
	  m_core->m_recSnapperParams);
    }
    void snapStop() {
      Guard guard(m_threadLock);
      if (m_snapThread) return;
      m_snapThread.join();
      m_snapThread = ZmThread();
    }

    inline bool active() {
      Guard guard(m_threadLock);
      return m_recvRunning;
    }

  private:
    void recv(Rx *rx) {
      m_core->raise(ZeEVENT(Info, "Recorder recv"));

      rx->startQueuing();
      using namespace MxMDStream;

      Broadcast &broadcast = m_core->broadcast();
      if (broadcast.attach() != Zi::OK) {
	m_attachSem.post();
	Guard guard(m_threadLock);
	m_recvRunning = 0;
	return;
      }

      m_attachSem.post();
      m_link->rxRun([](Rx *rx) {
	static_cast<Recorder *>(
	    static_cast<Link *>(rx)->engine())->recvMsg(rx);
      });
    }

    void recvMsg(Rx *rx) {
      using namespace MxMDStream;
      const Frame *frame;
      Broadcast &broadcast = m_core->broadcast();
      if (ZuUnlikely(!(frame = broadcast.shift()))) {
	if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) goto end;
	goto next;
      }
      if (ZuUnlikely(frame->type == Type::Detach)) {
	const Detach &detach = frame->as<Detach>();
	if (ZuUnlikely(detach.id == broadcast.id())) {
	  broadcast.shift2();
	  goto end;
	}
	broadcast.shift2();
	goto next;
      }
      {
	MsgRef msg = new Msg();

	if (frame->len > sizeof(Buf)) {
	  broadcast.shift2();
	  snapStop();
	  m_core->raise(ZeEVENT(Error,
	      ([name = MxTxtString(broadcast.config().name())](
		  const ZeEvent &, ZmStream &s) {
		s << '"' << name << "\": "
		"IPC shared memory ring buffer read error - "
		"message too big";
	      })));
	  goto end;
	}
	MxSeqNo seqNo = ++m_msgSeqNo;
	memcpy(msg->data().frame(), frame, sizeof(Frame) + frame->len);
	msg->data().frame()->seqNo = seqNo;

	ZmRef<MxQMsg> qmsg = new MxQMsg(MxFlags{}, ZmTime{}, msg);
	qmsg->load(rx->rxQueue().id, seqNo);
	rx->received(qmsg);
      }
      broadcast.shift2();

    next:
      m_link->rxRun([](Rx *rx) {
	static_cast<Recorder *>(
	  static_cast<Link *>(rx)->engine())->recvMsg(rx);
      });
      return;

    end:
      broadcast.detach();
      m_detachSem.post();
    }

    int write(const Frame *frame, ZeError *e) {
	    
	    
	    std::cout << "seqNo: " << frame->seqNo << std::endl;
	    std::cout << "len: " << frame->len << std::endl;
	    std::cout << "type: " << (int)frame->type << std::endl;
	    
	    
	    
	    
      return m_file.write((const void *)frame, sizeof(Frame) + frame->len, e);
    }

    void snap() {
      {
	Guard guard(m_ioLock);
	m_snapMsg = new Msg();
      }
      bool failed = !m_core->snapshot(*this);
      {
	Guard guard(m_ioLock);
	if (failed || !m_snapMsg) {
	  m_file.close();
	  guard.unlock();
	  recvStop();
	  return;
	}
      }
      m_link->rxInvoke([](Rx *rx) { rx->stopQueuing(1); });
      
      
      
      
      std::cout << "************ stopQueuing ***************"  << std::endl;
      
      
      
    }

  public:
    void writeMsg(MxQMsg *qmsg) {
      ZeError e;
      Guard guard(m_ioLock);

      if (ZuUnlikely(write(qmsg->template as<Msg>().frame(), &e)) != Zi::OK) {
      //if (ZuUnlikely(write(qmsg->template as<ZuAnyPOD>().template as<Msg>().frame(), &e)) != Zi::OK) {
	ZiFile::Path path = m_path;
	m_file.close();
	guard.unlock();
	recvStop();
	m_core->raise(ZeEVENT(Error, MxMDFileDiag(path, e)));
	return;
      }
    }

    Frame *push(unsigned size) {
      m_ioLock.lock();
      if (ZuUnlikely(!m_snapMsg)) {
	m_ioLock.unlock();
	return 0;
      }

      return m_snapMsg->frame();
    }
    void push2() {
      if (ZuUnlikely(!m_snapMsg)) { m_ioLock.unlock(); return; }
      ZeError e;

      if (ZuUnlikely(write(m_snapMsg->frame(), &e) != Zi::OK)) {
	m_snapMsg = 0;
	ZiFile::Path path = m_path;
	m_ioLock.unlock();
	m_core->raise(ZeEVENT(Error, MxMDFileDiag(path, e)));
	return;
      }
      m_ioLock.unlock();
    }

  private:
    MxSeqNo			m_msgSeqNo = 0;
    MxMDCore			*m_core;
    ZmSemaphore			m_attachSem;
    ZmSemaphore			m_detachSem;
    Lock			m_threadLock;
    ZmThread			  m_snapThread;
    bool			  m_recvRunning = false;
    Lock			m_ioLock;
    ZiFile::Path		  m_path;
    ZiFile			  m_file;
    ZmRef<Msg>			  m_snapMsg;
    ZmRef<MxLink<Link> >	m_link;
  };

  class Publisher;
friend class Publisher;
  class Publisher : public MxEngineApp, public MxEngine {

    class TCP;
  friend class TCP;
    class TCP : public ZiConnection {
    public:
      TCP(Publisher *publisher, const ZiCxnInfo &ci);

      ZuInline Publisher *publisher() const { return m_publisher; }

      void connected(ZiIOContext &);
      void disconnected() {}

      void send(const Frame *frame);

      template <typename MsgType>
      void send_(const Frame *frame)
      {
        using namespace MxMDPubSub::TCP;
        ZmRef<OutMsg> msg = new OutMsg();
        
	//memcpy(msg->out<MsgType>(), frame, sizeof(Frame) + frame->len);
	memcpy(msg->data(), frame, sizeof(Frame) + frame->len);
	
	
        msg->send(this);
	
	m_publisher->m_core->raise(ZeEVENT(Info, "Publisher TCP send_"));
      }

      void process(MxMDPubSub::TCP::InMsg *, ZiIOContext &);
      void disconnect();

    private:
      Publisher				*m_publisher;
      ZmRef<MxMDPubSub::TCP::InMsg>	m_in;
    };

    class UDP;
  friend class UDP;
    class UDP : public ZiConnection {
    public:
      UDP(Publisher *publisher, const ZiCxnInfo &ci, const ZiSockAddr &dest);

      ZuInline Publisher *publisher() const { return m_publisher; }

      void connected(ZiIOContext &io);
      void disconnected() {}

      void send(const Frame *frame);

      template <typename MsgType>
      void send_(const Frame *frame)
      {
	      
	      
	
      std::cout << "udp seqNo: " << frame->seqNo << std::endl;
      std::cout << "udp len: " << frame->len << std::endl;
      std::cout << "udp type: " << (int)frame->type << std::endl;
	      
	      
        using namespace MxMDPubSub::UDP;
        ZmRef<OutMsg> msg = new OutMsg();
        //memcpy(msg->out<MsgType>(), frame + sizeof(Frame), frame->len);
	memcpy(msg->data(), frame, sizeof(Frame) + frame->len);
        msg->send(this, m_remoteAddr);

	m_publisher->m_core->raise(ZeEVENT(Info, "Publisher UDP send_"));
      }

      void disconnect();

    private:
      Publisher		*m_publisher;
      //uint64_t		m_seqNo = 0;
      ZiSockAddr	m_remoteAddr;
  };

  public:
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;
    typedef MxMDStream::Msg Msg;

    // Rx (called from engine's rx thread)
    MxEngineApp::ProcessFn processFn() {
      return {};
    }

    // Tx (called from engine's tx thread)
    void sent(MxAnyLink *, MxQMsg *) { }
    void aborted(MxAnyLink *, MxQMsg *) { }
    void archive(MxAnyLink *, MxQMsg *) { }
    ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

    class Link : public MxLink<Link> {
    public:
      Link(MxID id) : MxLink<Link>{id} { }

      void init(MxEngine *engine) { MxLink<Link>::init(engine); }

      ZmTime reconnectInterval(unsigned reconnects) { return ZmTime{1}; }
      ZmTime reRequestInterval() { return ZmTime{1}; }

      // Rx
      void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
      void reRequest(const MxQueue::Gap &now) { }

      // Tx
      bool send_(MxQMsg *msg, bool more) {
	      
	      
        std::cout << "Publisher link send_" << std::endl;
	      
	      
        static_cast<Publisher *>(engine())->sendMsg(msg);
	sent_(msg);
        return true;
      }

      bool resend_(MxQMsg *msg, bool more){ return true; }

      bool sendGap_(const MxQueue::Gap &gap, bool more){ return true; }
      bool resendGap_(const MxQueue::Gap &gap, bool more){ return true; }

      MxID id() const { return MxAnyTx::id(); }

      void update(ZvCf *cf) { }
      void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo) { }

      void connect() { MxAnyLink::connected(); }
      void disconnect() { MxAnyLink::disconnected(); }
    };

  private:
    struct SnapQueue_HeapID {
      inline static const char *id() { return "MxMD.SnapQueue"; }
    };
  public:
    typedef MxQueueTx<Link> Tx;
    typedef ZmList<ZmRef<Msg>,
	      ZmListObject<ZuNull,
		ZmListLock<ZmNoLock,
		  ZmListHeapID<SnapQueue_HeapID> > > > SnapQueue;

    inline Publisher(MxMDCore *core) : m_core(core) { }

    void init() {
      ZmRef<ZvCf> cf = m_core->cf();
      MxEngine::init(m_core, this, m_core->mx(), cf->subset("publisher", false, true));
      m_link = MxEngine::updateLink<Link>("publisher", nullptr);
      m_link->init(this);
    }

    inline ~Publisher() = default;

    void tcpListen(const ZiListenInfo &) { }
    void tcpListenFailed(bool) {}
    ZiConnection *tcpConnected(const ZiCxnInfo &ci);
    void tcpConnected2(TCP *);

    void udpConnect();
    void udpConnectFailed(bool transient) {}
    ZiConnection *udpConnected(const ZiCxnInfo &ci);
    void udpConnected2(Publisher::UDP *udp, ZiIOContext &io);

    inline bool running() const {
      ReadGuard guard(m_threadLock);
      return m_snapThread || m_recvRunning;
    }

    bool start() {
      if (!m_core->broadcast().open()) return false;
      MxEngine::start();
      recvStart();
      ZmRef<ZvCf> cf = m_core->cf()->subset("publisher", false, true);
      auto port = static_cast<uint16_t>(strtoul(cf->get("localTcpPort", true).data(), nullptr, 10));

      m_core->raise(ZeEVENT(Info, "Publisher listen"));

      m_core->mx()->listen(
        ZiListenFn::Member<&Publisher::tcpListen>::fn(this),
        ZiFailFn::Member<&Publisher::tcpListenFailed>::fn(this),
	ZiConnectFn::Member<&Publisher::tcpConnected>::fn(this),
        ZiIP(), port, m_nAccepts);

      m_core->mx()->add(ZmFn<>::Member<&Publisher::udpConnect>::fn(this));
      return true;
    }

    void stop() {
      recvStop();
      MxEngine::stop();
      m_core->broadcast().close();
    }

    void recvStart() {
      {
	Guard guard(m_threadLock);
	if (m_recvRunning) return;
	m_recvRunning = 1;
      }

      m_link->txInvoke([](Tx *tx) {
	static_cast<Publisher *>(
	    static_cast<Link *>(tx)->engine())->recv(tx);
      });

      m_attachSem.wait();
    }
    void recvStop() {
      Guard guard(m_threadLock);
      if (!m_recvRunning) return;
      using namespace MxMDStream;
      m_core->broadcast().reqDetach();
      m_detachSem.wait();
      m_recvRunning = 0;
    }

    void snapStart() {
      Guard guard(m_threadLock);
      if (!!m_snapThread) return;
      m_snapThread = ZmThread(m_core, ThreadID::RecSnapper,
	  ZmFn<>::Member<&Publisher::snap>::fn(this),
	  m_core->m_recSnapperParams);
    }
    void snapStop() {
      Guard guard(m_threadLock);
      if (m_snapThread) return;
      m_snapThread.join();
      m_snapThread = ZmThread();
    }

    inline bool active() {
      Guard guard(m_threadLock);
      return m_recvRunning;
    }

  private:
    void recv(Tx *tx) {
      m_core->raise(ZeEVENT(Info, "Publisher recv"));

      using namespace MxMDStream;

      Broadcast &broadcast = m_core->broadcast();
      if (broadcast.attach() != Zi::OK) {
        m_core->raise(ZeEVENT(Info, "Publisher recv 3"));

	m_attachSem.post();
	Guard guard(m_threadLock);
	m_recvRunning = 0;
	return;
      }

      m_core->raise(ZeEVENT(Info, "Publisher recv 2"));
      
      m_attachSem.post();
      m_link->txRun([](Tx *tx) {
	static_cast<Publisher *>(
	    static_cast<Link *>(tx)->engine())->recvMsg(tx);
      });
    }

    void recvMsg(Tx *tx) {
      m_core->raise(ZeEVENT(Info, "Publisher recvMsg"));

      using namespace MxMDStream;
      const Frame *frame;
      Broadcast &broadcast = m_core->broadcast();
      if (ZuUnlikely(!(frame = broadcast.shift()))) {
	if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) goto end;
	goto next;
      }
      if (ZuUnlikely(frame->type == Type::Detach)) {
	const Detach &detach = frame->as<Detach>();
	if (ZuUnlikely(detach.id == broadcast.id())) {
	  broadcast.shift2();
	  goto end;
	}
	broadcast.shift2();
	goto next;
      }
      {
	MsgRef msg = new Msg();

	if (frame->len > sizeof(Buf)) {
	  broadcast.shift2();
	  snapStop();
	  m_core->raise(ZeEVENT(Error,
	      ([name = MxTxtString(broadcast.config().name())](
		  const ZeEvent &, ZmStream &s) {
		s << '"' << name << "\": "
		"IPC shared memory ring buffer read error - "
		"message too big";
	      })));
	  goto end;
	}

	std::cout << "---- " << (int)(frame->type) << std::endl;

	auto seqNo = ++m_msgSeqNo;
	memcpy(msg->frame(), frame, sizeof(Frame) + frame->len);
	msg->frame()->seqNo = seqNo;

	//std::cout << "---- " << (int)(msg->frame()->type) << std::endl;

	ZmRef<MxQMsg> qmsg = new MxQMsg(MxFlags{}, ZmTime{}, msg);

	//std::cout << "==== " << (int)(qmsg->payload->template as<MsgData>().frame()->type) << std::endl;

	qmsg->load(tx->txQueue().id, seqNo);
	tx->send(qmsg);
	
	m_core->raise(ZeEVENT(Info, "Publisher tx send"));
      }
      broadcast.shift2();

    next:
      m_link->txRun([](Tx *tx) {
	static_cast<Publisher *>(
	    static_cast<Link *>(tx)->engine())->recvMsg(tx); });
      return;

    end:
      broadcast.detach();
      m_detachSem.post();
    }

    void snap() {
      {
	Guard guard(m_ioLock);
	m_snapMsg = new Msg();
      }
      bool failed = !m_core->snapshot(*this);
      {
	Guard guard(m_ioLock);
	if (failed || !m_snapMsg) {
	  guard.unlock();
	  recvStop();
	  return;
	}
      }

      snapStop();
    }

  public:
    void sendMsg(MxQMsg *qmsg) {
      using namespace MxMDStream;
      
      
      /*
      auto frame = qmsg->payload->template as<MsgData>().frame();
      std::cout << "udp seqNo: " << frame->seqNo << std::endl;
      std::cout << "udp len: " << frame->len << std::endl;
      std::cout << "udp type: " << (int)frame->type << std::endl;*/
      
      
      

      Guard guard(m_ioLock);
      //m_udp->send(qmsg->template as<Msg>().frame());
      m_udp->send(qmsg->payload->template as<MsgData>().frame());
    }

    Frame *push(unsigned size) {
      m_ioLock.lock();
      if (ZuUnlikely(!m_snapMsg)) {
	m_ioLock.unlock();
	return 0;
      }

      return m_snapMsg->frame();
    }
    void push2() {
      if (ZuUnlikely(!m_snapMsg)) { m_ioLock.unlock(); return; }
      
      
      
      auto frame = m_snapMsg->frame();
      std::cout << "tcp seqNo: " << frame->seqNo << std::endl;
      std::cout << "tcp len: " << frame->len << std::endl;
      std::cout << "tcp type: " << (int)frame->type << std::endl;
      
      
      
      
      m_tcp->send(m_snapMsg->frame());
      m_ioLock.unlock();
    }

  private:
    //uint64_t		m_seqNo = 0;
    MxSeqNo			m_msgSeqNo = 0;
    MxMDCore			*m_core;
    ZmSemaphore			m_attachSem;
    ZmSemaphore			m_detachSem;
    Lock			m_threadLock;
    ZmThread			  m_snapThread;
    bool			  m_recvRunning = false;
    Lock			m_ioLock;
    ZmRef<Msg>			  m_snapMsg;
    ZmRef<MxLink<Link>>		m_link;
    ZmLock			m_stateLock;
    ZmRef<TCP>			  m_tcp;
    ZmRef<UDP>			  m_udp;
    unsigned			m_nAccepts = 10;
    ZiSockAddr			m_dest;
  };

  class Subscriber;
friend class Subscriber;
  class Subscriber : public MxEngineApp, public MxEngine {

    class TCP;
  friend class TCP;
    class TCP : public ZiConnection {
    public:
      TCP(Subscriber *subscriber, const ZiCxnInfo &ci);

      ZuInline Subscriber *subscriber() const { return m_subscriber; }

      void connected(ZiIOContext &);
      void disconnect();
      void disconnected() {}

      void sendLogin();

      void process(MxMDPubSub::TCP::InMsg *, ZiIOContext &);

    private:
      Subscriber			*m_subscriber;
      ZmRef<MxMDPubSub::TCP::InMsg>	m_in;
  };

    class UDP;
  friend class UDP;
    class UDP : public ZiConnection {
    public:
      UDP(Subscriber *subscriber, const ZiCxnInfo &ci);

      ZuInline Subscriber *subscriber() const { return m_subscriber; }

      void connected(ZiIOContext &io);
      void disconnect();
      void disconnected() {}

      //void recv();
      
      
      void process(MxMDPubSub::UDP::InMsg *inMsg, ZiIOContext &io)
      {
        m_subscriber->m_core->raise(ZeEVENT(Info, "##### Subscriber UDP process"));

        using namespace MxMDPubSub;
        using namespace MxMDPubSub::UDP;
        using namespace MxMDStream;

        if (ZuUnlikely(inMsg->scan())) {
          ZtHexDump msg_{"truncated UDP message", inMsg->data(), inMsg->length()};
          m_subscriber->m_core->raise(ZeEVENT(Warning,
            ([msg_ = ZuMv(msg_)](const ZeEvent &, ZmStream &s) {
	        s << "UDP " << msg_; })));
        }

        Frame &frame = inMsg->template as<Frame>();
	
	
	
	
	
      std::cout << "udp seqNo: " << frame.seqNo << std::endl;
      std::cout << "udp len: " << frame.len << std::endl;
      std::cout << "udp type: " << (int)frame.type << std::endl;
	
	
	
	

        MsgRef msg = new Msg();

        //Frame *frame = &hdr.as<Frame>();

        memcpy(msg->frame(), &frame, sizeof(Frame) + frame.len);

        ZmRef<MxQMsg> qmsg = new MxQMsg(MxFlags{}, ZmTime{}, msg);
        qmsg->load(m_subscriber->m_rx->rxQueue().id, frame.seqNo);

        m_subscriber->m_seqNo = frame.seqNo;
	
	std::cout << "++++++ " << m_subscriber->m_seqNo << std::endl;

        m_subscriber->m_rx->received(qmsg);

        //recv(io);
	m_in->recv(io);
      }

    private:
      Subscriber		*m_subscriber;
      ZmRef<MxMDPubSub::UDP::InMsg>	m_in;
    };

  public:
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;
    typedef MxMDStream::Msg Msg;

    struct State {
      enum _ {
        Stopped = 0,
        Starting,
        Running,
        Stopping,
        Mask		= 0x0003,

        PendingStart	= 0x0004,	// started while stopping
        PendingStop	= 0x0008,	// stopped while starting
      };
    };

    // Rx (called from engine's rx thread)   
    MxEngineApp::ProcessFn processFn() {
      return static_cast<MxEngineApp::ProcessFn>(
	  [](MxEngineApp *app, MxAnyLink *, MxQMsg *msg) {
	      static_cast<Subscriber *>(app)->pushMsg(msg);
      });
    }

    // Tx (called from engine's tx thread)
    void sent(MxAnyLink *, MxQMsg *) { }
    void aborted(MxAnyLink *, MxQMsg *) { }
    void archive(MxAnyLink *, MxQMsg *) { }
    ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }

    class Link : public MxLink<Link> {
    public:
      Link(MxID id) : MxLink<Link>{id} { }

      void init(MxEngine *engine) { MxLink<Link>::init(engine); }

      ZmTime reconnectInterval(unsigned reconnects) { return ZmTime{1}; }
      ZmTime reRequestInterval() { return ZmTime{1}; }

      // Rx
      void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
      void reRequest(const MxQueue::Gap &now) { }

      // Tx
      bool send_(MxQMsg *msg, bool more) { return true; }
      bool resend_(MxQMsg *msg, bool more){ return true; }

      bool sendGap_(const MxQueue::Gap &gap, bool more){ return true; }
      bool resendGap_(const MxQueue::Gap &gap, bool more){ return true; }

      MxID id() const { return MxAnyTx::id(); }

      void update(ZvCf *cf) { }
      void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo) { }

      void connect() { MxAnyLink::connected(); }
      void disconnect() { MxAnyLink::disconnected(); }
    };

    typedef MxQueueRx<Link> Rx;

    Subscriber(MxMDCore *core) : m_core(core) { }

    void init() {
      ZmRef<ZvCf> cf = m_core->cf();
      MxEngine::init(m_core, this, m_core->mx(), cf->subset("subscriber", false, true));
      m_link = MxEngine::updateLink<Link>("subscriber", nullptr);
      m_link->init(this);
    }

    inline ~Subscriber() = default;

    void start();
    void stop();

  private:
    void recv(Rx *rx);
    void pushMsg(MxQMsg *qmsg);

    void running();
    void stopped();

    bool pendingStart();
    bool pendingStop();

    bool pendingStart_();	// state lock held by caller
    bool pendingStop_();	// state lock held by caller

    void start_();
    void restart();
    void restart_();	// state lock held by caller

    void stop_();

    bool tcpError(TCP *tcp, ZiIOContext *io);
    bool udpError(UDP *udp, ZiIOContext *io);

    void tcpConnect();
    ZiConnection *tcpConnected(const ZiCxnInfo &ci);
    void tcpConnectFailed(bool transient);
    void tcpConnected2(TCP *);

    void loginReq(MxMDStream::Login *login);

    void udpConnect();
    ZiConnection *udpConnected(const ZiCxnInfo &ci);
    void udpConnectFailed(bool transient);
    void udpConnected2(UDP *);
    void udpReceived(MxQMsg *msg);
    void udpReceived_(MxQMsg *msg);

    void disconnect();

  private:
    MxMDCore			*m_core;

    ZmScheduler::Timer		m_restartTimer;

    ZmLock			m_stateLock;
    unsigned			  m_state;
    unsigned			  m_reconnects;
    ZmRef<TCP>			  m_tcp;
    ZmRef<UDP>			  m_udp;

    ZmRef<MxLink<Link>>		m_link;
    Rx 				*m_rx = nullptr;
    uint64_t			m_seqNo;
  };

  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  typedef ZmPLock ReplayLock;
  typedef ZmGuard<ZmPLock> ReplayGuard;
  typedef ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> ReplayVersion;

  bool replayStart(ReplayGuard &guard, ZuString path);
  void replay2();

  ZmRef<MxMDVenue> venue(MxID id);

  void timer();

  Lock			m_stateLock; // prevents overlapping start() / stop()

  ZmRef<Mx>		m_mx;
  ZmRef<ZvCf>		m_cf;
  ZmRef<CmdServer>	m_cmd;

  ZtString		m_recordCfPath;
  ZtString		m_replayCfPath;

  ZvThreadParams	m_recReceiverParams;
  ZvThreadParams	m_recSnapperParams;
  ZvThreadParams	m_snapperParams;

  Broadcast		m_broadcast;	// broadcasts updates
  Snapper		m_snapper;	// unicasts snapshots
  Recorder		m_recorder;	// records to file
  Publisher		m_publisher;
  Subscriber		m_subscriber;

  ZmRef<MxMDFeed>	m_localFeed;

  ZmScheduler::Timer	m_timer;

  ReplayLock		m_replayLock;
    ZtString		  m_replayPath;
    ZiFile		  m_replayFile;
    ZmRef<Msg>		  m_replayMsg;
    bool		  m_replayFilter = 0;
    ReplayVersion	  m_replayVersion;
    ZmTime		  m_timerNext;		// time of next timer event
};

#endif /* MxMDCore_HPP */
