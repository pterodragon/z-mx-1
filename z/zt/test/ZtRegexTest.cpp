//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZuLib.hpp>

#include <stdio.h>

#include <zlib/ZtString.hpp>
#include <zlib/ZtRegex.hpp>

int main()
{
  ZtString x = "/foo/bar/bah/leaf";
  const auto &leafName = ZtStaticRegexUTF8("[^/]+$");
  const auto &separator = ZtStaticRegexUTF8("/");
  const auto &nullSpace = ZtStaticRegexUTF8("\\s*");
  ZtRegex::Captures c;
  printf("x is \"%s\"\n", x.data());
  int i = leafName.m(x, c);
  printf("m/[^\\/]+$/ returned %d\n", i);
  printf("c.length() is %d\n", c.length());
  int n = c.length();
  for (i = 0; i < n; i++)
    printf("c[%d] = \"%.*s\"\n", i, c[i].length(), c[i].data());
  i = separator.sg(x, "/");
  printf("s/\\//\\//g returned %d\n", i);
  printf("x is \"%s\"\n", x.data());
  c.length(0);
  i = separator.split(x, c);
  printf("split /\\// returned %d\n", i);
  printf("c.length() is %d\n", c.length());
  n = c.length();
  for (i = 0; i < n; i++)
    printf("c[%d] = \"%.*s\"\n", i, c[i].length(), c[i].data());
  i = nullSpace.sg(x, "");
  printf("s/\\s*//g returned %d\n", i);
  printf("x is \"%s\"\n", x.data());
  c.length(0);
  i = nullSpace.split(x, c);
  printf("split /\\s*/ returned %d\n", i);
  printf("c.length() is %d\n", c.length());
  n = c.length();
  for (i = 0; i < n; i++)
    printf("c[%d] = \"%.*s\"\n", i, c[i].length(), c[i].data());
  
  {
    const auto &r = ZtStaticRegexUTF8("\\w+\\s+(?<name>\\w+)\\s+(?<age>\\d+)");
    ZtRegex::Captures captures;
    int name = r.index("name");
    int age = r.index("age");

    int i = r.m("foo Joe 42", captures);
    if (i >= 3) {
      ZtString out;
      out << "name=" << captures[name] << ", age=" << captures[age];
      puts(out);
    }
  }
}
