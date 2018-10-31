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

// thread class

#ifndef ZmThread_HPP
#define ZmThread_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuBox.hpp>
#include <ZuStringN.hpp>
#include <ZuPrint.hpp>

#include <ZmPlatform.hpp>
#include <ZmBitmap.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmCleanup.hpp>
#include <ZmFn.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

typedef ZmPlatform::ThreadID ZmThreadID;

struct ZmThreadPriority {
  enum _ {		// thread priorities
    RealTime = 0,
    High = 1,
    Normal = 2,
    Low = 3
  };
};

typedef ZuStringN<32> ZmThreadName;

struct ZmThreadInfo {
  ZmThreadName	name;
  int		id;
  ZmThreadID	tid;
  bool		main;
  bool		detached;
  unsigned	stackSize;
  int		priority;
  ZmBitmap	cpuset;
};

struct ZmAPI ZmThreadMgr {
  virtual void threadName(ZmThreadName &, unsigned tid) = 0;
};

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

struct ZmThreadParams {
public:
  inline ZmThreadParams() :
    m_detached(false), m_stackSize(0),
    m_priority(ZmThreadPriority::Normal),
    m_partition(0)
#ifndef _WIN32
    , m_createFn(&pthread_create)
#endif
    { }
  inline ZmThreadParams(const ZmThreadParams &p) :
    m_detached(p.m_detached), m_stackSize(p.m_stackSize),
    m_priority(p.m_priority),
    m_partition(p.m_partition), m_cpuset(p.m_cpuset)
#ifndef _WIN32
    , m_createFn(p.m_createFn)
#endif
    { }
  inline ZmThreadParams &operator =(const ZmThreadParams &p) {
    if (this != &p) {
      this->~ZmThreadParams();
      new (this) ZmThreadParams(p);
    }
    return *this;
  }

  inline ZmThreadParams &detached(bool b)
    { m_detached = b; return *this; }
  inline ZmThreadParams &stackSize(unsigned v)
    { m_stackSize = v; return *this; }
  inline ZmThreadParams &priority(int v)
    { m_priority = v; return *this; }
  inline ZmThreadParams &partition(unsigned v)
    { m_partition = v; return *this; }
  inline ZmThreadParams &cpuset(const ZmBitmap &b)
    { m_cpuset = b; return *this; }
#ifndef _WIN32
  inline ZmThreadParams &createFn(ZmThread_CreateFn fn)
    { m_createFn = fn; return *this; }
#endif

  inline bool detached() const { return m_detached; }
  inline unsigned stackSize() const { return m_stackSize; }
  inline int priority() const { return m_priority; }
  inline unsigned partition() const { return m_partition; }
  inline const ZmBitmap &cpuset() const { return m_cpuset; }
#ifndef _WIN32
  inline const ZmThread_CreateFn &createFn() const { return m_createFn; }
#endif

protected:
  bool			m_detached;
  unsigned		m_stackSize;
  int			m_priority;
  unsigned		m_partition;
  ZmBitmap		m_cpuset;
#ifndef _WIN32
  ZmThread_CreateFn	m_createFn;
#endif
};

class ZmAPI ZmThreadContext_ {
#ifndef _WIN32
  friend ZmAPI void *ZmThread_start(void *);
#else
  friend ZmAPI unsigned __stdcall ZmThread_start(void *);
#endif
protected:
  ZuInline ZmThreadContext_() :
#ifndef _WIN32
    m_pthread(0)
#ifdef linux
    , m_tid(0)
#endif
#else /* !_WIN32 */
    m_tid(0), m_handle(0)
#endif /* !_WIN32 */
    { }

public:
#ifndef _WIN32
  ZuInline pthread_t pthread() const { return m_pthread; }
#ifdef linux
  ZuInline pid_t tid_() const { return m_tid; }
#endif
#else /* !_WIN32 */
  ZuInline unsigned tid_() const { return m_tid; }
  ZuInline HANDLE handle() const { return m_handle; }
#endif /* !_WIN32 */

protected:
  void init();

#ifndef _WIN32
  pthread_t	m_pthread;
#ifdef linux
  pid_t		m_tid;
#endif
#else /* !_WIN32 */
  unsigned	m_tid;
  HANDLE	m_handle;
#endif /* !_WIN32 */
};

class ZmThreadContext;

template <> struct ZmCleanup<ZmThreadContext> {
  enum { Level = ZmCleanupLevel::Thread };
};

template <typename, bool> struct ZmSpecificCtor;

class ZmAPI ZmThreadContext : public ZmObject, public ZmThreadContext_ {
  friend struct ZmSpecificCtor<ZmThreadContext, true>;
#ifndef _WIN32
  friend ZmAPI void *ZmThread_start(void *);
#else
  friend ZmAPI unsigned __stdcall ZmThread_start(void *);
#endif
  friend class ZmThread;

