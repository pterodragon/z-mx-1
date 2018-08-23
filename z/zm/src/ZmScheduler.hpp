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
      while ((j = va_arg(args, int)) >= 0) m_cpusets[i] << j;
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

typedef ZuStringN<32> ZmSchedulerID;

class ZmSchedulerParams {
public:
  typedef ZmSchedulerID ID;

  inline ZmSchedulerParams() :
      m_nThreads(1), m_stackSize(0),
      m_priority(ZmThreadPriority::Normal), m_partition(0),
      m_quantum(.01), m_queueSize(131072),
      m_ll(false), m_spin(1000), m_timeout(1) { }

  template <typename S> inline ZmSchedulerParams &id(const S &s)
    { m_id = s; return *this; }
  inline ZmSchedulerParams &nThreads(unsigned v)
    { m_nThreads = v; return *this; }
  inline ZmSchedulerParams &stackSize(unsigned v)
    { m_stackSize = v; return *this; }
  inline ZmSchedulerParams &priority(unsigned v)
    { m_priority = v; return *this; }
  inline ZmSchedulerParams &partition(unsigned v)
    { m_partition = v; return *this; }
  template <typename T> inline ZmSchedulerParams &affinity(const T &t)
    { m_affinity = t; return *this; }
  template <typename T> inline ZmSchedulerParams &isolation(const T &t)
    { m_isolation = t; return *this; }
  template <typename T> inline ZmSchedulerParams &quantum(const T &t)
    { m_quantum = t; return *this; }

  inline ZmSchedulerParams &queueSize(unsigned v)
    { m_queueSize = v; return *this; }
  inline ZmSchedulerParams &ll(bool v)
    { m_ll = v; return *this; }
  inline ZmSchedulerParams &spin(unsigned v)
    { m_spin = v; return *this; }
  inline ZmSchedulerParams &timeout(unsigned v)
    { m_timeout = v; return *this; }

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

private:
  ID		m_id;
  unsigned	m_nThreads;
  unsigned	m_stackSize;
  unsigned	m_priority;
  unsigned	m_partition;
  ZmAffinity	m_affinity;
  ZmBitmap	m_isolation;
  ZmTime	m_quantum;

  unsigned	m_queueSize;
  bool		m_ll;
  unsigned	m_spin;
  unsigned	m_timeout;
};

class ZmAPI ZmScheduler : public ZmThreadMgr {
  ZmScheduler(const ZmScheduler &);
  ZmScheduler &operator =(const ZmScheduler &);	// prevent mis-use

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
  typedef ZmRef<ScheduleTree::Node> Timer;
  typedef ZmSchedulerID ID;
  enum State { Stopped = 0, Starting, Running, Draining, Drained, Stopping };

  // might throw ZmRingError
  ZmScheduler(ZmSchedulerParams params = ZmSchedulerParams());
  virtual ~ZmScheduler();

  void start();
  void drain();
  void stop();
  void reset(); // reset while stopped

  inline int state() const {
    ZmReadGuard<ZmLock> stateGuard(m_stateLock);
    return m_state;
  }

  template <typename L>
  inline auto stateLocked(L &&l) const {
    ZmReadGuard<ZmLock> stateGuard(m_stateLock);
    return l(m_state);
  }

public:
  enum { Update = 0, Advance, Defer };

