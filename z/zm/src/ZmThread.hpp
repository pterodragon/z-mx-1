//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// thread class

#ifndef ZmThread_HPP
#define ZmThread_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <cstddef>

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuStringN.hpp>
#include <zlib/ZuPrint.hpp>

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmBitmap.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmCleanup.hpp>
#include <zlib/ZmFn.hpp>
#include <zlib/ZmTime.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

using ZmThreadID = ZmPlatform::ThreadID;

namespace ZmThreadPriority {
  enum _ {		// thread priorities
    Unset = -1,
    RealTime = 0,
    High = 1,
    Normal = 2,
    Low = 3
  };
}

using ZmThreadName = ZuStringN<28>;

// display sequence:
//   name, id, tid, cpuUsage, cpuset, priority, sysPriority,
//   stackSize, partition, main, detached
struct ZmThreadTelemetry {
  ZmThreadName	name;
  uint64_t	tid = 0;	// primary key
  uint64_t	stackSize = 0;
  ZmBitmap	cpuset;
  double	cpuUsage = 0.0;	// graphable (*)
  int32_t	sysPriority = 0;
  int16_t	index = 0;	// index within thread pool (ZmScheduler, ...)
  uint16_t	partition = 0;
  int8_t	priority = -1;
  uint8_t	main = 0;
  uint8_t	detached = 0;
};

class ZmThreadContext;

#ifndef _WIN32
extern "C" { ZmExtern void *ZmThread_start(void *); }
#else
extern "C" { ZmExtern unsigned __stdcall ZmThread_start(void *); }
#endif

