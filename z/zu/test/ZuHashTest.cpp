//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include <ZuHash.hpp>
#include <ZuCmp.hpp>

#include "Analyze.hpp"

static int count[256] = { 0 };

template <int Size> struct HiBits_;

struct HiBits {
  inline static uint32_t hashBits(uint32_t i) { return i>>24; }
};

struct LoBits {
  inline static uint32_t hashBits(uint32_t i) { return i & 0xff; }
};

template <typename T, int Size> struct Rand;
template <typename T> struct Rand<T, 1> {
  inline static T rand() {
    T t;
    uint8_t *ZuMayAlias(t_) = (uint8_t *)&t;
    *t_ = ::rand() & 0xff;
    return t;
  }
};
template <typename T> struct Rand<T, 2> {
  inline static T rand() {
    T t;
    uint16_t *ZuMayAlias(t_) = (uint16_t *)&t;
    *t_ = ::rand() & 0xffff;
    return t;
  }
};
template <typename T> struct Rand<T, 4> {
  inline static T rand() {
    T t;
    uint16_t *ZuMayAlias(t_) = (uint16_t *)&t;
    t_[0] = ::rand() & 0xffff;
    t_[1] = ::rand() & 0xffff;
    return t;
  }
};
template <typename T> struct Rand<T, 8> {
  inline static T rand() {
    T t;
    uint16_t *ZuMayAlias(t_) = (uint16_t *)&t;
    t_[0] = ::rand() & 0xffff;
    t_[1] = ::rand() & 0xffff;
    t_[2] = ::rand() & 0xffff;
    t_[3] = ::rand() & 0xffff;
    return t;
  }
};

template <typename Bits, typename T> struct IntegerTest {
  static void run(const char *s) {
    memset(count, 0, 256 * sizeof(int));

    for (int i = 0; i < (1<<16); i++)
      count[Bits::hashBits(ZuHash<T>::hash(Rand<T, sizeof(T)>::rand()))]++;
    analyze(s, count, 256);
  }
};

template <typename Bits, typename T> struct FloatTest {
  static void run(const char *s) {
    memset(count, 0, 256 * sizeof(int));

    T f = (T)RAND_MAX + 1;
    for (int i = 0; i < (1<<16); i++) {
      T g = (T)rand() / f;
      count[Bits::hashBits(ZuHash<T>::hash(g))]++;
    }
    analyze(s, count, 256);
  }
};

static char *randomString()
{
  static char characters[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
#define rand_62() (((rand() & 0xff00) * 62)>>16)
#define rand_64() ((rand() & 0x3f00)>>8)
  int l = 16 + rand_64();
  char *buf = (char *)malloc(l + 1);

  for (int i = 0; i < l; i++) buf[i] = characters[rand_62()];
  buf[l] = 0;
  return buf;
}

template <typename Bits> struct StringTest {
  static void run(const char *s) {
    memset(count, 0, 256 * sizeof(int));

    for (int i = 0; i < (1<<16); i++) {
      char *buf = randomString();
      count[Bits::hashBits(ZuHash<char *>::hash(buf))]++;
      free(buf);
    }
    analyze(s, count, 256);
  }
};

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

void testString(const char *s)
{
  char buf[16];

  buf[0] = ' ';
  strcpy(buf + 1, s);
  if (ZuHash<char *>::hash(s) != ZuHash<char *>::hash(buf + 1) ||
      ZuHash<char *>::hash(s + 1) != ZuHash<char *>::hash(buf + 2)) {
    printf("Failed to hash \"%s\" to identical values\n", s);
#ifndef _WIN32
    ::_exit(1);
#else
    ::ExitProcess(1);
#endif
  }
}

int main()
{
  srand((unsigned int)time(0));
  IntegerTest<HiBits, char>::run("Hi char");
  IntegerTest<LoBits, char>::run("Lo char");
  IntegerTest<HiBits, unsigned char>::run("Hi unsigned char");
  IntegerTest<LoBits, unsigned char>::run("Lo unsigned char");
  IntegerTest<HiBits, signed char>::run("Hi signed char");
  IntegerTest<LoBits, signed char>::run("Lo signed char");
  IntegerTest<HiBits, short>::run("Hi short");
  IntegerTest<LoBits, short>::run("Lo short");
  IntegerTest<HiBits, unsigned short>::run("Hi unsigned short");
  IntegerTest<LoBits, unsigned short>::run("Lo unsigned short");
  IntegerTest<HiBits, int>::run("Hi int");
  IntegerTest<LoBits, int>::run("Lo int");
  IntegerTest<HiBits, unsigned int>::run("Hi unsigned int");
  IntegerTest<LoBits, unsigned int>::run("Lo unsigned int");
  IntegerTest<HiBits, long>::run("Hi long");
  IntegerTest<LoBits, long>::run("Lo long");
  IntegerTest<HiBits, unsigned long>::run("Hi unsigned long");
  IntegerTest<LoBits, unsigned long>::run("Lo unsigned long");
  IntegerTest<HiBits, long long>::run("Hi long long");
  IntegerTest<LoBits, long long>::run("Lo long long");
  IntegerTest<HiBits, unsigned long long>::run("Hi unsigned long long");
  IntegerTest<LoBits, unsigned long long>::run("Lo unsigned long long");
  IntegerTest<HiBits, wchar_t>::run("Hi wchar_t");
  IntegerTest<LoBits, wchar_t>::run("Lo wchar_t");
  FloatTest<HiBits, float>::run("Hi float");
  FloatTest<LoBits, float>::run("Lo float");
  FloatTest<HiBits, double>::run("Hi double");
  FloatTest<LoBits, double>::run("Lo double");
  FloatTest<HiBits, long double>::run("Hi long double");
  FloatTest<LoBits, long double>::run("Lo long double");
  StringTest<HiBits>::run("Hi string");
  StringTest<LoBits>::run("Lo string");
  testString("f");
  testString("fo");
  testString("foo");
  testString("foob");
  testString("fooba");
  testString("foobar");
  testString("foobar!");
  testString("foobar!!");
}
