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
#include <ZmLib.hpp>
#endif

#include <stdarg.h>

#include <ZuNew.hpp>
#include <ZuNull.hpp>
#include <ZuString.hpp>
#include <ZuStringN.hpp>
#include <ZuPrint.hpp>
#include <ZuID.hpp>

#include <ZmAtomic.hpp>
#include <ZmObject.hpp>
#include <ZmNoLock.hpp>
#include <ZmLock.hpp>
#include <ZmCondition.hpp>
#include <ZmSemaphore.hpp>
#include <ZmRing.hpp>
#include <ZmRBTree.hpp>
#include <ZmPQueue.hpp>
#include <ZmThread.hpp>
#include <ZmTime.hpp>
#include <ZmFn.hpp>
#include <ZmSpinLock.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251 4231 4355 4660)
#endif

class ZmAffinity {
public:
  enum Varargs_ { Varargs };
  enum { Next = -1, End = -2 };

  inline ZmAffinity() { init_(); }
  inline ~ZmAffinity() { final_(); }

  inline ZmAffinity(const ZmAffinity &a) { init_(a); }
  inline ZmAffinity &operator =(const ZmAffinity &a) {
    if (this != &a) { final_(); init_(a); }
    return *this;
  }

  inline ZmAffinity(unsigned n) { init_(n); }
  inline ZmAffinity(unsigned n, Varargs_ _, ...) {
    va_list args;
    va_start(args, _);
    init_(n, args);
    va_end(args);
  }
  inline ZmAffinity(unsigned n, va_list args) { init_(n, args); }

  inline void init() { final_(); init_(); }
  inline void init(unsigned n) { final_(); init_(n); }
  inline void init(unsigned n, Varargs_ _, ...) {
    va_list args;
    va_start(args, _);
    final_();
    init_(n, args);
    va_end(args);
  }
  inline void init(unsigned n, va_list args) { final_(); init_(n, args); }

  inline unsigned count() const { return m_count; }
  inline const ZmBitmap &operator [](unsigned i) const { return m_cpusets[i]; }
  inline ZmBitmap &operator [](unsigned i) { return m_cpusets[i]; }

protected:
  inline void final_() {
    if (m_cpusets) {
      for (unsigned i = 0; i < m_count; i++) m_cpusets[i].~ZmBitmap();
      ::free(m_cpusets);
    }
  }
  inline void init_() { m_count = 0, m_cpusets = 0; }
  inline void init_(const ZmAffinity &a) {
    m_count = 0;
    if (!(m_cpusets = (ZmBitmap *)::malloc(a.m_count * sizeof(ZmBitmap))))
      throw std::bad_alloc();
    for (unsigned i = 0; i < a.m_count; i++)
      new (&m_cpusets[i]) ZmBitmap(a.m_cpusets[i]);
    m_count = a.m_count;
  }
  inline void init_(unsigned n) {
    m_count = 0;
    if (!(m_cpusets = (ZmBitmap *)::malloc(n * sizeof(ZmBitmap))))
      throw std::bad_alloc();
    for (unsigned i = 0; i < n; i++) new (&m_cpusets[i]) ZmBitmap();
    m_count = n;
  }
  inline void init_(unsigned n, va_list args) {
    m_count = 0;
    if (!(m_cpusets = (ZmBitmap *)::malloc(n * sizeof(ZmBitmap))))
      throw std::bad_alloc();
    for (unsigned i = 0; i < n; i++) new (&m_cpusets[i]) ZmBitmap();
    m_count = n;
    int j;
    for (unsigned i = 0; i < n; i++) {
      while ((j = va_arg(args, unsigned)) >= 0) m_cpusets[i].set(j);
      if (j != Next) return;
    }
  }

public:
  template <typename S>
  inline ZmAffinity(const S &s,
      typename ZuIsCharString<S>::T *_ = 0) {
    init_();
    scan(s);
  }
  template <typename S>
  inline typename ZuIsCharString<S, ZmAffinity &>::T operator =(const S &s) {
    init();
    scan(s);
    return *this;
  }
  template <bool Store> inline unsigned scan_(ZuString s) {
    const char *data = s.data();
    unsigned length = s.length(), offset = 0;
    if (!length) return 0;
    unsigned count = 0;
    while (offset < length) {
      ZmBitmap cpuset;
      ZuBox<unsigned> tid;
      int offset_ = tid.scan(data + offset, length - offset);
      if (offset_ <= 0) break;
      offset += offset_;
      if (offset >= length || data[offset] != '=') break;
      ++offset;
      offset_ = cpuset.scan(ZuString(data + offset, length - offset));
      if (offset_ <= 0) break;
      if (!!cpuset) {
	if (tid >= count) count = tid + 1;
	if (Store) m_cpusets[tid] = ZuMv(cpuset);
      }
      offset += offset_;
      if (offset >= length || data[offset] != ':') break;
      ++offset;
    }
    return count;
  }
  inline void scan(ZuString s) {
    init(scan_<false>(s));
    scan_<true>(s);
  }
  template <typename S> inline void print(S &s) const {
    if (!m_count) return;
    bool first = true;
    for (unsigned i = 0; i < m_count; i++) {
      if (!m_cpusets[i]) continue;
      if (!first) s << ':';
      first = false;
      s << ZuBoxed(i) << '=' << m_cpusets[i];
    }
  }

private:
  unsigned	m_count;
  ZmBitmap	*m_cpusets;
};

