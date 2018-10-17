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

// MxEngine - Connectivity Framework

#ifndef MxEngine_HPP
#define MxEngine_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <ZmFn.hpp>
#include <ZmPolymorph.hpp>

#include <ZvCf.hpp>

#include <MxQueue.hpp>
#include <MxScheduler.hpp>
#include <MxMultiplex.hpp>

namespace MxEngineState {
  MxEnumValues(Stopped, Starting, Running, Stopping,
      StartPending,		// started while stopping
      StopPending);		// stopped while starting
  MxEnumNames("Stopped", "Starting", "Running", "Stopping",
      "StartPending", "StopPending");

  inline int rag(int i) {
    using namespace MxRAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] = { Red, Amber, Green, Amber, Amber, Amber };
    return values[i];
  }
}

namespace MxLinkState {
  MxEnumValues(
      Down,			// down (engine not started)
      Disabled,			// intentionally down (admin/ops disabled)
      Connecting,		// connecting (being brought up)
      Up,			// up/running
      Reconnecting,		// reconnecting following transient failure
      Failed,			// failed (non-transient)
      Disconnecting,		// disconnecting (being brought down)
      ConnectPending,		// brought up while disconnecting
      DisconnectPending);	// brought down while connecting
  MxEnumNames(
      "Down",
      "Disabled",
      "Connecting",
      "Up",
      "Reconnecting",
      "Failed",
      "Disconnecting",
      "ConnectPending",
      "DisconnectPending");

  inline int rag(int i) {
    using namespace MxRAG;
    if (i < 0 || i >= N) return Off;
    static const int values[N] =
      { Red, Off, Amber, Green, Amber, Red, Amber, Amber, Amber };
    return values[i];
  }
}

class MxEngine;

class MxAnyTx : public ZmPolymorph {
  MxAnyTx(const MxAnyTx &);	//prevent mis-use
  MxAnyTx &operator =(const MxAnyTx &);

protected:
  MxAnyTx(MxID id);
  
  void init(MxEngine *engine);

public:
  typedef MxMultiplex Mx;
  typedef ZmFn<MxQMsg *> AbortFn;

  struct IDAccessor : public ZuAccessor<MxAnyTx *, MxID> {
    inline static MxID value(const MxAnyTx *t) { return t->id(); }
  };

  ZuInline MxEngine *engine() const { return m_engine; }
  ZuInline Mx *mx() const { return m_mx; }
  ZuInline MxID id() const { return m_id; }

  virtual void send(MxQMsg *) = 0;
  virtual void abort(MxSeqNo) = 0;
  virtual void archived(MxSeqNo) = 0;

private:
  MxID			m_id;
  MxEngine		*m_engine = 0;
  Mx			*m_mx = 0;
};

class MxAnyTxPool : public MxAnyTx {
  MxAnyTxPool(const MxAnyTxPool &) = delete;
  MxAnyTxPool &operator =(const MxAnyTxPool &) = delete;

protected:
  inline MxAnyTxPool(MxID id) : MxAnyTx(id) { }

public:
  virtual MxQueue *txQueuePtr() = 0;
};

class MxAnyLink : public MxAnyTx {
  MxAnyLink(const MxAnyLink &) = delete;
  MxAnyLink &operator =(const MxAnyLink &) = delete;

friend class MxEngine;

protected:
  MxAnyLink(MxID id);

public:
  ZuInline int state(unsigned *reconnects = 0) const {
    ZmReadGuard<ZmLock> guard(m_stateLock);
    if (reconnects) *reconnects = m_reconnects;
    return m_state;
  }

  inline void up() { up_(true); }
  inline void down() { down_(true); }

  virtual void update(ZvCf *cf) = 0;
  virtual void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo) = 0;

protected:
  virtual MxQueue *rxQueuePtr() = 0;
  virtual MxQueue *txQueuePtr() = 0;

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  void connected();
  void disconnected();
  void reconnect();

  virtual ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

