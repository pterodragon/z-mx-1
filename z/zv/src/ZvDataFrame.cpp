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

// Data Frames

#include <zlib/ZvDataFrame.hpp>

using namespace ZvDataFrame;

bool DataFrame::load(const ZiFile::Path &path)
{
  ZeError e;
  using namespace Zfb::Load;
  if (Zfb::Load::load(path,
      LoadFn{this, [](FileMgr *this_, const uint8_t *buf, unsigned len) {
	return this_->load_(buf, len);
      }}, (1<<10) /* 1Kb */, e) != Zi::OK) {
    ZeLOG(Error, ZtString() <<
      "ZvDataFrame::DataFrame could not open \"" <<
      path << "\": " << e);
    return false;
  }
  return true;
}

bool DataFrame::load_(const uint8_t *buf, unsigned len)
{
  {
    Zfb::Verifier verifier(buf, len);
    if (!fbs::VerifyDataFrameBuffer(verifier)) return false;
  }
  using namespace Zfb::Load;
  auto df = fbs::GetDataFrame(buf);
  m_epoch = dateTime(df->epoch());
  return true;
}

bool DataFrame::save(const ZiFile::Path &path)
{
  Zfb::Builder fbb;
  fbb.Finish(save_(fbb));
  ZeError e;
  if (Zfb::Save::save(path, fbb, e) != Zi::OK) {
    ZeLOG(Error, ZtString() <<
      "ZvDataFrame::DataFrame could not create \"" <<
      path << "\": " << e);
    return false;
  }
  return true;
}

Zfb::Offset<fbs::DataFrame> DataFrame::save_(Zfb::Builder &fbb)
{
  using namespace Zfb::Save;
  return fbs::CreateDataFrame(fbb, dateTime(fbb, m_epoch));
}
