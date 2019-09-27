//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <zlib/ZuLib.hpp>

#include <cstdio>
#include <assert.h>

#include <zlib/ZuNetwork.hpp>
#include <zlib/ZuAssert.hpp>

template <typename T>
void assertS(T value, T swapped) {
  assert(sizeof(T) == 2);
  printf("-----Testing short: %04hX\n", value);
  T converted = ZuHtons(value);
#if Zu_BIGENDIAN
  printf("          expected: %04hX\n", value);
  printf("               got: %04hX\n", converted);
  assert(converted == value);
#else
  printf("          expected: %04hX\n", swapped);
  printf("               got: %04hX\n", converted);
  assert(converted == swapped);
#endif
}

template <typename T>
void assertL(T value, T swapped) {
  assert(sizeof(T) == 4);
  printf("------Testing long: %08X\n", value);
  T converted = ZuHtonl(value);
#if Zu_BIGENDIAN
  printf("          expected: %08X\n", value);
  printf("               got: %08X\n", converted);
  assert(converted == value);
#else
  printf("          expected: %08X\n", swapped);
  printf("               got: %08X\n", converted);
  assert(converted == swapped);
#endif
}

template <typename T>
void assertLL(T value, T swapped) {
  assert(sizeof(T) == 8);
  printf("-Testing long long: %016llX\n", value);
  T converted = ZuHtonll(value);
#if Zu_BIGENDIAN
  printf("          expected: %016llX\n", value);
  printf("               got: %016llX\n", converted);
  assert(converted == value);
#else
  printf("          expected: %016llX\n", swapped);
  printf("               got: %016llX\n", converted);
  assert(converted == swapped);
#endif
}

int main(int argc, char **argv)
{
  assertS((short)0x0102, (short)0x0201);
  assertS((short)0x1020, (short)0x2010);
  assertS((short)-14477, (short)29639);

  assertL(0x01020304, 0x04030201);
  assertL(0x10203040, 0x40302010);
  assertL(-148320899, 2110597367);
  
  assertLL(0x1020304050607080ULL, 0x8070605040302010ULL);
  assertLL(0x0102030405060708ULL, 0x0807060504030201ULL);

  {
    ZuBigEndian<uint32_t> i = 42;
    ZuLittleEndian<uint32_t> j = 42;
    i = i + 1;
    i += 1;
    i = -i;
    j = j + 1;
    j += 1;
    j = -j;
    assert(*(uint32_t *)&i != *(uint32_t *)&j);
    assert(i == j);
    i++;
    assert(i > j);
    assert(j < i);
    ++j;
    assert(i == j);
    assert((uint32_t)i == (uint32_t)-43);
    assert((uint32_t)j == (uint32_t)-43);
    i = 42;
    j = 42;
    printf("i = %.2x%.2x%.2x%.2x (big-endian 42 - \"big end first\")\n",
	((uint8_t *)&i)[0], ((uint8_t *)&i)[1],
	((uint8_t *)&i)[2], ((uint8_t *)&i)[3]);
    printf("j = %.2x%.2x%.2x%.2x (little-endian 42 - \"little end first\")\n",
	((uint8_t *)&j)[0], ((uint8_t *)&j)[1],
	((uint8_t *)&j)[2], ((uint8_t *)&j)[3]);
  }
  
  printf("passed\n");
  return 0;
}
