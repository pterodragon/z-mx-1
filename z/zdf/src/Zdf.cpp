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

// Data Frame

#include <zlib/Zdf.hpp>

using namespace Zdf;

DataFrame::DataFrame(ZvFields fields, ZuString name) : m_name(name)
{
  bool indexed = false;
  unsigned n = fields.length();
  {
    unsigned m = 0;
    for (unsigned i = 0; i < n; i++)
      if (fields[i].flags & ZvFieldFlags::Series) {
	if (!indexed && (fields[i].flags & ZvFieldFlags::Index))
	  indexed = true;
	m++;
      }
    if (!indexed) m++;
    indexed = false;
    m_series.size(m);
    m_fields.size(m);
  }
  for (unsigned i = 0; i < n; i++)
    if (fields[i].flags & ZvFieldFlags::Series) {
      ZuPtr<Series> series = new Series();
      if (!indexed && (fields[i].flags & ZvFieldFlags::Index)) {
	indexed = true;
	m_series.unshift(ZuMv(series));
	m_fields.unshift(&fields[i]);
      } else {
	m_series.push(ZuMv(series));
	m_fields.push(&fields[i]);
      }
    }
  if (!indexed) {
    m_series.unshift(ZuPtr<Series>{new Series()});
    m_fields.unshift(static_cast<ZvField *>(nullptr));
  }
}

void DataFrame::init(FileMgr *mgr)
{
  m_mgr = mgr;
  unsigned n = m_series.length();
  for (unsigned i = 0; i < n; i++) m_series[i]->init(mgr);
}

bool DataFrame::open()
{
  ZiFile::Path path = this->path();
  if (!ZiFile::mtime(path)) {
    m_epoch.now();
    return save(path);
  }
  if (!load(path)) return false;
  unsigned n = m_series.length();
  if (ZuUnlikely(!n)) return false;
  for (unsigned i = 0; i < n; i++) {
    auto field = m_fields[i];
    if (i || field)
      m_series[i]->open(m_name, field->id);
    else
      m_series[i]->open(m_name, "_0");
  }
  return true;
}

bool DataFrame::close()
{
  return save(path());
}

bool DataFrame::load(const ZiFile::Path &path)
{
  ZeError e;
  using namespace Zfb::Load;
  if (Zfb::Load::load(path,
      LoadFn{this, [](DataFrame *this_, const uint8_t *buf, unsigned len) {
	return this_->load_(buf, len);
      }}, (1<<10) /* 1Kb */, &e) != Zi::OK) {
    ZeLOG(Error, ZtString() <<
      "Zdf::DataFrame could not open \"" <<
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
  m_epoch = dateTime(df->epoch()).zmTime();
  return true;
}

bool DataFrame::save(const ZiFile::Path &path)
{
  Zfb::Builder fbb;
  fbb.Finish(save_(fbb));
  ZeError e;
  if (Zfb::Save::save(path, fbb, &e) != Zi::OK) {
    ZeLOG(Error, ZtString() <<
      "Zdf::DataFrame could not create \"" <<
      path << "\": " << e);
    return false;
  }
  return true;
}

Zfb::Offset<fbs::DataFrame> DataFrame::save_(Zfb::Builder &fbb)
{
  using namespace Zfb::Save;
  return fbs::CreateDataFrame(fbb, dateTime(fbb, ZtDate{m_epoch}));
}
