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

// Mx Queue

#ifndef ZvQueue_HPP
#define ZvQueue_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuPOD.hpp>
#include <zlib/ZuRef.hpp>
#include <zlib/ZuID.hpp>

#include <zlib/ZmPQueue.hpp>
#include <zlib/ZmFn.hpp>
#include <zlib/ZmRBTree.hpp>

#include <zlib/ZiIP.hpp>
#include <zlib/ZiIOBuf.hpp>
#include <zlib/ZiMultiplex.hpp>

struct ZvSeqNoCmp {
  ZuInline static int cmp(uint64_t v1, uint64_t v2) {
    int64_t d = v1 - v2; // handles wraparound ok
    return d < 0 ? -1 : d > 0 ? 1 : 0;
  }
  ZuInline static bool equals(uint64_t v1, uint64_t v2) { return v1 == v2; }
  ZuInline static bool null(uint64_t v) { return !v; }
  ZuInline static uint64_t null() { return 0; }
};
typedef ZuBox<uint64_t, ZvSeqNoCmp> ZvSeqNo;

struct ZvQMsgID {
  ZuID		linkID;
  ZvSeqNo	seqNo;

  void update(const ZvQMsgID &u) {
    linkID.update(u.linkID);
    seqNo.update(u.seqNo);
  }
  template <typename S> void print(S &s) const {
    s << "linkID=" << linkID << " seqNo=" << seqNo;
  }
};
template <> struct ZuPrint<ZvQMsgID> : public ZuPrintFn { };

struct ZvQItem {
  ZuInline ZvQItem(ZmRef<ZiAnyIOBuf> buf) :
      m_buf(ZuMv(buf)) { }
  ZuInline ZvQItem(ZmRef<ZiAnyIOBuf> buf, ZvQMsgID id) :
      m_buf(ZuMv(buf)), m_id(id) { }

  template <typename T = ZiAnyIOBuf>
  ZuInline const T *buf() const { return m_buf.ptr<T>(); }
  template <typename T = ZiAnyIOBuf>
  ZuInline T *buf() { return m_buf.ptr<T>(); }

  template <typename T = void *> ZuInline T owner() const { return (T)m_owner; }
  template <typename T> ZuInline void owner(T v) { m_owner = (void *)v; }

  ZuInline const ZvQMsgID id() const { return m_id; }

  ZuInline void load(const ZvQMsgID &id) { m_id = id; }
  ZuInline void unload() { m_id = ZvQMsgID{}; }

  ZuInline unsigned skip() const { return m_skip <= 0 ? 1 : m_skip; }
  ZuInline void skip(unsigned n) { m_skip = n; }

  ZuInline bool noQueue() const { return m_skip < 0; }
  ZuInline void noQueue(int b) { m_skip = -b; }

private:
  ZmRef<ZiAnyIOBuf>	m_buf;
  void			*m_owner = 0;
  ZvQMsgID		m_id;
  int32_t		m_skip = 0;	// -ve - no queuing; +ve - gap fill
};

class ZvQFn {
public:
  typedef ZvSeqNo Key;
  ZuInline ZvQFn(ZvQItem &item) : m_item(item) { }
  ZuInline ZvSeqNo key() const { return m_item.id().seqNo; }
  ZuInline unsigned length() const { return m_item.skip(); }
  ZuInline unsigned clipHead(unsigned n) { return length(); }
  ZuInline unsigned clipTail(unsigned n) { return length(); }
  ZuInline void write(const ZvQFn &) { }
  ZuInline unsigned bytes() const { return m_item.buf()->length; }

private:
  ZvQItem	&m_item;
};

struct ZvQMsg_HeapID { inline static const char *id() { return "ZvQMsg"; } };
typedef ZmPQueue<ZvQItem,
	  ZmPQueueNodeIsItem<true,
	    ZmPQueueObject<ZmPolymorph,
	      ZmPQueueFn<ZvQFn,
		ZmPQueueLock<ZmNoLock,
		  ZmPQueueHeapID<ZvQMsg_HeapID,
		    ZmPQueueBase<ZmObject> > > > > > > ZvQueue;
typedef ZvQueue::Node ZvQMsg;
typedef ZvQueue::Gap ZvQGap;

// ZvQueueRx - receive queue

// CRTP - application must conform to the following interface:
#if 0
struct Impl : public ZvQueueRx<Impl> {
  void process(ZvQMsg *);		// rx process

  void scheduleDequeue();
  void rescheduleDequeue();
  void idleDequeue();

  void scheduleReRequest();
  void cancelReRequest();

  void request(const ZvQueue::Gap &prev, const ZvQueue::Gap &now);
  void reRequest(const ZvQueue::Gap &now);
};
#endif

