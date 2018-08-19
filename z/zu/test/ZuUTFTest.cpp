//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ZuUTF.hpp>
#include <ZuArrayN.hpp>
#include <ZuStringN.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main()
{
  {
    ZuStringN<64> s;
    s.length(ZuUTF<char, uint16_t>::cvt(
	  ZuArray<char>(s.data(), s.size() - 1),
	  ZuArray<const uint16_t>(
	    (const uint16_t *)"H\00e\00l\00l\00o\00 \00W\00o\00r\00l\00d\00",
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
    ZuArrayN<uint16_t, 8> j;
    j.length(ZuUTF<uint16_t, uint32_t>::cvt(
	  ZuArray<uint16_t>(j.data(), j.size() - 1),
	  ZuArray<const uint32_t>(&u[0], 3)));
    CHECK(j.length() == 4);
    CHECK(j[0] == 0xd83d && j[1] == 0xdc04 && j[2] == 'x' && j[3] == 'y');
  }
}
