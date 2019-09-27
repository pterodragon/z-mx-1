//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZuLib.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <zlib/ZuPair.hpp>
#include <zlib/ZuTuple.hpp>
#include <zlib/ZuCmp.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

typedef ZuPair<int, int> VPair;
typedef ZuPair<const int &, const int &> RVPair;
typedef ZuPair<int &, int &> LVPair;
typedef ZuPair<int &&, int &&> MVPair;

template <typename P> P mkpair() {
  static int i = 42, j = 42;
  P p(i, j);
  P q = p;
  P &r = q;
  const P &s = r;
  return s;
}

template <typename P> P mvpair() {
  static int i = 42, j = 42;
  P p(static_cast<typename P::T1>(i), static_cast<typename P::T2>(j));
  P q = ZuMv(p);
  return q;
}

static unsigned copied = 0;
static unsigned moved = 0;

struct A {
  inline A() : i(0) { }
  inline A(int i_) : i(i_) { }
  inline A(const A &a) : i(a.i) { ++copied; }
  inline A &operator =(const A &a) { i = a.i; ++copied; return *this; }
  inline A(A &&a) : i(a.i) { ++moved; }
  inline A &operator =(A &&a) { i = a.i; ++moved; return *this; }
  inline int cmp(const A &a) const { return ZuCmp<int>::cmp(i, a.i); }
  inline int hash() const { return ZuHash<int>::hash(i); }
  inline bool operator !() const { return !i; }
  inline bool operator ==(const A &a) const { return i == a.i; }
  int i;
};
template <> struct ZuTraits<A> : public ZuGenericTraits<A> {
  enum { IsPrimitive = 0, IsComparable = 1, IsHashable = 1 };
};

ZuPair<A, A> mkapair() { return ZuMkPair(A(42), A(42)); }
ZuPair<A, A> passapair(ZuPair<A, A> a) { return a; }

ZuTuple<A, A, A> mkatuple() { return ZuMkTuple(A(42), A(42), A(42)); }
ZuTuple<A, A, A> passatuple(ZuTuple<A, A, A> a) { return a; }

ZuTupleFields(B_, 1, foo);
typedef B_<A, A, A> B;

int main()
{
  { VPair p = mkpair<VPair>(); CHECK(p.p1() == 42); }
  { RVPair p = mkpair<RVPair>(); CHECK(p.p1() == 42); }
  { LVPair p = mkpair<LVPair>(); CHECK(p.p1() == 42); }
  { MVPair p = mvpair<MVPair>(); CHECK(p.p1() == 42); }
  {
    copied = moved = 0;
    ZuPair<A, A> p = mkapair();
    CHECK(!copied && moved == 2 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuPair<A, A> p(mkapair());
    CHECK(!copied && moved == 2 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuPair<A, A> p(passapair(mkapair()));
    CHECK(!copied && moved == 4 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p = mkatuple();
    CHECK(!copied && moved == 3 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(mkatuple());
    CHECK(!copied && moved == 3 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(passatuple(mkatuple()));
    CHECK(!copied && moved == 6 && p.p1().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(passatuple(mkatuple()));
    A a = ZuMv(p.p1()), b = ZuMv(p.p2()), c = ZuMv(p.p3());
    CHECK(!copied && moved == 9);
    CHECK(a.i == 42 && b.i == 42 && c.i == 42);
  }
  {
    copied = moved = 0;
    B p(passatuple(mkatuple()));
    A a = ZuMv(p.foo()), b = ZuMv(p.p2()), c = ZuMv(p.p3());
    CHECK(!copied && moved == 12);
    CHECK(a.i == 42 && b.i == 42 && c.i == 42);
    B q = B().foo(42), r{p};
    p = q;
    r = ZuMv(q);
  }
}
