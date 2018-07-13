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

#include <MxMD.hpp>
#include <MxMDCmd.hpp>
#include <MxMDMsg.hpp>
#include <MxMDStream.hpp>

#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmThread.hpp>
#include <ZuPOD.hpp>

#include <ZePlatform.hpp>

#include <ZvCf.hpp>
#include <ZvRingCf.hpp>
#include <ZvThreadCf.hpp>
#include <ZvCmd.hpp>

#include <MxEngine.hpp>
#include <MxMultiplex.hpp>

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

  virtual void start();
  virtual void stop();
  virtual void final();

  virtual void record(ZuString path);
  virtual void stopRecording();
  virtual void stopStreaming();

  virtual void replay(
    ZuString path,
    MxDateTime begin = MxDateTime(),
    bool filter = true);
  virtual void stopReplaying();

  virtual void startTimer(MxDateTime begin = MxDateTime());
  virtual void stopTimer();

  virtual void dumpTickSizes(ZuString path, MxID venue = MxID());
  virtual void dumpSecurities(
      ZuString path, MxID venue = MxID(), MxID segment = MxID());
  virtual void dumpOrderBooks(
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

  void addTickSize_(ZuAnyPOD *);
  void addSecurity_(ZuAnyPOD *);
  void addOrderBook_(ZuAnyPOD *);

  // Engine Management
  virtual void addEngine(MxEngine *){}
  virtual void delEngine(MxEngine *){}
  virtual void engineState(MxEngine *, MxEnum){}

  // Link Management
  virtual void updateLink(MxAnyLink *){}
  virtual void delLink(MxAnyLink *){}
  virtual void linkState(MxAnyLink *, MxEnum, ZuString txt){}

  // Pool Management
  virtual void updateTxPool(MxAnyTxPool *){}
  virtual void delTxPool(MxAnyTxPool *){}

  // Queue Management
  virtual void addQueue(MxID id, bool tx, MxQueue *){}
  virtual void delQueue(MxID id, bool tx){}

  // Traffic Logging (logThread)
  /* Example usage:
  app.log(id, MxTraffic([](const Msg *msg, ZmTime &stamp, ZuString &data) {
    stamp = msg->stamp();
    data = msg->buf();
  }, msg)); */
  virtual void log(MxMsgID, MxTraffic){}

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
	      venue->id(), tbl->id(),
	      ts.minPrice(), ts.maxPrice(), ts.tickSize());
	});
      });
    }) || allSecurities([&snapshot](MxMDSecurity *security) -> uintptr_t {
      return !MxMDStream::addSecurity(snapshot,
	  security->key(), security->shard()->id(),
	  security->refData());
    }) || allOrderBooks([&snapshot](MxMDOrderBook *ob) -> uintptr_t {
      if (ob->legs() == 1) {
	return !MxMDStream::addOrderBook(snapshot,
	    ob->key(), ob->security()->key(),
	    ob->tickSizeTbl()->id(), ob->lotSizes());
      } else {
	MxSecKey securityKeys[MxMDNLegs];
	MxEnum sides[MxMDNLegs];
	MxFloat ratios[MxMDNLegs];
	for (unsigned i = 0, n = ob->legs(); i < n; i++) {
	  securityKeys[i] = ob->security(i)->key();
	  sides[i] = ob->side(i);
	  ratios[i] = ob->ratio(i);
	}
	return !MxMDStream::addCombination(snapshot,
	    ob->key(), ob->legs(), securityKeys, sides, ratios,
	    ob->tickSizeTbl()->id(), ob->lotSizes());
      }
    }) || allVenues([&snapshot](MxMDVenue *venue) -> uintptr_t {
      return venue->allSegments([&snapshot, venue](
	    MxID segment, MxEnum session, MxDateTime stamp) -> uintptr_t {
	return !MxMDStream::tradingSession(snapshot,
	    venue->id(), segment, session, stamp);
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
      return !MxMDStream::addOrder(snapshot,
	  order->orderBook()->key(), order->id(),
	  data.transactTime, data.side, data.rank,
	  data.price, data.qty, data.flags);
    })) return false;
    if (orderCount) return true;
    const MxMDPxLvlData &data = pxLevel->data();
    return MxMDStream::pxLevel(snapshot,
	pxLevel->obSide()->orderBook()->key(), pxLevel->side(),
	data.transactTime, false,
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
  class Recorder :
    public MxEngineApp, public MxEngine, public MxLink<Recorder> {
  public:
    typedef ZmPLock Lock;
    typedef ZmGuard<Lock> Guard;
    typedef ZmReadGuard<Lock> ReadGuard;
    typedef MxMDStream::Msg Msg;

    // Rx (called from engine's rx thread)
    virtual MxEngineApp::ProcessFn processFn(){ return {}; }

    // Tx (called from engine's tx thread)
    virtual void sent(MxAnyLink *, MxQMsg *){}
    virtual void aborted(MxAnyLink *, MxQMsg *){}
    virtual void archive(MxAnyLink *, MxQMsg *){}
    virtual ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo){ return 0; }

  private:
    struct SnapQueue_HeapID {
      inline static const char *id() { return "MxMD.SnapQueue"; }
    };
  public:
    typedef ZmList<ZmRef<Msg>,
	      ZmListObject<ZuNull,
		ZmListLock<ZmNoLock,
		  ZmListHeapID<SnapQueue_HeapID> > > > SnapQueue;

    inline Recorder(MxMDCore *core)
        :
            MxLink<Recorder>(*this, "recorder"),
            m_core(core)
    {
    }

    void init()
    {
        MxEngine::init(m_core, this, m_core->mx(), m_core->cf());
        appUpdateLink(this);
    }

    inline ~Recorder() { m_file.close(); }

    MxID id() const { return MxAnyTx::id(); }

    ZmTime reconnectInterval(unsigned reconnects){ return ZmTime{1}; }
    ZmTime reRequestInterval(){ return ZmTime{1}; }

    // Rx
    void request(const MxQueue::Gap &prev, const MxQueue::Gap &now){}
    void reRequest(const MxQueue::Gap &now){}

    // Tx
    bool send_(MxQMsg *msg, bool more)
    {
        ZeError e;
        write(msg->payload->template as<Msg>().frame(), &e);

        sent_(msg);
        return true;
    }

    bool resend_(MxQMsg *msg, bool more){ return true; }

    bool sendGap_(const MxQueue::Gap &gap, bool more){ return true; }
    bool resendGap_(const MxQueue::Gap &gap, bool more){ return true; }

    virtual void update(ZvCf *cf){}
    virtual void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo){}

    virtual void connect()
    {
        MxAnyLink::connected();
    }

    virtual void disconnect()
    {
        MxAnyLink::disconnected();
    }

    inline ZiFile::Path path() const {
      ReadGuard guard(m_ioLock);
      return m_path;
    }

    inline bool running() const {
      ReadGuard guard(m_threadLock);
      return m_snapThread || m_recvThread;
    }

    template <typename Path> bool start(Path &&path) {
      if (!fileOpen(ZuFwd<Path>(path))) return false;
      if (!m_core->broadcast().open()) return false;
      recvStart();
      snapStart();
      return true;
    }

    void stop() {
      snapStop();
      recvStop();
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
      Guard guard(m_threadLock);
      if (!!m_recvThread) return;
      {
	Guard ioGuard(m_ioLock);
	Rx::startQueuing();
      }
      m_recvThread = ZmThread(m_core, ThreadID::RecReceiver,
	  ZmFn<>::Member<&Recorder::recv>::fn(this),
	  m_core->m_recReceiverParams);
      m_connectSem.wait();
    }
    void recvStop() {
      Guard guard(m_threadLock);
      if (!m_recvThread) return;
      using namespace MxMDStream;
      m_core->broadcast().reqDetach();
      m_recvThread.join();
      m_recvThread = ZmThread();
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
      return !!m_recvThread;
    }

  private:
    void recv() {
      using namespace MxMDStream;
      const Frame *frame;

      Broadcast &broadcast = m_core->broadcast();
      if (!broadcast.open()) return;
      if (broadcast.attach() != Zi::OK) {
	broadcast.close();
	m_connectSem.post();
	Guard guard(m_threadLock);
	m_recvThread = ZmThread();
	return;
      }

      m_connectSem.post();

      for (;;) {
	if (ZuUnlikely(!(frame = broadcast.shift()))) {
	  if (ZuLikely(broadcast.readStatus() == Zi::EndOfFile)) break;
	  continue;
	}
	if (ZuUnlikely(frame->type == Type::Detach)) {
	  const Detach &detach = frame->as<Detach>();
	  if (ZuUnlikely(detach.id == broadcast.id())) {
	    broadcast.shift2();
	    break;
	  }
	  broadcast.shift2();
	  continue;
	}
	{
	  Guard guard(m_ioLock);
      MsgRef msg = new Msg();

	  if (frame->len > sizeof(Buf)) {
	    broadcast.shift2();
	    guard.unlock();
	    snapStop();
	    m_core->raise(ZeEVENT(Error,
		([name = MxTxtString(broadcast.config().name())](
		    const ZeEvent &, ZmStream &s) {
		  s << '"' << name << "\": "
		  "IPC shared memory ring buffer read error - "
		  "message too big";
		})));
	    break;
	  }
	  memcpy(msg->frame(), frame, sizeof(Frame) + frame->len);      
      ZmRef<MxQMsg> qmsg = new MxQMsg(MxFlags{}, ZmTime{}, msg);      
      qmsg->load(Rx::rxQueue().id, ++msgSeqNo);
	  Rx::received(qmsg);
	}
	broadcast.shift2();
      }
      broadcast.detach();
      broadcast.close();
    }

    int write(const Frame *frame, ZeError *e) {
#if 0
      ZiVec vec[2];
      ZiVec_ptr(vec[0]) = const_cast<void *>((const void *)frame);
      ZiVec_len(vec[0]) = sizeof(Frame);
      ZiVec_ptr(vec[1]) = const_cast<void *>((const void *)frame->ptr());
      ZiVec_len(vec[1]) = frame->len;
      return m_file.writev(vec, 2, e);
#endif
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
    Rx::stopQueuing(1);
      }
    }

    public:
    void process(MxQMsg *msg) {
      ZeError e;
      Guard guard(m_ioLock);
      if (ZuUnlikely(write(msg->payload->template as<Msg>().frame(), &e)) != Zi::OK) {
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
    inline void push2() {
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
    MxSeqNo msgSeqNo = 0;
    MxMDCore		*m_core;
    ZmSemaphore		m_connectSem;
    Lock		m_threadLock;
    ZmThread		  m_snapThread;
    ZmThread		  m_recvThread;
    Lock		m_ioLock;
    ZiFile::Path	  m_path;
    ZiFile		  m_file;
    ZmRef<Msg>		  m_snapMsg;
  };

  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  typedef ZmPLock ReplayLock;
  typedef ZmGuard<ZmPLock> ReplayGuard;
  typedef ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> ReplayVersion;

  bool replayStart(ReplayGuard &guard, ZuString path);
  void replay2();

  ZmRef<MxMDVenue> venue(MxID id);
  ZmRef<MxMDTickSizeTbl> tickSizeTbl(MxMDVenue *venue, ZuString id);

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
