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

// shard, handle
//
// ZmHandle is a special union of a ZmRef of a sharded object, and
// a pointer to a shard; in this way it can be used to create a new
// object within a shard, or to hold a reference to an existing object

#ifndef ZmShard_HPP
#define ZmShard_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmScheduler.hpp>

class ZmShard {
public:
  ZmShard(ZmScheduler *sched, unsigned tid) :
      m_sched(sched), m_tid(tid) { }

  ZmScheduler *sched() const { return m_sched; }
  unsigned tid() const { return m_tid; }

  template <typename ...Args> void run(Args &&... args) const
    { m_sched->run(m_tid, ZuFwd<Args>(args)...); }
  template <typename ...Args> void invoke(Args &&... args) const
    { m_sched->invoke(m_tid, ZuFwd<Args>(args)...); }

private:
  ZmScheduler	*m_sched;
  unsigned	m_tid;
};

template <class Shard_> class ZmSharded {
public:
  using Shard = Shard_;

  ZmSharded(Shard *shard) : m_shard(shard) { }

  Shard *shard() const { return m_shard; }

private:
  Shard		*m_shard;
};

template <typename T> class ZmHandle {
public:
  using Shard = typename T::Shard;

  ZmHandle() : m_ptr(0) { }
  ZmHandle(Shard *shard) :
    m_ptr((uintptr_t)(void *)shard | (uintptr_t)1) { }
  ZmHandle(T *o) { new (&m_ptr) ZmRef<T>(o); }
  ZmHandle(ZmRef<T> o) { new (&m_ptr) ZmRef<T>(ZuMv(o)); }
  ~ZmHandle() {
    if (m_ptr && !(m_ptr & 1)) {
      ZmRef<T> *ZuMayAlias(ptr) = reinterpret_cast<ZmRef<T> *>(&m_ptr);
      ptr->~ZmRef<T>();
    }
  }

  Shard *shard() const {
    if (!m_ptr) return 0;
    if (m_ptr & 1) return (Shard *)(void *)(m_ptr & ~(uintptr_t)1);
    return ((T *)(void *)m_ptr)->shard();
  }
  int shardID() const {
    Shard *shard = this->shard();
    return shard ? (int)shard->id() : -1;
  }
  T *object() const {
    if (!m_ptr || (m_ptr & 1)) return 0;
    return (T *)(void *)m_ptr;
  }

  ZmHandle(const ZmHandle &h) : m_ptr(h.m_ptr) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->ref();
  }
  ZmHandle(ZmHandle &&h) {
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) h.m_ptr = 0;
  }
  ZmHandle &operator =(const ZmHandle &h) {
    if (ZuUnlikely(this == &h)) return *this;
    T *o = object();
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->ref();
    if (o) o->deref();
    return *this;
  }
  ZmHandle &operator =(ZmHandle &&h) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) h.m_ptr = 0;
    return *this;
  }

  ZmHandle &operator =(T *o) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    new (&m_ptr) ZmRef<T>(o);
    return *this;
  }
  ZmHandle &operator =(ZmRef<T> o) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    new (&m_ptr) ZmRef<T>(ZuMv(o));
    return *this;
  }

  bool operator !() const { return !m_ptr || (m_ptr & 1); }
  ZuOpBool

private:
  void shardObject(Shard *&shard, T *&o) const {
    if (ZuUnlikely(!m_ptr)) {
      shard = 0;
      o = 0;
    } else if (ZuUnlikely(m_ptr & 1)) {
      shard = (Shard *)(void *)(m_ptr & ~(uintptr_t)1);
      o = 0;
    } else {
      o = (T *)(void *)m_ptr;
      shard = o->shard();
    }
  }

public:
  template <typename L>
  ZuNotMutable<L> invoke(L l) const {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    shard->invoke([l = ZuMv(l), shard, o]() { l(shard, o); });
  }
  template <typename L>
  ZuIsMutable<L> invoke(L l) const {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    shard->invoke([l = ZuMv(l), shard, o]() mutable { l(shard, o); });
  }
  template <typename L>
  void invokeMv(L l) {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    m_ptr = 0;
    ZmRef<T> *ZuMayAlias(ptr) = reinterpret_cast<ZmRef<T> *>(&o);
    shard->invoke([l = ZuMv(l), shard, o = ZuMv(*ptr)]() mutable {
      l(shard, ZuMv(o));
    });
  }

private:
  uintptr_t	m_ptr;
};

#endif /* ZmShard_HPP */
