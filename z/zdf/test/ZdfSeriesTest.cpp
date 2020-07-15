//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>
#include <zlib/ZuDemangle.hpp>

#include <zlib/ZdfCompress.hpp>
#include <zlib/ZdfSeries.hpp>
#include <zlib/ZdfMem.hpp>

void print(const char *s) {
  std::cout << s << '\n' << std::flush;
}
void print(const char *s, int64_t i) {
  std::cout << s << ' ' << i << '\n' << std::flush;
}
void ok(const char *s) { print(s); }
void ok(const char *s, int64_t i) { print(s, i); }
void fail(const char *s) { print(s); }
void fail(const char *s, int64_t i) { print(s, i); }
#define CHECK(x) ((x) ? ok("OK  " #x) : fail("NOK " #x))
#define CHECK2(x, y) ((x == y) ? ok("OK  " #x, x) : fail("NOK " #x, x))

int main()
{
  using namespace Zdf;
  using namespace ZdfCompress;
  Zdf::MemMgr mgr;
  mgr.init(nullptr, nullptr);
  Zdf::Series s;
  s.init(&mgr);
  s.open("test", "test");
  {
    auto w = s.writer<DeltaEncoder<>>();
    CHECK(w.write(ZuFixed{42, 0}));
    CHECK(w.write(ZuFixed{42, 0}));
    CHECK(w.write(ZuFixed{4301, 2}));
    CHECK(w.write(ZuFixed{4302, 2}));
    CHECK(w.write(ZuFixed{43030, 3}));
    CHECK(w.write(ZuFixed{43040, 3}));
    CHECK(w.write(ZuFixed{430500, 4}));
    CHECK(w.write(ZuFixed{430600, 4}));
    for (unsigned i = 0; i < 300; i++) {
      CHECK(w.write(ZuFixed{430700, 4}));
    }
  }
  CHECK(s.blkCount() == 4);
  {
    auto r = s.reader<DeltaDecoder<>>();
    ZuFixed v;
    CHECK(r.read(v)); CHECK(v.value == 42 && !v.exponent);
    CHECK(r.read(v)); CHECK(v.value == 42 && !v.exponent);
    CHECK(r.read(v)); CHECK(v.value == 4301 && v.exponent == 2);
    CHECK(r.read(v)); CHECK(v.value == 4302 && v.exponent == 2);
    CHECK(r.read(v)); CHECK(v.value == 43030 && v.exponent == 3);
    CHECK(r.read(v)); CHECK(v.value == 43040 && v.exponent == 3);
    CHECK(r.read(v)); CHECK(v.value == 430500 && v.exponent == 4);
    CHECK(r.read(v)); CHECK(v.value == 430600 && v.exponent == 4);
    for (unsigned i = 0; i < 300; i++) {
      CHECK(r.read(v));
      CHECK(v.value == 430700 && v.exponent == 4);
    }
    CHECK(!r.read(v));
  }
  {
    auto r = s.index<DeltaDecoder<>>(ZuFixed{425, 1});
    ZuFixed v;
    CHECK(r.read(v)); CHECK(v.value == 4301 && v.exponent == 2);
  }
  {
    auto r = s.index<DeltaDecoder<>>(ZuFixed{43019, 3});
    ZuFixed v;
    CHECK(r.read(v)); CHECK(v.value == 4302 && v.exponent == 2);
    r.purge();
  }
  CHECK(s.blkCount() == 3);
  {
    auto r = s.index<DeltaDecoder<>>(ZuFixed{44, 0});
    ZuFixed v;
    CHECK(!r);
    CHECK(!r.read(v));
  }
  {
    auto r = s.reader<DeltaDecoder<>>();
    ZuFixed v;
    CHECK(r.read(v)); CHECK(v.value == 4301 && v.exponent == 2);
  }
  {
    auto r = s.reader<DeltaDecoder<>>(208);
    ZuFixed v;
    for (unsigned i = 0; i < 100; i++) {
      CHECK(r.read(v));
      CHECK(v.value == 430700 && v.exponent == 4);
    }
    CHECK(!r.read(v));
  }
  s.close();
  // s.final();
  // mgr.final();
}