private:
  void up_(bool enable);
  void down_(bool disable);

  template <typename Impl, typename L>
  inline auto stateLocked(L &&l) {
    ZmGuard<ZmLock> guard(m_stateLock);
    return l(static_cast<Impl *>(this), m_state);
  }

private:
  ZmScheduler::Timer	m_reconnTimer;

  ZmLock		m_stateLock;
    int			  m_state = MxLinkState::Down;
    unsigned		  m_reconnects;
    bool		  m_enabled = true;
};

// Traffic Logging (timestamp, payload)
typedef ZmFn<ZmTime &, ZuString &> MxTraffic;

struct MxEngineMgr {
  // Engine Management
  virtual void addEngine(MxEngine *) = 0;
  virtual void delEngine(MxEngine *) = 0;
  virtual void engineState(MxEngine *, MxEnum, MxEnum) = 0; // MxEngineState

  // Link Management
  virtual void updateLink(MxAnyLink *) = 0;
  virtual void delLink(MxAnyLink *) = 0;
  virtual void linkState(MxAnyLink *, MxEnum, MxEnum) = 0; // MxLinkState

  // Pool Management
  virtual void updateTxPool(MxAnyTxPool *) = 0;
  virtual void delTxPool(MxAnyTxPool *) = 0;

  // Queue Management
  virtual void addQueue(MxID id, bool tx, MxQueue *) = 0;
  virtual void delQueue(MxID id, bool tx) = 0;

  // Exception handling
  virtual void exception(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

  // Traffic Logging (logThread)
  /* Example usage:
  app.log(id, MxTraffic([](const Msg *msg, ZmTime &stamp, ZuString &data) {
      stamp = msg->stamp();
      data = msg->buf();
    }, msg)); */
  virtual void log(MxMsgID, MxTraffic) = 0;
};

// Callbacks to the application from the engine implementation
struct MxEngineApp {
  virtual ZmRef<MxAnyLink> createLink(MxID) = 0;

  // Rx (called from engine's rx thread)
  typedef void (*ProcessFn)(MxEngineApp *, MxAnyLink *, MxQMsg *);
  virtual ProcessFn processFn() = 0;	// stashed in engine for performance

  // Tx (called from engine's tx thread)
  virtual void sent(MxAnyLink *, MxQMsg *) = 0;	// tx sent (persistent)
  virtual void aborted(MxAnyLink *, MxQMsg *) = 0;	// tx aborted
  virtual void archive(MxAnyLink *, MxQMsg *) = 0;	// tx archive
  virtual ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) = 0; // tx retrieve
};

// Note: When event/flow steering, referenced objects must remain
// in scope until eventually called by queued member functions; however
// this is only a routine problem for transient objects such as messages.
// It should not be necessary to frequently adjust the reference count of
// semi-persistent objects such as links and engines, since most of the time
// they will remain in scope. Furthermore, atomic operations on reference
// counts are somewhat expensive and should be minimized.
//
// Accordingly, when scheduling link or engine methods to be run later, 'this'
// can be directly captured as a raw pointer, without reference counting.
// As a corollary, code that deletes links or engines needs to take extra
// care not to destroy them while they could remain referenced by pending
// member functions; a relatively simple way of ensuring this is to perform
// multi-stage destruction as follows:
// 1] De-index the link/engine and disable it so it will not be used in future
// 2] Initialize a temporary semaphore
// 3] Enqueue a function that posts the semaphore onto each of the threads
//    that could potentially execute pending member functions for the
//    link/engine being deleted; each semaphore post will be executed after
//    all the other pending functions ahead of it
// 4] Wait for the semaphore to be posted as many times as there are threads
// 5] Destroy the link/engine, safe in the knowledge that no pending functions
//    that reference it can remain in existence

