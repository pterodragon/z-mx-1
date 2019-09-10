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

#ifndef MxQueue_HPP
#define MxQueue_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <ZuArrayN.hpp>
#include <ZuPOD.hpp>
#include <ZuRef.hpp>

#include <ZmPQueue.hpp>
#include <ZmFn.hpp>
#include <ZmRBTree.hpp>

#include <ZiIP.hpp>
#include <ZiMultiplex.hpp>

#include <MxMsgID.hpp>

struct MxQMsgData {
  ZuRef<ZuPolymorph>	payload;
  void			*owner_ = 0;
  MxMsgID		id;
  int32_t		skip_ = 0; // -ve - no queuing; +ve - gap fill
  uint32_t		length = 0; // occupied length of payload in bytes

  ZuInline MxQMsgData(ZuRef<ZuPolymorph> payload_) :
      payload(ZuMv(payload_)) { }
  ZuInline MxQMsgData(ZuRef<ZuPolymorph> payload_, unsigned length_) :
      payload(ZuMv(payload_)), length(length_) { }
  ZuInline MxQMsgData(
      ZuRef<ZuPolymorph> payload_, unsigned length_, MxMsgID id_) :
      payload(ZuMv(payload_)), id(id_), length(length_) { }

  template <typename T>
  ZuInline const T *ptr() const { return payload.ptr<T>(); }
  template <typename T>
  ZuInline T *ptr() { return payload.ptr<T>(); }

  template <typename T = void *> ZuInline T owner() const { return (T)owner_; }
  template <typename T> ZuInline void owner(T v) { owner_ = (void *)v; }

  ZuInline void load(const MxMsgID &id_) { id = id_; }
  ZuInline void unload() { id = MxMsgID{}; }

  ZuInline unsigned skip() const { return skip_ <= 0 ? 1 : skip_; }
  ZuInline void skip(unsigned n) { skip_ = n; }

  ZuInline bool noQueue() const { return skip_ < 0; }
  ZuInline void noQueue(int b) { skip_ = -b; }
};

class MxQFn {
public:
  typedef MxSeqNo Key;
  ZuInline MxQFn(MxQMsgData &item) : m_item(item) { }
  ZuInline MxSeqNo key() const { return m_item.id.seqNo; }
  ZuInline unsigned length() const { return m_item.skip(); }
  ZuInline unsigned clipHead(unsigned n) { return length(); }
  ZuInline unsigned clipTail(unsigned n) { return length(); }
  ZuInline void write(const MxQFn &) { }
  ZuInline unsigned bytes() const { return m_item.length; }

private:
  MxQMsgData	&m_item;
};

struct MxQMsg_HeapID { inline static const char *id() { return "MxQMsg"; } };
typedef ZmPQueue<MxQMsgData,
	  ZmPQueueNodeIsItem<true,
	    ZmPQueueObject<ZmPolymorph,
	      ZmPQueueFn<MxQFn,
		ZmPQueueLock<ZmNoLock,
		  ZmPQueueHeapID<MxQMsg_HeapID,
		    ZmPQueueBase<ZmObject> > > > > > > MxQueue;
typedef MxQueue::Node MxQMsg;
typedef MxQueue::Gap MxQGap;

// MxQueueRx - receive queue

// CRTP - application must conform to the following interface:
#if 0
struct Impl : public MxQueueRx<Impl> {
  void process(MxQMsg *);		// rx process

  void scheduleDequeue();
  void rescheduleDequeue();
  void idleDequeue();

  void scheduleReRequest();
  void cancelReRequest();

  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now);
  void reRequest(const MxQueue::Gap &now);
};
#endif

template <class Impl, class Lock_ = ZmNoLock>
class MxQueueRx : public ZmPQRx<Impl, MxQueue, Lock_> {
  typedef ZmPQRx<Impl, MxQueue, Lock_> Rx;

public:
  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

  ZuInline MxQueueRx() : m_queue(new MxQueue(MxSeqNo())) { }
  
  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline const MxQueue *rxQueue() const { return m_queue; }
  ZuInline MxQueue *rxQueue() { return m_queue; }

  ZuInline void rxInit(MxSeqNo seqNo) {
    if (seqNo > m_queue->head()) m_queue->head(seqNo);
  }

private:
  ZmRef<MxQueue>	m_queue;
};

// MxQueueTx - transmit queue
// MxQueueTxPool - transmit fan-out queue

#define MxQueueMaxPools 8	// max #pools a tx queue can be a member of

template <class Impl, class Lock> class MxQueueTxPool;

// CRTP - application must conform to the following interface:
#if 0
struct Impl : public MxQueueTx<Impl> {
  void archive_(MxQMsg *);			// tx archive (persistent)
  ZmRef<MxQMsg> retrieve_(MxSeqNo, MxSeqNo);	// tx retrieve

  void scheduleSend();
  void rescheduleSend();
  void idleSend();

  void scheduleResend();
  void rescheduleResend();
  void idleResend();

  void scheduleArchive();
  void rescheduleArchive();
  void idleArchive();

  void loaded_(MxQMsg *msg);		// may adjust readiness
  void unloaded_(MxQMsg *msg);		// ''

  // send_() must perform one of
  // 1] usual case (successful send): persist seqNo, return true
  // 2] stale message: abort message, return true
  // 3] transient failure (throttling, I/O, etc.): return false
  bool send_(MxQMsg *msg, bool more);
  // resend_() must perform one of
  // 1] usual case (successful resend): return true
  // 2] transient failure (throttling, I/O, etc.): return false
  bool resend_(MxQMsg *msg, bool more);

