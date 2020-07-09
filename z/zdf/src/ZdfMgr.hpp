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

// Data Series Manager

#ifndef ZdfMgr_HPP
#define ZdfMgr_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZmScheduler.hpp>
#include <zlib/ZmRef.hpp>

#include <zlib/ZvCf.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZdfBuf.hpp>

namespace Zdf {

using OpenFn = ZmFn<unsigned>;
class ZdfAPI Mgr : public BufMgr {
public:
  virtual void init(ZmScheduler *, ZvCf *);
  virtual void final();

  virtual bool open(
      unsigned seriesID, ZuString parent, ZuString name, OpenFn);
  virtual void close(unsigned seriesID);

  virtual bool loadHdr(unsigned seriesID, unsigned blkIndex, Hdr &hdr);
  virtual bool load(unsigned seriesID, unsigned blkIndex, void *buf);
  virtual void save(ZmRef<Buf>);

  virtual void purge(unsigned seriesID, unsigned blkIndex);

  virtual bool loadFile(
      ZuString name, Zfb::Load::LoadFn,
      unsigned maxFileSize, ZeError *e = nullptr);
  virtual bool saveFile(
      ZuString name, Zfb::Builder &fbb, ZeError *e = nullptr);
};

} // namespace Zdf

#endif /* ZdfMgr_HPP */