  // add(fn) - immediate execution (asynchronous) on any worker thread
  // run(tid, fn) - immediate execution (asynchronous) on a specific thread
  // invoke(tid, fn) - immediate execution on a specific thread
  //   unlike run(), invoke() will execute synchronously if the caller is
  //   already running on the specified thread

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
    run_(&m_threads[tid - 1], ZuFwd<Fn>(fn));
  }
  template <typename Fn, typename O>
  ZuInline void run(unsigned tid, Fn &&fn, O &&o) {
    ZmAssert(tid && tid <= m_nThreads);
    run_(&m_threads[tid - 1], ZuFwd<Fn>(fn), ZuFwd<O>(o));
  }

  template <typename Fn>
  ZuInline void invoke(unsigned tid, Fn &&fn) {
    ZmAssert(tid && tid <= m_nThreads);
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) { fn(); return; }
    run_(thread, ZuFwd<Fn>(fn));
  }
  template <typename Fn, typename O>
  ZuInline void invoke(unsigned tid, Fn &&fn, O &&o) {
    ZmAssert(tid && tid <= m_nThreads);
    Thread *thread = &m_threads[tid - 1];
    if (ZuLikely(ZmPlatform::getTID() == thread->tid)) {
      fn(ZuFwd<O>(o));
      return;
    }
    run_(thread, ZuFwd<Fn>(fn), ZuFwd<O>(o));
  }

  ZuInline void initThreadFn(ZmFn<> fn) { m_initThreadFn = ZuMv(fn); }

  ZuInline const char *id() const { return m_id; }
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
  inline unsigned count(unsigned tid) const {
    return m_threads[tid].ring.count();
  }

  inline bool ll() const { return m_threads[0].ring.config().ll(); }
  inline unsigned spin() const { return m_threads[0].ring.config().spin(); }
  inline unsigned timeout() const
    { return m_threads[0].ring.config().timeout(); }

protected:
  virtual void threadInit(unsigned tid);
  virtual void threadFinal(unsigned tid);

  void runThreads();

  void busy();
  void idle();

private:
  typedef ZmRing<ZmFn<> > Ring;

  struct Thread {
    ZmSpinLock	lock;
    Ring	ring;
    ZmThreadID	tid = 0;
    ZmThread	thread;
  };

  void timer();

  void work();

  void drained();

  void threadName(ZmThreadName &s, unsigned tid);

  template <typename ...Args>
  ZuInline void run_(Thread *thread, Args &&... args) {
    ZmGuard<ZmSpinLock> guard(thread->lock); // ensure serialized ring push()
    void *ptr;
    if (ZuLikely(ptr = thread->ring.push())) {
      new (ptr) ZmFn<>(ZuFwd<Args>(args)...);
      thread->ring.push2(ptr);
    } else {
      guard.unlock();
      // should never happen - the enqueuing thread will normally
      // be forced to wait for the dequeuing thread to drain the ring
      int status = thread->ring.writeStatus();
      ZuStringN<120> s;
      s << "FATAL - ITC - ZmScheduler::run_() - ZmRing::push() failed: ";
      if (status <= 0)
	s << ZmRingError(status);
      else
	s << ZuBoxed(status) << " bytes remaining";
      s << '\n';
#ifndef _WIN32
      std::cerr << s << std::flush;
#else
      MessageBoxA(0, s, "Thread Dispatch Failure", MB_ICONEXCLAMATION);
#endif
    }
  }

  bool add__(ZmFn<> &fn);
  bool run__(Thread *thread, ZmFn<> &fn);

  ID				m_id;
  unsigned			m_nThreads;
  unsigned			m_stackSize;
  unsigned			m_priority;
  unsigned			m_partition;
  ZmAffinity			m_affinity;
  ZmBitmap			m_isolation;
  ZmTime			m_quantum;

  ZmLock			m_stateLock;
    ZmCondition<ZmLock>		  m_stateCond;
    int				  m_state;
    unsigned			  m_drained;

  ZmPLock			m_scheduleLock;
    ZmSemaphore			  m_pending;
    ZmThread			  m_thread;
    ScheduleTree		  m_schedule;

  ZmAtomic<unsigned>		m_next;
  Thread			*m_threads;
  unsigned			m_nWorkers;
  Thread			**m_workers;

  ZmSpinLock			m_spawnLock;
    unsigned			  m_runThreads;

  ZmSemaphore			m_stopped;

  ZmFn<>			m_initThreadFn;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmScheduler_HPP */
