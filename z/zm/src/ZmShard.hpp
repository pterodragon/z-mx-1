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

// shard

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
  ZuInline ZmShard(ZmScheduler *sched, unsigned tid) :
      m_sched(sched), m_tid(tid) { }

  ZuInline ZmScheduler *sched() const { return m_sched; }
  ZuInline unsigned tid() const { return m_tid; }

  template <typename ...Args> ZuInline void run(Args &&... args) const
    { m_sched->run(m_tid, ZuFwd<Args>(args)...); }
  template <typename ...Args> ZuInline void invoke(Args &&... args) const
    { m_sched->invoke(m_tid, ZuFwd<Args>(args)...); }

private:
  ZmScheduler	*m_sched;
  unsigned	m_tid;
};

template <class Shard_> class ZmSharded {
public:
  typedef Shard_ Shard;

  ZuInline ZmSharded(Shard *shard) : m_shard(shard) { }

  ZuInline Shard *shard() const { return m_shard; }

private:
  Shard		*m_shard;
};

template <typename T> class ZmHandle {
public:
  typedef typename T::Shard Shard;

  ZuInline ZmHandle() : m_ptr(0) { }
  ZuInline ZmHandle(Shard *shard) :
    m_ptr((uintptr_t)(void *)shard | (uintptr_t)1) { }
  ZuInline ZmHandle(T *o) { new (&m_ptr) ZmRef<T>(o); }
  ZuInline ZmHandle(ZmRef<T> o) { new (&m_ptr) ZmRef<T>(ZuMv(o)); }
  ZuInline ~ZmHandle() {
    if (m_ptr && !(m_ptr & 1)) {
      ZmRef<T> *ZuMayAlias(ptr) = (ZmRef<T> *)&m_ptr;
      ptr->~ZmRef<T>();
    }
  }

  ZuInline Shard *shard() const {
    if (!m_ptr) return 0;
    if (m_ptr & 1) return (Shard *)(void *)(m_ptr & ~(uintptr_t)1);
    return ((T *)(void *)m_ptr)->shard();
  }
  ZuInline int shardID() const {
    Shard *shard = this->shard();
    return shard ? (int)shard->id() : -1;
  }
  ZuInline T *object() const {
    if (!m_ptr || (m_ptr & 1)) return 0;
    return (T *)(void *)m_ptr;
  }

  ZuInline ZmHandle(const ZmHandle &h) : m_ptr(h.m_ptr) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->ref();
  }
  ZuInline ZmHandle(ZmHandle &&h) {
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) h.m_ptr = 0;
  }
  ZuInline ZmHandle &operator =(const ZmHandle &h) {
    if (ZuUnlikely(this == &h)) return *this;
    T *o = object();
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->ref();
    if (o) o->deref();
    return *this;
  }
  ZuInline ZmHandle &operator =(ZmHandle &&h) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    m_ptr = h.m_ptr;
    if (m_ptr && !(m_ptr & 1)) h.m_ptr = 0;
    return *this;
  }

  ZuInline ZmHandle &operator =(T *o) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    new (&m_ptr) ZmRef<T>(o);
    return *this;
  }
  ZuInline ZmHandle &operator =(ZmRef<T> o) {
    if (m_ptr && !(m_ptr & 1)) ((T *)(void *)m_ptr)->deref();
    new (&m_ptr) ZmRef<T>(ZuMv(o));
    return *this;
  }

  ZuInline bool operator !() const { return !m_ptr || (m_ptr & 1); }
  ZuOpBool

private:
  ZuInline void shardObject(Shard *&shard, T *&o) const {
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
  ZuInline typename ZuNotMutable<L>::T invoke(L l) const {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    shard->invoke([l = ZuMv(l), shard, o]() { l(shard, o); });
  }
  template <typename L>
  ZuInline typename ZuIsMutable<L>::T invoke(L l) const {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    shard->invoke([l = ZuMv(l), shard, o]() mutable { l(shard, o); });
  }
  template <typename L>
  ZuInline void invokeMv(L l) {
    Shard *shard;
    T *o;
    shardObject(shard, o);
    m_ptr = 0;
    ZmRef<T> *ZuMayAlias(ptr) = (ZmRef<T> *)&o;
    shard->invoke([l = ZuMv(l), shard, o = ZuMv(*ptr)]() mutable {
      l(shard, ZuMv(o));
    });
  }

private:
  uintptr_t	m_ptr;
};

#endif /* ZmShard_HPP */
