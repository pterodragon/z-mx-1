//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <zlib/ZuInt.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuBox.hpp>
#include <zlib/ZuStringN.hpp>

#include <string>
#include <sstream>
#include <iostream>

template <typename V>
void fail(unsigned line, const char *s, const V &v)
{
  std::cerr << "FAIL: " << line << ':' << s << " - " << v << '\n';
#ifndef _WIN32
  ::_exit(1);
#else
  ::ExitProcess(1);
#endif
}

template <typename V1, typename V2>
void fail2(unsigned line, const char *s, const V1 &v1, const V2 &v2)
{
  std::cerr << "FAIL: " << line << ':' << s << " - " << v1 << ' ' << v2 << '\n';
#ifndef _WIN32
  ::_exit(1);
#else
  ::ExitProcess(1);
#endif
}

#define CHECK(x, v) ((x) ? (void)0 : (void)fail(__LINE__, #x, v))
#define CHECK2(x, v1, v2) ((x) ? (void)0 : (void)fail2(__LINE__, #x, v1, v2))

template <class Fmt, class Boxed> struct VFmt_ {
  static void _(ZuVFmt &fmt) {
    if (Fmt::Alt_ == 1) fmt.alt();
    if (Fmt::Comma_ != '\0') fmt.comma(Fmt::Comma_);
    if (Fmt::Hex_ == 1) fmt.hex(Fmt::Upper_);
  }
};
template <class Fmt, class Boxed,
  int Justification = Fmt::Justification_> struct VFmt;
template <class Fmt, class Boxed> struct VFmt<Fmt, Boxed, ZuFmt::Just::None> {
  static ZuVFmt _() {
    ZuVFmt fmt;
    fmt.fp(Fmt::NDP_, Fmt::Trim_);
    VFmt_<Fmt, Boxed>::_(fmt);
    return fmt;
  }
};
template <class Fmt, class Boxed> struct VFmt<Fmt, Boxed, ZuFmt::Just::Left> {
  static ZuVFmt _() {
    ZuVFmt fmt;
    fmt.left(Fmt::Width_, Fmt::Pad_);
    VFmt_<Fmt, Boxed>::_(fmt);
    return fmt;
  }
};
template <class Fmt, class Boxed> struct VFmt<Fmt, Boxed, ZuFmt::Just::Right> {
  static ZuVFmt _() {
    ZuVFmt fmt;
    fmt.right(Fmt::Width_, Fmt::Pad_);
    VFmt_<Fmt, Boxed>::_(fmt);
    return fmt;
  }
};
template <class Fmt, class Boxed> struct VFmt<Fmt, Boxed, ZuFmt::Just::Frac> {
  static ZuVFmt _() {
    ZuVFmt fmt;
    fmt.frac(Fmt::NDP_, Fmt::Trim_);
    VFmt_<Fmt, Boxed>::_(fmt);
    return fmt;
  }
};

template <class Fmt, typename S>
void test_std_string2(const char *type, const S &s, const char *s_)
{
  printf("%s %d '%c' %.*s %s\n",
      type, (int)Fmt::Width_, (int)Fmt::Comma_,
      ZuTraits<S>::length(s), ZuTraits<S>::data(s), s_);
  CHECK2(!strcmp(s.c_str(), s_), s.c_str(), s_);
}

template <typename T, class Fmt, typename S>
void test_std_string1(const char *type, T v, const char *s_)
{
  ZuBox<T> b = v;
  ZuVFmt vfmt = VFmt<Fmt, ZuBox<T> >::_();
  { S s; s += b.fmt(Fmt()); test_std_string2<Fmt, S>(type, s, s_); }
  { S s; s += b.vfmt(vfmt); test_std_string2<Fmt, S>(type, s, s_); }
}

template <class Fmt, typename S>
void test_std_stream2(const char *type, S &s, const char *s_)
{
  char buf[64];
  buf[s.rdbuf()->sgetn(buf, 63)] = 0;
  printf("%s %d '%c' %s %s\n",
      type, (int)Fmt::Width_, (int)Fmt::Comma_, buf, s_);
  CHECK2(!strcmp(buf, s_), buf, s_);
}

