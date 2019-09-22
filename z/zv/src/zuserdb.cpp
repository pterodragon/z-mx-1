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

// server-side user DB bootstrap tool

#include <iostream>

#include <ZvUserDB.hpp>

using namespace ZvUserDB;

void usage()
{
  std::cerr << "usage: zuserdb FILE USER ROLE PASSLEN\n";
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc != 5) usage();

  int passlen = atoi(argv[4]);

  if (passlen < 6 || passlen > 60) usage();

  const char *path = argv[1];

  Ztls::Random rng;

  rng.init();

  Mgr mgr(&rng, passlen, 6);

  ZtString passwd, secret;

  mgr.bootstrap(argv[2], argv[3], passwd, secret);

  ZeError e;
  if (mgr.save(path, 0, &e) != Zi::OK) {
    std::cerr << path << ": " << e;
    return 1;
  }

  std::cout << "passwd: " << passwd << "\nsecret: " << secret << '\n';
  return 0;
}