template <class Impl, class Lock_ = ZmNoLock>
class ZvQueueRx : public ZmPQRx<Impl, ZvQueue, Lock_> {
  typedef ZmPQRx<Impl, ZvQueue, Lock_> Rx;

public:
  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

  ZuInline ZvQueueRx() : m_queue(new ZvQueue(ZvSeqNo{})) { }
  
  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline const ZvQueue *rxQueue() const { return m_queue; }
  ZuInline ZvQueue *rxQueue() { return m_queue; }

  ZuInline void rxInit(ZvSeqNo seqNo) {
    if (seqNo > m_queue->head()) m_queue->head(seqNo);
  }

private:
  ZmRef<ZvQueue>	m_queue;
};

// ZvQueueTx - transmit queue
// ZvQueueTxPool - transmit fan-out queue

#define ZvQueueMaxPools 8	// max #pools a tx queue can be a member of

template <class Impl, class Lock> class ZvQueueTxPool;

// CRTP - application must conform to the following interface:
#if 0
struct Impl : public ZvQueueTx<Impl> {
  void archive_(ZvQMsg *);			// tx archive (persistent)
  ZmRef<ZvQMsg> retrieve_(ZvSeqNo, ZvSeqNo);	// tx retrieve

  void scheduleSend();
  void rescheduleSend();
  void idleSend();

  void scheduleResend();
  void rescheduleResend();
  void idleResend();

  void scheduleArchive();
  void rescheduleArchive();
  void idleArchive();

  void loaded_(ZvQMsg *msg);		// may adjust readiness
  void unloaded_(ZvQMsg *msg);		// ''

  // send_() must perform one of
  // 1] usual case (successful send): persist seqNo, return true
  // 2] stale message: abort message, return true
  // 3] transient failure (throttling, I/O, etc.): return false
  bool send_(ZvQMsg *msg, bool more);
  // resend_() must perform one of
  // 1] usual case (successful resend): return true
  // 2] transient failure (throttling, I/O, etc.): return false
  bool resend_(ZvQMsg *msg, bool more);

  // sendGap_() and resendGap_() return true, or false on transient failure
  bool sendGap_(const ZvQueue::Gap &gap, bool more);
  bool resendGap_(const ZvQueue::Gap &gap, bool more);
};

struct Impl : public ZvQueueTxPool<Impl> {
  // Note: below member functions have same signature as above
  void scheduleSend();
  void rescheduleSend();
  void idleSend();

  void scheduleResend();
  void rescheduleResend();
  void idleResend();

  void scheduleArchive();
  void rescheduleArchive();
  void idleArchive();

  void aborted_(ZvQMsg *msg);

  void loaded_(ZvQMsg *msg);		// may adjust readiness
  void unloaded_(ZvQMsg *msg);		// ''
};
#endif

template <class Impl, class Lock_ = ZmNoLock>
class ZvQueueTx : public ZmPQTx<Impl, ZvQueue, Lock_> {
  typedef ZmPQTx<Impl, ZvQueue, Lock_> Tx;
  typedef ZvQueueTxPool<Impl, Lock_> Pool;

  typedef ZuArrayN<Pool *, ZvQueueMaxPools> Pools;

public:
  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

protected:
  ZuInline const Lock &lock() const { return m_lock; }
  ZuInline Lock &lock() { return m_lock; }

public:
  ZuInline ZvQueueTx() : m_queue(new ZvQueue(ZvSeqNo{})) { }

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline const ZvSeqNo txSeqNo() const { return m_seqNo; }

  ZuInline const ZvQueue *txQueue() const { return m_queue; }
  ZuInline ZvQueue *txQueue() { return m_queue; }

  ZuInline void txInit(ZvSeqNo seqNo) {
    if (seqNo > m_seqNo) m_queue->head(m_seqNo = seqNo);
  }

  ZuInline void send() { Tx::send(); }
  void send(ZmRef<ZvQMsg> msg) {
    if (ZuUnlikely(msg->noQueue())) {
      Guard guard(m_lock);
      if (ZuUnlikely(!m_ready)) {
	guard.unlock();
	impl()->aborted_(ZuMv(msg));
	return;
      }
    }
    msg->load(ZvQMsgID{impl()->id(), m_seqNo++});
    impl()->loaded_(msg);
    Tx::send(ZuMv(msg));
  }
  bool abort(ZvSeqNo seqNo) {
    ZmRef<ZvQMsg> msg = Tx::abort(seqNo);
    if (msg) {
      impl()->aborted_(msg);
      impl()->unloaded_(msg);
      msg->unload();
      return true;
    }
    return false;
  }

  // unload all messages from queue
  void unload(ZmFn<ZvQMsg *> fn) {
    while (ZmRef<ZvQMsg> msg = m_queue->shift()) {
      impl()->unloaded_(msg);
      msg->unload();
      fn(msg);
    }
  }

