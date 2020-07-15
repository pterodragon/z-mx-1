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

template <typename Series, typename Decoder_>
class Reader {
public:
  using Decoder = Decoder_;

  Reader() { }
  Reader(const Reader &r) :
      m_series(r.m_series), m_buf(r.m_buf), m_exponent(r.m_exponent),
      m_decoder(r.m_decoder) { }
  Reader &operator =(const Reader &r) {
    this->~Reader(); // nop
    new (this) Reader{r};
    return *this;
  }
  Reader(Reader &&r) noexcept :
      m_series(r.m_series), m_buf(ZuMv(r.m_buf)), m_exponent(r.m_exponent),
      m_decoder(ZuMv(r.m_decoder)) { }
  Reader &operator =(Reader &&r) noexcept {
    this->~Reader(); // nop
    new (this) Reader{ZuMv(r)};
    return *this;
  }
  ~Reader() { }

private:
  Reader(const Series *s, ZmRef<Buf> buf, Decoder r) :
      m_series(s), m_buf(ZuMv(buf)), m_decoder(ZuMv(r)) {
    if (ZuUnlikely(!*this)) return;
    m_exponent = m_buf->hdr()->exponent();
  }

public:
  // start reading at offset
  static Reader reader(const Series *s, uint64_t offset = 0) {
    ZmRef<Buf> buf;
    auto decoder = s->template firstReader<Decoder>(buf, offset);
    return Reader{s, ZuMv(buf), ZuMv(decoder)};
  }
  // seek forward to offset
  void seekFwd(uint64_t offset) {
    if (ZuUnlikely(!*this)) return;
    m_decoder =
      m_series->template firstRdrFwd<Decoder>(m_buf, offset);
    m_exponent = m_buf->hdr()->exponent();
  }
  // seek reverse to offset
  void seekRev(uint64_t offset) {
    if (ZuUnlikely(!*this)) return;
    m_decoder =
      m_series->template firstRdrFwd<Decoder>(m_buf, offset);
    m_exponent = m_buf->hdr()->exponent();
  }

  // start reading at index value
  static Reader index(const Series *s, const ZuFixed &value) {
    ZmRef<Buf> buf;
    auto decoder = s->template firstIndex<Decoder>(buf, value);
    return Reader{s, ZuMv(buf), ZuMv(decoder)};
  }
  // seek forward to index value
  void indexFwd(const ZuFixed &value) {
    if (ZuUnlikely(!*this)) return;
    m_decoder =
      m_series->template firstIdxFwd<Decoder>(m_buf, value);
    m_exponent = m_buf->hdr()->exponent();
  }
  // seek reverse to index value
  void indexRev(const ZuFixed &value) {
    if (ZuUnlikely(!*this)) return;
    m_decoder =
      m_series->template firstIdxRev<Decoder>(m_buf, value);
    m_exponent = m_buf->hdr()->exponent();
  }

  bool read(ZuFixed &value) {
    if (ZuUnlikely(!*this)) return false;
    if (ZuUnlikely(!m_decoder.read(value.value))) {
      m_decoder = m_series->template nextReader<Decoder>(m_buf);
      if (ZuUnlikely(!m_decoder || !m_decoder.read(value.value)))
	return false;
      m_exponent = m_buf->hdr()->exponent();
    }
    value.exponent = m_exponent;
    return true;
  }

  void purge() {
    if (ZuUnlikely(!*this)) return;
    const_cast<Series *>(m_series)->purge_(m_buf->blkIndex);
  }

  uint64_t offset() const {
    if (ZuUnlikely(!*this)) return 0;
    return m_buf->hdr()->offset() + m_decoder.count();
  }

  ZuInline bool operator !() const { return !m_decoder; }
  ZuOpBool

private:
  const Series	*m_series = nullptr;
  ZmRef<Buf>	m_buf;
  unsigned	m_exponent = 0;
  Decoder	m_decoder;
};

template <typename Series, typename Encoder_>
class Writer {
  Writer(const Writer &) = delete;
  Writer &operator =(const Writer &) = delete;

public:
  using Encoder = Encoder_;

  Writer(Series *s) : m_series(s) { }

