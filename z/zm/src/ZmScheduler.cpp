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

#include <ZuBox.hpp>

#include <ZmScheduler.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

ZmScheduler::ZmScheduler(ZmSchedulerParams params) :
  m_id(params.id()),
  m_nThreads(params.nThreads()),
  m_stackSize(params.stackSize()),
  m_priority(params.priority()),
  m_partition(params.partition()),
  m_affinity(params.affinity()),
  m_isolation(params.isolation()),
  m_quantum(params.quantum()),
  m_stateCond(m_stateLock),
  m_state(Stopped),
  m_drained(0),
  m_nWorkers(0),
  m_runThreads(0)
{
  m_threads = new Thread[m_nThreads];
  m_workers = new Thread *[m_nThreads];
  for (unsigned i = 0; i < m_nThreads; i++) {
    unsigned tid = i + 1;
    Ring &ring = m_threads[i].ring;
    ring.init(ZmRingParams(params.queueSize()).
	ll(params.ll()).
	spin(params.spin()).
	timeout(params.timeout()).
	cpuset(tid < m_affinity.count() ? m_affinity[tid] : ZmBitmap()));
    int r;
    if ((r = ring.open(Ring::Read | Ring::Write)) != Ring::OK)
      throw ZmRingError(r);
    // if ((r = ring.attach()) != Ring::OK) throw ZmRingError(r);
    if (!(m_isolation && tid)) m_workers[m_nWorkers++] = &m_threads[i];
  }
}

ZmScheduler::~ZmScheduler()
{
  stop();
  for (unsigned i = 0; i < m_nThreads; i++) {
    Ring &ring = m_threads[i].ring;
    // ring.detach();
    ring.close();
  }
  delete [] m_workers;
  delete [] m_threads;
}

void ZmScheduler::runThreads()
{
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    if (m_state != Running && m_state != Starting) return;
  }

  {
    ZmGuard<ZmSpinLock> spawnGuard(m_spawnLock);
    ZmThreadParams params;
    params.stackSize(m_stackSize).priority(m_priority).partition(m_partition);
    while (m_runThreads < m_nThreads) {
      ZmBitmap cpuset;
      unsigned tid = ++m_runThreads;
      if (tid < m_affinity.count()) cpuset = m_affinity[tid];
      m_threads[tid - 1].thread = ZmThread(this, tid,
	  ZmFn<>::Member<&ZmScheduler::work>::fn(this),
	  params.cpuset(cpuset));
    }
  }
}

void ZmScheduler::start()
{
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    switch (m_state) {
      case Draining:
	do { m_stateCond.wait(); } while (m_state != Drained);
      case Drained:
	m_state = Running;
	m_stateCond.broadcast();
	return;
      case Starting:
	do { m_stateCond.wait(); } while (m_state != Running);
      case Running:
	return;
      case Stopping:
	do { m_stateCond.wait(); } while (m_state != Stopped);
      default:
	break;
    }

    m_state = Starting;
    m_stateCond.broadcast();
  }

  m_thread = ZmThread(this, 0,
      ZmFn<>::Member<&ZmScheduler::timer>::fn(this),
      ZmThreadParams().priority(m_priority).
	cpuset(!m_affinity.count() ? ZmBitmap() : m_affinity[0]));

  runThreads();

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    m_state = Running;
    m_stateCond.broadcast();
  }
}

void ZmScheduler::drain()
{
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    switch (m_state) {
      case Draining:
	do { m_stateCond.wait(); } while (m_state != Drained);
      case Drained:
	return;
      case Stopping:
	do { m_stateCond.wait(); } while (m_state != Stopped);
      case Stopped:
	return;
      case Starting:
	do { m_stateCond.wait(); } while (m_state != Running);
      default:
	break;
    }

    m_drained = 0;
    m_state = Draining;
    m_stateCond.broadcast();
  }

  for (unsigned i = 0; i < m_nThreads; i++)
    while (ZuUnlikely(!run__(
	    &m_threads[i], ZmFn<>::Member<&ZmScheduler::drained>::fn(this))))
      ZmPlatform::yield();

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    while (m_state != Drained) m_stateCond.wait();
  }
}

void ZmScheduler::drained()
{
  ZmGuard<ZmLock> stateGuard(m_stateLock);

  if (m_state != Draining) return;

  if (++m_drained < m_nThreads) return;

  m_drained = 0;
  m_state = Drained;
  m_stateCond.broadcast();
}

void ZmScheduler::stop()
{
  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    switch (m_state) {
      case Stopping:
	do { m_stateCond.wait(); } while (m_state != Stopped);
      case Stopped:
	return;
      case Draining:
	do { m_stateCond.wait(); } while (m_state != Drained);
      case Drained:
	break;
      case Starting:
	do { m_stateCond.wait(); } while (m_state != Running);
      default:
	break;
    }
    m_state = Stopping;
    m_stateCond.broadcast();
  }

  {
    ZmGuard<ZmSpinLock> spawnGuard(m_spawnLock);
    for (unsigned i = 0; i < m_nThreads; i++) {
      m_threads[i].ring.eof(true);
    }
    for (unsigned i = 0; i < m_nThreads; i++) {
      ZmThread &thread = m_threads[i].thread;
      if (!!thread) {
	thread.join();
	thread = ZmThread();
      }
    }
    for (unsigned i = 0; i < m_nThreads; i++)
      m_threads[i].ring.eof(false);
  }

  m_pending.post();

  m_stopped.reset();

  m_thread.join();
  m_thread = 0;

  {
    ZmGuard<ZmLock> stateGuard(m_stateLock);

    m_state = Stopped;
    m_stateCond.broadcast();
  }
}

