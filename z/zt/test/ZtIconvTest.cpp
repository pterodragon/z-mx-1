//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <zlib/ZuLib.hpp>

#include <iostream>

#include <zlib/ZuString.hpp>
#include <zlib/ZuTraits.hpp>

#include <zlib/ZmAssert.hpp>

#include <zlib/ZtString.hpp>
#include <zlib/ZtIconv.hpp>

int main(int argc, char **argv) {
  //"UTF-8", "SHIFT_JIS"
  if (argc != 3) {
    fprintf(stderr, "usage: %s to from\n", argv[0]);
    return 1;
  }

  static ZtIconv conv(argv[1], argv[2]);

  unsigned char title_[] = {
    0x81, 0x94, 0x88, 0xc0, 0x92, 0xe8, 0x91, 0x80, 0x8d,
    0xec, 0x93, 0xcd, 0x8f, 0x6f, 0x8f, 0x91, 0x81, 0x69,
    0x8e, 0xca, 0x81, 0x6a, 0x82, 0xcc, 0x8e, 0xf3, 0x97,
    0x9d, 0 };

  unsigned char body_[] = {
    0x96, 0xc1, 0x95, 0xbf, 0x81, 0x40, 0x81, 0x40, 0x81, 0x40,
    0x81, 0x40, 0x83, 0x8a, 0x83, 0x93, 0x83, 0x4b, 0x81, 0x5b,
    0x83, 0x6e, 0x83, 0x62, 0x83, 0x67, 0x81, 0x69, 0x82, 0x57,
    0x82, 0x51, 0x82, 0x4f, 0x82, 0x4f, 0x81, 0x6a, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x81, 0x40, 0x8a, 0x4a, 0x8e, 0x6e, 0x93, 0xfa, 0x81, 0x40,
    0x81, 0x40, 0x81, 0x40, 0x82, 0x52, 0x8c, 0x8e, 0x82, 0x50,
    0x82, 0x57, 0x93, 0xfa, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x81, 0x40, 0x89, 0xbf, 0x8a, 0x69, 0x81, 0x40, 0x81, 0x40,
    0x81, 0x40, 0x81, 0x40, 0x82, 0x58, 0x82, 0x56, 0x82, 0x58,
    0x89, 0x7e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x81, 0x40, 0x8e, 0xe6, 0x88, 0xf8, 0x8e, 0x51, 0x89, 0xc1,
    0x8e, 0xd2, 0x81, 0x40, 0x91, 0xe5, 0x98, 0x61, 0x8f, 0xd8,
    0x8c, 0x94, 0x83, 0x4c, 0x83, 0x83, 0x83, 0x73, 0x83, 0x5e,
    0x83, 0x8b, 0x81, 0x45, 0x83, 0x7d, 0x81, 0x5b, 0x83, 0x50,
    0x83, 0x62, 0x83, 0x63, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x8f, 0xda, 0x8d, 0xd7, 0x82, 0xcd, 0x8f, 0x8a, 0x95, 0xf1,
    0x82, 0xf0, 0x82, 0xb2, 0x97, 0x97, 0x82, 0xad, 0x82, 0xbe,
    0x82, 0xb3, 0x82, 0xa2, 0x81, 0x42, 0 };

  ZtString title(ZuArray<char>((char *)title_, sizeof(title_)), &conv);
  ZtString body(ZuArray<char>((char *)body_, sizeof(body_)), &conv);

  std::cout << "Title: (" << title << ")\nBody : (" << body << ")\n";

  return 0;
}
