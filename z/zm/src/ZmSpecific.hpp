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

// TLS with
// * deterministic destruction sequencing
// * iteration over all instances

// ZmSpecific<T>::instance() returns T * pointer
//
// ZmSpecific<T>::all(ZmFn<T *> fn) calls fn for all instances of T
//
// ZmCleanup<T>::Level determines order of destruction (per ZmCleanupLevel)
//
// ZmSpecific<T, false>::instance() can return null since T will not be
// constructed on-demand - use ZmSpecific<T, 0>::instance(new T(...))
//
// T must be ZuObject derived
//
// thread_local T v; can be replaced with:
// auto &v = *ZmSpecific<T>::instance();
// auto &v = ZmTLS([]() { return new T(); });
//
// thread_local T v(args); can be replaced with:
// auto &v = ZmTLS([]() { return new T(args...); });

#ifndef ZmSpecific_HPP
#define ZmSpecific_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#ifndef _WIN32
#include <alloca.h>
#endif

#include <stdlib.h>

#include <ZuCmp.hpp>
#include <ZuConversion.hpp>
#include <ZuCan.hpp>
#include <ZuPair.hpp>
#include <ZuIfT.hpp>

#include <ZmRef.hpp>
#include <ZmObject.hpp>
#include <ZmFn_.hpp>
#include <ZmCleanup.hpp>
#include <ZmGlobal.hpp>
#include <ZmStack_.hpp>

class ZmSpecific_;
struct ZmSpecific_Object;

extern "C" {
  ZmExtern void ZmSpecific_lock();
  ZmExtern void ZmSpecific_unlock();
#ifdef _WIN32
  ZmExtern void ZmSpecific_cleanup();
  ZmExtern void ZmSpecific_cleanup_add(ZmSpecific_Object *);
  ZmExtern void ZmSpecific_cleanup_del(ZmSpecific_Object *);
#endif
}

struct ZmAPI ZmSpecific_Object {
  typedef void (*DtorFn)(ZmSpecific_Object *);

  inline ZmSpecific_Object() { }
  inline ~ZmSpecific_Object() {
    ZmSpecific_lock();
    dtor();
  }

  inline void dtor() {
    if (ZuLikely(dtorFn))
      (*dtorFn)(this);
    else
      ZmSpecific_unlock();
  }

  DtorFn		dtorFn = nullptr;
  ZmObject		*ptr = nullptr;
  ZmSpecific_Object	*prev = nullptr;
  ZmSpecific_Object	*next = nullptr;
#ifdef _WIN32
  ZmPlatform::ThreadID	tid = 0;
  ZmSpecific_Object	*modPrev = nullptr;
  ZmSpecific_Object	*modNext = nullptr;
#endif
};

class ZmAPI ZmSpecific_ {
  using Object = ZmSpecific_Object;

public:
  ZmSpecific_() { }
  ~ZmSpecific_() {
    for (;;) {
      ZmSpecific_lock();
      Object *o = m_head; // LIFO
      if (!o) { ZmSpecific_unlock(); return; }
      o->dtor(); // unlocks
    }
  }

  void add(Object *o) {
    o->prev = nullptr;
    if (!(o->next = m_head))
      m_tail = o;
    else
      m_head->prev = o;
    m_head = o;
#ifdef _WIN32
    ZmSpecific_cleanup_add(o);
#endif
    ++m_count;
  }
  void del(Object *o) {
    if (!o->prev)
      m_head = o->next;
    else
      o->prev->next = o->next;
    if (!o->next)
      m_tail = o->prev;
    else
      o->next->prev = o->prev;
#ifdef _WIN32
    ZmSpecific_cleanup_del(o);
#endif
    --m_count;
  }

#ifdef _WIN32
  template <typename T>
  void set(ZmPlatform::ThreadID tid, T *ptr) {
  retry:
    for (Object *o = m_head; o; o = o->next)
      if (o->tid == tid)
	if (o->ptr != static_cast<ZmObject *>(ptr)) {
	  if (o->ptr) {
	    o->dtor(); // unlocks
	    ZmSpecific_lock();
	    goto retry;
	  }
	  o->ptr = ptr;
	  ZmREF(ptr);
	}
  }
  ZmObject *get(ZmPlatform::ThreadID tid) {
    for (Object *o = m_head; o; o = o->next)
      if (o->tid == tid && o->ptr) {
	return o->ptr;
      }
    return nullptr;
  }
#endif

