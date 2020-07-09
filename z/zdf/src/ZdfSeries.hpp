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

// Data Series

#ifndef ZdfSeries_HPP
#define ZdfSeries_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZuUnion.hpp>
#include <zlib/ZuFixed.hpp>
#include <zlib/ZuSort.hpp>

#include <zlib/ZmRef.hpp>

#include <zlib/ZtArray.hpp>

#include <zlib/ZdfBuf.hpp>
#include <zlib/ZdfMgr.hpp>

namespace Zdf {

template <typename Series, typename BufReader_>
class Reader {
public:
  using BufReader = BufReader_;

  Reader() { }
  Reader(const Reader &r) :
      m_series(r.m_series), m_buf(r.m_buf), m_bufReader(r.m_bufReader) { }
  Reader &operator =(const Reader &r) {
    this->~Reader(); // nop
    new (this) Reader{r};
    return *this;
  }
  Reader(Reader &&r) noexcept :
      m_series(r.m_series), m_buf(r.m_buf), m_bufReader(ZuMv(r.m_bufReader)) { }
  Reader &operator =(Reader &&r) noexcept {
    this->~Reader(); // nop
    new (this) Reader{ZuMv(r)};
    return *this;
  }
  ~Reader() { }

private:
  Reader(const Series *s, ZmRef<Buf> buf, BufReader r) :
      m_series(s), m_buf(ZuMv(buf)), m_bufReader(ZuMv(r)) { }

public:
  // start reading at offset
  static Reader reader(const Series *s, uint64_t offset = 0) {
    ZmRef<Buf> buf;
    auto bufReader = s->template firstReader<BufReader>(buf, offset);
    return Reader{s, ZuMv(buf), ZuMv(bufReader)};
  }
  // seek forward to offset
  void seekFwd(uint64_t offset) {
    m_bufReader = m_series->template firstRdrFwd<BufReader>(m_buf, offset);
  }
  // seek reverse to offset
  void seekRev(uint64_t offset) {
    m_bufReader = m_series->template firstRdrFwd<BufReader>(m_buf, offset);
  }

  // start reading at index value
  static Reader index(const Series *s, const ZuFixed &value) {
    ZmRef<Buf> buf;
    auto bufReader = s->template firstIndex<BufReader>(buf, value);
    return Reader{s, ZuMv(buf), ZuMv(bufReader)};
  }
  // seek forward to index value
  void indexFwd(const ZuFixed &value) {
    m_bufReader = m_series->template firstIdxFwd<BufReader>(m_buf, value);
  }
  // seek reverse to index value
  void indexRev(const ZuFixed &value) {
    m_bufReader = m_series->template firstIdxRev<BufReader>(m_buf, value);
  }

  bool read(ZuFixed &value) {
    if (ZuUnlikely(!m_bufReader)) return false;
    if (ZuUnlikely(!m_bufReader.read(value.value))) {
      m_bufReader = m_series->template nextReader<BufReader>(m_buf);
      if (ZuUnlikely(!m_bufReader || !m_bufReader.read(value.value)))
	return false;
    }
    value.exponent = m_buf->hdr()->exponent();
    return true;
  }

  void purge() { m_series->purge_(m_buf->blkIndex); }

  uint64_t offset() const {
    return m_buf->hdr()->offset() + m_bufReader.count();
  }

private:
  const Series	*m_series = nullptr;
  ZmRef<Buf>	m_buf;
  BufReader	m_bufReader;
};

template <typename Series, typename BufWriter_>
class Writer {
  Writer(const Writer &) = delete;
  Writer &operator =(const Writer &) = delete;

public:
  using BufWriter = BufWriter_;

  Writer(Series *s) : m_series(s) {
    m_bufWriter = s->template firstWriter<BufWriter>(m_buf);
  }

  Writer() { }
  Writer(Writer &&w) noexcept :
      m_series(w.m_series),
      m_buf(w.m_buf),
      m_bufWriter(ZuMv(w.m_bufWriter)) {
    w.m_series = nullptr;
    w.m_buf = nullptr;
  }
  Writer &operator =(Writer &&w) noexcept {
    this->~Writer(); // nop
    new (this) Writer{ZuMv(w)};
    return *this;
  }
  ~Writer() {
    m_buf->sync(m_bufWriter);
    m_series->save(m_buf);
  }

  bool write(const ZuFixed &value) {
    if (ZuUnlikely(!m_bufWriter)) return false;
    auto hdr = m_buf->hdr();
    bool eob = false;
    if (ZuUnlikely(!hdr->count())) {
      hdr->exponent(value.exponent);
    } else if (hdr->exponent() != value.exponent) {
      eob = true;
    }
    if (eob || !m_bufWriter.write(value.value)) {
      m_buf->sync(m_bufWriter);
      m_series->save(m_buf);
      m_bufWriter = m_series->template nextWriter<BufWriter>(m_buf);
      if (ZuUnlikely(!m_bufWriter)) return false;
      hdr = m_buf->hdr();
      hdr->exponent(value.exponent);
      if (ZuUnlikely(!m_bufWriter.write(value.value))) return false;
    }
    return true;
  }

