//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <iostream>

#include <zlib/ZuField.hpp>
#include <zlib/ZuPP.hpp>

namespace Foo {
  struct A {
    int i = 42;
    const char *j_ = "hello";
    const char *j() const { return j_; }
    void j(const char *s) { j_ = s; }
  };

  ZuFields(A, (Data, i), (Fn, j));

  struct B {
    int i = 42;
    const char *j_ = "hello";
    const char *j() const { return j_; }
  };

  ZuFields(B, (RdData, i), (RdFn, j));
}

int main()
{
  using A = Foo::A;
  using B = Foo::B;
  A a;
  ZuType<1, ZuFieldList<A>>::T::set(&a, "bye");
  ZuTypeAll<ZuFieldList<A>>::invoke([a = &a]<typename T>() mutable {
    std::cout << T::id() << '=' << T::get(a) << '\n';
  });
  B b;
  ZuTypeAll<ZuFieldList<B>>::invoke([b = &b]<typename T>() mutable {
    std::cout << T::id() << '=' << T::get(b) << '\n';
  });
  return 0;
}
