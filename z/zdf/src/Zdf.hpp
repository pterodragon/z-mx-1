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

#ifndef Zdf_HPP
#define Zdf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZuPtr.hpp>
#include <zlib/ZuUnion.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtString.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvField.hpp>

#include <zlib/ZdfCompress.hpp>
#include <zlib/ZdfSeries.hpp>

#include <zlib/dataframe_fbs.h>

namespace Zdf {

// Data d;
// ...
// DataFrame df{Data::fields(), "d"};
// ...
// auto w = df.writer();
// ZmTime time{ZmTime::Now};
// w.write(&d);
// ...
// AnyReader index, reader;
// ...
// df.index(index, 0, df.nsecs(time));		// index time to offset
// df.reader(reader, N, index.offset());	// seek to offset
// ...
// ZuFixed nsecs, value;
// index.read(nsecs);
// ZmTime then = df.time(nsecs);
// reader.read(value);

// typedefs for (de)compressors
using AbsReader_ = ZdfCompress::Reader;
template <typename Base> using DeltaReader__ = ZdfCompress::DeltaReader<Base>;
using DeltaReader_ = DeltaReader__<AbsReader_>;
using Delta2Reader_ = DeltaReader__<DeltaReader_>;

using AbsWriter_ = ZdfCompress::Writer;
template <typename Base> using DeltaWriter__ = ZdfCompress::DeltaWriter<Base>;
using DeltaWriter_ = DeltaWriter__<AbsWriter_>;
using Delta2Writer_ = DeltaWriter__<DeltaWriter_>;

// typedefs for reader/writer types
using AbsReader = Reader<Series, AbsReader_>;
using DeltaReader = Reader<Series, DeltaReader_>;
using Delta2Reader = Reader<Series, Delta2Reader_>;

using AbsWriter = Writer<Series, AbsWriter_>;
using DeltaWriter = Writer<Series, DeltaWriter_>;
using Delta2Writer = Writer<Series, Delta2Writer_>;

// run-time polymorphic reader
using AnyReader_ = ZuUnion<AbsReader, DeltaReader, Delta2Reader>;
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

  void init(const Series *s, unsigned flags, uint64_t offset) {
    if (flags & ZvFieldFlags::Delta)
      new (AnyReader_::init<DeltaReader>())
	DeltaReader{s->reader<DeltaReader_>(offset)};
    else if (flags & ZvFieldFlags::Delta2)
      new (AnyReader_::init<Delta2Reader>())
	Delta2Reader{s->reader<Delta2Reader_>(offset)};
    else
      new (AnyReader_::init<AbsReader>())
	AbsReader{s->reader<AbsReader_>(offset)};
  }
  void initIndex(const Series *s, unsigned flags, const ZuFixed &value) {
    if (flags & ZvFieldFlags::Delta)
      new (AnyReader_::init<DeltaReader>())
	DeltaReader{s->index<DeltaReader_>(value)};
    else if (flags & ZvFieldFlags::Delta2)
      new (AnyReader_::init<Delta2Reader>())
	Delta2Reader{s->index<Delta2Reader_>(value)};
    else
      new (AnyReader_::init<AbsReader>())
	AbsReader{s->index<AbsReader_>(value)};
  }
  bool read(ZuFixed &v) {
    return dispatch([&v](auto &&r) { return r.read(v); });
  }
  void seekFwd(uint64_t offset) {
    dispatch([offset](auto &&r) { r.seekFwd(offset); });
  }
  void seekRev(uint64_t offset) {
    dispatch([offset](auto &&r) { r.seekRev(offset); });
  }
  void indexFwd(ZuFixed value) {
    dispatch([&value](auto &&r) { r.indexFwd(value); });
  }
  void indexRev(ZuFixed value) {
    dispatch([&value](auto &&r) { r.indexRev(value); });
  }
};
// run-time polymorphic writer
using AnyWriter_ = ZuUnion<AbsWriter, DeltaWriter, Delta2Writer>;
struct AnyWriter : public AnyWriter_ {
  AnyWriter(const AnyWriter &r) = delete;
  AnyWriter &operator =(const AnyWriter &r) = delete;

  AnyWriter() = default;
  AnyWriter(AnyWriter &&r) : AnyWriter_{static_cast<AnyWriter_ &&>(r)} { }
  AnyWriter &operator =(AnyWriter &&r) {
    AnyWriter_::operator =(static_cast<AnyWriter_ &&>(r));
    return *this;
  }
  ~AnyWriter() = default;

