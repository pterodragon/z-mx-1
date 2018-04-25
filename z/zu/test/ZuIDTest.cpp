//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ZuID.hpp>
#include <ZuStringN.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

static void test(const char *s)
{
  puts(s);
  unsigned n = strlen(s);
  if (n > 8) n = 8;
  ZuID a(s);
  printf("%u %u\n", n, a.length());
  CHECK(a.length() == n);
  CHECK(!memcmp(a.data(), s, n));
  CHECK(a.string() == ZuString(s, n));
  ZuStringN<9> b; b << a;
  CHECK(a.string() == b);
}

int main()
{
  test("a");
  test("ab");
  test("abc");
  test("abcd");
  test("abcde");
  test("abcdef");
  test("abcdefg");
  test("abcdefgh");
  test("abcdefghi");
}