void ZmScheduler::reset()
{
  if (this->state() != Stopped) return;

  {
    ZmGuard<ZmSpinLock> spawnGuard(m_spawnLock);
    for (unsigned i = 0; i < m_nThreads; i++)
      if (!m_threads[i].thread) m_threads[i].ring.reset();
  }
}

void ZmScheduler::timer()
{
  for (;;) {
    {
      ZmGuard<ZmLock> stateGuard(m_stateLock);

      if (m_state == Stopping || m_state == Stopped) return;
    }

    {
      ZmTime minimum;

      {
	ZmGuard<ZmPLock> scheduleGuard(m_scheduleLock);
	Timer timer(m_schedule.minimum());

	if (timer) minimum = timer->key();
      }

      if (minimum)
	m_pending.timedwait(minimum);
      else
	m_pending.wait();

      {
	ZmGuard<ZmLock> stateGuard(m_stateLock);

	while (m_state == Draining || m_state == Drained)
	  m_stateCond.wait();
	if (m_state == Stopping || m_state == Stopped) return;
      }

      {
	ZmTime now(ZmTime::Now);
	now += m_quantum;
	ZmGuard<ZmPLock> scheduleGuard(m_scheduleLock);
	ScheduleTree::Iterator i(m_schedule, now, ScheduleTree::LessEqual);
	Timer timer;

	while (timer = i.iterate()) {
	  i.del();
	  {
	    Timer *ptr = (Timer *)timer->val().ptr;
	    if (ZuLikely(ptr)) *ptr = 0;
	  }
	  if (ZuUnlikely(!add_(timer->val().fn))) {
	    scheduleGuard.unlock();
	    m_schedule.add(timer);
	    ZmPlatform::sleep(m_quantum);
	    break;
	  }
	}
      }
    }
  }
}

void ZmScheduler::add(ZmFn<> fn)
{
  if (ZuUnlikely(!fn)) return;
  if (ZuUnlikely(!add_(fn)))
    run(0, ZuMv(fn), ZmTimeNow(), Update, 0);
}

bool ZmScheduler::add_(ZmFn<> &fn)
{
  if (ZuUnlikely(!m_nWorkers)) return 0;
  unsigned first = m_next++;
  unsigned next = first;
  do {
    Thread *thread = m_workers[next % m_nWorkers];
    ZmGuard<ZmSpinLock> guard(thread->lock); // ensure serialized ring push()
    void *ptr;
    if (ZuLikely(ptr = thread->ring.push())) {
      new (ptr) ZmFn<>(ZuMv(fn));
      thread->ring.push2(ptr);
      return 1;
    }
  } while (((next = m_next++) - first) < m_nWorkers);
  return 0;
}

void ZmScheduler::run(
    unsigned tid, ZmFn<> fn, ZmTime timeout, int mode, Timer *ptr)
{
  ZmAssert(tid <= m_nThreads);

  if (ZuUnlikely(!fn)) return;

  bool kick = true;

  {
    ZmGuard<ZmPLock> scheduleGuard(m_scheduleLock);

    Timer timer;

    if (ZuLikely(ptr)) {
      if (timer = ZuMv(*ptr)) {
	switch (mode) {
	  case Advance:
	    if (ZuUnlikely(timer->key() <= timeout))
	      { *ptr = ZuMv(timer); return; }
	    break;
	  case Defer:
	    if (ZuUnlikely(timer->key() >= timeout))
	      { *ptr = ZuMv(timer); return; }
	    break;
	}
	m_schedule.del(timer);
	timer = *ptr = 0;
      }
    }

    if (ZuUnlikely(timeout <= ZmTimeNow())) {
      if (ZuLikely(tid)) {
	if (ZuLikely(run__(&m_threads[tid - 1], fn))) return;
      } else {
	if (ZuLikely(add_(fn))) return;
      }
    }

    if (ZuLikely(timer = m_schedule.minimum())) kick = timeout < timer->key();

    timer = m_schedule.add(timeout, Timer_{tid, ZuMv(fn), (void *)ptr});

    if (ZuLikely(ptr)) *ptr = ZuMv(timer);
  }
  if (kick) m_pending.post();
}

void ZmScheduler::del(Timer *ptr)
{
  if (ZuUnlikely(!ptr)) return;
  ZmGuard<ZmPLock> scheduleGuard(m_scheduleLock);
  if (ZuLikely(*ptr)) { m_schedule.del(*ptr); *ptr = 0; }
}

void ZmScheduler::work()
{
  unsigned id = ZmThreadContext::self()->id();
  Thread *thread = &m_threads[id - 1];

  thread->tid = ZmPlatform::getTID();

  threadInit(id);

  for (;;) {
    if (ZmFn<> *ptr = thread->ring.shift()) {
      ZmFn<> fn = ZuMv(*ptr);
      ptr->~ZmFn<>();
      thread->ring.shift2();
      try { fn(); } catch (...) { }
    } else {
      if (thread->ring.readStatus() == Ring::EndOfFile) break;
    }
  }

  threadFinal(id);

  --m_runThreads;

  m_stopped.post();
}

void ZmScheduler::threadName(ZmThreadName &s, unsigned tid)
{
  s << m_id << ':' << ZuBoxed(tid);
}

void ZmScheduler::threadInit(unsigned) { }
void ZmScheduler::threadFinal(unsigned) { }

#ifdef _MSC_VER
#pragma warning(pop)
#endif
