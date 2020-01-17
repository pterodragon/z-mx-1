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
    s.length(ZuUTF<char, uint16_t>::cvt(
	  ZuArray<char>(s.data(), s.size() - 1),
	  ZuArray<const uint16_t>(
	    (const uint16_t *)"H\0e\0l\0l\0o\0 \0W\0o\0r\0l\0d\0",
	    11)));
    CHECK(s == "Hello World");
    s.length(ZuUTF<char, wchar_t>::cvt(
	  ZuArray<char>(s.data(), s.size() - 1),
	  ZuWString(L"Hello World", 11)));
    CHECK(s == "Hello World");
    {
      ZuWStringN<64> w;
      w.length(ZuUTF<wchar_t, char>::cvt(
	    ZuArray<wchar_t>(w.data(), w.size() - 1), s));
      CHECK(w == L"Hello World");
    }
  }
  {
    uint32_t u[3] = { 0x1f404, (uint32_t)'x', (uint32_t)'y' }; // cow
    {
      ZuArrayN<uint16_t, 8> j;
      j.length(ZuUTF<uint16_t, uint32_t>::cvt(
	    ZuArray<uint16_t>(j.data(), j.size() - 1),
	    ZuArray<const uint32_t>(&u[0], 3)));
      CHECK(j.length() == 4);
      CHECK(j[0] == 0xd83d && j[1] == 0xdc04 && j[2] == 'x' && j[3] == 'y');
    }
    {
      ZuArrayN<char, 16> j;
      j.length(ZuUTF<char, uint32_t>::cvt(
	    ZuArray<char>(j.data(), j.size() - 1), u));
      CHECK(j.length() == 6);
      CHECK(j == "\xf0\x9f\x90\x84xy");
      ZuArrayN<uint32_t, 4> k;
      k.length(ZuUTF<uint32_t, char>::cvt(
	    ZuArray<uint32_t>(k.data(), k.size() - 1), j));
      CHECK(k == u);
    }
  }
}