  template <typename T>
  inline void all_(ZmFn<T *> fn) {
    all_2(&fn);
  }

  template <typename T>
  void all_2(ZmFn<T *> *fn) {
    ZmSpecific_lock();

    if (ZuUnlikely(!m_count)) { ZmSpecific_unlock(); return; }

    Object **objects = 0;
#ifdef __GNUC__
    objects = (Object **)alloca(sizeof(Object *) * m_count);
#endif
#ifdef _MSC_VER
    __try {
      objects = (Object **)_alloca(sizeof(Object *) * m_count);
    } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
      _resetstkoflw();
      objects = 0;
    }
#endif

    if (ZuUnlikely(!objects)) { ZmSpecific_unlock(); return; }

    memset(objects, 0, sizeof(Object *) * m_count);
    all_3(objects, fn);
  }

  template <typename T>
  void all_3(Object **objects, ZmFn<T *> *fn) {
    unsigned j = 0, n = m_count;

    for (Object *o = m_head; j < n && o; ++j, o = o->next)
      if (!o->ptr)
	objects[j] = nullptr;
      else
	objects[j] = o;

#ifdef _WIN32
    qsort(objects, n, sizeof(Object *),
	[](const void *o1, const void *o2) -> int {
	  return ZuCmp<ZmPlatform::ThreadID>::cmp(
	      (*(Object **)o1)->tid, (*(Object **)o2)->tid);
	});
    {
      ZmPlatform::ThreadID lastTID = 0;
      for (j = 0; j < n; j++)
	if (Object *o = objects[j])
	  if (o->tid == lastTID)
	    objects[j] = nullptr;
	  else
	    lastTID = o->tid;
    }
#endif

    for (j = 0; j < n; j++)
      if (Object *o = objects[j])
	if (T *ptr = static_cast<T *>(o->ptr))
	  ZmREF(ptr);

    ZmSpecific_unlock();

    for (j = 0; j < n; j++)
      if (Object *o = objects[j])
	if (T *ptr = static_cast<T *>(o->ptr)) {
	  (*fn)(ptr);
	  ZmDEREF(ptr);
	}
  }


private:
  unsigned	m_count = 0;
  Object	*m_head = nullptr;
  Object	*m_tail = nullptr;
};

template <typename T, bool Construct = true> struct ZmSpecificCtor {
  static T *fn() { return new T(); }
};
template <typename T> struct ZmSpecificCtor<T, false> {
  static T *fn() { return nullptr; }
};
template <class T_, bool Construct_ = true,
  auto CtorFn = ZmSpecificCtor<T_, Construct_>::fn>
class ZmSpecific : public ZmGlobal, public ZmSpecific_ {
  ZmSpecific(const ZmSpecific &);
  ZmSpecific &operator =(const ZmSpecific &);	// prevent mis-use

public:
  typedef T_ T;

private:
  ZuCan(final, CanFinal);
  template <typename U> inline static typename
    ZuIfT<CanFinal<ZmCleanup<U>, void (*)(U *)>::OK, void>::T
      final(U *u) { ZmCleanup<U>::final(u); }
  template <typename U> inline static typename
    ZuIfT<!CanFinal<ZmCleanup<U>, void (*)(U *)>::OK, void>::T
      final(U *u) { }

