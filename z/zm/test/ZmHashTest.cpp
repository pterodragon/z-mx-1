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

/* test program */

#include <ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <ZuTraits.hpp>
#include <ZuHash.hpp>

#include <ZmRef.hpp>
#include <ZmHash.hpp>
#include <ZmList.hpp>
#include <ZmTime.hpp>
#include <ZmSemaphore.hpp>
#include <ZmThread.hpp>
#include <ZmSingleton.hpp>
#include <ZmSpecific.hpp>
#include <ZmPolymorph.hpp>

struct X : public ZmPolymorph {
  virtual void helloWorld();
};

void X::helloWorld() { puts("hello world"); }

struct Y : public X {
  virtual void helloWorld();
};

struct Z : public ZmObject { int m_z; };

template <> struct ZuTraits<Z> : public ZuGenericTraits<Z> {
  enum {  IsPrimitive = 0, IsReal = 0 };
};

struct ZCmp {
  inline static int cmp(Z *z1, Z *z2) { return z1->m_z - z2->m_z; }
  inline static bool equals(Z *z1, Z *z2) { return z1->m_z == z2->m_z; }
  inline static bool null(Z *z) { return !z; }
  inline static Z *null() { return 0; }
};

typedef ZmList<ZmRef<Z>, ZmListCmp<ZCmp> > ZList;
typedef ZmHash<int, ZmHashVal<ZmRef<Z> > > ZHash;

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

  for (i = 0; i < 10; i++) sema->post();
}

void semWait(ZmSemaphore *sema) {
  int i;

  for (i = 0; i < 10; i++) sema->wait();
}

struct S : public ZuObject {
  S() { m_i = 0; }

  inline void foo() { m_i++; }

  static void meyers() {
    for (int i = 0; i < 100000; i++) {
      static ZmRef<S> s_ = new S();
      ZmRef<S> s = s_;

      s->foo();
    }
  }

  static void singleton() {
    for (int i = 0; i < 100000; i++) {
      ZmRef<S> s = ZmSingleton<S>::instance();

      s->foo();
    }
  }

  static void specific() {
    for (int i = 0; i < 100000; i++) {
      ZmRef<S> s = ZmSpecific<S>::instance();

      s->foo();
    }
  }

  int	m_i;
};

struct I {
  inline I(int i) : m_i(i) { }
  inline I(const I &i) : m_i(i.m_i) { }
  inline I &operator =(const I &i)
    { if (this != &i) m_i = i.m_i; return *this; }
  inline int hash() const { return m_i; }
  inline int cmp(const I &i) const { return ZuCmp<int>::cmp(m_i, i.m_i); }
  inline bool operator ==(const I &i) const { return m_i == i.m_i; }
  inline bool operator !() const { return !m_i; }
  int	m_i;
};

template <> struct ZuTraits<I> : public ZuGenericTraits<I> {
  enum _ { IsHashable = 1, IsComparable = 1 };
};

struct J : public ZmObject {
  struct IAccessor : public ZuAccessor<J *, const I &> {
    inline static const ::I &value(const J *j) { return j->m_i; }
  };
  inline J(int i) : m_i(i) { }
  inline J(const J &j) : m_i(j.m_i.m_i) { }
  inline J &operator =(const J &j) {
    if (this != &j) m_i.m_i = j.m_i.m_i;
    return *this;
  }
  I	m_i;
};

int main(int argc, char **argv)
{
  ZmTime overallStart, overallEnd;

  overallStart.now();

  if (argc > 1) hashTestSize = atoi(argv[1]);

  ZmThread r[80];
  int j, k;
  int n = ZmPlatform::getncpu();

  for (k = 0; k < 10; k++) {
    ZmRef<ZHash> hash2 = new ZHash(
	ZmHashParams().bits(2).loadFactor(1.0).cBits(1));

    printf("hash count, bits, cbits: %d, %d, %d\n",
      hash2->count(), hash2->bits(), hash2->cBits());
    printf("spawning %d threads...\n", n);

    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++) r[j] =
      ZmThread(0, 0, ZmFn<>::Bound<&hashIt>::fn(hash2.ptr()));

    for (j = 0; j < n; j++) r[j].join(0);

    end.now();
    end -= start;
    printf("hash time: %d.%.3d\n", (int)end.sec(), (int)(end.nsec() / 1000000));

    printf("%d threads finished\n", n);
    printf("hash count, bits: %d, %d\n", hash2->count(), hash2->bits());
  }

  for (k = 0; k < 10; k++) {
    ZmRef<ZHash> hash2 = new ZHash(
	ZmHashParams().bits(4).loadFactor(1.0).cBits(4));

    printf("hash count, bits, cbits: %d, %d, %d\n",
      hash2->count(), hash2->bits(), hash2->cBits());
    printf("spawning %d threads...\n", n);

    ZmTime start, end;

    start.now();

    for (j = 0; j < n; j++)
      r[j] = ZmThread(0, 0, ZmFn<>::Bound<&hashIt>::fn(hash2.ptr()));

    for (j = 0; j < n; j++) r[j].join(0);

    end.now();
    end -= start;
    printf("hash time: %d.%.3d\n", (int)end.sec(), (int)(end.nsec() / 1000000));

    printf("%d threads finished\n", n);
    printf("hash count, bits: %d, %d\n", hash2->count(), hash2->bits());
  }

  overallEnd.now();
  overallEnd -= overallStart;
  printf("overall time: %d.%.3d\n",
    (int)overallEnd.sec(), (int)(overallEnd.nsec() / 1000000));

  {
    typedef ZmHash<ZmRef<J>,
	      ZmHashIndex<J::IAccessor> > H;
    ZmRef<H> h_ = new H();
    H &h = *h_;
    for (int k = 0; k < 100; k++) h.add(ZmRef<J>(new J(k)));
    for (int k = 0; k < 100; k++) {
      I i(k);
      ZmRef<J> j = h.findKey(i);
      printf("%d ", k);
    }
    puts("");
    for (int k = 0; k < 100; k++) h.add(ZmRef<J>(new J(42)));
    {
      H::ReadIndexIterator i(h, I(42));
      ZmRef<J> k;
      while (k = i.iterateKey()) {
	printf("%d ", k->m_i.m_i);
      }
      puts("");
    }
  }
}
