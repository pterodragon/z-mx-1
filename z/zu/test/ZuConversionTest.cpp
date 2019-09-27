//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZuLib.hpp>

#include <stdio.h>

#include <zlib/ZuConversion.hpp>

struct A { };
struct B : public A { };
struct C { operator A() { return A(); } };

int main()
{
  printf("ZuConversion<void, void>::Exists %d\n", (int)ZuConversion<void, void>::Exists);
  printf("ZuConversion<void, void>::Same %d\n", (int)ZuConversion<void, void>::Same);
  printf("ZuConversion<void, void>::Base %d\n", (int)ZuConversion<void, void>::Base);
  printf("ZuConversion<void, A>::Exists %d\n", (int)ZuConversion<void, A>::Exists);
  printf("ZuConversion<void, A>::Same %d\n", (int)ZuConversion<void, A>::Same);
  printf("ZuConversion<void, A>::Base %d\n", (int)ZuConversion<void, A>::Base);
  printf("ZuConversion<A, void>::Exists %d\n", (int)ZuConversion<A, void>::Exists);
  printf("ZuConversion<A, void>::Same %d\n", (int)ZuConversion<A, void>::Same);
  printf("ZuConversion<A, void>::Base %d\n", (int)ZuConversion<A, void>::Base);
  printf("ZuConversion<void *, void *>::Exists %d\n", (int)ZuConversion<void *, void *>::Exists);
  printf("ZuConversion<void *, void *>::Same %d\n", (int)ZuConversion<void *, void *>::Same);
  printf("ZuConversion<void *, void *>::Base %d\n", (int)ZuConversion<void *, void *>::Base);
  printf("ZuConversion<A *, void *>::Exists %d\n", (int)ZuConversion<A *, void *>::Exists);
  printf("ZuConversion<A *, void *>::Same %d\n", (int)ZuConversion<A *, void *>::Same);
  printf("ZuConversion<A *, void *>::Base %d\n", (int)ZuConversion<A *, void *>::Base);
  printf("ZuConversion<void *, A *>::Exists %d\n", (int)ZuConversion<void *, A *>::Exists);
  printf("ZuConversion<void *, A *>::Same %d\n", (int)ZuConversion<void *, A *>::Same);
  printf("ZuConversion<void *, A *>::Base %d\n", (int)ZuConversion<void *, A *>::Base);
  printf("ZuConversion<A, A>::Exists %d\n", (int)ZuConversion<A, A>::Exists);
  printf("ZuConversion<A, A>::Same %d\n", (int)ZuConversion<A, A>::Same);
  printf("ZuConversion<A, A>::Base %d\n", (int)ZuConversion<A, A>::Base);
  printf("ZuConversion<A, B>::Exists %d\n", (int)ZuConversion<A, B>::Exists);
  printf("ZuConversion<A, B>::Same %d\n", (int)ZuConversion<A, B>::Same);
  printf("ZuConversion<A, B>::Base %d\n", (int)ZuConversion<A, B>::Base);
  printf("ZuConversion<B, A>::Exists %d\n", (int)ZuConversion<B, A>::Exists);
  printf("ZuConversion<B, A>::Same %d\n", (int)ZuConversion<B, A>::Same);
  printf("ZuConversion<B, A>::Base %d\n", (int)ZuConversion<B, A>::Base);
  printf("ZuConversion<A, C>::Exists %d\n", (int)ZuConversion<A, C>::Exists);
  printf("ZuConversion<A, C>::Same %d\n", (int)ZuConversion<A, C>::Same);
  printf("ZuConversion<A, C>::Base %d\n", (int)ZuConversion<A, C>::Base);
  printf("ZuConversion<C, A>::Exists %d\n", (int)ZuConversion<C, A>::Exists);
  printf("ZuConversion<C, A>::Same %d\n", (int)ZuConversion<C, A>::Same);
  printf("ZuConversion<C, A>::Base %d\n", (int)ZuConversion<C, A>::Base);
  printf("ZuConversion<A *, A *>::Exists %d\n", (int)ZuConversion<A *, A *>::Exists);
  printf("ZuConversion<A *, A *>::Same %d\n", (int)ZuConversion<A *, A *>::Same);
  printf("ZuConversion<A *, A *>::Base %d\n", (int)ZuConversion<A *, A *>::Base);
  printf("ZuConversion<A *, B *>::Exists %d\n", (int)ZuConversion<A *, B *>::Exists);
  printf("ZuConversion<A *, B *>::Same %d\n", (int)ZuConversion<A *, B *>::Same);
  printf("ZuConversion<A *, B *>::Base %d\n", (int)ZuConversion<A *, B *>::Base);
  printf("ZuConversion<B *, A *>::Exists %d\n", (int)ZuConversion<B *, A *>::Exists);
  printf("ZuConversion<B *, A *>::Same %d\n", (int)ZuConversion<B *, A *>::Same);
  printf("ZuConversion<B *, A *>::Base %d\n", (int)ZuConversion<B *, A *>::Base);
}