#ifndef _WIN32
extern "C" {
  typedef int (*ZmThread_CreateFn)(
      pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
};
#endif

class ZmThreadParams {
public:
  ZmThreadParams &&name(ZuString s)
    { m_name = s; return ZuMv(*this); }
  ZmThreadParams &&stackSize(unsigned v)
    { m_stackSize = v; return ZuMv(*this); }
  ZmThreadParams &&priority(int v)
    { m_priority = v; return ZuMv(*this); }
  ZmThreadParams &&partition(unsigned v)
    { m_partition = v; return ZuMv(*this); }
  ZmThreadParams &&cpuset(const ZmBitmap &b)
    { m_cpuset = b; return ZuMv(*this); }
  ZmThreadParams &&detached(bool b)
    { m_detached = b; return ZuMv(*this); }
#ifndef _WIN32
  ZmThreadParams &&createFn(ZmThread_CreateFn fn)
    { m_createFn = fn; return ZuMv(*this); }
#endif

  const ZmThreadName &name() const { return m_name; }
  unsigned stackSize() const { return m_stackSize; }
  int priority() const { return m_priority; }
  unsigned partition() const { return m_partition; }
  const ZmBitmap &cpuset() const { return m_cpuset; }
  bool detached() const { return m_detached; }
#ifndef _WIN32
  const ZmThread_CreateFn &createFn() const { return m_createFn; }
#endif

private:
  ZmThreadName		m_name;
  unsigned		m_stackSize = 0;
  int			m_priority = -1;
  unsigned		m_partition = 0;
  ZmBitmap		m_cpuset;
#ifndef _WIN32
  ZmThread_CreateFn	m_createFn = &pthread_create;
#endif
  bool			m_detached = false;
};

class ZmAPI ZmThreadContext_ {
#ifndef _WIN32
  friend ZmAPI void *ZmThread_start(void *);
#else
  friend ZmAPI unsigned __stdcall ZmThread_start(void *);
#endif
protected:
  ZmThreadContext_() { }

public:
  ZuInline bool main() const { return m_main; }

#ifndef _WIN32
  ZuInline pthread_t pthread() const { return m_pthread; }
#ifdef linux
  ZuInline pid_t tid() const { return m_tid; }
#else
  ZuInline pthread_t tid() const { return m_pthread; }
#endif
  ZuInline clockid_t cid() const { return m_cid; }
  double cpuUsage() const {
    ZmTime cpuLast = m_cpuLast;
    ZmTime rtLast = m_rtLast;
    clock_gettime(m_cid, &m_cpuLast);
    m_rtLast.now();
    if (ZuUnlikely(!cpuLast || !rtLast)) return 0.0;
    double cpuDelta = (m_cpuLast - cpuLast).dtime();
    double rtDelta = (m_rtLast - rtLast).dtime();
    return cpuDelta / rtDelta;
  }
  int32_t sysPriority() const {
    struct sched_param p;
#ifdef linux
    sched_getparam(m_tid, &p);
#else
    sched_getparam(0, &p);
#endif
    return p.sched_priority;
  }
#else /* !_WIN32 */
  ZuInline unsigned tid() const { return m_tid; }
  ZuInline HANDLE handle() const { return m_handle; }
  double cpuUsage() const {
    ULONG64 cpuLast = m_cpuLast;
    ULONG64 rtLast = m_rtLast;
    QueryThreadCycleTime(m_handle, &m_cpuLast);
    m_rtLast = __rdtsc();
    if (ZuUnlikely(!cpuLast || !rtLast)) return 0.0;
    return (double)(m_cpuLast - cpuLast) / (double)(m_rtLast - rtLast);
  }
  int32_t sysPriority() const {
    return GetThreadPriority(m_handle);
  }
#endif /* !_WIN32 */

protected:
  void init();

  bool			m_main = false;
#ifndef _WIN32
  pthread_t		m_pthread = 0;
#ifdef linux
  pid_t			m_tid = 0;
#endif
  clockid_t		m_cid = 0;
  mutable ZmTime	m_cpuLast;
  mutable ZmTime	m_rtLast;
#else /* !_WIN32 */
  unsigned		m_tid = 0;
  HANDLE		m_handle = 0;
  mutable ULONG64	m_cpuLast = 0;
  mutable ULONG64	m_rtLast = 0;
#endif /* !_WIN32 */
};

template <> struct ZmCleanup<ZmThreadContext> {
  enum { Level = ZmCleanupLevel::Thread };
};

template <typename, bool> struct ZmSpecificCtor;

class ZmThread;

class ZmAPI ZmThreadContext : public ZmObject, public ZmThreadContext_ {
  friend ZmSpecificCtor<ZmThreadContext, true>;
#ifndef _WIN32
  friend ZmAPI void *ZmThread_start(void *);
#else
  friend ZmAPI unsigned __stdcall ZmThread_start(void *);
#endif
  friend ZmThread;

  ZmThreadContext() // only called via self() for unmanaged threads
    { init(); }
  template <typename Fn>
  ZmThreadContext(int index, Fn &&fn, const ZmThreadParams &params) :
      m_fn(ZuFwd<Fn>(fn)),
      m_index(index), m_name(params.name()),
      m_stackSize(params.stackSize()), m_priority(params.priority()),
      m_partition(params.partition()), m_cpuset(params.cpuset()),
      m_result(nullptr),
      m_detached(params.detached()) { }

public:
  ~ZmThreadContext() { }

  void init();

  void prioritize(int priority);
  void bind(unsigned partition, const ZmBitmap &cpuset);

  inline static ZmThreadContext *self();

  ZuInline ZmThreadID tid() const {
#ifndef _WIN32
#ifdef linux
    return m_tid;
#else
    return m_pthread;
#endif
#else /* !_WIN32 */
    return m_tid;
#endif /* !_WIN32 */
  }

  ZuInline const ZmFn<> &fn() const { return m_fn; }

  ZuInline int index() const { return m_index; }
  ZuInline const ZmThreadName &name() const { return m_name; }

  ZuInline unsigned stackSize() const { return m_stackSize; }
  ZuInline int priority() const { return m_priority; }

  ZuInline unsigned partition() const { return m_partition; }
  ZuInline const ZmBitmap &cpuset() const { return m_cpuset; }

  ZuInline void *result() const { return m_result; }

  ZuInline bool detached() const { return m_detached; }

  void telemetry(ZmThreadTelemetry &data) const;

  template <typename S> void print(S &s) const {
    s << this->name() << " (" << ZuBoxed(tid()) << ") [" << m_cpuset << "] "
      << ZuBoxed(cpuUsage() * 100.0).fmt(ZuFmt::FP<2>()) << '%';
  }

private:
  inline static ZmThreadContext *self(ZmThreadContext *c);

  void prioritize();
  void bind();

  ZmFn<>	m_fn;

  int		m_index = -1;	// index within thread pool/group
  ZmThreadName	m_name;
  unsigned	m_stackSize = 0;
  int		m_priority = -1;
  unsigned	m_partition = 0;
  ZmBitmap	m_cpuset;

  void		*m_result = nullptr;

  bool		m_detached = false;
};

template <> struct ZuPrint<ZmThreadContext> : public ZuPrintFn { };
template <> struct ZuPrint<ZmThreadContext *> : public ZuPrintDelegate {
  template <typename S>
  static void print(S &s, const ZmThreadContext *v) {
    if (!v)
      s << "null";
    else
      s << *v;
  }
};

#define ZmSelf() (ZmThreadContext::self())

class ZmAPI ZmThread {
public:
  using Context = ZmThreadContext;
  using ID = ZmThreadID;

  ZmThread() { }
  template <typename Fn>
  ZmThread(int id, Fn &&fn, ZmThreadParams params = ZmThreadParams()) {
    run(id, ZuFwd<Fn>(fn), params); // sets m_context
  }

  ZuInline ZmThread(const ZmThread &t) : m_context(t.m_context) { }
  ZuInline ZmThread &operator =(const ZmThread &t) {
    m_context = t.m_context;
    return *this;
  }

  ZuInline ZmThread(Context *context) : m_context(context) { }
  ZuInline ZmThread &operator =(Context *context) {
    m_context = context;
    return *this;
  }

  int run(int id, ZmFn<> fn, ZmThreadParams params = ZmThreadParams());

  int join(void **status = 0);

  ZmRef<Context> context() { return m_context; }

  ZuInline ID tid() const {
    if (!m_context) return 0;
    return m_context->tid();
  }

  ZuInline int cmp(const ZmThread &t) const {
    return ZuCmp<ID>::cmp(tid(), t.tid());
  }
  ZuInline bool less(const ZmThread &t) const {
    return tid() < t.tid();
  }
  ZuInline bool equals(const ZmThread &t) const {
    return tid() == t.tid();
  }

  ZuInline bool operator ==(const ZmThread &t) const { return equals(t); }
  ZuInline bool operator !=(const ZmThread &t) const { return !equals(t); }
  ZuInline bool operator >(const ZmThread &t) const { return t.less(*this); }
  ZuInline bool operator >=(const ZmThread &t) const { return !less(t); }
  ZuInline bool operator <(const ZmThread &t) const { return less(t); }
  ZuInline bool operator <=(const ZmThread &t) const { return !t.less(*this); }

  ZuInline uint32_t hash() { return ZuHash<ID>::hash(tid()); }

  ZuInline bool operator !() const { return !m_context; }
  ZuOpBool

  template <class S> struct CSV_ {
    CSV_(S &stream) : m_stream(stream) { 
      m_stream <<
	"name,tid,cpuUsage,cpuSet,sysPriority,priority,"
	"stackSize,partition,main,detached\n";
    }
    void print(const ZmThreadContext *tc) {
      ZmThreadTelemetry data;
      static ZmPLock lock;
      ZmGuard<ZmPLock> guard(lock);
      tc->telemetry(data);
      m_stream << data.name
	<< ',' << data.tid
	<< ',' << ZuBoxed(data.cpuUsage * 100.0).fmt(ZuFmt::FP<2>())
	<< ",\"" << ZmBitmap(data.cpuset) << '"'
	<< ',' << ZuBoxed(data.sysPriority)
	<< ',' << ZuBoxed(data.priority)
	<< ',' << data.stackSize
	<< ',' << ZuBoxed(data.partition)
	<< ',' << ZuBoxed(data.main)
	<< ',' << ZuBoxed(data.detached) << '\n';
    }
    S &stream() { return m_stream; }

  private:
    S	&m_stream;
  };
  struct CSV {
    template <typename S> void print(S &s) const {
      CSV_<S> csv(s);
      ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{&csv,
	  [](CSV_<S> *csv, ZmThreadContext *tc) { csv->print(tc); }});
    }
  };
  ZuInline static CSV csv() { return CSV(); }

private:
  ZmRef<Context>	m_context;
};

template <> struct ZuTraits<ZmThread> : public ZuBaseTraits<ZmThread> {
  enum { IsComparable = 1, IsHashable = 1 };
};

template <> struct ZuPrint<ZmThread::CSV> : public ZuPrintFn { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <zlib/ZmSpecific.hpp>

inline ZmThreadContext *ZmThreadContext::self() {
  return ZmSpecific<ZmThreadContext>::instance();
}
inline ZmThreadContext *ZmThreadContext::self(ZmThreadContext *c) {
  return ZmSpecific<ZmThreadContext>::instance(c);
}

#endif /* ZmThread_HPP */
