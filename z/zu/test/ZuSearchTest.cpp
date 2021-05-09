//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <iostream>

#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuSort.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main(int argc, char **argv)
{
  if (argc != 2) {
    std::cerr << "usage: ZuSearchTest N\n" << std::flush;
    exit(1);
  }

  ZuArrayN<int, 10> foo{1, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
  ZuArrayN<int, 10> bar{1, 1, 1, 1, 1, 1, 1, 1, 1, 9 };

  int j = atoi(argv[1]);
  unsigned i;

  i = ZuInterSearch<false>(&foo[0], 10, [j](int i) { return j - i; });
  i = ZuSearchPos(i);
  std::cout << i << '\n';

  i = ZuInterSearch<false>(&bar[0], 10, [j](int i) { return j - i; });
  i = ZuSearchPos(i);
  std::cout << i << '\n';
}
