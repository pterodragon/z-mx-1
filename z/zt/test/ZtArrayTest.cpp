//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>
#include <assert.h>

#include <ZtArray.hpp>
#include <ZtString.hpp>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

struct E {
  E() : m_ptr((void *)this), m_copied(0) { }
  ~E() { m_ptr = 0; }

  E(const E &e) : m_ptr((void *)this), m_copied(e.m_copied + 1) { }

  inline void *ptr() const { return m_ptr; }
  inline int copied() const { return m_copied; }

  void	*m_ptr;
  int	m_copied;
};

void validate(const ZtArray<E> &a, unsigned length)
{
  assert(a.length() == length);
  unsigned n = a.length();
  for (unsigned i = 0; i < n; i++) {
    assert(a[i].ptr() == (const void *)&a[i]);
    printf("%d:%d ", i, a[i].copied());
  }
  putc('\n', stdout);
}

struct Foo {
  inline Foo() { }
  template <typename S> inline Foo(const S &s) : bar(s) { }
  ZtString bar;
};

#include <vector>

int main()
{
  {
    E e[8];
    ZtArray<E> a;
    a = ZtArray<E>(e, 8);
    ZtArray<E> b;

    validate(a, 8);
    a.splice(0, 0, e, 4);
    validate(a, 12);
    a.splice(b, 0, 4);
    validate(a, 8);
    validate(b, 4);
    a.splice(0, 0, b);
    validate(a, 12);
    validate(b, 4);
    b.splice(8, 100, b);
    validate(b, 12);
    for (int i = 0; i < 8; i++) a.push(a.shift());
    validate(a, 12);
  }
#if 0
  {
    ZtArray<ZtString> b = (const char *[]){ "hello", "world" };
    puts(b[0]);
    puts(b[1]);
  }
#endif

  {
    std::vector<const char *> v = { { "hello", "world" } };
    ZtArray<ZuString> b = v;
    puts(b[0]);
    puts(b[1]);
  }

  {
    ZtArray<Foo> a;
    a.push(Foo("hello"));
    a.push(Foo("world"));
    puts(a[0].bar);
    puts(a[1].bar);
  }

  {
    // typedef const char *P;
    // typedef P N[];
    ZtArray<const char *> a { std::initializer_list<const char *>{ [1] = "Foo", [0] = "Bar" } };
    puts(a[0]);
    puts(a[1]);
  }

  return 0;
}
