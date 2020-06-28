//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ZiIP::(at your option) any later version.
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

#ifndef ZvDataFrame_HPP
#define ZvDataFrame_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <zlib/ZiLib.hpp>
#endif

#include <zlib/ZuByteSwap.hpp>
#include <zlib/ZuUnion.hpp>

#include <zlib/ZmHeap.hpp>
#include <zlib/ZmList.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtString.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvSeries.hpp>
#include <zlib/ZvSeriesRW.hpp>

#include <zlib/dataframe_fbs.h>

namespace ZvDataFrame {
  using namespace ZvSeries;

  using Series = ZvSeries::FileSeries;

  // FIXME - need array of readers, array of writers when
  // reading/writing data frames
  //
  // FIXME - need automatic recording (writerarray.update(void *ptr))

// FIXME - need to group FileStream into FileTable, [0] is time, [1]... fields
// FIXME - call timeRdr = index([0], time) on time series,
//         then reader = reader([N], timeRdr.offset()) on other series

  // typedefs for buffer-level reader/writer types
  using Reader_ = ZvSeriesRW::Reader;
  template <typename Base> using DeltaReader__ = ZvSeriesRW::DeltaReader<Base>;
  using DeltaReader_ = DeltaReader__<Reader_>;
  using Delta2Reader_ = DeltaReader__<DeltaReader_>;

  using Writer_ = ZvSeriesRW::Writer;
  template <typename Base> using DeltaWriter__ = ZvSeriesRW::DeltaWriter<Base>;
  using DeltaWriter_ = DeltaWriter__<Writer_>;
  using Delta2Writer_ = DeltaWriter__<DeltaWriter_>;

  // typedefs for series-level reader/writer types
  using Reader = Reader<Series, Reader_>;
  using DeltaReader = Reader<Series, DeltaReader_>;
  using Delta2Reader = Reader<Series, Delta2Reader_>;

  using Writer = Writer<Series, Writer_>;
  using DeltaWriter = Writer<Series, DeltaWriter_>;
  using Delta2Writer = Writer<Series, Delta2Writer_>;

  // run-time polymorphic reader
  using AnyReader_ = ZuUnion<Reader, DeltaReader, Delta2Reader>;
  struct AnyReader : public AnyReader_ {
    AnyReader() = default;
    AnyReader(const AnyReader &r) : AnyReader_{r} { }
    AnyReader(AnyReader &&r) : AnyReader_{static_cast<AnyReader_ &&>(r)} { }
    AnyReader &operator =(const AnyReader &r) {
      AnyReader_::operator =(r);
      return *this;
    }
    AnyReader &operator =(AnyReader &&r) {
      AnyReader_::operator =(static_cast<AnyReader_ &&>(r));
      return *this;
    }
    ~AnyReader() = default;

    void init(uint64_t offset, const Series *s) {
      unsigned flags = s->field()->flags;
      if (flags & ZvFieldFlags::Delta)
	new (AnyReader_::init<DeltaReader>())
	  DeltaReader{s->reader<DeltaReader_>(offset)};
      else if (flags & ZvFieldFlags::Delta2)
	new (AnyReader_::init<Delta2Reader>())
	  Delta2Reader{s->reader<Delta2Reader_>(offset)};
      else
	new (AnyReader_::init<Reader>())
	  Reader{s->reader<Reader_>(offset)};
    }
    void initIndex(int64_t value, const Series *s) {
      unsigned flags = s->field()->flags;
      if (flags & ZvFieldFlags::Delta)
	new (AnyReader_::init<DeltaReader>())
	  DeltaReader{s->index<DeltaReader_>(value)};
      else if (flags & ZvFieldFlags::Delta2)
	new (AnyReader_::init<Delta2Reader>())
	  Delta2Reader{s->index<Delta2Reader_>(value)};
      else
	new (AnyReader_::init<Reader>())
	  Reader{s->index<Reader_>(value)};
    }
    bool read(int64_t &v) {
      return dispatch([&v](auto &&r) { return r.read(v); });
    }
    void seekFwd(uint64_t offset) {
      dispatch([offset](auto &&r) { r.seekFwd(offset); });
    }
    void seekRev(uint64_t offset) {
      dispatch([offset](auto &&r) { r.seekRev(offset); });
    }
    void indexFwd(int64_t value) {
      dispatch([value](auto &&r) { r.indexFwd(value); });
    }
    void indexRev(int64_t value) {
      dispatch([value](auto &&r) { r.indexRev(value); });
    }
  };
  // run-time polymorphic writer
  using AnyWriter_ = ZuUnion<Writer, DeltaWriter, Delta2Writer>;
  struct AnyWriter : public AnyWriter_ {
    AnyWriter() = default;
    AnyWriter(const AnyWriter &r) : AnyWriter_{r} { }
    AnyWriter(AnyWriter &&r) : AnyWriter_{static_cast<AnyWriter_ &&>(r)} { }
    AnyWriter &operator =(const AnyWriter &r) {
      AnyWriter_::operator =(r);
      return *this;
    }
    AnyWriter &operator =(AnyWriter &&r) {
      AnyWriter_::operator =(static_cast<AnyWriter_ &&>(r));
      return *this;
    }
    ~AnyWriter() = default;

