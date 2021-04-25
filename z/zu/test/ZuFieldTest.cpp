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

  ZuFieldDef(A, (Data, i), (Fn, j));
}

int main()
{
  using A = Foo::A;
  A a;
  ZuTypeAll<ZuFields<A>>::invoke([a = &a]<typename T>() mutable {
    std::cout << T::id() << '=' << T::get(a) << '\n';
  });
  return 0;
}
