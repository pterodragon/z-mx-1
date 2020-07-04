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

// Data Series

#ifndef ZdfSeries_HPP
#define ZdfSeries_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZuPtr.hpp>
#include <zlib/ZuByteSwap.hpp>
#include <zlib/ZuUnion.hpp>
#include <zlib/ZuSort.hpp>

#include <zlib/ZmHeap.hpp>
#include <zlib/ZmList.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmHash.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtString.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/ZvField.hpp>
#include <zlib/ZvCf.hpp>

namespace Zdf {

#pragma pack(push, 1)
struct Hdr {
  using UInt64 = ZuLittleEndian<uint64_t>;

  UInt64	offset_ = 0;
  UInt64	cle_ = 0;	// count/length/exponent

private:
  constexpr static uint64_t countMask() { return 0xfffffffULL; }
  constexpr static unsigned lengthShift() { return 28U; }
  constexpr static uint64_t lengthMask() { return countMask()<<lengthShift(); }
public:
  constexpr static uint64_t lengthMax() { return countMask(); }
private:
  constexpr static uint64_t expMask_() { return 0x1fULL; }
  constexpr static unsigned expShift() { return 56U; }
  constexpr static uint64_t expMask() { return expMask_()<<expShift(); }

  uint64_t cle() const { return cle_; }
  void cle(uint64_t v) { cle_ = v; }

public:
  // offset (as a value count) of this buffer
  uint64_t offset() const { return offset_; }
  void offset(uint64_t v) { offset_ = v; }
  // count of values in this buffer
  unsigned count() const {
    return cle() & countMask();
  }
  void count(uint64_t v) {
    cle((cle() & ~countMask()) | v);
  }
  // length of this buffer in bytes
  unsigned length() const {
    return (cle() & lengthMask())>>lengthShift();
  }
  void length(uint64_t v) {
    cle((cle() & ~lengthMask()) | (v<<lengthShift()));
  }
  // negative decimal exponent
  unsigned exponent() const {
    return (cle() & expMask())>>expShift();
  }
  void exponent(uint64_t v) {
    cle((cle() & ~expMask()) | (v<<expShift()));
  }
};
#pragma pack(pop)

struct BufLRUNode_ {
  BufLRUNode_() = delete;
  BufLRUNode_(void *mgr_, unsigned seriesID_, unsigned blkIndex_) :
      mgr(mgr_), seriesID(seriesID_), blkIndex(blkIndex_) { }
  BufLRUNode_(const BufLRUNode_ &) = default;
  BufLRUNode_ &operator =(const BufLRUNode_ &) = default;
  BufLRUNode_(BufLRUNode_ &&) = default;
  BufLRUNode_ &operator =(BufLRUNode_ &&) = default;
  ~BufLRUNode_() = default;

  void		*mgr;
  unsigned	seriesID;
  unsigned	blkIndex;
};
struct BufLRU_HeapID {
  static const char *id() { return "ZdfSeries.BufLRU"; }
};
using BufLRU =
  ZmList<BufLRUNode_,
    ZmListObject<ZuNull,
      ZmListNodeIsItem<true,
	ZmListHeapID<ZuNull,
	  ZmListLock<ZmNoLock> > > > >;
using BufLRUNode = BufLRU::Node;

// TCP over Ethernet maximum payload is 1460 (without Jumbo frames)
enum { BufSize = 1460 };

template <typename Heap>
class Buf_ : public ZmPolymorph, public BufLRUNode, public Heap {
  using PinLock = ZmPLock;
  using PinGuard = ZmGuard<PinLock>;
  using PinReadGuard = ZmReadGuard<PinLock>;

public:
  enum { Size = BufSize };

  template <typename ...Args>
  Buf_(Args &&... args) : BufLRUNode{ZuFwd<Args>(args)...} { }

  bool pin() {
    PinGuard guard(m_pinLock);
    if (m_pinned) return false;
    return m_pinned = true;
  }
  bool pinned() const {
    PinReadGuard guard(m_pinLock);
    return m_pinned;
  }
  template <typename L>
  void unpin(L l) {
    PinGuard guard(m_pinLock);
    m_pinned = false;
    l(this);
  }