// generic printing
template <> struct ZuPrint<ZmAffinity> : public ZuPrintFn { };

class ZmAPI ZmSchedParams {
  ZmSchedParams(const ZmSchedParams &) = delete;
  ZmSchedParams &operator =(const ZmSchedParams &) = delete;

  struct ThreadNames {
    ThreadNames(const ThreadNames &) = delete;
    ThreadNames &operator =(const ThreadNames &) = delete;
    ThreadNames() = delete;

    inline ThreadNames(unsigned n) { data = new ZmThreadName[n]; }
    inline ~ThreadNames() { delete [] data; }

    inline ThreadNames(ThreadNames &&a) {
      data = a.data;
      a.data = nullptr;
    }
    inline ThreadNames &operator =(ThreadNames &&a) {
      data = a.data;
      a.data = nullptr;
      return *this;
    }

    ZuInline ZmThreadName &operator [](int i) { return data[i]; }
    ZuInline const ZmThreadName &operator [](int i) const { return data[i]; }

    inline void length(unsigned n) {
      delete [] data;
      data = new ZmThreadName[n];
    }

    ZmThreadName	*data;
  };

public:
  ZmSchedParams() = default;
  ZmSchedParams(ZmSchedParams &&) = default;
  ZmSchedParams &operator =(ZmSchedParams &&) = default;

  typedef ZuID ID;

  template <typename S> inline ZmSchedParams &&id(const S &s)
    { m_id = s; return ZuMv(*this); }
  inline ZmSchedParams &&nThreads(unsigned v)
    { m_names.length(m_nThreads = v); return ZuMv(*this); }
  inline ZmSchedParams &&stackSize(unsigned v)
    { m_stackSize = v; return ZuMv(*this); }
  inline ZmSchedParams &&priority(unsigned v)
    { m_priority = v; return ZuMv(*this); }
  inline ZmSchedParams &&partition(unsigned v)
    { m_partition = v; return ZuMv(*this); }
  template <typename T> inline ZmSchedParams &&affinity(const T &t)
    { m_affinity = t; return ZuMv(*this); }
  template <typename T> inline ZmSchedParams &&isolation(const T &t)
    { m_isolation = t; return ZuMv(*this); }
  template <typename T> inline ZmSchedParams &&quantum(const T &t)
    { m_quantum = t; return ZuMv(*this); }

  inline ZmSchedParams &&queueSize(unsigned v)
    { m_queueSize = v; return ZuMv(*this); }
  inline ZmSchedParams &&ll(bool v)
    { m_ll = v; return ZuMv(*this); }
  inline ZmSchedParams &&spin(unsigned v)
    { m_spin = v; return ZuMv(*this); }
  inline ZmSchedParams &&timeout(unsigned v)
    { m_timeout = v; return ZuMv(*this); }

  inline ID id() const { return m_id; }
  inline unsigned nThreads() const { return m_nThreads; }
  inline unsigned stackSize() const { return m_stackSize; }
  inline unsigned priority() const { return m_priority; }
  inline unsigned partition() const { return m_partition; }
  inline const ZmAffinity &affinity() const { return m_affinity; }
  inline const ZmBitmap &isolation() const { return m_isolation; }
  inline const ZmTime &quantum() const { return m_quantum; }

  inline unsigned queueSize() const { return m_queueSize; }
  inline bool ll() const { return m_ll; }
  inline unsigned spin() const { return m_spin; }
  inline unsigned timeout() const { return m_timeout; }

  inline const ZmThreadName &name(unsigned tid) const {
    static ZmThreadName null;
    if (!tid || tid > m_nThreads) return null;
    return m_names[tid - 1];
  }
  template <typename S> inline void name(unsigned tid, S &&s) {
    if (!tid || tid > m_nThreads) return;
    m_names[tid - 1] = ZuFwd<S>(s);
  }
  template <typename S> inline unsigned tid(const S &s) {
    if (unsigned tid = ZuBox<unsigned>(s))
      return tid;
    for (unsigned tid = 0, n = m_nThreads; tid < n; tid++)
      if (s == m_names[tid]) return tid + 1;
    return 0;
  }

private:
  ID		m_id;
  unsigned	m_nThreads = 1;
  unsigned	m_stackSize = 0;
  unsigned	m_priority = ZmThreadPriority::Normal;
  unsigned	m_partition = 0;
  ZmAffinity	m_affinity;
  ZmBitmap	m_isolation;
  ZmTime	m_quantum = .01;

  unsigned	m_queueSize = 131072;
  bool		m_ll = false;
  unsigned	m_spin = 1000;
  unsigned	m_timeout = 1;

  ThreadNames	m_names = ThreadNames(m_nThreads);
};