    void init(Series *s) {
      unsigned flags = s->field()->flags;
      if (flags & ZvFieldFlags::Delta)
	new (AnyWriter_::init<DeltaWriter>())
	  DeltaWriter{s->writer<DeltaWriter_>()};
      else if (flags & ZvFieldFlags::Delta2)
	new (AnyWriter_::init<Delta2Writer>())
	  Delta2Writer{s->writer<Delta2Writer_>()};
      else
	new (AnyWriter_::init<Writer>())
	  Writer{s->writer<Writer_>()};
    }
    bool write(int64_t v) {
      return dispatch([v](auto &&w) { return w.write(v); });
    }
  };

  class ZvAPI DataFrame {
  public:
    DataFrame(ZvFields fields, ZuString name) : m_name(name) {
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
      }
      for (unsigned i = 0; i < n; i++)
	if (fields[i].flags & ZvFieldFlags::Series) {
	  ZuPtr<Series> series = new Series{&fields[i]};
	  if (!indexed && (fields[i].flags & ZvFieldFlags::Index)) {
	    indexed = true;
	    m_series.unshift(ZuMv(series));
	  } else
	    m_series.push(ZuMv(series));
	}
      if (!indexed)
	m_series.unshift(ZuPtr<Series>{new Series{nullptr}});
    }

    void init(FileMgr *mgr) {
      m_mgr = mgr;
      unsigned n = m_series.length();
      for (unsigned i = 0; i < n; i++) series[i]->init(mgr);
    }

    bool open() {
      ZiFile::Path path = this->path();
      if (!ZiFile::mtime(path)) {
	m_epoch.now();
	return save(path);
      }
      if (!load(path)) return false;
      unsigned n = m_series.length();
      for (unsigned i = 0; i < n; i++) m_series[i]->open(m_name);
      return true;
    }
    bool close() {
      return save(path());
    }

    const ZtString &name() const { return m_name; }
    const ZmTime &epoch() const { return m_epoch; }

    unsigned length() const { return m_series.length(); }

    // FIXME - series lookup by field name
    // FIXME - writer - returns Writer, containing
    // array of AnyWriter, with write(ptr),
    // uses AnyWriter::write(field->scalar(ptr)) for each series

    const Series &series(unsigned i) const { return m_series[i]; }
    Series &series(unsigned i) { return m_series[i]; }

  private:
    ZiFile::Path path() const {
      return ZiFile::append(m_mgr->dir(), m_name + ".df");
    }

    bool load(const ZiFile::Path &path);
    bool load_(const uint8_t *buf, unsigned len);

    bool save(const ZiFile::Path &path);
    Zfb::Offset<fbs::DataFrame> save_(Zfb::Builder &);

  private:
    ZtString			m_name;
    ZtArray<ZuPtr<Series>>	m_series;
    FileMgr			*m_mgr = nullptr;
    ZmTime			m_epoch;
  };
}

#endif /* ZvDataFrame_HPP */
