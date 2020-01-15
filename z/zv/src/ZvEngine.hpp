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

// ZvEngine - Connectivity Framework

#ifndef ZvEngine_HPP
#define ZvEngine_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZmFn.hpp>
#include <zlib/ZmPolymorph.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvQueue.hpp>
#include <zlib/ZvScheduler.hpp>
#include <zlib/ZvMultiplex.hpp>
#include <zlib/ZvTelemetry.hpp>

class ZvEngine;
namespace ZvEngineState = ZvTelemetry::EngineState;
namespace ZvLinkState = ZvTelemetry::LinkState;
using ZvTraffic = ZvTelemetry::Traffic;

class ZvAPI ZvAnyTx : public ZmPolymorph {
  ZvAnyTx(const ZvAnyTx &);	//prevent mis-use
  ZvAnyTx &operator =(const ZvAnyTx &);

protected:
  ZvAnyTx(ZuID id);
  
  void init(ZvEngine *engine);

public:
  using Mx = ZvMultiplex;
  using AbortFn = ZmFn<ZvQMsg *>;

  struct IDAccessor : public ZuAccessor<ZvAnyTx *, ZuID> {
    inline static ZuID value(const ZvAnyTx *t) { return t->id(); }
  };

  ZuInline ZvEngine *engine() const { return m_engine; }
  ZuInline Mx *mx() const { return m_mx; }
  ZuInline ZuID id() const { return m_id; }

  template <typename T = uintptr_t>
  ZuInline T appData() const { return (T)m_appData; }
  template <typename T>
  ZuInline void appData(T v) { m_appData = (uintptr_t)v; }

private:
  ZuID			m_id;
  ZvEngine		*m_engine = 0;
  Mx			*m_mx = 0;
  uintptr_t		m_appData = 0;
};

class ZvAPI ZvAnyTxPool : public ZvAnyTx {
  ZvAnyTxPool(const ZvAnyTxPool &) = delete;
  ZvAnyTxPool &operator =(const ZvAnyTxPool &) = delete;

protected:
  inline ZvAnyTxPool(ZuID id) : ZvAnyTx(id) { }

public:
  virtual ZvQueue *txQueuePtr() = 0;
};

class ZvAPI ZvAnyLink : public ZvAnyTx {
  ZvAnyLink(const ZvAnyLink &) = delete;
  ZvAnyLink &operator =(const ZvAnyLink &) = delete;

friend class ZvEngine;

  using StateLock = ZmLock;
  using StateGuard = ZmGuard<StateLock>;
  using StateReadGuard = ZmReadGuard<StateLock>;

protected:
  ZvAnyLink(ZuID id);

public:
  ZuInline int state() const { return m_state.load_(); }
  ZuInline unsigned reconnects() const { return m_reconnects.load_(); }

  using Telemetry = ZvTelemetry::Link;

  void telemetry(Telemetry &data) const;

  inline void up() { up_(true); }
  inline void down() { down_(true); }

  virtual void update(ZvCf *cf) = 0;
  virtual void reset(ZvSeqNo rxSeqNo, ZvSeqNo txSeqNo) = 0;

  inline ZvSeqNo rxSeqNo() const {
    if (const ZvQueue *queue = rxQueue()) return queue->head();
    return 0;
  }
  inline ZvSeqNo txSeqNo() const {
    if (const ZvQueue *queue = txQueue()) return queue->tail();
    return 0;
  }

  virtual ZvQueue *rxQueue() = 0;
  virtual const ZvQueue *rxQueue() const = 0;
  virtual ZvQueue *txQueue() = 0;
  virtual const ZvQueue *txQueue() const = 0;

protected:
  virtual void connect() = 0;
  virtual void disconnect() = 0;

  void connected();
  void disconnected();
  void reconnecting();	// transition direct from Up to Connecting
  void reconnect(bool immediate);

  virtual ZmTime reconnInterval(unsigned) { return ZmTime{1}; }

private:
  void up_(bool enable);
  void down_(bool disable);

  void reconnect_();

  void deleted_();	// called from ZvEngine::delLink

private:
  ZmScheduler::Timer	m_reconnTimer;

