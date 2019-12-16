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

#include <stdio.h>

#ifndef _WIN32
#include <sched.h>
#ifdef linux
#include <syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#endif

#include <iostream>

#include <zlib/ZmSingleton.hpp>
#include <zlib/ZmSpecific.hpp>
#include <zlib/ZmTopology.hpp>

#include <zlib/ZmThread.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

// hwloc topology

ZmTopology *ZmTopology::instance()
{
  return ZmSingleton<ZmTopology>::instance();
}

ZmTopology::ZmTopology() : m_errorFn(0)
{
  ZmPLock_init(m_lock);
  hwloc_topology_init(&m_hwloc);
  hwloc_topology_load(m_hwloc);
}

ZmTopology::~ZmTopology()
{
  ZmPLock_lock(m_lock);
  hwloc_topology_destroy(m_hwloc);
  ZmPLock_unlock(m_lock);
  ZmPLock_final(m_lock);
}

void ZmTopology::errorFn(ErrorFn fn)
{
  instance()->m_errorFn = fn;
}

void ZmTopology::error(int errNo)
{
  if (ErrorFn fn = instance()->m_errorFn) (*fn)(errNo);
}

#ifdef _WIN32
struct HandleCloser : public ZmObject {
  ~HandleCloser() { if (h) CloseHandle(h); }
  HANDLE h = 0;
};
#endif

struct ZmThread_Main {
  ZmThread_Main() { is(true); }
  bool is(bool b = false) {
    thread_local bool _ = b;
    return _;
  }
};
static ZmThread_Main ZmThread_main;

void ZmThreadContext_::init()
{
  m_main = ZmThread_main.is();
#ifndef _WIN32
  m_pthread = pthread_self();
#ifdef linux
  m_tid = syscall(SYS_gettid);
#endif
  pthread_getcpuclockid(m_pthread, &m_cid);
  m_rtLast.now();
#else /* !_WIN32 */
  m_tid = GetCurrentThreadId();
  DuplicateHandle(
    GetCurrentProcess(),
    GetCurrentThread(),
    GetCurrentProcess(),
    &m_handle,
    0,
    FALSE,
    DUPLICATE_SAME_ACCESS);
  ZmSpecific<HandleCloser>::instance()->h = m_handle;
  m_rtLast = __rdtsc();
#endif /* !_WIN32 */
}

void ZmThreadContext::init()
{
  ZmThreadContext_::init();
  {
#ifndef _WIN32
    size_t n;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &n);
    m_stackSize = n;
#else /* !_WIN32 */
    PULONG low, high;
    void GetCurrentThreadStackLimits(&low, &high);
    m_stackSize = (high - low) * sizeof(ULONG);
#endif /* !_WIN32 */
  }
  if (!m_name) {
    if (main())
      m_name = "main";
    else
      m_name = ZuBoxed(tid());
  }
}

void ZmThreadContext::telemetry(ZmThreadTelemetry &data) const {
  data.name = m_name;
  data.tid = tid();
  data.stackSize = m_stackSize;
  data.cpuset = m_cpuset;
  data.cpuUsage = cpuUsage();
  data.sysPriority = sysPriority();
  data.index = m_index;
  data.priority = m_priority;
  data.partition = m_partition;
  data.main = this->main();
  data.detached = m_detached;
}

void ZmThreadContext::prioritize(int priority)
{
  m_priority = priority;
  prioritize();
}

