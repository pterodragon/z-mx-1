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

// scheduler with thread pool

#ifndef ZmScheduler_HPP
#define ZmScheduler_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <stdarg.h>

#include <zlib/ZuNew.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuStringN.hpp>
#include <zlib/ZuMvArray.hpp>
#include <zlib/ZuID.hpp>

#include <zlib/ZmAtomic.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmCondition.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmRing.hpp>
#include <zlib/ZmDRing.hpp>
#include <zlib/ZmRBTree.hpp>
#include <zlib/ZmThread.hpp>
#include <zlib/ZmTime.hpp>
#include <zlib/ZmFn.hpp>
#include <zlib/ZmSpinLock.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251 4231 4355 4660)
#endif

class ZmSchedTParams : public ZmThreadParams {
public:
  inline ZmSchedTParams &&isolated(bool b)
    { m_isolated = b; return ZuMv(*this); }

  ZuInline bool isolated() const { return m_isolated; }

private:
  bool		m_isolated = false;
};

class ZmAPI ZmSchedParams {
  ZmSchedParams(const ZmSchedParams &) = delete;
  ZmSchedParams &operator =(const ZmSchedParams &) = delete;

public:
  ZmSchedParams() = default;
  ZmSchedParams(ZmSchedParams &&) = default;
  ZmSchedParams &operator =(ZmSchedParams &&) = default;

  using Thread = ZmSchedTParams;
  using Threads = ZuMvArray<Thread>;

  using ID = ZuID;

  template <typename S> inline ZmSchedParams &&id(const S &s)
    { m_id = s; return ZuMv(*this); }
  inline ZmSchedParams &&nThreads(unsigned v)
    { m_threads.length((m_nThreads = v) + 1); return ZuMv(*this); }
  inline ZmSchedParams &&stackSize(unsigned v)
    { m_stackSize = v; return ZuMv(*this); }
  inline ZmSchedParams &&priority(unsigned v)
    { m_priority = v; return ZuMv(*this); }
  inline ZmSchedParams &&partition(unsigned v)
    { m_partition = v; return ZuMv(*this); }
  template <typename T> inline ZmSchedParams &&quantum(T &&v)
    { m_quantum = ZuFwd<T>(v); return ZuMv(*this); }

  inline ZmSchedParams &&queueSize(unsigned v)
    { m_queueSize = v; return ZuMv(*this); }
  inline ZmSchedParams &&ll(bool v)
    { m_ll = v; return ZuMv(*this); }
  inline ZmSchedParams &&spin(unsigned v)
    { m_spin = v; return ZuMv(*this); }
  inline ZmSchedParams &&timeout(unsigned v)
    { m_timeout = v; return ZuMv(*this); }

  template <typename L>
  inline ZmSchedParams &&thread(unsigned tid, L l) {
    l(m_threads[tid]);
    return ZuMv(*this);
  }
  inline Thread &thread(unsigned tid) { return m_threads[tid]; }

  inline ID id() const { return m_id; }
  inline unsigned nThreads() const { return m_nThreads; }
  inline unsigned stackSize() const { return m_stackSize; }
  inline int priority() const { return m_priority; }
  inline unsigned partition() const { return m_partition; }
  inline const ZmTime &quantum() const { return m_quantum; }

  inline unsigned queueSize() const { return m_queueSize; }
  inline bool ll() const { return m_ll; }
  inline unsigned spin() const { return m_spin; }
  inline unsigned timeout() const { return m_timeout; }

  inline const Thread &thread(unsigned tid) const { return m_threads[tid]; }

public:
  template <typename S> inline unsigned tid(const S &s) {
    unsigned tid;
    if (tid = ZuBox0(unsigned)(s)) return tid;
    for (tid = 0; tid <= m_nThreads; tid++)
      if (s == m_threads[tid].name()) return tid;
    return 0;
  }

private:
  ID		m_id;
  unsigned	m_nThreads = 1;
  unsigned	m_stackSize = 0;
  int		m_priority = -1;
  unsigned	m_partition = 0;
  ZmTime	m_quantum = .01;

  unsigned	m_queueSize = 131072;
  unsigned	m_spin = 1000;
  unsigned	m_timeout = 1;

  Threads	m_threads = Threads{m_nThreads + 1};

  bool		m_ll = false;
};

class ZmAPI ZmScheduler {
  ZmScheduler(const ZmScheduler &) = delete;
  ZmScheduler &operator =(const ZmScheduler &) = delete;