  StateLock		m_stateLock;
    ZmAtomic<int>	  m_state = ZvLinkState::Down;
    ZmAtomic<unsigned>	  m_reconnects;
    bool		  m_enabled = true;
};

// Callbacks to the application from the engine implementation
struct ZvAPI ZvEngineApp {
  virtual ZmRef<ZvAnyLink> createLink(ZuID) = 0;
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

class ZvAPI ZvEngine : public ZmPolymorph {
  ZvEngine(const ZvEngine &);	//prevent mis-use
  ZvEngine &operator =(const ZvEngine &);

friend class ZvAnyLink;

  using Lock = ZmRWLock;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  using StateLock = ZmLock;
  using StateGuard = ZmGuard<StateLock>;
  using StateReadGuard = ZmReadGuard<StateLock>;

public:
  struct IDAccessor : public ZuAccessor<ZvEngine *, ZuID> {
    ZuInline static ZuID value(const ZvEngine *e) { return e->id(); }
  };

  using Sched = ZvScheduler;
  using Mx = ZvMultiplex;
  using Mgr = ZvTelemetry::EngineMgr;
  using App = ZvEngineApp;

  inline ZvEngine() : m_state(ZvEngineState::Stopped) { }

  void init(Mgr *mgr, App *app, Mx *mx, ZvCf *cf) {
    m_mgr = mgr,
    m_app = app;
    m_id = cf->get("id", true);
    m_mx = mx;
    if (ZuString s = cf->get("rxThread"))
      m_rxThread = mx->tid(s);
    else
      m_rxThread = mx->rxThread();
    if (ZuString s = cf->get("txThread"))
      m_txThread = mx->tid(s);
    else
      m_txThread = mx->txThread();
  }
  void final();

  ZuInline Mgr *mgr() const { return m_mgr; }
  ZuInline ZvEngineApp *app() const { return m_app; }
  ZuInline ZuID id() const { return m_id; }
  ZuInline Mx *mx() const { return m_mx; }
  ZuInline unsigned rxThread() const { return m_rxThread; }
  ZuInline unsigned txThread() const { return m_txThread; }

  ZuInline int state() const { return m_state.load_(); }

  void start();
  void stop();

