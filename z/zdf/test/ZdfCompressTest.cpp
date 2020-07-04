//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>
#include <zlib/ZuDemangle.hpp>

#include <zlib/ZdfCompress.hpp>

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

template <typename Reader, typename Writer>
void test() {
  uint8_t p[4096], n[4096];
  for (int64_t i = 0; i < 63; i++) {
    int64_t j = 1; j <<= i;
    {
      Writer pw{p, p + 4096};
      Writer nw{n, n + 4096};
      for (int64_t k = 0; k < 10; k++) {
	for (unsigned l = 0; l < 10; l++) CHECK(pw.write(j + k));
	CHECK(pw.write(j + k + 1));
	CHECK(pw.write(j + k + 2));
	CHECK(pw.write(j + k + 4));
	CHECK(pw.write(j + k + 8));
	CHECK(pw.write(j + k * k));
	CHECK(nw.write(-(j + k)));
	CHECK(nw.write(-(j + k + 1)));
	CHECK(nw.write(-(j + k + 2)));
	CHECK(nw.write(-(j + k + 4)));
	CHECK(nw.write(-(j + k + 8)));
	CHECK(nw.write(-(j + k * k)));
      }
      std::cout <<
	ZuDemangle<200>{typeid(Writer).name()} << " +ve: " << pw.count() << ' ' << (pw.pos() - p) << '\n';
      std::cout <<
	ZuDemangle<200>{typeid(Writer).name()} << " -ve: " << nw.count() << ' ' << (nw.pos() - n) << '\n';
    }
    {
      Reader pr{p, p + 4096};
      Reader nr{n, n + 4096};
      int64_t v;
      for (int64_t k = 0; k < 10; k++) {
	for (unsigned l = 0; l < 10; l++) {
	  CHECK(pr.read(v)); CHECK2(v, (j + k));
	}
	CHECK(pr.read(v)); CHECK2(v, (j + k + 1));
	CHECK(pr.read(v)); CHECK2(v, (j + k + 2));
	CHECK(pr.read(v)); CHECK2(v, (j + k + 4));
	CHECK(pr.read(v)); CHECK2(v, (j + k + 8));
	CHECK(pr.read(v)); CHECK2(v, (j + k * k));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k)));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k + 1)));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k + 2)));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k + 4)));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k + 8)));
	CHECK(nr.read(v)); CHECK2(v, (-(j + k * k)));
      }
    }
  }
}

int main()
{
  using namespace ZdfCompress;
  test<Reader, Writer>();
  test<DeltaReader<>, DeltaWriter<>>();
  test<DeltaReader<DeltaReader<>>, DeltaWriter<DeltaWriter<>>>();
}
