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

/* test program */

#include <zlib/ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuHash.hpp>

#include <zlib/ZmRef.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmHash.hpp>
#include <zlib/ZmList.hpp>
#include <zlib/ZmTime.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmThread.hpp>
#include <zlib/ZmSingleton.hpp>
#include <zlib/ZmSpecific.hpp>

#define mb() __asm__ __volatile__("":::"memory")

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

struct X : public ZmObject {
  X() : x(0) { }
  virtual ~X() { }
  virtual void helloWorld();
  void inc() { ++x; }
  unsigned x;
};

void X::helloWorld() { puts("hello world"); }

struct Y : public X {
  virtual void helloWorld();
};

struct Z : public ZmObject { int m_z; };

template <> struct ZuTraits<Z> : public ZuGenericTraits<Z> {
  enum { IsReal = 0, IsPrimitive = 0 };
};

struct ZCmp {
  static int cmp(Z *z1, Z *z2) { return z1->m_z - z2->m_z; }
  static bool equals(Z *z1, Z *z2) { return z1->m_z == z2->m_z; }
  static bool null(Z *z) { return !z; }
  static const ZmRef<Z> &null() { static const ZmRef<Z> z; return z; }
};

using ZList = ZmList<ZmRef<Z>, ZmListCmp<ZCmp> >;
using ZHash = ZmHash<int, ZmHashVal<ZmRef<Z> > >;

using ZList2 = ZmList<ZuStringN<20>, ZmListNodeIsItem<true> >;

void Y::helloWorld() { puts("hello world [Y]"); }

ZmRef<X> foo(X *xPtr) { return(xPtr); }

int hashTestSize = 1000;

void hashIt(ZHash *h) {
  ZmRef<Z> z = new Z;
  int j;

  for (j = 0; j < hashTestSize; j++)
    h->add(j, z);
  for (j = 0; j < hashTestSize; j++)
    h->del(j);
  for (j = 0; j < hashTestSize; j++) {
    h->add(j, z);
    h->del(j);
  }
}

void semPost(ZmSemaphore *sema) {
  int i;

  ZmSpecific<X>::instance()->inc();
  for (i = 0; i < 10; i++) sema->post();
}

void semWait(ZmSemaphore *sema) {
  int i;

  ZmSpecific<X>::instance()->inc();
  for (i = 0; i < 10; i++) sema->wait();
}

struct S : public ZmObject {
  S() { m_i = 0; m_j++; }

  void foo() { m_i++; }

  static void meyers() {
    for (int i = 0; i < 100000; i++) {
      static ZmRef<S> s_ = new S();
      S *s = s_;
      s->foo();
      mb();
    }
  }

  static void singleton() {
    for (int i = 0; i < 100000; i++) {
      S *s = ZmSingleton<S>::instance();
      s->foo();
      mb();
    }
  }

  static void specific() {
    for (int i = 0; i < 100000; i++) {
      S *s = ZmSpecific<S>::instance();
      s->foo();
      mb();
    }
  }

  static void tls() {
    for (int i = 0; i < 100000; i++) {
      thread_local ZmRef<S> s = new S();
      s->foo();
      mb();
    }
  }

  int			m_i;
  static unsigned	m_j;
};

unsigned S::m_j = 0;

struct W {
  void fn(const char *prefix, ZmThreadContext *c) {
    const ZmThreadName &s = c->name();
    if (!s)
      printf("%s: %d\n", prefix, (int)c->tid());
    else
      printf("%s: %.*s\n", prefix, s.length(), s.data());
  }
  void fn1(ZmThreadContext *c) { fn("list1", c); }
  void fn2(ZmThreadContext *c) { fn("list2", c); }
  void post() { m_sem.post(); }
  void wait() { m_sem.wait(); }
  ZmSemaphore				m_sem;
};

struct O : public ZmObject {
  O() : referenced(0), dereferenced(0) { }
#ifdef ZmObject_DEBUG
  void ref(const void *referrer = 0) const {
    ++referenced;
    ZmObject::ref(referrer);
  }
#else
  void ref() const { ++referenced; }
#endif
#ifdef ZmObject_DEBUG
  bool deref(const void *referrer = 0) const {
    ++dereferenced;
    return ZmObject::deref(referrer);
  }
#else
  bool deref() const { return ++dereferenced >= referenced; }
#endif
  mutable unsigned referenced, dereferenced;
};