  void sync() { // in-memory sync to readers
    m_buf->sync(m_bufWriter);
  }

private:
  Series	*m_series = nullptr;
  ZmRef<Buf>	m_buf;
  BufWriter	m_bufWriter;
};

class Series {
template <typename, typename> friend class Reader;
template <typename, typename> friend class Writer;

public:
  void init(Mgr *mgr) {
    m_mgr = mgr;
    m_seriesID = mgr->alloc(
	BufUnloadFn{this, [](Series *this_, BufLRUNode *node) {
	  this_->unloadBuf(node);
	}});
  }
  void final() {
    m_blks.null();
  }

  Mgr *mgr() const { return m_mgr; }
  unsigned seriesID() const { return m_seriesID; }

protected:
  void open_(unsigned blkOffset = 0) {
    m_blkOffset = blkOffset;
    Hdr hdr;
    for (unsigned i = 0; loadHdr(i + blkOffset, hdr); i++)
      new (Blk::new_<Hdr>(m_blks.push())) Hdr{hdr};
  }

public:
  bool open(ZuString parent, ZuString name) {
    return mgr()->open(this->seriesID(), parent, name, OpenFn{
      this, [](Series *this_, unsigned blkOffset) {
	this_->open_(blkOffset);
      }});
  }
  void close() {
    mgr()->close(this->seriesID());
  }

  uint64_t count() const {
    unsigned n = m_blks.length();
    if (ZuUnlikely(!n)) return 0;
    auto hdr = this->hdr(m_blks[n - 1]);
    return hdr->offset() + hdr->count();
  }

  template <typename BufReader>
  auto reader(uint64_t offset = 0) const {
    return Reader<Series, BufReader>::reader(this, offset);
  }
  template <typename BufReader>
  auto index(const ZuFixed &value) const {
    return Reader<Series, BufReader>::index(this, value);
  }

  template <typename BufWriter>
  auto writer() { return Writer<Series, BufWriter>{this}; }

private:
  using Blk = ZuUnion<Hdr, ZmRef<Buf>>;

  static const Hdr *hdr_(const ZuNull &) { return nullptr; } // never called
  static const Hdr *hdr_(const Hdr &hdr) { return &hdr; }
  static const Hdr *hdr_(const ZmRef<Buf> &buf) { return buf->hdr(); }
  static const Hdr *hdr(const Blk &blk) {
    return blk.cdispatch([](auto &&v) { return hdr_(v); }); 
  }

  Buf *loadBuf(unsigned blkIndex) const {
    auto &blk = m_blks[blkIndex];
    ZmRef<Buf> buf;
    Buf *buf_;
    if (ZuLikely(blk.contains<ZmRef<Buf>>())) {
      buf_ = blk.v<ZmRef<Buf>>().ptr();
      m_mgr->use(buf_);
    } else {
      if (ZuUnlikely(!blk.contains<Hdr>())) return nullptr;
      m_mgr->shift(); // might call unloadBuf()
      buf = load(blkIndex + m_blkOffset);
      if (!buf) return nullptr;
      buf_ = buf.ptr();
      const_cast<Blk &>(blk).v<ZmRef<Buf>>(ZuMv(buf));
      m_mgr->push(buf_);
    }
    return buf_;
  }

  void unloadBuf(BufLRUNode *node) {
    auto &lruBlk = m_blks[node->blkIndex];
    if (ZuLikely(lruBlk.contains<ZmRef<Buf>>())) {
      Hdr hdr = *(lruBlk.v<ZmRef<Buf>>()->hdr());
      lruBlk.v<Hdr>(hdr);
    }
  }

  template <typename BufReader>
  BufReader firstReader_(
      ZmRef<Buf> &buf, unsigned search, uint64_t offset) const {
    if (!ZuSearchFound(search)) goto null;
    {
      unsigned blkIndex = ZuSearchPos(search);
      if (blkIndex >= m_blks.length()) goto null;
      if (!(buf = loadBuf(blkIndex))) goto null;
      {
	auto reader = buf->reader<BufReader>();
	offset -= buf->hdr()->offset();
	if (!reader.seek(offset)) goto null;
	return reader;
      }
    }
  null:
    buf = nullptr;
    return BufReader{};
  }
  template <typename BufReader>
  BufReader firstIndex_(
      ZmRef<Buf> &buf, unsigned search, const ZuFixed &value) const {
    unsigned blkIndex = ZuSearchPos(search);
    if (blkIndex >= m_blks.length()) goto null;
    if (!(buf = loadBuf(blkIndex))) goto null;
    {
      auto reader = buf->reader<BufReader>();
      bool found = reader.search(
	  [&value, exponent = buf->hdr()->exponent()](int64_t skip) {
	    return value >= ZuFixed{skip, exponent};
	  });
      if (!found) goto null;
      return reader;
    }
  null:
    buf = nullptr;
    return BufReader{};
  }