  typedef ZmSpecific_Object Object;

public:
  ZmSpecific() { }
  ~ZmSpecific() { }

private:
  using ZmSpecific_::add;
  using ZmSpecific_::del;
#ifdef _WIN32
  using ZmSpecific_::set;
  using ZmSpecific_::get;
#endif

  static Object *local_() {
    thread_local Object o;
    return &o;
  }

  void dtor_(Object *o) {
    T *ptr;
    if (ptr = static_cast<T *>(o->ptr)) {
      this->del(o);
      o->ptr = nullptr;
    }
    ZmSpecific_unlock();
    if (ptr) {
      final(ptr); // calls ZmCleanup<T>::final() if available
      ZmDEREF(ptr);
    }
  }

  static void dtor__(Object *o) { global()->dtor_(o); }

  inline T *create_() {
#ifdef _WIN32
    T *ptr = nullptr;
    Object *o = local_();
    ZmSpecific_lock();
    if (!o->ptr) {
      o->dtorFn = dtor__;
      o->tid = ZmPlatform::getTID_();
    }
  retry:
    if (o->ptr = get(o->tid = ZmPlatform::getTID_())) {
      T *ptr_ = static_cast<T *>(o->ptr);
      add(o);
      ZmREF(ptr_);
      ZmSpecific_unlock();
      if (ptr) delete ptr;
      return ptr_;
    }
    if (!ptr) {
      ZmSpecific_unlock();
      ptr = CtorFn();
      ZmSpecific_lock();
      goto retry;
    }
    o->ptr = ptr;
    add(o);
    ZmREF(ptr);
    ZmSpecific_unlock();
    return ptr;
#else
    T *ptr = CtorFn();
    Object *o = local_();
    ZmSpecific_lock();
    o->dtorFn = dtor__;
    o->ptr = ptr;
    add(o);
    ZmREF(ptr);
    ZmSpecific_unlock();
    return ptr;
#endif
  }

  inline T *instance_(T *ptr) {
    Object *o = local_();
    ZmSpecific_lock();
    if (!o->ptr) {
      o->dtorFn = dtor__;
#ifdef _WIN32
      o->tid = ZmPlatform::getTID_();
#endif
    }
  retry:
    if (!o->ptr) {
      o->ptr = ptr;
      add(o);
      ZmREF(ptr);
    } else {
      dtor_(o); // unlocks
      ZmSpecific_lock();
      goto retry;
    }
#ifdef _WIN32
    set(o->tid, ptr);
#endif
    ZmSpecific_unlock();
    return ptr;
  }

  ZuInline static ZmSpecific *global() {
    return ZmGlobal::global<ZmSpecific>();
  }

public:
  template <bool Construct = Construct_>
  ZuInline static typename ZuIfT<Construct, T *>::T create() {
    return global()->create_();
  }
  template <bool Construct = Construct_>
  ZuInline static typename ZuIfT<!Construct, T *>::T create() {
    return nullptr;
  }
  ZuInline static T *instance() {
    ZmObject *ptr;
    if (ZuLikely(ptr = local_()->ptr)) return static_cast<T *>(ptr);
    return create();
  }
  inline static T *instance(T *ptr) {
    return global()->instance_(ptr);
  }

  inline static void all(ZmFn<T *> fn) { return global()->all_(fn); }
};

template <typename L> struct ZmTLS_Ctor {
  static auto fn() { return (*(L *)(void *)0)(); }
  typedef decltype(fn()) (*Fn)();
  enum { OK = ZuConversion<L, Fn>::Exists };
};
template <typename L>
ZuInline auto ZmTLS(L l, typename ZuIfT<ZmTLS_Ctor<L>::OK>::T *_ = 0) {
  typedef typename ZuDeref<decltype(*ZmTLS_Ctor<L>::fn())>::T T;
  return ZmSpecific<T, true, ZmTLS_Ctor<L>::fn>::instance();
}

#endif /* ZmSpecific_HPP */