  uint8_t *data() { return &m_data[0]; }
  const uint8_t *data() const { return &m_data[0]; }

  const Hdr *hdr() const {
    return reinterpret_cast<const Hdr *>(data());
  }
  Hdr *hdr() { return reinterpret_cast<Hdr *>(data()); }

  template <typename Reader>
  Reader reader() {
    auto start = data() + sizeof(Hdr);
    return Reader{start, start + hdr()->length()};
  }

  template <typename Writer>
  Writer writer() {
    auto start = data() + sizeof(Hdr);
    auto end = data() + Size;
    return Writer{start, end};
  }
  template <typename Writer>
  void sync(const Writer &writer) {
    auto hdr = this->hdr();
    hdr->count(writer.count());
    auto start = data() + sizeof(Hdr);
    hdr->length(writer.pos() - start);
  }

  unsigned space() const {
    auto start = data() + sizeof(Hdr) + hdr()->length();
    auto end = data() + Size;
    if (start >= end) return 0;
    return end - start;
  }

private:
  PinLock		m_pinLock;
    bool		  m_pinned = false;

  uint8_t		m_data[Size];
};
struct Buf_HeapID {
  static const char *id() { return "ZdfSeries.Buf"; }
};
using Buf = Buf_<ZmHeap<Buf_HeapID, sizeof(Buf_<ZuNull>)>>;

using BufUnloadFn = ZmFn<Buf *>;

template <typename Impl> class Series_;

class ZdfAPI Mgr {
template <typename Impl> friend class Series_;

public:
  void init(unsigned maxBufs);
  void final() { }

  unsigned alloc(BufUnloadFn unloadFn);
  void free(unsigned seriesID);

protected:
  void shift() {
    if (m_lru.count() >= m_maxBufs) {
      auto lru_ = m_lru.shiftNode();
      if (ZuLikely(lru_)) {
	Buf *lru = static_cast<Buf *>(lru_);
	if (lru->pinned()) {
	  m_lru.push(ZuMv(lru_));
	  m_maxBufs = m_lru.count() + 1;
	  return;
	}
	(m_unloadFn[lru->seriesID])(lru);
      }
    }
  }
  void push(BufLRUNode *node) { m_lru.push(node); }
  void use(BufLRUNode *node) { m_lru.push(m_lru.del(node)); }
  void del(BufLRUNode *node) { m_lru.del(node); }

  void purge(unsigned seriesID, unsigned blkIndex); // caller unloads

private:
  BufLRU		m_lru;
  ZtArray<BufUnloadFn>	m_unloadFn;	// allocates seriesID
  unsigned		m_maxBufs = 0;
};

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
    m_series->save_(m_buf);
  }

  // FIXME - support both in-memory and on-disk, at this level
  // and in Zdf::DataFrame

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
      m_series->save_(m_buf);
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