  auto offsetSearch(uint64_t offset) const {
    return [offset](const Blk &blk) -> int {
      auto hdr = Series::hdr(blk);
      auto hdrOffset = hdr->offset();
      if (offset < hdrOffset) return -static_cast<int>(hdrOffset - offset);
      hdrOffset += hdr->count();
      if (offset >= hdrOffset) return static_cast<int>(offset - hdrOffset) + 1;
      return 0;
    };
  }
  template <typename BufReader>
  auto indexSearch(const ZuFixed &value) const {
    return [this, value](const Blk &blk) -> int {
      unsigned blkIndex = &const_cast<Blk &>(blk) - &m_blks[0];
      auto buf = loadBuf(blkIndex);
      if (!buf) return -1;
      auto reader = buf->template reader<BufReader>();
      ZuFixed value_{static_cast<int64_t>(0), buf->hdr()->exponent()};
      if (!reader.read(value_.value)) return -1;
      if (value < value_) {
	int64_t delta = value_.adjust(value.exponent) - value.value;
	if (ZuUnlikely(delta >= static_cast<int64_t>(INT_MAX))) return INT_MIN;
	return -static_cast<int>(delta);
      } else {
	int64_t delta = value.value - value_.adjust(value.exponent);
	if (ZuUnlikely(delta >= static_cast<int64_t>(INT_MAX))) return INT_MAX;
	return static_cast<int>(delta) + 1;
      }
    };
  }

  template <typename BufReader>
  BufReader firstReader(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<BufReader>(buf,
	ZuInterSearch(&m_blks[0], m_blks.length(), offsetSearch(offset)),
	offset);
  }
  template <typename BufReader>
  BufReader firstRdrFwd(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<BufReader>(buf,
	ZuInterSearch(
	  &m_blks[buf->blkIndex], m_blks.length() - buf->blkIndex,
	  offsetSearch(offset)),
	offset);
  }
  template <typename BufReader>
  BufReader firstRdrRev(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<BufReader>(buf,
	ZuInterSearch(&m_blks[0], buf->blkIndex + 1, offsetSearch(offset)),
	offset);
  }

  template <typename BufReader>
  BufReader firstIndex(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(
	  &m_blks[0], m_blks.length(),
	  indexSearch<BufReader>(value)),
	value);
  }
  template <typename BufReader>
  BufReader firstIdxFwd(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(
	  &m_blks[buf->blkIndex], m_blks.length() - buf->blkIndex,
	  indexSearch<BufReader>(value)),
	value);
  }
  template <typename BufReader>
  BufReader firstIdxRev(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(
	  &m_blks[0], buf->blkIndex + 1,
	  indexSearch<BufReader>(value)),
	value);
  }

  template <typename BufReader>
  BufReader nextReader(ZmRef<Buf> &buf) const {
    unsigned blkIndex = buf->blkIndex + 1;
    if (blkIndex >= m_blks.length()) goto null;
    if (!(buf = loadBuf(blkIndex))) goto null;
    return buf->reader<BufReader>();
  null:
    buf = nullptr;
    return BufReader{};
  }

  template <typename BufWriter>
  BufWriter firstWriter(ZmRef<Buf> &buf) {
    return nextWriter<BufWriter>(buf);
  }
  template <typename BufWriter>
  BufWriter nextWriter(ZmRef<Buf> &buf) {
    unsigned blkIndex;
    uint64_t offset;
    if (ZuLikely(buf)) {
      blkIndex = buf->blkIndex + 1;
      const auto *hdr = buf->hdr();
      offset = hdr->offset() + hdr->count();
    } else {
      blkIndex = 0;
      offset = 0;
    }
    m_mgr->shift(); // might call unloadBuf()
    buf = new Buf{m_mgr, m_seriesID, blkIndex};
    new (Blk::new_<ZmRef<Buf>>(m_blks.push())) ZmRef<Buf>{buf};
    new (buf->hdr()) Hdr{offset, 0};
    m_mgr->push(buf);
    return buf->writer<BufWriter>();
  }

  void purge_(unsigned blkIndex) {
    mgr()->purge(this->seriesID(), m_blkOffset += blkIndex);
    {
      unsigned n = m_blks.length();
      if (n > blkIndex) n = blkIndex;
      for (unsigned i = 0; i < n; i++) {
	auto &blk = m_blks[i];
	if (blk.contains<ZmRef<Buf>>())
	  m_mgr->del(blk.v<ZmRef<Buf>>().ptr());
      }
    }
    m_blks.splice(0, blkIndex);
  }

  bool loadHdr(unsigned i, Hdr &hdr) const {
    return mgr()->loadHdr(this->seriesID(), i, hdr);
  }
  ZmRef<Buf> load(unsigned i) const {
    ZmRef<Buf> buf = new Buf{mgr(), this->seriesID(), i};
    if (mgr()->load(this->seriesID(), i, buf->data()))
      return buf;
    return nullptr;
  }
  void save(ZmRef<Buf> buf) const {
    return mgr()->save(ZuMv(buf));
  }

private:
  Mgr		*m_mgr = nullptr;
  ZtArray<Blk>	m_blks;
  unsigned	m_seriesID = 0;
  unsigned	m_blkOffset = 0;
};

} // namespace Zdf

#endif /* ZdfSeries_HPP */
