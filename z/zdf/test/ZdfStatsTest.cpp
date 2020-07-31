//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>
#include <zlib/ZuDemangle.hpp>

#include <zlib/ZdfStats.hpp>

void print(const char *s) {
  std::cout << s << '\n' << std::flush;
}
void print(const char *s, int64_t i) {
  std::cout << s << ' ' << i << '\n' << std::flush;
}
void ok(const char *s) { } // { print(s); }
void ok(const char *s, int64_t i) { } // { print(s, i); }
void fail(const char *s) { print(s); }
void fail(const char *s, int64_t i) { print(s, i); }
#define CHECK(x) ((x) ? ok("OK  " #x) : fail("NOK " #x))
#define CHECK2(x, y) ((x == y) ? ok("OK  " #x, x) : fail("NOK " #x, x))

template <typename T>
void describe(const T &w) {
  std::cout << "iteration\n";
  for (auto i = w.begin(); i != w.end(); ++i)
    std::cout << i->first << ' ' << i->second << '\n';
  std::cout << "\norder\n";
  for (unsigned i = 0,n = w.count(); i < n; i++) {
    auto j = w.order(i);
    std::cout << j->first << ' ' << j->second << '\n';
  }
  std::cout << "\nstats\n";
  std::cout <<
    "count=" << ZuBoxed(w.count()) <<
    " min=" << ZuBoxed(w.minimum()) <<
    " max=" << ZuBoxed(w.maximum()) <<
    " mean=" << ZuBoxed(w.mean()) <<
    " stddev=" << ZuBoxed(w.std()) <<
    " median=" << ZuBoxed(w.median()) <<
    " 80%=" << ZuBoxed(w.rank(0.80)) <<
    " 95%=" << ZuBoxed(w.rank(0.95)) << "\n\n";
}

int main()
{
  using namespace Zdf;
  Zdf::StatsTree w;
  {
    describe(w);
    w.add(ZuFixed{42, 0});
    describe(w);
    w.add(ZuFixed{421, 1});
    describe(w);
    w.add(ZuFixed{4200, 2});
    describe(w);
    w.add(ZuFixed{42200, 3});
    describe(w);
    w.add(ZuFixed{42000, 3});
    describe(w);
    w.add(ZuFixed{423000, 4});
    describe(w);
    w.add(ZuFixed{424000, 4});
    w.add(ZuFixed{424000, 4});
    w.add(ZuFixed{424000, 4});
    describe(w);
    w.del(ZuFixed{42, 0});
    w.del(ZuFixed{424, 1});
    describe(w);
    w.del(ZuFixed{42, 0});
    w.del(ZuFixed{424, 1});
    describe(w);
    w.del(ZuFixed{42, 0});
    w.del(ZuFixed{424, 1});
    describe(w);
    w.add(ZuFixed{420000, 4});
    w.del(ZuFixed{422, 1});
    w.del(ZuFixed{423, 1});
    describe(w);
  }
}