class MxEngine : public ZmPolymorph {
  MxEngine(const MxEngine &);	//prevent mis-use
  MxEngine &operator =(const MxEngine &);

friend class MxAnyLink;

public:
  struct IDAccessor : public ZuAccessor<MxEngine *, MxID> {
    ZuInline static MxID value(const MxEngine *e) { return e->id(); }
  };

  typedef MxScheduler Sched;
  typedef MxMultiplex Mx;
  typedef MxEngineMgr Mgr;
  typedef MxEngineApp App;
  typedef App::ProcessFn ProcessFn;

  inline MxEngine() : m_state(MxEngineState::Stopped) { }

  void init(Mgr *mgr, App *app, Mx *mx, ZvCf *cf) {
    m_mgr = mgr,
    m_app = app;
    m_processFn = app->processFn();
    m_id = cf->get("id", true);
    m_mx = mx;
    m_rxThread = cf->getInt("rxThread", 1, mx->nThreads() + 1, true);
    m_txThread = cf->getInt("txThread", 1, mx->nThreads() + 1, true);
  }
  void final();

  ZuInline MxEngineMgr *mgr() const { return m_mgr; }
  ZuInline MxEngineApp *app() const { return m_app; }
  ZuInline MxID id() const { return m_id; }
  ZuInline Mx *mx() const { return m_mx; }
  ZuInline unsigned rxThread() const { return m_rxThread; }
  ZuInline unsigned txThread() const { return m_txThread; }

  ZuInline int state() const {
    ZmReadGuard<ZmLock> guard(m_stateLock);
    return m_state;
  }

  void start();
  void stop();

