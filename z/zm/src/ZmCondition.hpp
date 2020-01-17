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

// condition variable

#ifndef ZmCondition_HPP
#define ZmCondition_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmTime.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmPLock.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4101 4251)
#endif

template <typename Lock> class ZmCondition;

#ifndef _WIN32
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define ZmCondition_Native 0
#else
#define ZmCondition_Native 1
typedef pthread_cond_t ZmCondition_;
#define ZmCondition_init(c) pthread_cond_init(&c, 0)
#define ZmCondition_final(c) pthread_cond_destroy(&c)
#define ZmCondition_wait(c, l) pthread_cond_wait(&c, &l)
inline static int ZmCondition_timedwait_(
    ZmCondition_ &c, ZmLock_ &l, const ZmTime &t) {
  timespec t_;
  t.convertToNative(t_);
  return pthread_cond_timedwait(&c, &l, &t_) ? -1 : 0;
}
#define ZmCondition_timedwait(c, l, t) ZmCondition_timedwait_(c, l, t)
#define ZmCondition_signal(c) pthread_cond_signal(&c)
#define ZmCondition_broadcast(c) pthread_cond_broadcast(&c)
#endif
#else
#define ZmCondition_Native 0
#endif

#if ZmCondition_Native == 0
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmSpecific.hpp>
#include <zlib/ZmSemaphore.hpp>

class ZmCondition_;

static void ZmCondition_wait_(ZmCondition_ &, ZmPLock_ &);
static int ZmCondition_timedwait_(ZmCondition_ &, ZmPLock_ &, const ZmTime &);
static void ZmCondition_signal_(ZmCondition_ &);
static void ZmCondition_broadcast_(ZmCondition_ &);

class ZmCondition_Thread : public ZmObject {
friend struct ZmSpecificCtor<ZmCondition_Thread>;
template <typename> friend class ZmCondition;
friend void ZmCondition_wait_(ZmCondition_ &, ZmPLock_ &);
friend int ZmCondition_timedwait_(ZmCondition_ &, ZmPLock_ &, const ZmTime &);
friend void ZmCondition_signal_(ZmCondition_ &);
friend void ZmCondition_broadcast_(ZmCondition_ &);
  ZmCondition_Thread() : m_waiting(false), m_next(0), m_prev(0) { }
  ZmSemaphore		m_sem;
  bool			m_waiting;
  ZmCondition_Thread	*m_next;
  ZmCondition_Thread	*m_prev;
};

class ZmCondition_ {
template <typename> friend class ZmCondition;
friend void ZmCondition_wait_(ZmCondition_ &, ZmPLock_ &);
friend int ZmCondition_timedwait_(ZmCondition_ &, ZmPLock_ &, const ZmTime &);
friend void ZmCondition_signal_(ZmCondition_ &);
friend void ZmCondition_broadcast_(ZmCondition_ &);
  ZmCondition_() : m_head(0), m_tail(0) { }
  ZmPLock		m_lock;
  ZmCondition_Thread	*m_head;
  ZmCondition_Thread	*m_tail;
};

#define ZmCondition_init(c) (void)0
#define ZmCondition_final(c) (void)0
inline static void ZmCondition_wait_(ZmCondition_ &c, ZmPLock_ &l) {
  ZmCondition_Thread *s = ZmSpecific<ZmCondition_Thread>::instance();
  s->m_next = 0;
  s->m_waiting = true;
  {
    ZmGuard<ZmPLock> guard(c.m_lock);
    s->m_prev = c.m_tail;
    if (c.m_head)
      c.m_tail->m_next = s;
    else
      c.m_head = s;
    c.m_tail = s;
  }
  ZmPLock_unlock(l);
  s->m_sem.wait();
  ZmPLock_lock(l);
}
#define ZmCondition_wait(c, l) ZmCondition_wait_(c, l) 
inline static int ZmCondition_timedwait_(ZmCondition_ &c, ZmPLock_ &l,
					 const ZmTime &t) {
  ZmCondition_Thread *s = ZmSpecific<ZmCondition_Thread>::instance();
  s->m_next = 0;
  s->m_waiting = true;
  {
    ZmGuard<ZmPLock> guard(c.m_lock);
    s->m_prev = c.m_tail;
    if (c.m_head)
      c.m_tail->m_next = s;
    else
      c.m_head = s;
    c.m_tail = s;
  }
  ZmPLock_unlock(l);
  if (s->m_sem.timedwait(t)) {
    ZmGuard<ZmPLock> guard(c.m_lock);
    if (s->m_waiting) {
      if (s->m_prev)
	s->m_prev->m_next = s->m_next;
      else
	c.m_head = s->m_next;
      if (s->m_next)
	s->m_next->m_prev = s->m_prev;
      else
	c.m_tail = s->m_prev;
      s->m_waiting = false;
      guard.unlock();
      ZmPLock_lock(l);
      return -1;
    }
    guard.unlock();
    s->m_sem.wait();
  }
  ZmPLock_lock(l);
  return 0;
}
#define ZmCondition_timedwait(c, l, t) ZmCondition_timedwait_(c, l, t)
inline static void ZmCondition_signal_(ZmCondition_ &c)
{
  ZmGuard<ZmPLock> guard(c.m_lock);
  if (ZmCondition_Thread *thread = c.m_head) {
    if (!(c.m_head = thread->m_next)) {
      c.m_tail = 0;
    } else {
      c.m_head->m_prev = 0;
    }
    thread->m_waiting = false;
    thread->m_sem.post();
  }
}
#define ZmCondition_signal(c) ZmCondition_signal_(c)
inline static void ZmCondition_broadcast_(ZmCondition_ &c)
{
  ZmGuard<ZmPLock> guard(c.m_lock);
  while (ZmCondition_Thread *thread = c.m_head) {
    if (!(c.m_head = thread->m_next)) {
      c.m_tail = 0;
    } else {
      c.m_head->m_prev = 0;
    }
    thread->m_waiting = false;
    thread->m_sem.post();
  }
}
#define ZmCondition_broadcast(c) ZmCondition_broadcast_(c)
#endif

template <class Lock> class ZmCondition {
  ZmCondition();
  ZmCondition(const ZmCondition &);
  ZmCondition &operator =(const ZmCondition &);		// prevent mis-use

  typedef typename Lock::Wait Wait;

public:
  ZuInline ZmCondition(Lock &lock) : m_lock(lock) { ZmCondition_init(m_cond); }
  ZuInline ~ZmCondition() { ZmCondition_final(m_cond); }

  void wait() {
    Wait wait(m_lock.wait());

    ZmCondition_wait(m_cond, m_lock.m_lock);
  }
  int timedWait(ZmTime timeout) {
    Wait wait(m_lock.wait());

    return ZmCondition_timedwait(m_cond, m_lock.m_lock, timeout);
  }
  void signal() { ZmCondition_signal(m_cond); }
  void broadcast() { ZmCondition_broadcast(m_cond); }

  Lock &lock() { return(m_lock); }

private:
  Lock		&m_lock;
  ZmCondition_	m_cond;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZmCondition_HPP */