  Writer() { }
  Writer(Writer &&w) noexcept :
      m_series(w.m_series),
      m_buf(ZuMv(w.m_buf)),
      m_exponent(w.m_exponent),
      m_encoder(ZuMv(w.m_encoder)) {
    w.m_series = nullptr;
    w.m_buf = nullptr;
    // w.m_exponent = 0;
  }
  Writer &operator =(Writer &&w) noexcept {
    this->~Writer(); // nop
    new (this) Writer{ZuMv(w)};
    return *this;
  }
  ~Writer() {
    sync();
    save();
  }

  void sync() {
    if (ZuLikely(m_buf)) m_buf->sync(m_encoder, m_exponent, m_encoder.last());
  }

  void save() {
    if (ZuLikely(m_buf)) m_series->save(m_buf);
  }

  bool write(const ZuFixed &value) {
    bool eob;
    if (ZuUnlikely(!m_buf)) {
      m_encoder = m_series->template firstWriter<Encoder>(m_buf);
      if (ZuUnlikely(!m_buf)) return false;
      m_exponent = value.exponent;
      eob = false;
    } else {
      eob = value.exponent != m_exponent;
    }
    if (eob || !m_encoder.write(value.value)) {
      sync();
      save();
      m_encoder = m_series->template nextWriter<Encoder>(m_buf);
      if (ZuUnlikely(!m_buf)) return false;
      m_exponent = value.exponent;
      if (ZuUnlikely(!m_encoder.write(value.value))) return false;
    }
    return true;
  }

private:
  Series	*m_series = nullptr;
  ZmRef<Buf>	m_buf;
  unsigned	m_exponent = 0;
  Encoder	m_encoder;
};

class Series {
template <typename, typename> friend class Reader;
template <typename, typename> friend class Writer;

public:
  Series() = default;
  ~Series() { final(); }

  void init(Mgr *mgr) {
    m_mgr = mgr;
    m_seriesID = mgr->alloc(
	BufUnloadFn{this, [](Series *this_, BufLRUNode *node) {
	  this_->unloadBuf(node);
	}});
  }
  void final() {
    if (m_mgr) m_mgr->free(m_seriesID);
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
    return m_mgr->open(m_seriesID, parent, name, OpenFn{
      this, [](Series *this_, unsigned blkOffset) {
	this_->open_(blkOffset);
      }});
  }
  void close() {
    m_mgr->close(m_seriesID);
  }

  // number of blocks
  unsigned blkCount() const { return m_blks.length(); }

  // value count (length of series in #values)
  uint64_t count() const {
    unsigned n = m_blks.length();
    if (ZuUnlikely(!n)) return 0;
    auto hdr = this->hdr(m_blks[n - 1]);
    return hdr->offset() + hdr->count();
  }

  // length in bytes (compressed)
  uint64_t length() const {
    unsigned n = m_blks.length();
    if (ZuUnlikely(!n)) return 0;
    auto hdr = this->hdr(m_blks[n - 1]);
    return (n - 1) * BufSize + hdr->length();
  }

  template <typename Decoder>
  auto reader(uint64_t offset = 0) const {
    return Reader<Series, Decoder>::reader(this, offset);
  }
  template <typename Decoder>
  auto index(const ZuFixed &value) const {
    return Reader<Series, Decoder>::index(this, value);
  }

  template <typename Encoder>
  auto writer() { return Writer<Series, Encoder>{this}; }

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

