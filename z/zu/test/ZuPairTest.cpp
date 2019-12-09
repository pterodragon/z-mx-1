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
  P p(static_cast<typename P::T0>(i), static_cast<typename P::T1>(j));
  P q = ZuMv(p);
  return q;
}

static unsigned copied = 0;
static unsigned moved = 0;

struct A {
  A() : i(0) { }
  A(int i_) : i(i_) { }
  A(const A &a) : i(a.i) { ++copied; }
  A &operator =(const A &a) { i = a.i; ++copied; return *this; }
  A(A &&a) : i(a.i) { ++moved; }
  A &operator =(A &&a) { i = a.i; ++moved; return *this; }
  int cmp(const A &a) const { return ZuCmp<int>::cmp(i, a.i); }
  int hash() const { return ZuHash<int>::hash(i); }
  bool operator !() const { return !i; }
  bool operator ==(const A &a) const { return i == a.i; }
  int i;
};
template <> struct ZuTraits<A> : public ZuGenericTraits<A> {
  enum { IsPrimitive = 0, IsComparable = 1, IsHashable = 1 };
};

ZuPair<A, A> mkapair() { return ZuMkPair(A(42), A(42)); }
ZuPair<A, A> passapair(ZuPair<A, A> a) { return a; }

ZuTuple<A, A, A> mkatuple() { return ZuMkTuple(A(42), A(42), A(42)); }
ZuTuple<A, A, A> passatuple(ZuTuple<A, A, A> a) { return a; }

ZuTupleFields(B_, foo);
typedef B_<A, A, A> B;

int main()
{
  { VPair p = mkpair<VPair>(); CHECK(p.p<0>() == 42); }
  { RVPair p = mkpair<RVPair>(); CHECK(p.p<0>() == 42); }
  { LVPair p = mkpair<LVPair>(); CHECK(p.p<0>() == 42); }
  { MVPair p = mvpair<MVPair>(); CHECK(p.p<0>() == 42); }
  {
    copied = moved = 0;
    ZuPair<A, A> p = mkapair();
    CHECK(!copied && moved == 2 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuPair<A, A> p(mkapair());
    CHECK(!copied && moved == 2 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuPair<A, A> p(passapair(mkapair()));
    CHECK(!copied && moved == 4 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p = mkatuple();
    CHECK(!copied && moved == 5 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(mkatuple());
    CHECK(!copied && moved == 5 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(passatuple(mkatuple()));
    CHECK(!copied && moved == 8 && p.p<0>().i == 42);
  }
  {
    copied = moved = 0;
    ZuTuple<A, A, A> p(passatuple(mkatuple()));
    A a = ZuMv(p.p<0>()), b = ZuMv(p.p<1>()), c = ZuMv(p.p<2>());
    CHECK(!copied && moved == 11);
    CHECK(a.i == 42 && b.i == 42 && c.i == 42);
  }
  {
    copied = moved = 0;
    B p(passatuple(mkatuple()));
    A a = ZuMv(p.foo()), b = ZuMv(p.p<1>()), c = ZuMv(p.p<2>());
    CHECK(!copied && moved == 14);
    CHECK(a.i == 42 && b.i == 42 && c.i == 42);
    B q = B().foo(42), r{p};
    p = q;
    r = ZuMv(q);
  }
}