// CRTP - implementation must conform to the following interface:
#if 0
struct Impl : public Series<Impl, Buf> {
using Blk = typename Series<Impl, Buf>::Blk;
bool loadHdr(unsigned i, Hdr &hdr) {
  // ... load hdr for block i
}
ZmRef<Buf> load(unsigned i) {
  ZmRef<Buf> buf = new Buf{this->mgr(), this->seriesID(), i};
  // ... load block i into buf
  return buf;
}
void save(ZmRef<Buf> buf) {
  // ... save buf
}
void purge(unsigned i) {
  // ... GC any storage for blocks < i
}
};
#endif
using OpenFn = ZmFn<unsigned>;
template <typename Impl> class Series_ {
  Impl *impl() { return static_cast<Impl *>(this); }
  const Impl *impl() const { return static_cast<const Impl *>(this); }

template <typename, typename> friend class Reader;
template <typename, typename> friend class Writer;

public:
  void init(Mgr *mgr) {
    m_mgr = mgr;
    m_seriesID = mgr->alloc(
	BufUnloadFn{this, [](Series_ *this_, BufLRUNode *node) {
	  this_->unloadBuf(node);
	}});
  }
  void final() {
    m_mgr->free(m_seriesID);
    m_blks.null();
  }

  Mgr *mgr() const { return m_mgr; }
  unsigned seriesID() const { return m_seriesID; }

protected:
  void open_(unsigned blkOffset = 0) {
    m_blkOffset = blkOffset;
    Hdr hdr;
    for (unsigned i = 0; impl()->loadHdr(i + blkOffset, hdr); i++)
      new (Blk::new_<Hdr>(m_blks.push())) Hdr{hdr};
  }

public:
  uint64_t count() const {
    unsigned n = m_blks.length();
    if (ZuUnlikely(!n)) return 0;
    auto hdr = this->hdr(m_blks[n - 1]);
    return hdr->offset() + hdr->count();
  }

  template <typename BufReader>
  auto reader(uint64_t offset = 0) const {
    return Reader<Impl, BufReader>::reader(impl(), offset);
  }
  template <typename BufReader>
  auto index(const ZuFixed &value) const {
    return Reader<Impl, BufReader>::index(impl(), value);
  }

  template <typename BufWriter>
  auto writer() { return Writer<Impl, BufWriter>{impl()}; }

private:
  using Blk = ZuUnion<Hdr, ZmRef<Buf>>;

  static const Hdr *hdr_(const ZuNull &) { return nullptr; } // never called
  static const Hdr *hdr_(const Hdr &hdr) { return &hdr; }
  static const Hdr *hdr_(const ZmRef<Buf> &buf) { return buf->hdr(); }
  static const Hdr *hdr(const Blk &blk) {
    return blk.cdispatch([](auto &&v) { return Series_::hdr_(v); }); 
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
      buf = impl()->load(blkIndex + m_blkOffset);
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
      auto hdr = Series_::hdr(blk);
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

  void save_(ZmRef<Buf> buf) const {
    impl()->save(ZuMv(buf));
  }

  void purge_(unsigned blkIndex) {
    impl()->purge(m_blkOffset += blkIndex);
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

private:
  Mgr		*m_mgr = nullptr;
  ZtArray<Blk>	m_blks;
  unsigned	m_seriesID = 0;
  unsigned	m_blkOffset = 0;
};

using FileLRU =
  ZmList<ZuNull,
    ZmListObject<ZuNull,
      ZmListNodeIsItem<true,
	ZmListHeapID<ZuNull,
	  ZmListLock<ZmNoLock> > > > >;
using FileLRUNode = FileLRU::Node;

ZuDeclTuple(FileID, (unsigned, seriesID), (unsigned, index));
ZuDeclTuple(FilePos, (unsigned, index), (unsigned, offset));

struct File_ : public ZmObject, public ZiFile, public FileLRUNode {
  template <typename ...Args>
  File_(Args &&... args) : id{ZuFwd<Args>(args)...} { }

  FileID	id;
};
struct File_IDAccessor : public ZuAccessor<File_, FileID> {
  ZuInline static const FileID &value(const File_ &file) {
    return file.id;
  }
};

struct File_HeapID {
  static const char *id() { return "ZdfSeries.File"; }
};
using FileHash =
  ZmHash<File_,
    ZmHashObject<ZmObject,
      ZmHashNodeIsKey<true,
	ZmHashIndex<File_IDAccessor,
	  ZmHashHeapID<File_HeapID,
	    ZmHashLock<ZmNoLock> > > > > >;

class ZdfAPI FileMgr : public Mgr {
public:
  struct Config {
    Config(const Config &) = delete;
    Config &operator =(const Config &) = delete;
    Config() = default;
    Config(Config &&) = default;
    Config &operator =(Config &&) = default;

    Config(ZvCf *cf) {
      dir = cf->get("dir", true);
      coldDir = cf->get("coldDir", true);
      writeThread = cf->get("writeThread", true);
      maxFileSize = cf->getInt("maxFileSize", 1, 1U<<30, false, 10<<20);
    }

    ZiFile::Path	dir;
    ZiFile::Path	coldDir;
    ZmThreadName	writeThread;
    unsigned		maxFileSize = 0;
  };

  void init(ZmScheduler *sched, Config config);
  void final();

  const ZiFile::Path &dir() const { return m_dir; }
  const ZiFile::Path &coldDir() const { return m_coldDir; }

  bool open(
      unsigned seriesID, ZuString parent, ZuString name, OpenFn openFn);
  void close(unsigned seriesID);

  ZmRef<File_> getFile(FileID fileID, bool create);
  ZmRef<File_> openFile(FileID fileID, bool create);
  void archiveFile(FileID fileID);

  bool loadHdr(
      unsigned seriesID, unsigned blkIndex, Hdr &hdr);
  bool load(
      unsigned seriesID, unsigned blkIndex, void *buf);
  template <typename Buf>
  void save(ZmRef<Buf> buf) {
    if (!buf->pin()) return;
    m_sched->run(m_writeTID, ZmFn<>{ZuMv(buf), [](Buf *buf) {
      buf->unpin([](Buf *buf) {
	static_cast<FileMgr *>(buf->mgr)->save_(
	    buf->seriesID, buf->blkIndex, buf->data());
      });
    }});
  }

  void purge(unsigned seriesID, unsigned blkIndex);

private:
  void save_(unsigned seriesID, unsigned blkIndex, const void *buf);

  struct SeriesData {
    ZiFile::Path	path;
    unsigned		minFileIndex = 0;	// earliest file index
    unsigned		fileBlks = 0;

    unsigned fileSize() const { return fileBlks * BufSize; }
  };

  const ZiFile::Path &pathName(unsigned seriesID) {
    return m_series[seriesID].path;
  }
  ZiFile::Path fileName(const FileID &fileID) {
    return fileName(pathName(fileID.seriesID()), fileID.index());
  }
  static ZiFile::Path fileName(const ZiFile::Path &path, unsigned index) {
    return ZiFile::append(path, ZuStringN<20>() <<
	ZuBox<unsigned>(index).hex(ZuFmt::Right<8>()) << ".sdb");
  }
  FilePos pos(unsigned seriesID, unsigned blkIndex) {
    auto fileBlks = m_series[seriesID].fileBlks;
    return FilePos{blkIndex / fileBlks, (blkIndex % fileBlks) * BufSize};
  }

  FileLRUNode *shift() {
    if (m_lru.count() >= m_maxOpenFiles) return m_lru.shiftNode();
    return nullptr;
  }
  void push(FileLRUNode *node) { m_lru.push(node); }
  void use(FileLRUNode *node) { m_lru.push(m_lru.del(node)); }
  void del(FileLRUNode *node) { m_lru.del(node); }

  void fileRdError_(unsigned seriesID, ZiFile::Offset, int, ZeError);
  void fileWrError_(unsigned seriesID, ZiFile::Offset, ZeError);

private:
  ZtArray<SeriesData>	m_series;	// indexed by seriesID
  ZuPtr<FileHash>	m_files;
  FileLRU		m_lru;
  ZmScheduler		*m_sched = nullptr;
  ZiFile::Path		m_dir;
  ZiFile::Path		m_coldDir;
  unsigned		m_writeTID = 0;	// write thread ID
  unsigned		m_maxFileSize;	// maximum file size
  unsigned		m_maxOpenFiles;	// maximum #files open
  unsigned		m_fileLoads = 0;
  unsigned		m_fileMisses = 0;
};

class Series : public Series_<Series> {
public:
  using Base = Series_<Series>;
friend Base;

  FileMgr *mgr() const { return static_cast<FileMgr *>(Base::mgr()); }

  void init(FileMgr *mgr) { Base::init(mgr); }

  bool open(ZuString parent, ZuString name) {
    return mgr()->open(this->seriesID(), parent, name, OpenFn{
      this, [](Series *this_, unsigned blkOffset) {
	this_->open_(blkOffset);
      }});
  }
  void close() {
    mgr()->close(this->seriesID());
    Base::final();
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
  void purge(unsigned i) {
    mgr()->purge(this->seriesID(), i);
  }
};

} // namespace Zdf

#endif /* ZdfSeries_HPP */