#ifndef _WIN32
struct ZmThread_Priorities {
  ZmThread_Priorities() {
    fifo = sched_get_priority_max(SCHED_FIFO);
    rr = sched_get_priority_max(SCHED_RR);
  }
  int	fifo, rr;
};
void ZmThreadContext::prioritize()
{
  static ZmThread_Priorities p;
  if (m_priority == ZmThreadPriority::RealTime) {
    int policy = !!m_cpuset ? SCHED_FIFO : SCHED_RR;
    sched_param s;
    s.sched_priority = !!m_cpuset ? p.fifo : p.rr;
    int r = pthread_setschedparam(m_pthread, policy, &s);
    if (r) {
      std::cerr << 
	"pthread_setschedparam() failed: " << ZuBoxed(r) << ' ' <<
	strerror(r) << '\n' << std::flush;
    }
  } else if (m_priority < 0)
    m_priority = ZmThreadPriority::Normal;
}
#else /* !_WIN32 */
void ZmThreadContext::prioritize()
{
  static int p[] = {
    THREAD_PRIORITY_TIME_CRITICAL,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_BELOW_NORMAL
  };
  if (m_priority >= 0)
    SetThreadPriority(
	m_handle, p[m_priority > 3 ? 3 : m_priority]);
}
#endif /* !_WIN32 */

void ZmThreadContext::bind(unsigned partition, const ZmBitmap &cpuset)
{
  m_partition = partition;
  if (!cpuset) return;
  m_cpuset = cpuset;
  bind();
}

void ZmThreadContext::bind()
{
  if (!m_cpuset) return;
  if (hwloc_set_cpubind(
	ZmTopology::hwloc(), m_cpuset, HWLOC_CPUBIND_THREAD) < 0) {
    int errno_ = errno;
    std::cerr << 
      "bind " << ZuBoxed(tid()) << " to cpuset " << m_cpuset <<
      " failed: " << strerror(errno_) << '\n' << std::flush;
    ZmTopology::error(errno_);
  }
  hwloc_get_cpubind(
      ZmTopology::hwloc(), m_cpuset, HWLOC_CPUBIND_THREAD);
}

#ifndef _WIN32
void *ZmThread_start(void *c_)
#else
ZmAPI unsigned __stdcall ZmThread_start(void *c_)
#endif
{
  ZmThreadContext *c = ZmThreadContext::self((ZmThreadContext *)c_);
  c->deref();
  c->init();
#ifndef _WIN32
#ifdef linux
  pthread_setname_np(c->m_pthread, c->name());
#endif
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
  c->bind();
  ZmFn<> fn = c->m_fn;
  c->m_fn = ZmFn<>();
  return c->m_result = (void *)(fn)();
#else
  c->bind();
  if (c->m_detached) {
    CloseHandle(c->m_handle);
    c->m_handle = 0;
  }
  ZmFn<> fn = c->m_fn;
  c->m_fn = ZmFn<>();
  c->m_result = (void *)(fn)();
  _endthreadex(0);
  return 0;
#endif
}

int ZmThread::run(int id, ZmFn<> fn, ZmThreadParams params)
{
  m_context = new ZmThreadContext(id, fn, params);
  ZmREF(m_context);
#ifndef _WIN32
  {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (params.detached())
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (params.stackSize())
      pthread_attr_setstacksize(&attr, params.stackSize());
    // pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    if ((*params.createFn())(
	  &m_context->m_pthread, &attr,
	  &ZmThread_start, (void *)m_context.ptr()) < 0) {
      pthread_attr_destroy(&attr);
      ZmDEREF(m_context);
      m_context = 0;
      return -1;
    }
    pthread_attr_destroy(&attr);
  }
  m_context->prioritize();
#else
  HANDLE h = (HANDLE)_beginthreadex(
      0, params.stackSize(), &ZmThread_start,
      (void *)m_context.ptr(), CREATE_SUSPENDED, &m_context->m_tid);
  if (!h) {
    ZmDEREF(m_context);
    m_context = 0;
    return -1;
  }
  m_context->m_handle = h;
  m_context->prioritize();
  ResumeThread(h);
#endif
  return 0;
}

int ZmThread::join(void **status)
{
  if (!m_context || m_context->m_detached) return -1;
#ifndef _WIN32
  int r = pthread_join(m_context->m_pthread, status);
  m_context = 0;
  return r;
#else
  WaitForSingleObject(m_context->m_handle, INFINITE);
  CloseHandle(m_context->m_handle);
  if (status) *status = m_context->m_result;
  m_context = 0;
  return 0;
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