  ZuInline void ackd(ZvSeqNo seqNo) {
    if (m_seqNo < seqNo) m_seqNo = seqNo;
    Tx::ackd(seqNo);
  }

  ZuInline void txReset(ZvSeqNo seqNo = ZvSeqNo{}) {
    Tx::txReset(m_seqNo = seqNo);
  }

  // fails silently if ZvQueueMaxPools exceeded
  void join(Pool *g) {
    Guard guard(m_lock);
    m_pools << g;
  }
  void leave(Pool *g) {
    Guard guard(m_lock);
    unsigned i, n = m_pools.length();
    for (i = 0; i < n; i++)
      if (m_pools[i] == g) {
	m_pools.splice(i, 1);
	return;
      }
  }

  inline void ready() { // ready to send immediately
    Guard guard(m_lock);
    ready_(ZmTime(0, 1));
  }
  inline void ready(ZmTime next) { // ready to send at time next
    Guard guard(m_lock);
    ready_(next);
  }
  inline void unready() { // not ready to send
    Guard guard(m_lock);
    unready_();
  }
protected:
  void ready_(ZmTime next);
  void unready_();

private:

  ZvSeqNo		m_seqNo;
  ZmRef<ZvQueue>	m_queue;

  Lock			m_lock;
    Pools		  m_pools;
    unsigned		  m_poolOffset = 0;
    ZmTime		  m_ready;
};

template <class Impl, class Lock_ = ZmNoLock>
class ZvQueueTxPool : public ZvQueueTx<Impl, Lock_> {
  struct Queues_HeapID {
    inline static const char *id() { return "ZvQueueTxPool.Queues"; }
  };

  typedef ZvQueue::Gap Gap;
  typedef ZvQueueTx<Impl, Lock_> Tx;

  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

  typedef ZmRBTree<ZmTime,
	    ZmRBTreeVal<ZmRef<Tx>,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<Queues_HeapID> > > > > Queues;

public:
  ZuInline void loaded_(ZvQMsg *) { }   // may be overridden by Impl
  ZuInline void unloaded_(ZvQMsg *) { } // ''

  bool send_(ZvQMsg *msg, bool more) {
    if (ZmRef<Tx> next = next_()) {
      next->send(msg);
      sent_(msg);
      return true;
    }
    return false;
  }
  ZuInline bool resend_(ZvQMsg *, bool) { return true; } // unused
  ZuInline void aborted_(ZvQMsg *) { } // unused

  ZuInline bool sendGap_(const Gap &, bool) { return true; } // unused
  ZuInline bool resendGap_(const Gap &, bool) { return true; } // unused

  ZuInline void sent_(ZvQMsg *msg) { Tx::ackd(msg->id().seqNo + 1); }
  ZuInline void archive_(ZvQMsg *msg) { Tx::archived(msg->id().seqNo + 1); }
  ZuInline ZmRef<ZvQMsg> retrieve_(ZvSeqNo, ZvSeqNo) { // unused
    return nullptr;
  }

  inline ZmRef<Tx> next_() {
    Guard guard(this->lock());
    return m_queues.minimumVal();
  }

  void ready_(Tx *queue, ZmTime prev, ZmTime next) {
    Guard guard(this->lock());
    typename Queues::Node *node = 0;
    if (!prev || !(node = m_queues.del(prev, queue))) {
      if (node) delete node;
      m_queues.add(next, queue);
      if (m_queues.count() == 1) {
	Tx::ready_(next);
	guard.unlock();
	Tx::start();
	return;
      }
    } else {
      node->key() = next;
      m_queues.add(node);
    }
    Tx::ready_(m_queues.minimumKey());
  }

  void unready_(Tx *queue, ZmTime prev) {
    Guard guard(this->lock());
    typename Queues::Node *node = 0;
    if (!prev || !(node = m_queues.del(prev, queue))) return;
    delete node;
    if (!m_queues.count()) Tx::unready_();
  }

  Queues	m_queues;	// guarded by Tx::lock()
};

template <class Impl, class Lock_>
void ZvQueueTx<Impl, Lock_>::ready_(ZmTime next)
{
  unsigned i, n = m_pools.length();
  unsigned o = (m_poolOffset = (m_poolOffset + 1) % n);
  for (i = 0; i < n; i++)
    m_pools[(i + o) % n]->ready_(this, m_ready, next);
  m_ready = next;
}

template <class Impl, class Lock_>
void ZvQueueTx<Impl, Lock_>::unready_()
{
  unsigned i, n = m_pools.length();
  unsigned o = (m_poolOffset = (m_poolOffset + 1) % n);
  for (i = 0; i < n; i++)
    m_pools[(i + o) % n]->unready_(this, m_ready);
  m_ready = ZmTime();
}

#endif /* ZvQueue_HPP */