template <typename T, class Fmt, typename S>
void test_std_stream1(const char *type, T v, const char *s_)
{
  ZuBox<T> b = v;
  ZuVFmt vfmt = VFmt<Fmt, ZuBox<T> >::_();
  { S s; s << b.fmt(Fmt()); test_std_stream2<Fmt, S>(type, s, s_); }
  { S s; s << b.vfmt(vfmt); test_std_stream2<Fmt, S>(type, s, s_); }
}

template <typename T, class Fmt>
void test_std(T v, const char *s)
{
  test_std_string1<T, Fmt, std::string>("std::string", v, s);
  test_std_stream1<T, Fmt, std::stringstream>("std::stringstream", v, s);
}

template <typename T, class Fmt>
void test(const char *type, T v, const char *s)
{
  puts(type);
  test_std<T, Fmt>(v, s);
  test_std<T, Fmt>(v, s);
  ZuBox<T> i;
  CHECK(!*i, i);
  i = v;
  CHECK(*i, i);
  CHECK(i > ZuBox<T>(), i);
  CHECK(ZuBox<T>((T)-i) > ZuBox<T>(), i);
  CHECK2(i > (v - 1), i, v);
  ZuVFmt vfmt = VFmt<Fmt, ZuBox<T> >::_();
  char buf[64], buf2[64];
  printf("%d %d %d\n", (int)strlen(s),
      (int)i.fmt(Fmt()).length(),
      (int)i.vfmt(vfmt).length());
  CHECK2(i.fmt(Fmt()).length() >= strlen(s), i, s);
  CHECK2(i.vfmt(vfmt).length() >= strlen(s), i, s);
  buf[i.fmt(Fmt()).print(buf)] = 0;
  buf2[i.vfmt(vfmt).print(buf2)] = 0;
  printf("%s %s %s\n", s, buf, buf2);
  CHECK2(!strcmp(s, buf), s, buf);
  CHECK2(!strcmp(s, buf2), s, buf2);
  ZuBox<T> j(Fmt(), buf, strlen(buf));
  printf("%lld %lld\n", (long long)i, (long long)j);
  CHECK2(i == j, i, j);
}

template <typename T, class Fmt>
void testf(const char *type, T v, const char *s, T d = (T)0)
{
  test_std<T, ZuFmt::Comma<',', Fmt> >(v, s);
  ZuBox<T> f;
  CHECK(!*f, f);
  f = v;
  CHECK(*f, f);
  CHECK(f > ZuBox<T>(), f);
  CHECK(ZuBox<T>(-f) > ZuBox<T>(), f);
  CHECK2(f > (v - 1), f, v);
  char buf[64], buf2[64];
  buf[f.fmt(Fmt()).print(buf)] = 0;
  buf2[f.fmt(ZuFmt::Comma<',', Fmt>()).print(buf2)] = 0;
  printf("%s %s %s\n", s, buf, buf2);
  CHECK2(!strcmp(s, buf2), s, buf2);
  ZuBox<T> g, h;
  g.scan(buf, strlen(buf));
  h.scan(buf2, strlen(buf2));
  T i = (T)strtod(buf, 0);
  printf("%.12f %.12f %.12f %.12f %.12f\n",
	 (double)f, (double)g, (double)h, (double)i, (double)d);
  CHECK2((f > g) ? ((f - g) <= d) : ((g - f) <= d), f, g);
}

int foo() { return 42; }
ZuBox<int> bar() { return ZuBox<int>(); }
ZuBox<int> bah() { return ZuBox<int>(42); }

typedef ZuBox<int> BoxedInt;

int foo2() { return 42; }
BoxedInt bar2() { return BoxedInt(); }
BoxedInt bah2() { return BoxedInt(42); }

unsigned itoa(char *buf, int i) {
  bool negative = 0;
  unsigned j;
  if (i < 0) { *buf++ = '-'; j = -i; negative = 1; }
  else j = i;
  unsigned n = 11;
  do { buf[--n] = (j % 10) + '0'; j /= 10; } while (j);
  unsigned o = 11 - n;
  if (n) memmove(buf, buf + n, o);
  buf[o] = 0;
  return o + negative;
}

