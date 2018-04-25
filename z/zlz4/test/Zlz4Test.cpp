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

/* test program */

#include <ZuLib.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <ZuTraits.hpp>
#include <ZuHash.hpp>

#include <lz4.h>

int main(int argc, char **argv)
{
  char buf[] = "Hello World";
  char cbuf[LZ4_compressBound(sizeof(buf))];
  char dbuf[sizeof(buf)];
  int n = LZ4_compress(buf, cbuf, sizeof(buf));
  printf("%d\n", n);
  LZ4_uncompress(cbuf, dbuf, sizeof(buf));
  puts(buf);
  puts(dbuf);
}
