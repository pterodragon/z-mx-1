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

/* backtrace test program */

#include <zlib/ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <zlib/ZmBackTrace.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#ifdef ZDEBUG
#include <zlib/ZmTrap.hpp>
#endif

template <bool X> struct Foo;
template <> struct Foo<1> {
  static ZmBackTrace d() {
    ZmBackTrace p, q;
    p.capture();
    q = p;
    return q;
  }
};

ZmBackTrace c() {
  return Foo<1>::d();
}

ZmBackTrace b() {
  return c();
}

void a(ZmBackTrace &t) {
  t = b();
}

#ifdef ZmObject_DEBUG
struct A : public ZmObject { };

ZmRef<A> p() { ZmRef<A> a = new A(); a->ZmObject::debug(); return a; }

ZmRef<A> q() { return p(); }

ZmRef<A> r() { return q(); }

ZmRef<A> s() { return r(); }

void dump(void *, const void *referrer, const ZmBackTrace *bt)
{
  std::cout << ZuBoxPtr(referrer).hex() << ":\n" << *bt;
}
#endif

#ifdef ZDEBUG
void crash(int *x) { *x = 0; }

void bar(int *x) { crash(x); }

void foo() { bar(0); }
#endif

int main()
{
  {
    ZmBackTrace t;
    a(t);
    std::cout << t;
  }

#ifdef ZmObject_DEBUG
  ZmRef<A> a = s();

  a->ZmObject::dump(0, &dump);
#endif

#ifdef ZDEBUG
  {
    ZmTrap::trap();
    //foo();
  }
#endif
}