int main()
{
  test<char, ZuFmt::Default>("char", 42, "42");
  test<char, ZuFmt::Right<3> >("char", 42, "042");

  test<unsigned char, ZuFmt::Default>("unsigned char", 42, "42");
  test<unsigned char, ZuFmt::Right<3> >("unsigned char", 42, "042");

  test<signed char, ZuFmt::Default>("signed char", 42, "42");
  test<signed char, ZuFmt::Right<3> >("signed char", 42, "042");
  test<signed char, ZuFmt::Left<3, '_'> >("signed char", 42, "42_");
  test<signed char, ZuFmt::Default>("signed char", -42, "-42");
  test<signed char, ZuFmt::Right<5> >("signed char", -42, "-0042");
  test<signed char, ZuFmt::Left<5, '_'> >("signed char", -42, "-42__");
  test<signed char, ZuFmt::Frac<5> >("signed char", 42, "00042");

  test<unsigned short, ZuFmt::Default>("unsigned short", 42, "42");
  test<unsigned short, ZuFmt::Right<3> >("unsigned short", 42, "042");
  test<unsigned short, ZuFmt::Right<3> >("unsigned short", 420, "420");
  test<unsigned short, ZuFmt::Right<7, '_', ZuFmt::Comma<','> > >(
      "unsigned short", 42420, "_42,420");

  test<short, ZuFmt::Default>("short", 42, "42");
  test<short, ZuFmt::Right<3> >("short", 42, "042");
  test<short, ZuFmt::Left<3, '_'> >("short", 42, "42_");
  test<short, ZuFmt::Default>("short", -42, "-42");
  test<short, ZuFmt::Right<5> >("short", -42, "-0042");
  test<short, ZuFmt::Left<5, '_'> >("short", -42, "-42__");
  test<short, ZuFmt::Frac<5> >("short", 42, "00042");
  test<short, ZuFmt::Frac<5> >("short", 420, "0042");
  test<short, ZuFmt::Frac<5, '0'> >("short", 420, "00420");
  test<short, ZuFmt::Frac<5, '_'> >("short", 420, "0042_");

  test<unsigned int, ZuFmt::Default>("unsigned int", 42, "42");
  test<unsigned int, ZuFmt::Right<3> >("unsigned int", 42, "042");
  test<unsigned int, ZuFmt::Right<3> >("unsigned int", 420, "420");

  test<int, ZuFmt::Default>("int", 42, "42");
  test<int, ZuFmt::Right<3> >("int", 42, "042");
  test<int, ZuFmt::Left<3, '_'> >("int", 42, "42_");
  test<int, ZuFmt::Default>("int", -42, "-42");
  test<int, ZuFmt::Right<5> >("int", -42, "-0042");
  test<int, ZuFmt::Left<5, '_'> >("int", -42, "-42__");
  test<int, ZuFmt::Frac<5> >("int", 42, "00042");
  test<int, ZuFmt::Frac<5> >("int", 420, "0042");
  test<int, ZuFmt::Frac<5, '0'> >("int", 420, "00420");
  test<int, ZuFmt::Frac<5, '_'> >("int", 420, "0042_");
  test<int, ZuFmt::Right<12, '_', ZuFmt::Comma<','> > >(
      "int", -1420420, "__-1,420,420");

  test<unsigned long, ZuFmt::Default>("unsigned long", 42, "42");
  test<unsigned long, ZuFmt::Right<3> >("unsigned long", 42, "042");
  test<unsigned long, ZuFmt::Right<3> >("unsigned long", 420, "420");

  test<long, ZuFmt::Default>("long", 42, "42");
  test<long, ZuFmt::Right<3> >("long", 42, "042");
  test<long, ZuFmt::Left<3, '_'> >("long", 42, "42_");
  test<long, ZuFmt::Default>("long", -42, "-42");
  test<long, ZuFmt::Right<5> >("long", -42, "-0042");
  test<long, ZuFmt::Left<5, '_'> >("long", -42, "-42__");
  test<long, ZuFmt::Frac<5> >("long", 42, "00042");
  test<long, ZuFmt::Frac<5> >("long", 420, "0042");
  test<long, ZuFmt::Frac<5, '0'> >("long", 420, "00420");
  test<long, ZuFmt::Frac<5, '_'> >("long", 420, "0042_");
  test<long, ZuFmt::Right<12, '_', ZuFmt::Comma<','> > >(
      "long", -1420420, "__-1,420,420");

  test<unsigned long long, ZuFmt::Default>("unsigned long long", 42, "42");
  test<unsigned long long, ZuFmt::Right<3> >("unsigned long long", 42, "042");
  test<unsigned long long, ZuFmt::Right<3> >("unsigned long long", 420, "420");

  test<long long, ZuFmt::Default>("long long", 42, "42");
  test<long long, ZuFmt::Right<3> >("long long", 42, "042");
  test<long long, ZuFmt::Left<3, '_'> >("long long", 42, "42_");
  test<long long, ZuFmt::Default>("long long", -42, "-42");
  test<long long, ZuFmt::Right<5> >("long long", -42, "-0042");
  test<long long, ZuFmt::Left<5, '_'> >("long long", -42, "-42__");
  test<long long, ZuFmt::Frac<5> >("long long", 42, "00042");
  test<long long, ZuFmt::Frac<5> >("long long", 420, "0042");
  test<long long, ZuFmt::Frac<5, '0'> >("long long", 420, "00420");
  test<long long, ZuFmt::Frac<5, '_'> >("long long", 420, "0042_");
  test<long long, ZuFmt::Right<22, '_', ZuFmt::Comma<','> > >(
      "long long", -14242012345678, "___-14,242,012,345,678");
  test<long long, ZuFmt::Left<22, '_', ZuFmt::Comma<','> > >(
      "long long", -14242012345678, "-14,242,012,345,678___");
  test<long long, ZuFmt::Frac<19, '_'> >(
      "long long", 14242012345000, "0000014242012345___");

  {
    ZuBox<int> i;
    i.scan("-", 1); CHECK(!i, i);
    i.scan("-0", 2); CHECK(!i, i);
    i.scan("0", 1); CHECK(!i, i);
    i.scan("420", 0); CHECK(!*i, i);
    i.scan("420", 2); CHECK(i == 42, i);
    i.scan("420", 2); CHECK(i == 42, i);
    char buf[256];
    i = 0;
    buf[i.print(buf)] = 0;
    CHECK(!strcmp(buf, "0"), buf);
  }

  // 7-8 SD
  testf<float, ZuFmt::FP<-2> >("float", (float)16777216, "16,777,216");
  testf<float, ZuFmt::FP<2> >("float", (float)16777216, "16,777,216.00");
  testf<float, ZuFmt::FP<> >("float", 0.0F, "0");
  testf<float, ZuFmt::FP<> >("float", 42.0F, "42");
  testf<float, ZuFmt::FP<-2> >("float", 42.0F, "42");
  testf<float, ZuFmt::FP<-2> >("float", 42.004F, "42", 42.004F - 42.0F);
  testf<float, ZuFmt::FP<-2> >("float", 42.006F, "42.01", 42.01F - 42.006F);
  testf<float, ZuFmt::FP<2, '0'> >("float", 42.004F, "42.00", 42.004F - 42.0F);
  testf<float, ZuFmt::FP<2, '0'> >("float", 42.006F, "42.01", 42.01F - 42.006F);
  testf<float, ZuFmt::FP<> >("float", 42.000004F, "42.000004");
  testf<float, ZuFmt::FP<> >("float", 42.000006F, "42.000008");
  testf<float, ZuFmt::FP<-2> >("float", .42F, "0.42");
  testf<float, ZuFmt::FP<-3> >("float", .42F, "0.42");
  testf<float, ZuFmt::FP<> >("float", -42.0F, "-42");
  testf<float, ZuFmt::FP<-2> >("float", -42.0F, "-42");
  testf<float, ZuFmt::FP<-2> >("float", -42.004F, "-42", -42.0F - -42.004F);
  testf<float, ZuFmt::FP<-2> >("float", -42.006F, "-42.01", -42.006F - -42.01F);
  testf<float, ZuFmt::FP<-2> >("float", -.42F, "-0.42");
  testf<float, ZuFmt::FP<-3> >("float", -.42F, "-0.42");
  testf<float, ZuFmt::FP<-3> >("float", 100.0001F, "100", 100.0001F - 100.0F);
  testf<float, ZuFmt::FP<-3> >("float", 1100.101F, "1,100.101");
  testf<float, ZuFmt::FP<3, '0'> >("float", 100.0001F, "100.000", 100.0001F - 100.0F);
  testf<float, ZuFmt::FP<3, '0'> >("float", 1100.101F, "1,100.101");
  testf<float, ZuFmt::FP<> >("float", 100.0001F, "100.0001");
  testf<float, ZuFmt::FP<> >("float", 1100.101F, "1,100.101");
  testf<float, ZuFmt::FP<0, '0'> >("float", 4200100.0F, "4,200,100", 1.0F);
  // 16 SD
  testf<double, ZuFmt::FP<> >("double", 0.0, "0");
  testf<double, ZuFmt::FP<> >("double", 42.0, "42");
  testf<double, ZuFmt::FP<-2> >("double", 42.0, "42");
  testf<double, ZuFmt::FP<-2> >("double", 42.004, "42", 42.004 - 42.0);
  testf<double, ZuFmt::FP<-2> >("double", 42.006, "42.01", 42.01 - 42.006);
  testf<double, ZuFmt::FP<2, '0'> >("double", 42.004, "42.00", 42.004 - 42.0);
  testf<double, ZuFmt::FP<2, '0'> >("double", 42.006, "42.01", 42.01 - 42.006);
  testf<double, ZuFmt::FP<> >("double", 42.00000000000004, "42.00000000000004");
  testf<double, ZuFmt::FP<> >("double", 42.00000000000006, "42.00000000000006");
  testf<double, ZuFmt::FP<-2> >("double", .42, "0.42");
  testf<double, ZuFmt::FP<-3> >("double", .42, "0.42");
  testf<double, ZuFmt::FP<> >("double", -42.0, "-42");
  testf<double, ZuFmt::FP<-2> >("double", -42.0, "-42");
  testf<double, ZuFmt::FP<-2> >("double", -42.004, "-42", .004);
  testf<double, ZuFmt::FP<-2> >("double", -42.006, "-42.01", .004);
  testf<double, ZuFmt::FP<-2> >("double", -.42, "-0.42");
  testf<double, ZuFmt::FP<-3> >("double", -.42, "-0.42");
  testf<double, ZuFmt::FP<-6> >("double", 100.0000001, "100", 100.0000001 - 100.0);
  testf<double, ZuFmt::FP<-6> >("double", 1100.1000001, "1,100.1", 1100.1000001 - 1100.1);
  testf<double, ZuFmt::FP<6, '0'> >("double", 100.0000001, "100.000000", 100.0000001 - 100.0);
  testf<double, ZuFmt::FP<6, '0'> >("double", 1100.1000001, "1,100.100000", 1100.1000001 - 1100.1);
  testf<double, ZuFmt::FP<> >("double", 100.0000000000001, "100.0000000000001");
  testf<double, ZuFmt::FP<> >("double", 1100.100000000001, "1,100.100000000001");
  testf<double, ZuFmt::FP<-6> >("double", 42000100.0000001, "42,000,100", 42000100.0000001 - 42000100.0);
  testf<double, ZuFmt::FP<-2> >("double", 41.999, "42", 42 - 41.999);
  testf<double, ZuFmt::FP<> >("double", 8.981016216, "8.981016216");
  testf<double, ZuFmt::FP<-2> >("double", 99.999, "100", 100 - 99.999);
  testf<double, ZuFmt::FP<> >("double", 0.437464744, "0.437464744");
  testf<double, ZuFmt::FP<9> >("double", 12.673215776-12.061490938, "0.611724838", .0000000001);
  // 20 SD
  testf<long double, ZuFmt::FP<> >("long double", 0.0L, "0");
  testf<long double, ZuFmt::FP<> >("long double", 42.0L, "42");
  testf<long double, ZuFmt::FP<-2> >("long double", 42.0L, "42");
  testf<long double, ZuFmt::FP<-2> >("long double", 42.004L, "42", 42.004L - 42.0L);
  testf<long double, ZuFmt::FP<-2> >("long double", 42.006L, "42.01", 42.01L - 42.006L);
  testf<long double, ZuFmt::FP<2, '0'> >("long double", 42.004L, "42.00", 42.004L - 42.0L);
  testf<long double, ZuFmt::FP<2, '0'> >("long double", 42.006L, "42.01", 42.01L - 42.006L);
  testf<long double, ZuFmt::FP<-2> >("long double", .42L, "0.42");
  testf<long double, ZuFmt::FP<-3> >("long double", .42L, "0.42");
  testf<long double, ZuFmt::FP<> >("long double", -42.0L, "-42");
  testf<long double, ZuFmt::FP<-2> >("long double", -42.0L, "-42");
  testf<long double, ZuFmt::FP<-2> >("long double", -42.004L, "-42", 42.004L - 42.0L);
  testf<long double, ZuFmt::FP<-2> >("long double", -42.006L, "-42.01", 42.01L - 42.006L);
  testf<long double, ZuFmt::FP<-2> >("long double", -.42L, "-0.42");
  testf<long double, ZuFmt::FP<-3> >("long double", -.42L, "-0.42");
  testf<long double, ZuFmt::FP<-6> >("long double", 100.0000001L, "100", 100.0000001L - 100.0L);
  testf<long double, ZuFmt::FP<-6> >("long double", 1100.1000001L, "1,100.1", 1100.1000001L - 1100.1L);
  testf<long double, ZuFmt::FP<6, '0'> >("long double", 100.0000001L, "100.000000", 100.0000001L - 100.0L);
  testf<long double, ZuFmt::FP<6, '0'> >("long double", 1100.1000001L, "1,100.100000", 1100.1000001L - 1100.1L);
  testf<long double, ZuFmt::FP<> >("long double", 100.00000000000000001L, "100.00000000000000001");
  testf<long double, ZuFmt::FP<> >("long double", 1100.100000000001L, "1,100.100000000001");
  testf<long double, ZuFmt::FP<-6> >("long double", 42000100.0000001L, "42,000,100", 42000100.0000001L - 42000100.0L);

  //testf<long double, ZuFmt::FP<-1, '\0', ZuFmt::Right<10, '_'> > >("long double", -1100.100000000001L, "____-1,100.100000000001");

  {
    ZuBox<double> f;
    f.scan(".", 1); CHECK(!(int)f, f);
    f.scan("-", 1); CHECK(!*f, f);
    f.scan("-0", 2); CHECK(!(int)f, f);
    f.scan("-.", 2); CHECK(!(int)f, f);
    f.scan("0", 1); CHECK(!(int)f, f);
    f.scan("42.001", 6); CHECK((int)(f * 1000) == 42001, f);
    f.scan("42.001", 0); CHECK(ZuCmp<double>::null(f), f);
    f.scan("42.001", 5); CHECK((int)(f * 1000) == 42000, f);
    f.scan("42.001", 6); CHECK((int)(f * 1000) == 42001, f);
    f.scan((const char *)0, -1); CHECK(ZuCmp<double>::null(f), f);
    f.scan("", -1); CHECK(ZuCmp<double>::null(f), f);
    f.scan("nan", -1); CHECK(ZuCmp<double>::null(f), f);
    f.scan((const char *)0, 0); CHECK(ZuCmp<double>::null(f), f);
    f.scan("", 0); CHECK(ZuCmp<double>::null(f), f);
    f.scan("nan", 3); CHECK(ZuCmp<double>::null(f), f);
    f.scan("inf", 3); CHECK(ZuCmp<double>::inf(f), f);
    f.scan("-inf", 4); CHECK(ZuCmp<double>::inf(-f), f);
    f.scan("inf", 3); CHECK(ZuCmp<double>::inf(f), f);
    f.scan("-inf", 4); CHECK(ZuCmp<double>::inf(-f), f);
    CHECK(!f.scan("inf", 2), f);
    CHECK(!f.scan("nan", 2), f);
    char buf[256];
    f = 0;
    buf[f.print(buf)] = 0;
    CHECK(!strcmp(buf, "0"), buf);
  }

  {
    int i;
    ZuBox<int> j;
    BoxedInt k;
    i = foo();
    printf("%d\n", i);
    j = i, i = j++, ++j;
    CHECK2(j == i + 2, i, j);
    j = i, i = j--, --j;
    CHECK2(j == i - 2, i, j);
    i = bar();
    printf("%d\n", i);
    i = bah();
    printf("%d\n", (int)(j = ZuBoxed(i)));
    j = foo();
    printf("%d\n", (int)j);
    j = bar();
    printf("%d\n", (int)j);
    j = bah();
    printf("%d\n", (int)j);
    k = foo();
    printf("%d\n", (int)k);
    k = bar();
    printf("%d\n", (int)k);
    k = bah();
    printf("%d\n", (int)k);
    i = foo2();
    printf("%d\n", i);
    i = bar2();
    printf("%d\n", i);
    i = bah2();
    printf("%d\n", (int)(j = ZuBoxed(i)));
    j = foo2();
    printf("%d\n", (int)j);
    j = bar2();
    printf("%d\n", (int)j);
    j = bah2();
    printf("%d\n", (int)j);
    k = foo2();
    printf("%d\n", (int)k);
    k = bar2();
    printf("%d\n", (int)k);
    k = bah2();
    printf("%d\n", (int)k);
  }

  {
    int x = 42;
    std::cout << "Hex (4, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Right<4> >()) << '\n';
    std::cout << "Hex (3, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Right<3> >()) << '\n';
    std::cout << "Hex (2, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Right<2> >()) << '\n';
    std::cout << "Hex (1, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Right<1> >()) << '\n';
    std::cout << "Hex (-1, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Left<1> >()) << '\n';
    std::cout << "Hex (-2, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Left<2> >()) << '\n';
    std::cout << "Hex (-3, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Left<3> >()) << '\n';
    std::cout << "Hex (-4, uppercase, int) 42: " << ZuBoxed(x).fmt(ZuFmt::Hex<1, ZuFmt::Left<4> >()) << '\n';
  }

  {
    struct timespec start, end;

    {
      char buf[30];
      for (int i = INT_MIN; i < INT_MAX - 420; i += 420) {
	itoa(buf, i);
	CHECK2(i == atoi(buf), i, buf);
	buf[0] = 0;
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (int i = INT_MIN; i < INT_MAX - 42; i += 42) {
	itoa(buf, i);
	buf[0] = 0;
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "itoa: " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }

    {
      ZuStringN<30> buf;
      for (int i = INT_MIN; i < INT_MAX - 420; i += 420) {
	buf << ZuBoxed(i);
	CHECK2(i == ZuBox<int>(buf), i, buf.data());
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (int i = INT_MIN; i < INT_MAX - 42; i += 42) {
	buf << ZuBoxed(i);
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "ZuBox<int>::fmt(): " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }

    {
      ZuVFmt fmt;
      ZuStringN<30> buf;
      for (int i = INT_MIN; i < INT_MAX - 420; i += 420) {
	buf << ZuBoxed(i).vfmt(fmt);
	CHECK2(i == ZuBox<int>(buf), i, buf.data());
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (int i = INT_MIN; i < INT_MAX - 42; i += 42) {
	buf << ZuBoxed(i).vfmt(fmt);
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "ZuBox<int>::vfmt(): " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }

    {
      char buf[30];
      for (double d = INT_MIN; d < INT_MAX - 4201; d += 4200.000420) {
	sprintf(buf, "%f", d);
	double e = d - strtod(buf, 0);
	CHECK2(e < .00001 && e > -.00001, e, buf);
	buf[0] = 0;
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (double d = INT_MIN; d < INT_MAX - 421; d += 420.000420) {
	sprintf(buf, "%f", d);
	buf[0] = 0;
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "sprintf(\"%f\"): " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }

    {
      ZuStringN<30> buf;
      for (double d = INT_MIN; d < INT_MAX - 4201; d += 4200.000420) {
	buf << ZuBoxed(d);
	ZuBox<double> e(buf);
	// printf("d: %f buf: %.*s e: %f diff: %g, epsilon: %g\n", d, buf.length(), buf.data(), (double)e, (double)e - d, (double)ZuFP<double>::epsilon_(d));
	CHECK2(ZuBoxed(d) ==~ e, e, buf.data());
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (double d = INT_MIN; d < INT_MAX - 421; d += 420.000420) {
	buf << ZuBoxed(d);
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "ZuBox<double>::fmt(): " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }

    {
      ZuVFmt fmt;
      ZuStringN<30> buf;
      for (double d = INT_MIN; d < INT_MAX - 4201; d += 4200.000420) {
	buf << ZuBoxed(d).vfmt(fmt);
	CHECK2(ZuBoxed(d) ==~ ZuBox<double>(buf), d, buf.data());
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &start);
      for (double d = INT_MIN; d < INT_MAX - 421; d += 420.000420) {
	buf << ZuBoxed(d).vfmt(fmt);
	buf.null();
      }
      clock_gettime(CLOCK_REALTIME, &end);
      if (end.tv_nsec < start.tv_nsec) {
	end.tv_nsec = (end.tv_nsec + 1000000000L) - start.tv_nsec;
	end.tv_sec = (end.tv_sec - 1) - start.tv_sec;
      } else {
	end.tv_nsec -= start.tv_nsec;
	end.tv_sec -= start.tv_sec;
      }
      std::cout << "ZuBox<double>::vfmt(): " <<
	ZuBoxed(end.tv_sec) << '.' << ZuBoxed(end.tv_nsec).fmt(ZuFmt::Frac<9>()) << '\n';
    }
  }
}
