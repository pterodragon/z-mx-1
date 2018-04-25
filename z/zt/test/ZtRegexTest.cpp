//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>

#include <ZtString.hpp>
#include <ZtRegex.hpp>

int main()
{
  ZtString x = "/foo/bar/bah/leaf";
  static ZtRegex leafName("[^/]+$", PCRE_UTF8);
  static ZtRegex separator("/", PCRE_UTF8);
  static ZtRegex nullSpace("\\s*", PCRE_UTF8);
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
    static ZtRegex re("\\w+\\s+(?<name>\\w+)\\s+(?<age>\\d+)", PCRE_UTF8);
    ZtRegex::Captures captures;
    int name = re.index("name");
    int age = re.index("age");

    int i = re.m("foo Joe 42", captures);
    if (i >= 3) {
      ZtString out;
      out << "name=" << captures[name] << ", age=" << captures[age];
      puts(out);
    }
  }
}