  inline ZmThreadContext() : // only called via self() for unmanaged threads
      m_mgr(0), m_id(-1),
      m_detached(false),
      m_stackSize(0), m_priority(ZmThreadPriority::Normal),
      m_partition(0),
      m_result(0) { init(); }
  template <typename Fn>
  inline ZmThreadContext(
    ZmThreadMgr *mgr, int id, Fn &&fn, const ZmThreadParams &params) :
      m_mgr(mgr), m_id(id),
      m_fn(ZuFwd<Fn>(fn)),
      m_detached(params.detached()),
      m_stackSize(params.stackSize()), m_priority(params.priority()),
      m_partition(params.partition()), m_cpuset(params.cpuset()),
      m_result(0) { }

public:
  ~ZmThreadContext() { }

  void manage(ZmThreadMgr *mgr, int id);
  void prioritize(int priority);
  void bind(unsigned partition, const ZmBitmap &cpuset);

  ZuInline ZmThreadMgr *mgr() { return m_mgr; }
  ZuInline int id() const { return m_id; }

  static ZmThreadContext *self();

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

  bool main() const;

  void name(ZmThreadName &) const;

  ZuInline bool detached() const { return m_detached; }
  ZuInline const ZmFn<> &fn() const { return m_fn; }

  ZuInline unsigned stackSize() const { return m_stackSize; }
  ZuInline int priority() const { return m_priority; }

  ZuInline unsigned partition() const { return m_partition; }
  ZuInline const ZmBitmap &cpuset() const { return m_cpuset; }

  ZuInline void *result() const { return m_result; }

  inline void info(ZmThreadInfo &info) const {
    name(info.name);
    info.id = m_id;
    info.tid = tid();
    info.main = this->main();
    info.detached = m_detached;
    info.stackSize = m_stackSize;
    info.priority = m_priority;
    info.cpuset = m_cpuset;
  }

  template <typename S> inline void print(S &s) const {
    ZmThreadName name;
    this->name(name);
    s << name << " (" << ZuBoxed(tid()) << ") [" << m_cpuset << "]";
  }

private:
  static ZmThreadContext *self(ZmThreadContext *c);

  void prioritize();
  void bind();

  ZmThreadMgr	*m_mgr;
  int		m_id;

  ZmFn<>	m_fn;
  bool		m_detached;

  unsigned	m_stackSize;
  int		m_priority;

  unsigned	m_partition;
  ZmBitmap	m_cpuset;

  void		*m_result;
};

template <> struct ZuPrint<ZmThreadContext> : public ZuPrintFn { };
template <> struct ZuPrint<ZmThreadContext *> : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const ZmThreadContext *v) {
    if (!v)
      s << "null";
    else
      s << *v;
  }
};

#define ZmSelf() (ZmThreadContext::self())

class ZmAPI ZmThread {
public:
  typedef ZmThreadContext Context;
  typedef ZmThreadID ID;

  inline ZmThread() { }
  template <typename Fn>
  inline ZmThread(ZmThreadMgr *mgr, int id,
      Fn &&fn, ZmThreadParams params = ZmThreadParams()) {
    run(mgr, id, ZuFwd<Fn>(fn), params); // sets m_context
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

  int run(ZmThreadMgr *mgr, int id,
      ZmFn<> fn, ZmThreadParams params = ZmThreadParams());

  int join(void **status = 0);

  inline ZmRef<Context> context() { return m_context; }

  ZuInline operator ID() const { return tid(); }

  ZuInline ID tid() const {
    if (!m_context) return 0;
    return m_context->tid();
  }

  ZuInline bool operator ==(const ZmThread &t) {
    return tid() == t.tid();
  }
  ZuInline int cmp(const ZmThread &t) const {
    return ZuCmp<ID>::cmp(tid(), t.tid());
  }
  ZuInline uint32_t hash() { return ZuHash<ID>::hash(tid()); }

  ZuInline bool operator !() const { return !m_context; }
  ZuOpBool

  typedef ZmFn<const ZmThreadInfo &> InfoFn;
  static void info(InfoFn fn);

  template <class S> struct CSV_ {
    CSV_(S &stream) : m_stream(stream) { 
      m_stream << "name,id,tid,main,detached,stackSize,priority,cpuset\n";
    }
    void print(const ZmThreadInfo &info) {
      m_stream <<
	info.name << ',' <<
	info.id << ',' <<
	info.tid << ',' <<
	info.main << ',' <<
	info.detached << ',' <<
	info.stackSize << ',' <<
	info.priority << ',' <<
	info.cpuset << '\n';
    }
    S &stream() { return m_stream; }

  private:
    S	&m_stream;
  };
  struct CSV {
    template <typename S> void print(S &s) const {
      CSV_<S> csv(s);
      ZmThread::info(InfoFn::Member<&CSV_<S>::print>::fn(&csv));
    }
  };
  inline static CSV csv() { return CSV(); }
  static void dump();

private:
  ZmRef<Context>	m_context;
};

template <> struct ZuTraits<ZmThread> : public ZuGenericTraits<ZmThread> {
  enum { IsComparable = 1, IsHashable = 1 };
};

template <> struct ZuPrint<ZmThread::CSV> : public ZuPrintFn { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <ZmSpecific.hpp>

ZuInline ZmThreadContext *ZmThreadContext::self() {
  return ZmSpecific<ZmThreadContext>::instance();
}
ZuInline ZmThreadContext *ZmThreadContext::self(ZmThreadContext *c) {
  return ZmSpecific<ZmThreadContext>::instance(c);
}

#endif /* ZmThread_HPP */