  void init(Series *s, unsigned flags) {
    if (flags & ZvFieldFlags::Delta)
      new (AnyWriter_::init<DeltaWriter>())
	DeltaWriter{s->writer<DeltaWriter_>()};
    else if (flags & ZvFieldFlags::Delta2)
      new (AnyWriter_::init<Delta2Writer>())
	Delta2Writer{s->writer<Delta2Writer_>()};
    else
      new (AnyWriter_::init<AbsWriter>())
	AbsWriter{s->writer<AbsWriter_>()};
  }

  bool write(ZuFixed v) {
    return dispatch([v](auto &&w) { return w.write(v); });
  }

  void sync() {
    return dispatch([](auto &&w) { w.sync(); });
  }
};

class ZdfAPI DataFrame {
public:
  DataFrame(ZvFields fields, ZuString name);

  const ZtString &name() const { return m_name; }
  const ZmTime &epoch() const { return m_epoch; }

  void init(Mgr *mgr);

  bool open(ZeError *e = nullptr);
  bool close(ZeError *e = nullptr);

  class ZdfAPI Writer {
  friend DataFrame;
    Writer(DataFrame *df) : m_df(df) {
      unsigned n = df->nSeries();
      m_writers.length(n);
      for (unsigned i = 0; i < n; i++) df->writer_(m_writers[i], i);
    }

  public:
    void write(const void *ptr) {
      unsigned n = m_writers.length();
      if (ZuUnlikely(!n)) return;
      ZuFixed v;
      for (unsigned i = 0; i < n; i++) {
	auto field = m_df->field(i);
	if (i || field) {
	  if (field->type == ZvFieldType::Time)
	    v = m_df->nsecs(field->u.time(ptr));
	  else
	    v = field->u.scalar(ptr);
	} else
	  v = m_df->nsecs(ZmTimeNow());
	m_writers[i].write(v);
      }
    }

    void sync() {
      unsigned n = m_writers.length();
      for (unsigned i = 0; i < n; i++)
	m_writers[i].sync();
    }

  private:
    DataFrame		*m_df;
    ZtArray<AnyWriter>	m_writers;
  };
  Writer writer() { return Writer{this}; }

friend Writer;
private:
  void writer_(AnyWriter &w, unsigned i) {
    auto field = m_fields[i];
    unsigned flags = field ? field->flags : ZvFieldFlags::Delta;
    w.init(m_series[i], flags);
  }
public:
  void reader(AnyReader &r, unsigned i, uint64_t offset = 0) {
    auto field = m_fields[i];
    unsigned flags = field ? field->flags : ZvFieldFlags::Delta;
    r.init(m_series[i], flags, offset);
  }
  void index(AnyReader &r, unsigned i, const ZuFixed &value) {
    auto field = m_fields[i];
    unsigned flags = field ? field->flags : ZvFieldFlags::Delta;
    r.initIndex(m_series[i], flags, value);
  }

  unsigned nSeries() const { return m_series.length(); }
  const Series *series(unsigned i) const { return m_series[i]; }
  Series *series(unsigned i) { return m_series[i]; }
  const ZvField *field(unsigned i) const { return m_fields[i]; }

private:
  constexpr static const uint64_t pow10_9() { return 1000000000UL; }
public:
  ZuFixed nsecs(ZmTime t) {
    t -= m_epoch;
    return ZuFixed{
      static_cast<uint64_t>(t.sec()) * pow10_9() + t.nsec(), 9 };
  }
  ZmTime time(const ZuFixed &v) {
    uint64_t n = v.adjust(9);
    uint64_t p = pow10_9();
    return ZmTime{static_cast<time_t>(n / p), static_cast<long>(n % p)} +
      m_epoch;
  }

private:
  bool load(ZeError *e = nullptr);
  bool load_(const uint8_t *buf, unsigned len);

  bool save(ZeError *e = nullptr);
  Zfb::Offset<fbs::DataFrame> save_(Zfb::Builder &);

private:
  ZtString			m_name;
  ZtArray<ZuPtr<Series>>	m_series;
  ZtArray<const ZvField *>	m_fields;
  Mgr				*m_mgr = nullptr;
  ZmTime			m_epoch;
};

} // namespace Zdf

#endif /* Zdf_HPP */