  struct ScheduleTree_HeapID {
    inline static const char *id() { return "ZmScheduler.ScheduleTree"; }
  };

  struct Timer_ {
    unsigned	tid;
    ZmFn<>	fn;
    void	*ptr;
  };

  using ScheduleTree =
    ZmRBTree<ZmTime,
      ZmRBTreeVal<Timer_,
	ZmRBTreeLock<ZmNoLock,
	  ZmRBTreeHeapID<ScheduleTree_HeapID>>>>;

public:
  using Ring = ZmRing<ZmFn<>>;
  using OverRing_ =  ZmDRing<ZmFn<>, ZmDRingLock<ZmNoLock>>;
  struct OverRing : public OverRing_ {
    using Lock = ZmPLock;
    using Guard = ZmGuard<Lock>;
    using ReadGuard = ZmReadGuard<Lock>;

    ZuInline void push(ZmFn<> fn) {
      Guard guard(m_lock);
      OverRing_::push(ZuMv(fn));
      ++m_inCount;
    }
    ZuInline void unshift(ZmFn<> fn) {
      Guard guard(m_lock);
      OverRing_::unshift(ZuMv(fn));
      --m_outCount;
    }
    ZuInline ZmFn<> shift() {
      Guard guard(m_lock);
      ZmFn<> fn = OverRing_::shift();
      if (fn) ++m_outCount;
      return fn;
    }
    void stats(uint64_t &inCount, uint64_t &outCount) const {
      ReadGuard guard(m_lock);
      inCount = m_inCount;
      outCount = m_outCount;
    }

    Lock	m_lock;
    unsigned	  m_inCount = 0;
    unsigned	  m_outCount = 0;
  };
  enum { OverRing_Increment = 128 };
  using Timer = ZmRef<ScheduleTree::Node>;
  using ID = ZmSchedParams::ID;
  enum { Stopped = 0, Starting, Running, Draining, Drained, Stopping, N };
  inline static const char *stateName(int i) {
    static const char *names[] =
      { "Stopped", "Starting", "Running", "Draining", "Drained", "Stopping" };
    if (i < 0 || i >= N) return "Unknown";
    return names[i];
  }

  // might throw ZmRingError
  ZmScheduler(ZmSchedParams params = ZmSchedParams());
  virtual ~ZmScheduler();

  ZuInline const ZmSchedParams &params() const { return m_params; }
protected:
  ZuInline ZmSchedParams &params_() { return m_params; }

public:
  void start();
  void drain();
  void stop();
  void reset(); // reset while stopped

  inline int state() const {
    ZmReadGuard<ZmLock> stateGuard(m_stateLock);
    return m_state;
  }

  void wakeFn(unsigned tid, ZmFn<> fn);

  enum { Update = 0, Advance, Defer }; // mode

  // add(fn) - immediate execution (asynchronous) on any worker thread
  // run(tid, fn) - immediate execution (asynchronous) on a specific thread
  // invoke(tid, fn) - immediate execution on a specific thread
  //   unlike run(), invoke() will execute synchronously if the caller is
  //   already running on the specified thread; returns true if synchronous

  // run(tid, fn, timeout, mode, &timer) - deferred execution
  //   tid == 0 - run on any worker thread
  //   mode:
  //     Update - (re)schedule regardless
  //     Advance - reschedule unless outstanding timeout is sooner
  //     Defer - reschedule unless outstanding timeout is later

  // add(fn, timeout) -
  //   shorthand for run(0, fn, timeout, Update, 0)
  // add(fn, timeout, &timer) -
  //   shorthand for run(0, fn, timeout, Update, &timer)
  // add(fn, timeout, mode, &timer) -
  //   shorthand for run(0, fn, timeout, mode, &timer)

  // del(&timer) - cancel timer

  void add(ZmFn<> fn);

  ZuInline void add(ZmFn<> fn, ZmTime timeout)
    { run(0, ZuMv(fn), timeout, Update, 0); }
  ZuInline void add(ZmFn<> fn, ZmTime timeout, Timer *ptr)
    { run(0, ZuMv(fn), timeout, Update, ptr); }
  ZuInline void add(ZmFn<> fn, ZmTime timeout, int mode, Timer *ptr)
    { run(0, ZuMv(fn), timeout, mode, ptr); }

  ZuInline void run(unsigned tid, ZmFn<> fn, ZmTime timeout)
    { run(tid, ZuMv(fn), timeout, Update, 0); }
  ZuInline void run(unsigned tid, ZmFn<> fn, ZmTime timeout, Timer *ptr)
    { run(tid, ZuMv(fn), timeout, Update, ptr); }
  void run(unsigned tid, ZmFn<> fn, ZmTime timeout, int mode, Timer *ptr);