  template <typename ...Args> ZuInline void rxRun(Args &&... args)
    { m_mx->run(m_rxThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void rxRun_(Args &&... args)
    { m_mx->run_(m_rxThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void rxInvoke(Args &&... args)
    { m_mx->invoke(m_rxThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void txRun(Args &&... args)
    { m_mx->run(m_txThread, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void txInvoke(Args &&... args)
    { m_mx->invoke(m_txThread, ZuFwd<Args>(args)...); }

  ZuInline void mgrAddEngine() { mgr()->addEngine(this); }
  ZuInline void mgrDelEngine() { mgr()->delEngine(this); }
  ZuInline void mgrUpdEngine() { mgr()->updEngine(this); }

  ZuInline ZmRef<ZvAnyLink> appCreateLink(ZuID id) {
    return app()->createLink(id);
  }
  ZuInline void mgrUpdLink(ZvAnyLink *link) { mgr()->updLink(link); }

  // Note: ZvQueues are contained in Link and TxPool
  ZuInline void mgrAddQueue(ZuID id, bool tx, ZvQueue *queue)
    { mgr()->addQueue(id, tx, queue); }
  ZuInline void mgrDelQueue(ZuID id, bool tx)
    { mgr()->delQueue(id, tx); }

  // generic O.S. error logging
  inline auto osError(const char *op, int result, ZeError e) {
    return [id = m_id, op, result, e](const ZeEvent &, ZmStream &s) {
      s << id << " - " << op << " - " << Zi::resultName(result) << " - " << e;
    };
  }

  ZuInline void mgrAlert(ZmRef<ZeEvent> e) const {
    mgr()->alert(ZuMv(e));
  }

  ZuInline void log(ZvQMsgID id, ZvTraffic traffic) const {
    mgr()->log(id, traffic);
  }

  using Telemetry = ZvTelemetry::Engine;

  // display sequence:
  //   id, state, nLinks, up, down, disabled, transient, reconn, failed,
  //   mxID, rxThread, txThread
  void telemetry(Telemetry &data) const;

private:
  using TxPools = ZmRBTree<ZmRef<ZvAnyTxPool>,
	    ZmRBTreeIndex<ZvAnyTxPool::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > >;
  using Links = ZmRBTree<ZmRef<ZvAnyLink>,
	    ZmRBTreeIndex<ZvAnyLink::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock> > > >;

public:
  ZmRef<ZvAnyTxPool> txPool(ZuID id) {
    ReadGuard guard(m_lock);
    return m_txPools.findKey(id);
  }
  template <typename TxPool>
  ZmRef<ZvAnyTxPool> updateTxPool(ZuID id, ZvCf *cf) {
    Guard guard(m_lock);
    ZmRef<TxPool> pool;
    if (pool = m_txPools.findKey(id)) {
      guard.unlock();
      pool->update(cf);
      return pool;
    }
    pool = new TxPool(*this, id);
    m_txPools.add(pool);
    guard.unlock();
    pool->update(cf);
    mgrAddQueue(id, 1, pool->txQueuePtr());
    return pool;
  }
  ZmRef<ZvAnyTxPool> delTxPool(ZuID id) {
    Guard guard(m_lock);
    ZmRef<ZvAnyTxPool> txPool;
    if (txPool = m_txPools.delKey(id)) {
      guard.unlock();
      mgrDelQueue(id, 1);
    }
    return txPool;
  }

  ZmRef<ZvAnyLink> link(ZuID id) {
    ReadGuard guard(m_lock);
    return m_links.findKey(id);
  }
  ZmRef<ZvAnyLink> updateLink(ZuID id, ZvCf *cf) {
    Guard guard(m_lock);
    ZmRef<ZvAnyLink> link;
    if (link = m_links.findKey(id)) {
      guard.unlock();
      link->update(cf);
      mgrUpdLink(link);
      return link;
    }
    link = appCreateLink(id);
    link->init(this);
    m_links.add(link);
    guard.unlock();
    linkState(link, -1, link->state());
    link->update(cf);
    mgrUpdLink(link);
    mgrAddQueue(id, 0, link->rxQueue());
    mgrAddQueue(id, 1, link->txQueue());
    return link;
  }
  ZmRef<ZvAnyLink> delLink(ZuID id) {
    Guard guard(m_lock);
    ZmRef<ZvAnyLink> link;
    if (link = m_links.delKey(id)) {
      guard.unlock();
      mgrDelQueue(id, 0);
      mgrDelQueue(id, 1);
      link->deleted_();	// calls linkState(), mgrUpdLink()
    }
    return link;
  }
  ZuInline unsigned nLinks() const {
    ReadGuard guard(m_lock);
    return m_links.count();
  }
  template <typename Link, typename L> uintptr_t allLinks(L l) {
    auto i = m_links.readIterator();
    while (ZvAnyLink *link = i.iterateKey())
      if (uintptr_t v = l(static_cast<Link *>(link))) return v;
    return 0;
  }

private:
  void linkState(ZvAnyLink *, int prev, int next);

  void start_();
  void stop_();

private:
  ZuID				m_id;
  Mgr				*m_mgr = 0;
  App				*m_app = 0;
  ZmRef<Mx>			m_mx;
  unsigned			m_rxThread = 0;
  unsigned			m_txThread = 0;

  Lock				m_lock;
    TxPools			  m_txPools;	// from csv
    Links			  m_links;	// from csv

  StateLock			m_stateLock;
    ZmAtomic<int>		  m_state;
    unsigned			  m_down = 0;		// #links down
    unsigned			  m_disabled = 0;	// #links disabled
    unsigned			  m_transient = 0;	// #links transient
    unsigned			  m_up = 0;		// #links up
    unsigned			  m_reconn = 0;		// #links reconnecting
    unsigned			  m_failed = 0;		// #links failed
};

template <typename Impl, typename Base>
class ZvTx : public Base {
  using Tx = ZvQueueTx<Impl>;

public:
  using Mx = ZvMultiplex;
  using Gap = ZvQueue::Gap;
  using AbortFn = ZvAnyTx::AbortFn;

  inline ZvTx(ZuID id) : Base(id) { }
  
  inline void init(ZvEngine *engine) { Base::init(engine); }

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline void scheduleSend() { txInvoke([](Tx *tx) { tx->send(); }); }
  ZuInline void rescheduleSend() { txRun([](Tx *tx) { tx->send(); }); }
  ZuInline void idleSend() { }

  ZuInline void scheduleResend() { txInvoke([](Tx *tx) { tx->resend(); }); }
  ZuInline void rescheduleResend() { txRun([](Tx *tx) { tx->resend(); }); }
  ZuInline void idleResend() { }

  ZuInline void scheduleArchive() { rescheduleArchive(); }
  ZuInline void rescheduleArchive() { txRun([](Tx *tx) { tx->archive(); }); }
  ZuInline void idleArchive() { }

  template <typename L> ZuInline void txRun(L &&l)
    { this->engine()->txRun(ZmFn<>{impl()->tx(), ZuFwd<L>(l)}); }
  template <typename L> ZuInline void txRun(L &&l) const
    { this->engine()->txRun(ZmFn<>{impl()->tx(), ZuFwd<L>(l)}); }
  template <typename L> ZuInline void txInvoke(L &&l)
    { this->engine()->txInvoke(impl()->tx(), ZuFwd<L>(l)); }
  template <typename L> ZuInline void txInvoke(L &&l) const
    { this->engine()->txInvoke(impl()->tx(), ZuFwd<L>(l)); }
};

// CRTP - implementation must conform to the following interface:
#if 0
struct TxPoolImpl : public ZvTxPool<TxPoolImpl> {
  ZmTime reconnInterval(unsigned reconnects); // optional - defaults to 1sec
};
#endif

template <typename Impl> class ZvTxPool :
  public ZvTx<Impl, ZvAnyTxPool>,
  public ZvQueueTxPool<Impl> {

  using Base = ZvTx<Impl, ZvAnyTxPool>;

public:
  using Tx_ = ZmPQTx<Impl, ZvQueue, ZmNoLock>;
  using Tx = ZvQueueTxPool<Impl>;

  inline ZvTxPool(ZuID id) : Base(id) { }

  const ZvQueue *txQueue() const { return ZvQueueTxPool<Impl>::txQueue(); }
  ZvQueue *txQueue() { return ZvQueueTxPool<Impl>::txQueue(); }

  ZuInline const Tx *tx() const { return static_cast<const Tx *>(this); }
  ZuInline Tx *tx() { return static_cast<Tx *>(this); }

  void send(ZmRef<ZvQMsg> msg) {
    msg->owner(tx());
    this->engine()->txInvoke(ZuMv(msg), [](ZmRef<ZvQMsg> msg) {
      msg->owner<Tx *>()->send(ZuMv(msg));
    });
  }
  template <typename L>
  void abort(ZvSeqNo seqNo, L l) {
    this->txInvoke([seqNo, l = ZuMv(l)](Tx *tx) mutable {
      l(tx->abort(seqNo));
    });
  }

private:
  // prevent direct call from Impl - must be called via txRun/txInvoke
  using Tx_::start;		// Tx - start
  using Tx_::stop;		// Tx - stop
  // using Tx::send;		// Tx - send (from app)
  // using Tx::abort;		// Tx - abort (from app)
  using Tx::unload;		// Tx - unload all messages (for reload)
  using Tx::txReset;		// Tx - reset sequence numbers
  using Tx::join;		// Tx - join pool
  using Tx::leave;		// Tx - leave pool
  // should not be called from Impl at all
  using Tx_::ackd;		// handled by ZvQueueTxPool
  using Tx_::resend;		// handled by ZvTx
  using Tx_::archive;		// handled by ZvTx
  using Tx_::archived;		// handled by ZvQueueTxPool
  using Tx::ready;		// handled by ZvQueueTxPool
  using Tx::unready;		// handled by ZvQueueTxPool
  using Tx::ready_;		// internal to ZvQueueTxPool
  using Tx::unready_;		// internal to ZvQueueTxPool
};

// CRTP - implementation must conform to the following interface:
// (Note: can be derived from TxPoolImpl above)
#if 0
struct Link : public ZvLink<Link> {
  ZmTime reconnInterval(unsigned reconnects); // optional - defaults to 1sec

  // Rx
  void process(ZvQMsg *);
  ZmTime reReqInterval(); // resend request interval
  void request(const ZvQueue::Gap &prev, const ZvQueue::Gap &now);
  void reRequest(const ZvQueue::Gap &now);

  // Tx
  void loaded_(ZvQMsg *msg);
  void unloaded_(ZvQMsg *msg);

  bool send_(ZvQMsg *msg, bool more); // true on success
  bool resend_(ZvQMsg *msg, bool more); // true on success
  void aborted_(ZvQMsg *msg);

  bool sendGap_(const ZvQueue::Gap &gap, bool more); // true on success
  bool resendGap_(const ZvQueue::Gap &gap, bool more); // true on success
};
#endif

template <typename Impl> class ZvLink :
  public ZvTx<Impl, ZvAnyLink>,
  public ZvQueueRx<Impl>, public ZvQueueTx<Impl> {

  using Base = ZvTx<Impl, ZvAnyLink>;

public:
  using Rx_ = ZmPQRx<Impl, ZvQueue, ZmNoLock>;
  using Rx = ZvQueueRx<Impl>;
  using Tx_ = ZmPQTx<Impl, ZvQueue, ZmNoLock>;
  using Tx = ZvQueueTx<Impl>;

  using AbortFn = ZvAnyTx::AbortFn;

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline const Rx *rx() const { return static_cast<const Rx *>(this); }
  ZuInline Rx *rx() { return static_cast<Rx *>(this); }
  ZuInline const Tx *tx() const { return static_cast<const Tx *>(this); }
  ZuInline Tx *tx() { return static_cast<Tx *>(this); }

  inline ZvLink(ZuID id) : Base(id) { }

  inline void init(ZvEngine *engine) { Base::init(engine); }

  ZuInline void scheduleDequeue() { rescheduleDequeue(); }
  ZuInline void rescheduleDequeue() { rxRun([](Rx *rx) { rx->dequeue(); }); }
  ZuInline void idleDequeue() { }

  using RRLock = ZmPLock;
  using RRGuard = ZmGuard<RRLock>;

  ZuInline void scheduleReRequest() {
    RRGuard guard(m_rrLock);
    if (ZuLikely(!m_rrTime)) scheduleReRequest_(guard);
  }
  ZuInline void rescheduleReRequest() {
    RRGuard guard(m_rrLock);
    scheduleReRequest_(guard);
  }
  ZuInline void scheduleReRequest_(RRGuard &guard) {
    ZmTime interval = impl()->reReqInterval();
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

  const ZvQueue *rxQueue() const { return ZvQueueRx<Impl>::rxQueue(); }
  ZvQueue *rxQueue() { return ZvQueueRx<Impl>::rxQueue(); }
  const ZvQueue *txQueue() const { return ZvQueueTx<Impl>::txQueue(); }
  ZvQueue *txQueue() { return ZvQueueTx<Impl>::txQueue(); }

  using ZvAnyLink::rxSeqNo;
  using ZvAnyLink::txSeqNo;

  template <typename L, typename ...Args>
  ZuInline void rxRun(L &&l, Args &&... args)
    { this->engine()->rxRun(ZmFn<>{rx(), ZuFwd<L>(l)}, ZuFwd<Args>(args)...); }
  template <typename L, typename ...Args>
  ZuInline void rxRun(L &&l, Args &&... args) const
    { this->engine()->rxRun(ZmFn<>{rx(), ZuFwd<L>(l)}, ZuFwd<Args>(args)...); }
  template <typename L> ZuInline void rxRun_(L &&l)
    { this->engine()->rxRun_(ZmFn<>{rx(), ZuFwd<L>(l)}); }
  template <typename L> ZuInline void rxRun_(L &&l) const
    { this->engine()->rxRun_(ZmFn<>{rx(), ZuFwd<L>(l)}); }
  template <typename L> ZuInline void rxInvoke(L &&l)
    { this->engine()->rxInvoke(rx(), ZuFwd<L>(l)); }
  template <typename L> ZuInline void rxInvoke(L &&l) const
    { this->engine()->rxInvoke(rx(), ZuFwd<L>(l)); }

  // ensure passed lambdas are stateless and match required signature
  template <typename L> struct RcvdLambda_ {
    typedef void (*Fn)(Rx *);
    enum { OK = ZuConversion<L, Fn>::Exists };
  };
  template <typename L, bool = RcvdLambda_<L>::OK>
  struct RcvdLambda;
  template <typename L> struct RcvdLambda<L, true> {
    using T = void;
    ZuInline static void invoke(Rx *rx) { (*(L *)(void *)0)(rx); }
  };

  template <typename L>
  typename RcvdLambda<L>::T received(ZmRef<ZvQMsg> msg, L) {
    msg->owner(rx());
    this->engine()->rxInvoke(ZuMv(msg), [](ZmRef<ZvQMsg> msg) {
      Rx *rx = msg->owner<Rx *>();
      rx->received(ZuMv(msg));
      RcvdLambda<L>::invoke(rx);
    });
  }
  void received(ZmRef<ZvQMsg> msg) {
    msg->owner(rx());
    this->engine()->rxInvoke(ZuMv(msg), [](ZmRef<ZvQMsg> msg) {
      Rx *rx = msg->owner<Rx *>();
      rx->received(ZuMv(msg));
    });
  }

  void send(ZmRef<ZvQMsg> msg) {
    // permit callers to use code of the form send(mkMsg(...))
    // (i.e. without needing to explicitly check mkMsg() success/failure)
    if (ZuUnlikely(!msg)) return;
    msg->owner(tx());
    this->engine()->txInvoke(ZuMv(msg), [](ZmRef<ZvQMsg> msg) {
      msg->owner<Tx *>()->send(ZuMv(msg));
    });
  }
  template <typename L>
  void abort(ZvSeqNo seqNo, L l) {
    this->txInvoke([seqNo, l = ZuMv(l)](Tx *tx) mutable {
      l(tx->abort(seqNo));
    });
  }
  void archived(ZvSeqNo seqNo)
    { this->txInvoke([seqNo](Tx *tx) { tx->archived(seqNo); }); }

private:
  // prevent direct call from Impl - must be called via rx/tx Run/Invoke
  using Rx_::rxReset;		// Rx - reset sequence numbers
  using Rx_::startQueuing;	// Rx - start queuing
  using Rx_::stopQueuing;	// Rx - stop queuing (start processing)
  // using Rx_::received;	// Rx - received (from network)
  using Tx_::start;		// Tx - start
  using Tx_::stop;		// Tx - stop
  using Tx_::ackd;		// Tx - ackd (due to received message)
  using Tx_::archived;		// Tx - archived (following call to archive_)
  // using Tx::send;		// Tx - send (from app)
  // using Tx::abort;		// Tx - abort (from app)
  using Tx::unload;		// Tx - unload all messages (for reload)
  using Tx::txReset;		// Tx - reset sequence numbers
  using Tx::join;		// Tx - join pool
  using Tx::leave;		// Tx - leave pool
  using Tx::ready;		// Tx - inform pool(s) of readiness
  using Tx::unready;		// Tx - inform pool(s) not ready
  // should not be called from Impl at all
  using Rx_::dequeue;		// handled by ZvLink
  using Rx_::reRequest;		// handled by ZvLink
  using Tx_::resend;		// handled by ZvTx
  using Tx_::archive;		// handled by ZvTx
  using Tx::ready_;		// internal to ZvQueueTx
  using Tx::unready_;		// internal to ZvQueueTx

  ZmScheduler::Timer	m_rrTimer;

  RRLock		m_rrLock;
    ZmTime		  m_rrTime;
};

#endif /* ZvEngine_HPP */