//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZuLib.hpp>

#include <iostream>

#include <zlib/ZuStringN.hpp>

#include <zlib/ZuFixed.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main()
{
  // check basic string scan
  CHECK((double)(ZuFixed{"0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"."}.fp()) == 0.0);
  CHECK((double)(ZuFixed{".0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"0."}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"0.0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"-0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"-."}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"-.0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"-0."}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"-0.0"}.fp()) == 0.0);
  CHECK((double)(ZuFixed{"1000.42"}.fp()) == 1000.42);
  CHECK((double)(ZuFixed{"-1000.42"}.fp()) == -1000.42);
  // check basic value scanning
  {
    auto v = ZuFixed{"1000.42"};
    CHECK((ZuStringN<44>() << v.value) == "1000420000000000000000");
    CHECK((double)(v.fp()) == 1000.42);
    v = ZuFixed{"-1000.4200000000000000001"};
    CHECK((ZuStringN<44>() << v.value) == "-1000420000000000000000");
    CHECK((double)(v.fp()) == -1000.42);
  }
  // check leading/trailing zeros
  CHECK((double)(ZuFixed{"001"}.fp()) == 1.0);
  CHECK((double)(ZuFixed{"1.000"}.fp()) == 1.0);
  CHECK((double)(ZuFixed{"001.000"}.fp()) == 1.0);
  CHECK((double)(ZuFixed{"00.100100100"}.fp()) == .1001001);
  CHECK((double)(ZuFixed{"0.10010010"}.fp()) == .1001001);
  CHECK((double)(ZuFixed{".1001001"}.fp()) == .1001001);
  {
    // check basic multiply
    double v = (ZuFixed{"1000.42"} * ZuFixed{2.5}).fp();
    CHECK(v == 2501.05);
    v = (ZuFixed{"-1000.42"} * ZuFixed{2.5}).fp();
    CHECK(v == -2501.05);
  }
  {
    // check overflow multiply
    ZuFixed f{"10000000000000000"};
    int128_t v = (f * f).value;
    CHECK(!*ZuBoxed(v));
    f = 10;
    v = (f * f).value;
    CHECK((double)(ZuFixed{ZuFixed::NoScale, v}.fp()) == 100.0);
  }
  {
    // check underflow multiply
    ZuFixed f{".000000000000000001"};
    CHECK((int)f.value == 1);
    auto v = (f * f).value;
    CHECK(!v);
    ZuFixed g{".00000000000000001"};
    CHECK((int)g.value == 10);
    v = (g * ZuFixed{".1"}).value;
    CHECK((ZuFixed{ZuFixed::NoScale, v}.fp() == .000000000000000001L));
    v = (g * ZuFixed{".01"}).value;
    CHECK(!(int)v);
  }
  CHECK((!*ZuFixed{""}));
  // check overflow/underflow strings
  CHECK((!*ZuFixed{"1000000000000000000"}));
  CHECK((!ZuFixed{".0000000000000000001"}));
  // check formatted printing
  {
    ZuStringN<60> s;
    s << ZuFixed{"42000.42"}.fmt(ZuFmt::Comma<>());
    CHECK(s == "42,000.42");
  }
}