  // sendGap_() and resendGap_() return true, or false on transient failure
  bool sendGap_(const MxQueue::Gap &gap, bool more);
  bool resendGap_(const MxQueue::Gap &gap, bool more);
};

struct Impl : public MxQueueTxPool<Impl> {
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

  void aborted_(MxQMsg *msg);

  void loaded_(MxQMsg *msg);		// may adjust readiness
  void unloaded_(MxQMsg *msg);		// ''
};
#endif

template <class Impl, class Lock_ = ZmNoLock>
class MxQueueTx : public ZmPQTx<Impl, MxQueue, Lock_> {
  typedef ZmPQTx<Impl, MxQueue, Lock_> Tx;
  typedef MxQueueTxPool<Impl, Lock_> Pool;

  typedef ZuArrayN<Pool *, MxQueueMaxPools> Pools;

public:
  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

protected:
  ZuInline const Lock &lock() const { return m_lock; }
  ZuInline Lock &lock() { return m_lock; }

public:
  ZuInline MxQueueTx() : m_queue(new MxQueue(MxSeqNo())) { }

  ZuInline const Impl *impl() const { return static_cast<const Impl *>(this); }
  ZuInline Impl *impl() { return static_cast<Impl *>(this); }

  ZuInline const MxSeqNo txSeqNo() const { return m_seqNo; }

  ZuInline const MxQueue *txQueue() const { return m_queue; }
  ZuInline MxQueue *txQueue() { return m_queue; }

  ZuInline void txInit(MxSeqNo seqNo) {
    if (seqNo > m_seqNo) m_queue->head(m_seqNo = seqNo);
  }

  ZuInline void send() { Tx::send(); }
  void send(ZmRef<MxQMsg> msg) {
    if (ZuUnlikely(msg->noQueue())) {
      Guard guard(m_lock);
      if (ZuUnlikely(!m_ready)) {
	guard.unlock();
	impl()->aborted_(ZuMv(msg));
	return;
      }
    }
    msg->load(MxMsgID{impl()->id(), m_seqNo++});
    impl()->loaded_(msg);
    Tx::send(ZuMv(msg));
  }
  bool abort(MxSeqNo seqNo) {
    ZmRef<MxQMsg> msg = Tx::abort(seqNo);
    if (msg) {
      impl()->aborted_(msg);
      impl()->unloaded_(msg);
      msg->unload();
      return true;
    }
    return false;
  }

  // unload all messages from queue
  void unload(ZmFn<MxQMsg *> fn) {
    while (ZmRef<MxQMsg> msg = m_queue->shift()) {
      impl()->unloaded_(msg);
      msg->unload();
      fn(msg);
    }
  }

  ZuInline void ackd(MxSeqNo seqNo) {
    if (m_seqNo < seqNo) m_seqNo = seqNo;
    Tx::ackd(seqNo);
  }

  ZuInline void txReset(MxSeqNo seqNo = MxSeqNo()) {
    Tx::txReset(m_seqNo = seqNo);
  }

  // fails silently if MxQueueMaxPools exceeded
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

  MxSeqNo		m_seqNo;
  ZmRef<MxQueue>	m_queue;

  Lock			m_lock;
    Pools		  m_pools;
    unsigned		  m_poolOffset = 0;
    ZmTime		  m_ready;
};

template <class Impl, class Lock_ = ZmNoLock>
class MxQueueTxPool : public MxQueueTx<Impl, Lock_> {
  struct Queues_HeapID {
    inline static const char *id() { return "MxQueueTxPool.Queues"; }
  };

  typedef MxQueue::Gap Gap;
  typedef MxQueueTx<Impl, Lock_> Tx;

  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;

  typedef ZmRBTree<ZmTime,
	    ZmRBTreeVal<ZmRef<Tx>,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<Queues_HeapID> > > > > Queues;

public:
  ZuInline void loaded_(MxQMsg *) { }   // may be overridden by Impl
  ZuInline void unloaded_(MxQMsg *) { } // ''

  bool send_(MxQMsg *msg, bool more) {
    if (ZmRef<Tx> next = next_()) {
      next->send(msg);
      sent_(msg);
      return true;
    }
    return false;
  }
  ZuInline bool resend_(MxQMsg *, bool) { return true; } // unused
  ZuInline void aborted_(MxQMsg *) { } // unused

  ZuInline bool sendGap_(const Gap &, bool) { return true; } // unused
  ZuInline bool resendGap_(const Gap &, bool) { return true; } // unused

  ZuInline void sent_(MxQMsg *msg) { Tx::ackd(msg->id.seqNo + 1); }
  ZuInline void archive_(MxQMsg *msg) { Tx::archived(msg->id.seqNo + 1); }
  ZuInline ZmRef<MxQMsg> retrieve_(MxSeqNo, MxSeqNo) { // unused
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
void MxQueueTx<Impl, Lock_>::ready_(ZmTime next)
{
  unsigned i, n = m_pools.length();
  unsigned o = (m_poolOffset = (m_poolOffset + 1) % n);
  for (i = 0; i < n; i++)
    m_pools[(i + o) % n]->ready_(this, m_ready, next);
  m_ready = next;
}

template <class Impl, class Lock_>
void MxQueueTx<Impl, Lock_>::unready_()
{
  unsigned i, n = m_pools.length();
  unsigned o = (m_poolOffset = (m_poolOffset + 1) % n);
  for (i = 0; i < n; i++)
    m_pools[(i + o) % n]->unready_(this, m_ready);
  m_ready = ZmTime();
}

#endif /* MxQueue_HPP */