class ZmAPI ZmScheduler : public ZmThreadMgr {
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

  typedef ZmRBTree<ZmTime,
	    ZmRBTreeVal<Timer_,
	      ZmRBTreeLock<ZmNoLock,
		ZmRBTreeHeapID<ScheduleTree_HeapID> > > > ScheduleTree;

public:
  typedef ZmRing<ZmFn<> > Ring;
  typedef ZmRef<ScheduleTree::Node> Timer;
  typedef ZmSchedParams::ID ID;
  enum State { Stopped = 0, Starting, Running, Draining, Drained, Stopping };

  // might throw ZmRingError
  ZmScheduler(ZmSchedParams params = ZmSchedParams());
  virtual ~ZmScheduler();

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
    ZmAssert(tid && tid <= m_nThreads);
    runWake_(&m_threads[tid - 1], ZmFn<>{ZuFwd<Fn>(fn)});
  }
  template <typename Fn>
  ZuInline void run_(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_nThreads);
    run__(&m_threads[tid - 1], ZmFn<>{ZuFwd<Fn>(fn)});
  }

  template <typename Fn>
  ZuInline void invoke(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_nThreads);
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) { fn(); return; }
    runWake_(thread, ZmFn<>{ZuFwd<Fn>(fn)});
  }
  template <typename Fn, typename O>
  ZuInline void invoke(unsigned tid, Fn &&fn, O &&o) {
    ZmAssert(tid && tid <= m_nThreads);
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) {
      fn(ZuFwd<O>(o));
      return;
    }
    runWake_(thread, ZmFn{ZuFwd<Fn>(fn), ZuFwd<O>(o)});
  }

  ZuInline void threadInit(ZmFn<> fn) { m_threadInitFn = ZuMv(fn); }
  ZuInline void threadFinal(ZmFn<> fn) { m_threadFinalFn = ZuMv(fn); }

  ZuInline ID id() const { return m_id; }
  ZuInline unsigned nWorkers() const { return m_nWorkers; }
  ZuInline int workerID(unsigned i) const {
    if (ZuLikely(i < m_nWorkers))
      return (m_workers[i] - &m_threads[0]) + 1;
    return -1;
  }
  ZuInline unsigned nThreads() const { return m_nThreads; }
  ZuInline unsigned stackSize() const { return m_stackSize; }
  ZuInline unsigned priority() const { return m_priority; }
  ZuInline unsigned partition() const { return m_partition; }
  ZuInline const ZmAffinity &affinity() const { return m_affinity; }
  ZuInline ZmBitmap affinity(int id) const {
    return (id >= 0 && id < (int)m_affinity.count()) ?
      m_affinity[id] : ZmBitmap();
  }
  ZuInline const ZmBitmap &isolation() const { return m_isolation; }
  ZuInline ZmTime quantum() const { return m_quantum; }

  inline unsigned size() const {
    return m_threads[0].ring.size() * m_nThreads;
  }
  inline unsigned count() const {
    unsigned count = 0;
    for (unsigned i = 0; i < m_nThreads; i++)
      count += m_threads[i].ring.count();
    return count;
  }
  inline const Ring &ring(unsigned tid) const {
    return m_threads[tid - 1].ring;
  }

  inline bool ll() const { return m_threads[0].ring.params().ll(); }
  inline unsigned spin() const { return m_threads[0].ring.params().spin(); }
  inline unsigned timeout() const
    { return m_threads[0].ring.params().timeout(); }

  void threadName(unsigned tid, ZmThreadName &s) const;

  inline const ZmThreadName &name(unsigned tid) const {
    static ZmThreadName null;
    if (!tid || tid > m_nThreads) return null;
    return m_names[tid - 1];
  }
  template <typename S> inline void name(unsigned tid, S &&s) {
    if (!tid || tid > m_nThreads) return;
    m_names[tid - 1] = ZuFwd<S>(s);
  }
  template <typename S> inline unsigned tid(const S &s) {
    if (unsigned tid = ZuBox<unsigned>(s))
      return tid;
    for (unsigned tid = 0, n = m_nThreads; tid < n; tid++)
      if (s == m_names[tid]) return tid + 1;
    return 0;
  }

protected:
  void runThreads();

  void busy();
  void idle();

private:
  struct Thread {
    ZmSpinLock	lock;
    Ring	ring;
    ZmFn<>	wakeFn;
    ZmThreadID	tid = 0;
    ZmThread	thread;
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

  ID				m_id;
  unsigned			m_nThreads = 0;
  unsigned			m_stackSize = 0;
  unsigned			m_priority = 0;
  unsigned			m_partition = 0;
  ZmAffinity			m_affinity;
  ZmBitmap			m_isolation;
  ZmTime			m_quantum;
  
  ZmThreadName			*m_names;

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

  ZmSpinLock			m_spawnLock;
    unsigned			  m_runThreads = 0;

  ZmSemaphore			m_stopped;

  ZmFn<>			m_threadInitFn;
  ZmFn<>			m_threadFinalFn;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmScheduler_HPP */
