//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <zlib/ZuUTF.hpp>
#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuStringN.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main()
{
  {
    ZuStringN<64> s;
    s.length(ZuUTF<char, uint16_t>::cvt(s.buf(), 
	  { (const uint16_t *)"H\0e\0l\0l\0o\0 \0W\0o\0r\0l\0d\0", 11}));
    CHECK(s == "Hello World");
    s.length(ZuUTF<char, wchar_t>::cvt(s.buf(),
	  ZuWString(L"Hello World", 11)));
    CHECK(s == "Hello World");
    s = L"Hello World";
    CHECK(s == "Hello World");
    {
      ZuWStringN<64> w;
      w.length(ZuUTF<wchar_t, char>::cvt(w.buf(), s));
      CHECK(w == L"Hello World");
      w = "Hello World";
      CHECK(w == L"Hello World");
    }
  }
  {
    uint32_t u[3] = { 0x1f404, (uint32_t)'x', (uint32_t)'y' }; // cow
    {
      ZuArrayN<uint16_t, 8> j;
      j.length(ZuUTF<uint16_t, uint32_t>::cvt(j.buf(),
	    ZuArray<const uint32_t>(&u[0], 3)));
      CHECK(j.length() == 4);
      CHECK(j[0] == 0xd83d && j[1] == 0xdc04 && j[2] == 'x' && j[3] == 'y');
      CHECK(ZuUTF32::width(u[0]) == 2);
      CHECK(ZuUTF32::width(u[1]) == 1);
    }
    {
      ZuArrayN<char, 16> j;
      j.length(ZuUTF<char, uint32_t>::cvt(j.buf(), u));
      CHECK(j.length() == 6);
      CHECK(j.equals("\xf0\x9f\x90\x84xy"));
      std::cout << j << '\n';
      ZuArrayN<uint32_t, 4> k;
      k.length(ZuUTF<uint32_t, char>::cvt(k.buf(), j));
      CHECK(k.equals(u));
    }
  }
}