  template <typename Decoder>
  Decoder firstReader_(
      ZmRef<Buf> &buf, unsigned search, uint64_t offset) const {
    unsigned blkIndex = ZuSearchPos(search);
    if (blkIndex >= m_blks.length()) goto null;
    if (!(buf = loadBuf(blkIndex))) goto null;
    {
      auto reader = buf->reader<Decoder>();
      auto offset_ = buf->hdr()->offset();
      if (offset_ >= offset) return reader;
      if (!reader.seek(offset - offset_)) goto null;
      return reader;
    }
  null:
    buf = nullptr;
    return Decoder{};
  }
  template <typename Decoder>
  Decoder firstIndex_(
      ZmRef<Buf> &buf, unsigned search, const ZuFixed &value) const {
    unsigned blkIndex = ZuSearchPos(search);
    if (blkIndex >= m_blks.length()) goto null;
    if (!(buf = loadBuf(blkIndex))) goto null;
    {
      auto reader = buf->reader<Decoder>();
      bool found = reader.search(
	  [&value, exponent = buf->hdr()->exponent()](int64_t skip) {
	    return value < ZuFixed{skip, exponent};
	  });
      if (!found) goto null;
      return reader;
    }
  null:
    buf = nullptr;
    return Decoder{};
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
  template <typename Decoder>
  auto indexSearch(const ZuFixed &value) const {
    return [this, value](const Blk &blk) -> int {
      unsigned blkIndex = &const_cast<Blk &>(blk) - &m_blks[0];
      auto buf = loadBuf(blkIndex);
      if (!buf) return -1;
      auto reader = buf->template reader<Decoder>();
      auto hdr = buf->hdr();
      ZuFixed value_{static_cast<int64_t>(0), hdr->exponent()};
      if (!reader.read(value_.value)) return -1;
      value_.value = value_.adjust(value.exponent);
      if (value.value < value_.value) {
	int64_t delta = value_.value - value.value;
	if (ZuUnlikely(delta >= static_cast<int64_t>(INT_MAX)))
	  return INT_MIN;
	return -static_cast<int>(delta);
      }
      value_.value = hdr->last;
      value_.value = value_.adjust(value.exponent);
      if (value.value > value_.value) {
	int64_t delta = value.value - value_.value;
	if (ZuUnlikely(delta >= static_cast<int64_t>(INT_MAX)))
	  return INT_MAX;
	return static_cast<int>(delta);
      }
      return 0;
    };
  }

  template <typename Decoder>
  Decoder firstReader(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<Decoder>(buf,
	ZuInterSearch(&m_blks[0], m_blks.length(), offsetSearch(offset)),
	offset);
  }
  template <typename Decoder>
  Decoder firstRdrFwd(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<Decoder>(buf,
	ZuInterSearch(
	  &m_blks[buf->blkIndex], m_blks.length() - buf->blkIndex,
	  offsetSearch(offset)),
	offset);
  }
  template <typename Decoder>
  Decoder firstRdrRev(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<Decoder>(buf,
	ZuInterSearch(&m_blks[0], buf->blkIndex + 1, offsetSearch(offset)),
	offset);
  }

  template <typename Decoder>
  Decoder firstIndex(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<Decoder>(buf,
	ZuInterSearch(
	  &m_blks[0], m_blks.length(),
	  indexSearch<Decoder>(value)),
	value);
  }
  template <typename Decoder>
  Decoder firstIdxFwd(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<Decoder>(buf,
	ZuInterSearch(
	  &m_blks[buf->blkIndex], m_blks.length() - buf->blkIndex,
	  indexSearch<Decoder>(value)),
	value);
  }
  template <typename Decoder>
  Decoder firstIdxRev(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<Decoder>(buf,
	ZuInterSearch(
	  &m_blks[0], buf->blkIndex + 1,
	  indexSearch<Decoder>(value)),
	value);
  }

  template <typename Decoder>
  Decoder nextReader(ZmRef<Buf> &buf) const {
    unsigned blkIndex = buf->blkIndex + 1;
    if (blkIndex >= m_blks.length()) goto null;
    if (!(buf = loadBuf(blkIndex))) goto null;
    return buf->reader<Decoder>();
  null:
    buf = nullptr;
    return Decoder{};
  }

  template <typename Encoder>
  Encoder firstWriter(ZmRef<Buf> &buf) {
    return nextWriter<Encoder>(buf);
  }
  template <typename Encoder>
  Encoder nextWriter(ZmRef<Buf> &buf) {
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
    return buf->writer<Encoder>();
  }

  void purge_(unsigned blkIndex) {
    m_mgr->purge(m_seriesID, m_blkOffset += blkIndex);
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
    return m_mgr->loadHdr(m_seriesID, i, hdr);
  }
  ZmRef<Buf> load(unsigned i) const {
    ZmRef<Buf> buf = new Buf{m_mgr, m_seriesID, i};
    if (m_mgr->load(m_seriesID, i, buf->data()))
      return buf;
    return nullptr;
  }
  void save(ZmRef<Buf> buf) const {
    return m_mgr->save(ZuMv(buf));
  }

private:
  Mgr		*m_mgr = nullptr;
  ZtArray<Blk>	m_blks;
  unsigned	m_seriesID = 0;
  unsigned	m_blkOffset = 0;
};

} // namespace Zdf

#endif /* ZdfSeries_HPP */