int main(int argc, char **argv)
{
  ZmTime overallStart, overallEnd;

  overallStart.now();

  if (argc > 1) hashTestSize = atoi(argv[1]);

  ZmRef<X> x = new X;

  {
    ZmRef<X> nullPtr;
    ZmRef<X> nullPtr_;

    if (!nullPtr) puts("null test 1 ok");

    nullPtr = x;
    if (nullPtr) puts("null test 2 ok");

    nullPtr = 0;
    if (!nullPtr) puts("null test 3 ok");

    nullPtr_ = x;
    if (nullPtr_) puts("null test 5 ok");

    nullPtr_ = nullPtr;
    if (!nullPtr_) puts("null test 6 ok");

    nullPtr = x;
    if (nullPtr) puts("null test 7 ok");

    nullPtr = nullPtr_;
    if (!nullPtr) puts("null test 8 ok");

    nullPtr_ = (X *)0;
    if (!nullPtr_) puts("null test 9 ok");
  }

  {
    ZmRef<X> xPtr = foo(x);
    ZmRef<X> xPtr_ = foo(x);

    if ((X *)xPtr == &(*xPtr)) puts("cast test 1 ok");
    if ((X *)xPtr_ == &(*xPtr_)) puts("cast test 2 ok");
  }

  {
    ZmRef<X> xPtr(x);

    xPtr->helloWorld();

    ZmRef<X> xPtr2 = x;

    (*xPtr2).helloWorld();

    xPtr = x;

    if (xPtr == xPtr2) puts("equality test 1 ok");
    if (xPtr == (ZmRef<X>)xPtr2) puts("equality test 2 ok");

    xPtr->helloWorld();

    X *xRealPtr = (X *)xPtr2;

    xRealPtr->helloWorld();
  }

  {
    ZmRef<Y> y = new Y;
    { ZmRef<Y> y2 = new Y; }

    ZmRef<Y> yPtr = y;
    ZmRef<X> xPtr = (ZmRef<X>)y;

    xPtr->helloWorld();
    yPtr->helloWorld();
    ((ZmRef<Y>)(Y *)(X *)xPtr)->helloWorld();
  }

  ZmRef<ZHash> hash = new ZHash(ZmHashParams().bits(8));
  ZmRef<Z> z = new Z;

  z->m_z = 1;

  hash->add(0, z);
  hash->add(1, z);
  hash->del(0);

  {
    ZHash::Iterator i(*hash);

    if ((Z *)i.iterate()->val() != (Z *)z)
      puts("collection test failed");
  }

  {
    ZList list;
    ZList list1;
    ZList list2;
    ZmRef<Z> z = new Z;

    z->m_z = 1234;

    list.add(z);
    list.add(z);
    list1.add(z);
    list2.add(z);
    list.del(z);
    list1.add(z);
    list2.add(z);
    z = list1.shift();
    if (z->m_z != 1234) puts("list1 test 1 failed");
    z = list2.shift();
    if (z->m_z != 1234) puts("list2 test 1 failed");
    list.del(z);
    z = list1.shift();
    if (z->m_z != 1234) puts("list1 test 2 failed");
    z = list2.shift();
    if (z->m_z != 1234) puts("list2 test 2 failed");

    ZList list3;
    ZmRef<Z> z2 = new Z, z3 = new Z;

    z2->m_z = 2345;
    z3->m_z = 3456;
    list1.add(z);
    list2.add(z);
    list3.add(z);
    list1.add(z2);
    list2.add(z2);
    list3.add(z2);
    list1.add(z3);
    list2.add(z3);
    list3.add(z3);
#ifdef ZmRef_DEBUG
    printf("z: "); z->debug();
    printf("z2: "); z2->debug();
    printf("z3: "); z3->debug();
#endif
    z = list1.shift();
    if (z->m_z != 1234) puts("list1 test 3 failed");
    z = list2.pop();
    if (z->m_z != 3456) puts("list2 test 3 failed");
    z = list1.shift();
    if (z->m_z != 2345) puts("list1 test 4 failed");
    z = list2.pop();
    if (z->m_z != 2345) puts("list2 test 4 failed");
    z = list1.shift();
    if (z->m_z != 3456) puts("list1 test 5 failed");
    z = list2.pop();
    if (z->m_z != 1234) puts("list2 test 5 failed");

    puts("list3 iteration 1");
    {
      ZList::Iterator iter(list3);

      while (z = iter.iterate())
	printf("%d\n", z->m_z);
    }

    puts("list3 iteration 2");
    {
      ZList::Iterator iter(list3);

      while (z = iter.iterate())
	printf("%d\n", z->m_z);
    }

    puts("list3 iteration 3");
    {
      ZList::Iterator iter(list3);

      while (z = iter.iterate())
	printf("%d\n", z->m_z);
    }

    puts("list tests 1 ok");
  }
  {
    ZList2 list;
    list.add(ZmRef<ZList2::Node>(new ZList2::Node("foo")));
    list.add(ZmRef<ZList2::Node>(new ZList2::Node("bar")));
    list.add(ZmRef<ZList2::Node>(new ZList2::Node("baz")));
    {
      ZList2::Iterator iter(list);
      ZList2::NodeRef z;

      while (z = iter.iterateNode()) puts(*z);
    }
  }

  ZmThread r[80];
  int j;

  {
    ZmSemaphore *sema = new ZmSemaphore;

    puts("spawning 80 threads...");

    for (j = 0; j < 80; j++)
      r[j] = ZmThread(0, ZmFn<>::Bound<&semPost>::fn(sema));

    for (j = 0; j < 80; j++) sema->wait();

    for (j = 0; j < 80; j++) r[j].join(0);

    puts("80 threads finished");

    puts("spawning 80 threads...");

    for (j = 0; j < 40; j++)
      r[j] = ZmThread(0, ZmFn<>::Bound<&semWait>::fn(sema));

    for (j = 40; j < 80; j++)
      r[j] = ZmThread(0, ZmFn<>::Bound<&semPost>::fn(sema));

    for (j = 0; j < 80; j++) r[j].join(0);

    puts("80 threads finished");

    ZmTime start, end;

    start.now();

    for (j = 0; j < 1000000; j++) {
      sema->post();
      sema->wait();
    }

    end.now();
    end -= start;
    printf("sem post/wait time: %d.%.3d / 1000000 = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), end.dtime() / 1000000.0);

    delete sema;
  }

  {
    puts("starting ZmPLock lock/unlock time test");

    ZmPLock lock;

    ZmTime start, end;

    start.now();

    for (j = 0; j < 1000000; j++) { lock.lock(); lock.unlock(); }

    end.now();
    end -= start;
    printf("lock/unlock time: %d.%.3d / 1000000 = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), end.dtime() / 1000000.0);
  }

  {
    puts("starting ref/deref time test");

    ZmRef<ZmObject> l = new ZmObject;

    ZmTime start, end;

    start.now();

    for (j = 0; j < 1000000; j++) { l->ref(); mb(); }

    end.now();
    end -= start;
    printf("ref time: %d.%.3d / 1000000 = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), end.dtime() / 1000000.0);

    start.now();

    for (j = 0; j < 1000000; j++) { l->deref(); mb(); }

    end.now();
    end -= start;
    printf("deref time: %d.%.3d / 1000000 = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), end.dtime() / 1000000.0);
  }

  int n = ZmPlatform::getncpu();
  int t = n * 1000000;

  {
    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, ZmFn<>::Ptr<&S::meyers>::fn());
    for (j = 0; j < n; j++)
      r[j].join(0);

    end.now();
    end -= start;

    printf("Meyers singleton time: %d.%.3d / %d = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), t,
	end.dtime() / (double)t);
    printf("S() called %u times\n", S::m_j); S::m_j = 0;
  }

  {
    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, ZmFn<>::Ptr<&S::singleton>::fn());
    for (j = 0; j < n; j++)
      r[j].join(0);

    end.now();
    end -= start;
    printf("ZmSingleton::instance() time: %d.%.3d / %d = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), t,
	end.dtime() / (double)t);
    printf("S() called %u times\n", S::m_j); S::m_j = 0;
  }

  {
    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, ZmFn<>::Ptr<&S::specific>::fn());
    for (j = 0; j < n; j++)
      r[j].join(0);

    end.now();
    end -= start;
    printf("ZmSpecific::instance() time: %d.%.3d / %d = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), t,
	end.dtime() / (double)t);
    printf("S() called %u times\n", S::m_j); S::m_j = 0;
  }

  {
    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, ZmFn<>::Ptr<&S::tls>::fn());
    for (j = 0; j < n; j++)
      r[j].join(0);

    end.now();
    end -= start;
    printf("thread_local time: %d.%.3d / %d = %.16f\n",
	(int)end.sec(), (int)(end.nsec() / 1000000), t,
	end.dtime() / (double)t);
    printf("S() called %u times\n", S::m_j); S::m_j = 0;
  }


  overallEnd.now();
  overallEnd -= overallStart;
  printf("overall time: %d.%.3d\n",
    (int)overallEnd.sec(), (int)(overallEnd.nsec() / 1000000));

  {
    W w;
    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, ZmFn<>::Member<&W::wait>::fn(&w));
    ZmPlatform::sleep(1);
    ZmSpecific<ZmThreadContext>::all(
	ZmFn<ZmThreadContext *>::Member<&W::fn1>::fn(&w));
    for (j = 0; j < n; j++) w.post();
    for (j = 0; j < n; j++) r[j].join(0);
    ZmSpecific<ZmThreadContext>::all(
	ZmFn<ZmThreadContext *>::Member<&W::fn2>::fn(&w));
  }

  {
    ZmRef<O> p;
    {
      ZmRef<O> o = new O();

      CHECK(o->referenced == 1 && !o->dereferenced);
      p = o;
    }
    CHECK(p->referenced == 2 && p->dereferenced == 1);
    ZmRef<O> q = ZuMv(p);
    CHECK(!p);
    CHECK(q->referenced == 2 && q->dereferenced == 1);
  }
}
