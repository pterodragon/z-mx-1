//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>

#include <zlib/ZuSeries.hpp>

// #define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))
#define CHECK(x) ((x) ? 0 : puts("NOK " #x))

template <typename Reader, typename Writer>
void test() {
  uint8_t p[512], n[512];
  for (int64_t i = 0; i < 63; i++) {
    int64_t j = 1; j <<= i;
    {
      Writer pw{p, p + 512};
      Writer nw{n, n + 512};
      for (int64_t k = 0; k < 10; k++) {
	for (unsigned l = 0; l < 10; l++) pw.write(j + k);
	pw.write(j + k + 1);
	pw.write(j + k + 2);
	pw.write(j + k + 4);
	pw.write(j + k + 8);
	pw.write(j + k * k);
	nw.write(-(j + k));
	nw.write(-(j + k + 1));
	nw.write(-(j + k + 2));
	nw.write(-(j + k + 4));
	nw.write(-(j + k + 8));
	nw.write(-(j + k * k));
      }
    }
    {
      Reader pr{p, p + 512};
      Reader nr{n, n + 512};
      int64_t v;
      for (int64_t k = 0; k < 10; k++) {
	for (unsigned l = 0; l < 10; l++) {
	  CHECK(pr.read(v)); CHECK(v == (j + k));
	}
	CHECK(pr.read(v)); CHECK(v == (j + k + 1));
	CHECK(pr.read(v)); CHECK(v == (j + k + 2));
	CHECK(pr.read(v)); CHECK(v == (j + k + 4));
	CHECK(pr.read(v)); CHECK(v == (j + k + 8));
	CHECK(pr.read(v)); CHECK(v == (j + k * k));
	CHECK(nr.read(v)); CHECK(v == (-(j + k)));
	CHECK(nr.read(v)); CHECK(v == (-(j + k + 1)));
	CHECK(nr.read(v)); CHECK(v == (-(j + k + 2)));
	CHECK(nr.read(v)); CHECK(v == (-(j + k + 4)));
	CHECK(nr.read(v)); CHECK(v == (-(j + k + 8)));
	CHECK(nr.read(v)); CHECK(v == (-(j + k * k)));
      }
    }
  }
}

int main()
{
  using namespace ZuSeries;
  // test<Reader, Writer>();
  test<DeltaReader, DeltaWriter>();
  // test<Delta2Reader, Delta2Writer>();
}
