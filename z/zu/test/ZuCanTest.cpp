//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <zlib/ZuCan.hpp>
#include <zlib/ZuIfT.hpp>

struct A {
  void foo(int i) { printf("foo %d\n", i); }
};

struct B {
  static void bar(int i) { printf("bar %d\n", i); }
};

struct C {
  template <typename T>
  void baz(T t) { int i = t; printf("baz %d\n", i); }
  void operator ()() const { puts("C"); }
};

ZuCan(foo, CanFoo);
ZuCan(bar, CanBar);
ZuCan(baz, CanBaz);

template <typename T>
typename ZuIfT<CanFoo<T, void (T::*)(int)>::OK>::T foo(T *t, int i) { t->foo(i); }
template <typename T>
typename ZuIfT<!CanFoo<T, void (T::*)(int)>::OK>::T foo(T *, int) { }

template <typename T>
typename ZuIfT<CanBar<T, void (*)(int)>::OK>::T bar(int i) { T::bar(i); }
template <typename T>
typename ZuIfT<!CanBar<T, void (*)(int)>::OK>::T bar(int) { }

template <typename T>
typename ZuIfT<CanBaz<T, void (T::*)(int)>::OK>::T baz(T *t, int i) { t->baz(i); }
template <typename T>
typename ZuIfT<!CanBaz<T, void (T::*)(int)>::OK>::T baz(T *, int) { }

template <typename T>
typename ZuIfT<ZuCanOpFn<T, void (T::*)() const>::OK>::T fn(T *t) { t->operator ()(); }
template <typename T>
typename ZuIfT<!ZuCanOpFn<T, void (T::*)() const>::OK>::T fn(T *) { }

int main()
{
  A a;
  B b;
  C c;
  foo(&a, 41);
  foo(&b, 42); // cannot
  bar<A>(43); // cannot
  bar<B>(44);
  baz(&a, 45); // cannot
  baz(&c, 46);
  fn(&a); // cannot
  fn(&c);
}
