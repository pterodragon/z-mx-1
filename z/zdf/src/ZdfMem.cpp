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

#include <zlib/ZdfMem.hpp>

using namespace Zdf;

void MemMgr::shift() { }
void MemMgr::push(BufLRUNode *) { }
void MemMgr::use(BufLRUNode *) { }
void MemMgr::del(BufLRUNode *) { }
void MemMgr::purge(unsigned, unsigned) { }

void MemMgr::init(ZmScheduler *, ZvCf *) { BufMgr::init(UINT_MAX); }

void MemMgr::final() { BufMgr::final(); }

bool MemMgr::open(unsigned, ZuString, ZuString, OpenFn openFn)
{
  return true;
}

void MemMgr::close(unsigned) { }

bool MemMgr::loadHdr(unsigned, unsigned, Hdr &)
{
  return false;
}

bool MemMgr::load(unsigned, unsigned, void *)
{
  return false;
}

void MemMgr::save(ZmRef<Buf>)
{
}

bool MemMgr::loadFile(
    ZuString, Zfb::Load::LoadFn,
    unsigned, ZeError *e)
{
  if (e) *e = ZiENOENT;
  return false;
}

bool MemMgr::saveFile(ZuString, Zfb::Builder &, ZeError *e)
{
  if (e) *e = ZiENOENT;
  return false;
}
