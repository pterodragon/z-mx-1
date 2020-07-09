//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// Data Series In-Memory

#ifndef ZdfMem_HPP
#define ZdfMem_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZdfMgr.hpp>

namespace Zdf {

class ZdfAPI MemMgr : public Mgr {
public:
  void shift();
  void push(BufLRUNode *);
  void use(BufLRUNode *);
  void del(BufLRUNode *);
  void purge(unsigned, unsigned);

  void init(ZmScheduler *, ZvCf *);
  void final();

  bool open(unsigned, ZuString, ZuString, OpenFn openFn);
  void close(unsigned);

  bool loadHdr(unsigned, unsigned, Hdr &);
  bool load(unsigned, unsigned, void *);
  void save(ZmRef<Buf>);

  bool loadFile(ZuString, Zfb::Load::LoadFn, unsigned, ZeError *e);
  bool saveFile(ZuString, Zfb::Builder &, ZeError *e);
};

} // namespace Zdf

#endif /* ZdfMem_HPP */