  template <typename ...Args> ZuInline void rxRun(Args &&... args)
    { m_mx->run(m_rxThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void rxInvoke(Args &&... args)
    { m_mx->invoke(m_rxThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void txRun(Args &&... args)
    { m_mx->run(m_txThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void txInvoke(Args &&... args)
    { m_mx->invoke(m_txThread, ZuFwd<Args>(args)...); }

  bool isRx() { return m_mx->runningTID() == (int)m_rxThread; }
  bool isTx() { return m_mx->runningTID() == (int)m_txThread; }

  ZuInline void appAddEngine() { mgr()->addEngine(this); }
  ZuInline void appDelEngine() { mgr()->delEngine(this); }
  ZuInline void appEngineState(MxEnum prev, MxEnum next) {
    mgr()->engineState(this, prev, next);
  }

  ZuInline ZmRef<MxAnyLink> appCreateLink(MxID id) {
    return app()->createLink(id);
  }
  ZuInline void appUpdateLink(MxAnyLink *link) { mgr()->updateLink(link); }
  ZuInline void appDelLink(MxAnyLink *link) { mgr()->delLink(link); }
  ZuInline void appLinkState(MxAnyLink *link, MxEnum prev, MxEnum next)
    { mgr()->linkState(link, prev, next); }

  ZuInline void appUpdateTxPool(MxAnyTxPool *pool)
    { mgr()->updateTxPool(pool); }
  ZuInline void appDelTxPool(MxAnyTxPool *pool) { mgr()->delTxPool(pool); }

  // Note: MxQueues are contained in Link and TxPool
  ZuInline void appAddQueue(MxID id, bool tx, MxQueue *queue)
    { mgr()->addQueue(id, tx, queue); }
  ZuInline void appDelQueue(MxID id, bool tx)
    { mgr()->delQueue(id, tx); }

  // generic O.S. error logging
  inline auto osError(const char *op, int result, ZeError e) {
    return [id = m_id, op, result, e](const ZeEvent &, ZmStream &s) {
      s << id << " - " << op << " - " << Zi::resultName(result) << " - " << e;
    };
  }

  ZuInline void appException(ZmRef<ZeEvent> e) { mgr()->exception(ZuMv(e)); }

  ZuInline void appProcess(MxAnyLink *link, MxQMsg *msg)
    { (*m_processFn)(app(), link, msg); }

  ZuInline void log(MxMsgID id, MxTraffic traffic) { mgr()->log(id, traffic); }

private:
  typedef ZmRWLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef ZmRBTree<ZmRef<MxAnyTxPool>,
	    ZmRBTreeIndex<MxAnyTxPool::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > > TxPools;
  typedef ZmRBTree<ZmRef<MxAnyLink>,
	    ZmRBTreeIndex<MxAnyLink::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > > Links;

public:
  ZmRef<MxAnyTxPool> txPool(MxID id) {
    ReadGuard guard(m_lock);
    return m_txPools.findKey(id);
  }
  template <typename TxPool>
  ZmRef<MxAnyTxPool> updateTxPool(MxID id, ZvCf *cf) {
    Guard guard(m_lock);
    ZmRef<TxPool> pool;
    if (pool = m_txPools.findKey(id)) {
      guard.unlock();
      pool->update(cf);
      appUpdateTxPool(pool);
      return pool;
    }
    pool = new TxPool(*this, id);
    m_txPools.add(pool);
    guard.unlock();
    pool->update(cf);
    appUpdateTxPool(pool);
    appAddQueue(id, 1, pool->txQueuePtr());
    return pool;
  }
  ZmRef<MxAnyTxPool> delTxPool(MxID id) {
    Guard guard(m_lock);
    ZmRef<MxAnyTxPool> txPool;
    if (txPool = m_txPools.delKey(id)) {
      guard.unlock();
      appDelQueue(id, 1);
      appDelTxPool(txPool);
    }
    return txPool;
  }

  ZmRef<MxAnyLink> link(MxID id) {
    ReadGuard guard(m_lock);
    return m_links.findKey(id);
  }
  ZmRef<MxAnyLink> updateLink(MxID id, ZvCf *cf) {
    Guard guard(m_lock);
    ZmRef<MxAnyLink> link;
    if (link = m_links.findKey(id)) {
      guard.unlock();
      link->update(cf);
      appUpdateLink(link);
      return link;
    }
    link = appCreateLink(id);
    link->init(this);
    link->update(cf);
    m_links.add(link);
    guard.unlock();
    linkState(link, -1, link->state());
    appUpdateLink(link);
    appAddQueue(id, 0, link->rxQueuePtr());
    appAddQueue(id, 1, link->txQueuePtr());
    return link;
  }
  ZmRef<MxAnyLink> delLink(MxID id) {
    Guard guard(m_lock);
    ZmRef<MxAnyLink> link;
    if (link = m_links.delKey(id)) {
      guard.unlock();
      appDelQueue(id, 0);
      appDelQueue(id, 1);
      appDelLink(link);
    }
    return link;
  }
  ZuInline unsigned nLinks() const { return m_links.count(); }
  template <typename L> uintptr_t allLinks(L &&l) {
    auto i = m_links.readIterator();
    while (MxAnyLink *link = i.iterateKey())
      if (uintptr_t v = l(link)) return v;
    return 0;
  }

private:
  void linkState(MxAnyLink *, int prev, int next);

  void start_();
  void stop_();

private:
  MxID				m_id;
  Mgr				*m_mgr = 0;
  App				*m_app = 0;
  ProcessFn			m_processFn = 0;
  ZmRef<Mx>			m_mx;
  unsigned			m_rxThread = 0;
  unsigned			m_txThread = 0;

  Lock				m_lock;
    TxPools			  m_txPools;	// from csv
    Links			  m_links;	// from csv

  ZmLock			m_stateLock;
    int				  m_state;
    unsigned			  m_down = 0;		// #links down
    unsigned			  m_disabled = 0;	// #links disabled
    unsigned			  m_transient = 0;	// #links transient
    unsigned			  m_up = 0;		// #links up
    unsigned			  m_reconn = 0;		// #links reconnecting
    unsigned			  m_failed = 0;		// #links failed
};

template <typename Impl, typename Base>
class MxTx : public Base {
  typedef MxQueueTx<Impl> Tx;

public:
  typedef MxMultiplex Mx;
  typedef MxQueue::Gap Gap;
  typedef MxAnyTx::AbortFn AbortFn;

  inline MxTx(MxID id) : Base(id) { }
  
  inline void init(MxEngine *engine) { Base::init(engine); }

  ZuInline const Impl &impl() const { return static_cast<const Impl &>(*this); }
  ZuInline Impl &impl() { return static_cast<Impl &>(*this); }

  ZuInline void scheduleSend() { if (!m_sending.xch(1)) rescheduleSend(); }
  ZuInline void rescheduleSend() { txRun([](Tx *tx) { tx->send(); }); }
  ZuInline void idleSend() { m_sending = 0; }

  ZuInline void scheduleResend()
    { if (!m_resending.xch(1)) rescheduleResend(); }
  ZuInline void rescheduleResend() { txRun([](Tx *tx) { tx->resend(); }); }
  ZuInline void idleResend() { m_resending = 0; }

  ZuInline void scheduleArchive()
    { if (!m_archiving.xch(1)) rescheduleArchive(); }
  ZuInline void rescheduleArchive() { txRun([](Tx *tx) { tx->archive(); }); }
  ZuInline void idleArchive() { m_archiving = 0; }

  template <typename L> ZuInline void txRun(L &&l)
    { this->engine()->txRun(ZmFn{ZuFwd<L>(l), impl().tx()}); }
  template <typename L> ZuInline void txRun(L &&l) const
    { this->engine()->txRun(ZmFn{ZuFwd<L>(l), impl().tx()}); }
  template <typename L> ZuInline void txInvoke(L &&l)
    { this->engine()->txInvoke(ZuFwd<L>(l), impl().tx()); }
  template <typename L> ZuInline void txInvoke(L &&l) const
    { this->engine()->txInvoke(ZuFwd<L>(l), impl().tx()); }

private:
  ZmAtomic<unsigned>	m_sending;
  ZmAtomic<unsigned>	m_resending;
  ZmAtomic<unsigned>	m_archiving;
};

// CRTP - implementation must conform to the following interface:
#if 0
struct TxPoolImpl : public MxTxPool<TxPoolImpl> {
  ZmTime reconnInterval(unsigned reconnects); // optional - defaults to 1sec
};
#endif

template <typename Impl> class MxTxPool :
  public MxTx<Impl, MxAnyTxPool>,
  public MxQueueTxPool<Impl> {
  typedef MxTx<Impl, MxAnyTxPool> Base;
public:
  typedef ZmPQTx<Impl, MxQueue, ZmNoLock> Tx_;
  typedef MxQueueTxPool<Impl> Tx;

  inline MxTxPool(MxID id) : Base(id) { }

  ZuInline const Tx *tx() const { return static_cast<const Tx *>(this); }
  ZuInline Tx *tx() { return static_cast<Tx *>(this); }

  MxQueue *txQueuePtr() { return &(this->txQueue()); }

#if 0
  void send(MxQMsg *msg)
    { this->txInvoke([msg = ZmMkRef(msg)](Tx *tx) { tx->send(msg); }); }
  void abort(MxSeqNo seqNo)
    { this->txInvoke([seqNo](Tx *tx) { tx->abort(seqNo); }); }
#endif

private:
  // prevent direct call from Impl - must be called via txRun/txInvoke
  using Tx_::start;		// Tx - start
  using Tx_::stop;		// Tx - stop
  using Tx::send;		// Tx - send (from app)
  using Tx::abort;		// Tx - abort (from app)
  using Tx::unload;		// Tx - unload all messages (for reload)
  using Tx::txReset;		// Tx - reset sequence numbers
  using Tx::join;		// Tx - join pool
  using Tx::leave;		// Tx - leave pool
  // should not be called from Impl at all
  using Tx_::ackd;		// handled by MxQueueTxPool
  using Tx_::resend;		// handled by MxTx
  using Tx_::archive;		// handled by MxTx
  using Tx_::archived;		// handled by MxQueueTxPool
  using Tx::ready;		// handled by MxQueueTxPool
  using Tx::unready;		// handled by MxQueueTxPool
  using Tx::ready_;		// internal to MxQueueTxPool
  using Tx::unready_;		// internal to MxQueueTxPool
};

// CRTP - implementation must conform to the following interface:
// (Note: can be derived from TxPoolImpl above)
#if 0
struct LinkImpl : public MxLink<LinkImpl> {
  ZmTime reconnInterval(unsigned reconnects); // optional - defaults to 1sec

  // Rx
  ZmTime reReqInterval(); // resend request interval
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now);
  void reRequest(const MxQueue::Gap &now);

  // Tx
  bool send_(MxQMsg *msg, bool more); // true on success
  bool resend_(MxQMsg *msg, bool more); // true on success

  bool sendGap_(const MxQueue::Gap &gap, bool more); // true on success
  bool resendGap_(const MxQueue::Gap &gap, bool more); // true on success
};
#endif

template <typename Impl> class MxLink :
  public MxTx<Impl, MxAnyLink>,
  public MxQueueRx<Impl>, public MxQueueTx<Impl> {
  typedef MxTx<Impl, MxAnyLink> Base;
public:
  typedef ZmPQRx<Impl, MxQueue, ZmNoLock> Rx_;
  typedef MxQueueRx<Impl> Rx;
  typedef ZmPQTx<Impl, MxQueue, ZmNoLock> Tx_;
  typedef MxQueueTx<Impl> Tx;

  typedef MxAnyTx::AbortFn AbortFn;

  ZuInline const Impl &impl() const { return static_cast<const Impl &>(*this); }
  ZuInline Impl &impl() { return static_cast<Impl &>(*this); }

  ZuInline const Rx *rx() const { return static_cast<const Rx *>(this); }
  ZuInline Rx *rx() { return static_cast<Rx *>(this); }
  ZuInline const Tx *tx() const { return static_cast<const Tx *>(this); }
  ZuInline Tx *tx() { return static_cast<Tx *>(this); }

  inline MxLink(MxID id) : Base(id) { }

  inline void init(MxEngine *engine,
      MxSeqNo rxSeqNo = MxSeqNo(), MxSeqNo txSeqNo = MxSeqNo()) {
    Base::init(engine);
    Rx::init(rxSeqNo);
    Tx::init(txSeqNo);
  }

  ZuInline void process(MxQMsg *msg) {
    this->engine()->appProcess(this, msg);
  }

  ZuInline void scheduleDequeue() {
    if (!m_dequeuing.xch(1)) rescheduleDequeue();
  }
  ZuInline void rescheduleDequeue() { rxRun([](Rx *rx) { rx->dequeue(); }); }
  ZuInline void idleDequeue() { m_dequeuing = 0; }

  typedef ZmPLock RRLock;
  typedef ZmGuard<RRLock> RRGuard;

  ZuInline void scheduleReRequest() {
    RRGuard guard(m_rrLock);
    if (ZuLikely(!m_rrTime)) scheduleReRequest_(guard);
  }
  ZuInline void rescheduleReRequest() {
    RRGuard guard(m_rrLock);
    scheduleReRequest_(guard);
  }
  ZuInline void scheduleReRequest_(RRGuard &guard) {
    ZmTime interval = impl().reReqInterval();
    if (!interval) return;
    m_rrTime.now();
    ZmTime rrTime = (m_rrTime += interval);
    guard.unlock();
    rxRun([](Rx *rx) { rx->reRequest(); }, rrTime, &m_rrTimer);
  }
  ZuInline void cancelReRequest() {
    this->mx()->del(&m_rrTimer);
    {
      RRGuard guard(m_rrLock);
      m_rrTime = ZmTime();
    }
  }

  // Impl/MxQueueTx callbacks (send_() must call sent_()/aborted_())
  ZuInline void sent_(MxQMsg *msg) // tx sent (persistent)
    { this->engine()->app()->sent(this, msg); }
  ZuInline void aborted_(MxQMsg *msg) // tx aborted
    { this->engine()->app()->aborted(this, msg); }

  // MxQueueTx/ZmPQTx callbacks
  ZuInline void archive_(MxQMsg *msg) // tx archive - app calls archived()
    { this->engine()->app()->archive(this, msg); }
  ZuInline ZmRef<MxQMsg> retrieve_(MxSeqNo seqNo) // tx retrieve
    { return this->engine()->app()->retrieve(this, seqNo); }

  MxQueue *rxQueuePtr() { return &(this->rxQueue()); }
  MxQueue *txQueuePtr() { return &(this->txQueue()); }

  template <typename L, typename ...Args>
  ZuInline void rxRun(L &&l, Args &&... args)
    { this->engine()->rxRun(ZmFn{ZuFwd<L>(l), rx()}, ZuFwd<Args>(args)...); }
  template <typename L, typename ...Args>
  ZuInline void rxRun(L &&l, Args &&... args) const
    { this->engine()->rxRun(ZmFn{ZuFwd<L>(l), rx()}, ZuFwd<Args>(args)...); }
  template <typename L> ZuInline void rxInvoke(L &&l)
    { this->engine()->rxInvoke(ZuFwd<L>(l), rx()); }
  template <typename L> ZuInline void rxInvoke(L &&l) const
    { this->engine()->rxInvoke(ZuFwd<L>(l), rx()); }

#if 0
  void send(MxQMsg *msg)
    { this->txInvoke([msg = ZmMkRef(msg)](Tx *tx) { tx->send(msg); }); }
  void abort(MxSeqNo seqNo)
    { this->txInvoke([seqNo](Tx *tx) { tx->abort(seqNo); }); }
  void archived(MxSeqNo seqNo)
    { this->txInvoke([seqNo](Tx *tx) { tx->archived(seqNo); }); }
#endif

private:
  // prevent direct call from Impl - must be called via rx/tx Run/Invoke
  using Rx_::rxReset;		// Rx - reset sequence numbers
  using Rx_::startQueuing;	// Rx - start queuing
  using Rx_::stopQueuing;	// Rx - stop queuing (start processing)
  using Rx_::received;		// Rx - received (from network)
  using Tx_::start;		// Tx - start
  using Tx_::stop;		// Tx - stop
  using Tx_::ackd;		// Tx - ackd (due to received message)
  using Tx_::archived;		// Tx - archived (following call to archive)
  using Tx::send;		// Tx - send (from app)
  using Tx::abort;		// Tx - abort (from app)
  using Tx::unload;		// Tx - unload all messages (for reload)
  using Tx::txReset;		// Tx - reset sequence numbers
  using Tx::join;		// Tx - join pool
  using Tx::leave;		// Tx - leave pool
  using Tx::ready;		// Tx - inform pool(s) of readiness
  using Tx::unready;		// Tx - inform pool(s) not ready
  // should not be called from Impl at all
  using Rx_::dequeue;		// handled by MxLink
  using Rx_::reRequest;		// handled by MxLink
  using Tx_::resend;		// handled by MxTx
  using Tx_::archive;		// handled by MxTx
  using Tx::ready_;		// internal to MxQueueTx
  using Tx::unready_;		// internal to MxQueueTx

  ZmScheduler::Timer	m_rrTimer;

  RRLock		m_rrLock;
    ZmTime		  m_rrTime;

  ZmAtomic<unsigned>	m_dequeuing = 0;
};

#endif /* MxEngine_HPP */