  void del(Timer *);				// cancel job

  template <typename Fn>
  ZuInline void run(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_params.nThreads());
    runWake_(&m_threads[tid - 1], ZmFn<>{ZuFwd<Fn>(fn)});
  }
  template <typename Fn>
  ZuInline void run_(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_params.nThreads());
    run__(&m_threads[tid - 1], ZmFn<>{ZuFwd<Fn>(fn)});
  }

  template <typename Fn>
  ZuInline void invoke(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_params.nThreads());
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) { fn(); return; }
    runWake_(thread, ZmFn<>{ZuFwd<Fn>(fn)});
  }
  template <typename O, typename Fn>
  ZuInline void invoke(unsigned tid, ZmRef<O> o, Fn &&fn) {
    ZmAssert(tid && tid <= m_params.nThreads());
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) {
      fn(ZuMv(o));
      return;
    }
    runWake_(thread, ZmFn<>::mvFn(ZuMv(o), ZuFwd<Fn>(fn)));
  }
  template <typename O, typename Fn>
  ZuInline void invoke(unsigned tid, O *o, Fn &&fn) {
    ZmAssert(tid && tid <= m_params.nThreads());
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) {
      fn(o);
      return;
    }
    runWake_(thread, ZmFn<>{o, ZuFwd<Fn>(fn)});
  }

  ZuInline void threadInit(ZmFn<> fn) { m_threadInitFn = ZuMv(fn); }
  ZuInline void threadFinal(ZmFn<> fn) { m_threadFinalFn = ZuMv(fn); }

  ZuInline unsigned nWorkers() const { return m_nWorkers; }
  ZuInline unsigned workerID(unsigned i) const {
    if (ZuLikely(i < m_nWorkers))
      return (m_workers[i] - &m_threads[0]) + 1;
    return 0;
  }

  inline unsigned size() const {
    return m_threads[0].ring.size() * m_params.nThreads();
  }
  inline unsigned count() const {
    unsigned count = 0;
    for (unsigned i = 0, n = m_params.nThreads(); i < n; i++)
      count += m_threads[i].ring.count();
    return count;
  }
  inline const Ring &ring(unsigned tid) const {
    return m_threads[tid - 1].ring;
  }
  inline const OverRing &overRing(unsigned tid) const {
    return m_threads[tid - 1].overRing;
  }

  inline unsigned tid(ZuString s) {
    if (unsigned tid = ZuBox0(unsigned)(s))
      return tid;
    for (unsigned tid = 0, n = m_params.nThreads(); tid <= n; tid++)
      if (s == m_params.thread(tid).name()) return tid;
    return 0;
  }

protected:
  void runThreads();

  void busy();
  void idle();

private:
  using SpawnLock = ZmPLock;
  using SpawnGuard = ZmGuard<SpawnLock>;

  struct Thread {
    Ring		ring;
    ZmFn<>		wakeFn;
    ZmThreadID		tid = 0;
    ZmThread		thread;
    ZmAtomic<unsigned>	overCount;
    OverRing		overRing;	// fallback overflow ring
  };

  ZuInline void wake(Thread *thread) { (thread->wakeFn)(); }

  void timer();
  bool timerAdd(ZmFn<> &fn);

  void runWake_(Thread *thread, ZmFn<> fn);
  bool tryRunWake_(Thread *thread, ZmFn<> &fn);
  bool run__(Thread *thread, ZmFn<> fn);
  bool tryRun__(Thread *thread, ZmFn<> &fn);

  void work();

  void drained();

  ZmSchedParams			m_params;

  ZmLock			m_stateLock;
    ZmCondition<ZmLock>		  m_stateCond;
    int				  m_state = -1;
    unsigned			  m_drained = 0;

  ZmPLock			m_scheduleLock;
    ZmSemaphore			  m_pending;
    ZmThread			  m_thread;
    ScheduleTree		  m_schedule;

  ZmAtomic<unsigned>		m_next;
  Thread			*m_threads;
  unsigned			m_nWorkers = 0;
  Thread			**m_workers;

  SpawnLock			m_spawnLock;
    unsigned			  m_runThreads = 0;

  ZmSemaphore			m_stopped;

  ZmFn<>			m_threadInitFn;
  ZmFn<>			m_threadFinalFn;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmScheduler_HPP */
