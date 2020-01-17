//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZtLib.hpp>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <zlib/ZtDate.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

struct Null {
  template <typename S> void print(S &s) const { s << "null"; }
};
template <> struct ZuPrint<Null> : public ZuPrintFn { };

void test(ZtDate d1)
{
  ZuStringN<32> fix;
  ZtDate::FIXFmt<-9, Null> fmt;
  fix << d1.fix(fmt);
  puts(fix);
  ZtDate d2(ZtDate::FIX, fix);
  puts(ZuStringN<32>() << d2.fix(fmt));
  CHECK(d1 == d2);
}

int main(int argc, char **argv)
{
  test(ZtDate((time_t)0));
  test(ZtDate(1, 1, 1));
  test(ZtDateNow());

  if (argc < 2) { fputs("usage: ZtDateFixTest N\n", stderr); ZmPlatform::exit(1); }
  unsigned n = atoi(argv[1]);
  {
    ZtDate::FIXFmt<-9, Null> fmt;
    ZuStringN<32> fix;
    ZmTime start, end;
    start.now();
    for (unsigned i = 0; i < n; i++) {
      ZtDate d1(ZtDate::Now);
      fix << d1.fix(fmt);
      ZtDate d2(ZtDate::FIX, fix);
    }
    end.now();
    end -= start;
    double d1 = end.dtime() / (double)n;
    printf("time per cycle 1: %.9f\n", d1);

    start.now();
    for (unsigned i = 0; i < n; i++) {
      ZtDate d1(ZtDate::Now);
      fix << d1.fix(fmt);
    }
    end.now();
    end -= start;
    double d2 = end.dtime() / (double)n;
    printf("time per cycle 2: %.9f\n", d2);

    start.now();
    for (unsigned i = 0; i < n; i++) {
      ZtDate d1(ZtDate::Now);
    }
    end.now();
    end -= start;
    double d3 = end.dtime() / (double)n;
    printf("time per cycle 3: %.9f\n", d3);

    printf("time per FIX format print: %.9f\n", d2 - d3);
    printf("time per FIX format scan: %.9f\n", d1 - d2);
  }
}
