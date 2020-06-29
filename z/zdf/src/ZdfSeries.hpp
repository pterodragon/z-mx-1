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

#include <zlib/ZvField.hpp>

namespace Zdf {

#pragma pack(push, 1)
struct Hdr {
  using UInt64 = ZuLittleEndian<uint64_t>;

private:
  constexpr static uint64_t countMask() { return 0xfffffffULL; }
  constexpr static uint64_t lengthMask() { return countMask()<<28U; }
  constexpr static unsigned lengthShift() { return 28U; }
public:
  constexpr static uint64_t lengthMax() { return countMask(); }
private:
  constexpr static uint64_t exponentMask_() { return 0xffULL; }
  constexpr static uint64_t exponentMask() { return exponentMask_()<<56U; }
  constexpr static unsigned exponentShift() { return 56U; }

  uint64_t cle() const { return cle_; }
  void cle(uint64_t v) { cle_ = v; }

public:
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
    return (cle() & exponentMask())>>exponentShift();
  }
  void exponent(uint64_t v) {
    cle(cle() & ~exponentMask()) | (v<<exponentShift());
  }
  // offset (as a value count) of this buffer
  uint64_t offset() const { return offset_; }
  void offset(uint64_t v) { offset_ = v; }

  UInt64	offset_ = 0;
  UInt64	cle_ = 0;	// count/length/exponent
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

  void	*mgr;
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
using BufUnloadFn = ZmFn<BufLRUNode *>;

class ZdfAPI Mgr {
public:
  void init(unsigned maxBufs);

  unsigned alloc(BufUnloadFn unloadFn);
  void free(unsigned seriesID);

protected:
  void shift() {
    if (m_lru.count() >= m_maxBufs) {
      auto node = m_lru.shiftNode();
      (m_unloadFn[node->seriesID])(node.ptr());
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

// TCP over Ethernet maximum payload is 1460 (without Jumbo frames)
enum { BufSize = 1460 };

template <typename Heap>
class Buf_ : public ZmPolymorph, public BufLRUNode, public Heap {
public:
  enum { Size = BufSize };

  Buf() { new (&data[0]) Hdr{}; }

  uint8_t *data() { return &m_data[0]; }
  const uint8_t *data() const { return &m_data[0]; }

private:
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
    auto start = data() + sizeof(Hdr) + hdr()->length();
    auto end = data() + Size;
    return Writer{start, end};
  }
  template <typename Writer>
  void endWriting(const Writer &writer) {
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
  uint8_t	m_data[Size];
};
struct Buf_HeapID {
  static const char *id() { return "ZdfSeries.Buf"; }
};
using Buf = Buf_<ZmHeap<Buf_HeapID, sizeof(Buf_<ZuNull>)>>;

template <typename Series, typename BufReader_>
class Reader {
public:
  using Buf = typename Series::Buf;
  using BufReader = BufReader_;

  Reader() { }
  Reader(const Reader &r) :
      m_series(r.m_series), m_blkIndex(r.m_blkIndex),
      m_buf(r.m_buf), m_bufReader(r.m_bufReader) { }
  Reader &operator =(const Reader &r) {
    this->~Reader(); // nop
    new (this) Reader{r};
    return *this;
  }
  Reader(Reader &&r) noexcept :
      m_series(r.m_series), m_blkIndex(r.m_blkIndex),
      m_buf(r.m_buf), m_bufReader(ZuMv(r.m_bufReader)) { }
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
      m_bufReader = m_series->template nextReader<Reader>(m_buf);
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
  using Buf = typename Series::Buf;
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
    m_buf->endWriting(m_bufWriter);
    m_series->save_(m_buf);
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
      m_buf->endWriting(m_bufWriter);
      m_series->save_(m_buf);
      m_bufWriter = m_series->template nextWriter<BufWriter>(m_buf);
      if (ZuUnlikely(!m_bufWriter)) return false;
      hdr = m_buf->hdr();
      hdr->exponent(value.exponent);
      if (ZuUnlikely(!m_bufWriter.write(value.value))) return false;
    }
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
template <typename Impl> class Series {
  Impl *impl() { return static_cast<Impl *>(this); }
  const Impl *impl() const { return static_cast<const Impl *>(this); }

public:
  void init(Mgr *mgr) {
    m_mgr = mgr;
    m_seriesID = mgr->alloc(
	BufUnloadFn{this, [](Series *this_, BufLRUNode *node) {
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
    unsigned n = m_blks.length()
    if (ZuUnlikely(!n)) return 0;
    auto hdr = this->hdr(m_blks[n - 1]);
    return hdr->offset() + hdr->count();
  }

  template <typename BufReader>
  auto reader(uint64_t offset = 0) const {
    return Reader<Impl, BufReader>::reader(this, offset);
  }
  template <typename BufReader>
  auto index(const ZuFixed &value) const {
    return Reader<Impl, BufReader>::index(this, value);
  }

  template <typename BufWriter>
  auto writer() { return Writer<Impl, BufWriter>{this}; }

private:
  using Blk = ZuUnion<Hdr, ZmRef<Buf>>;

  static const Hdr *hdr_(const ZuNull &) { return nullptr; } // never called
  static const Hdr *hdr_(const Hdr &hdr) { return &hdr; }
  static const Hdr *hdr_(const ZmRef<Buf> &buf) { return buf->hdr(); }
  static const Hdr *hdr(const Blk &blk) {
    return blk.cdispatch([](auto &&v) { return Series::hdr_(v); }); 
  }

  Buf *loadBuf(unsigned blkIndex) const {
    auto &blk = m_blks[blkIndex];
    ZmRef<Buf> buf;
    Buf *buf_;
    if (ZuUnlikely(!blk.contains<ZmRef<Buf>>())) {
      if (ZuUnlikely(!blk.contains<Hdr>())) return nullptr;
      m_mgr->shift(); // might call unloadBuf()
      buf = impl()->load(blkIndex + m_blkOffset);
      if (!buf) return nullptr;
      buf_ = buf.ptr();
      blk.v<ZmRef<Buf>>(ZuMv(buf));
      m_mgr->push(buf_);
    } else {
      buf_ = blk.v<ZmRef<Buf>>().ptr();
      m_mgr->use(buf_);
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

  template <typename Reader>
  Reader firstReader_(
      ZmRef<Buf> &buf, unsigned search, uint64_t offset) const {
    if (!ZuSearchFound(search)) goto null;
    unsigned blkIndex;
    if (!(buf = loadBuf(blkIndex = ZuSearchPos(search)))) goto null;
    {
      auto reader = buf->reader<BufReader>();
      offset -= buf->hdr()->offset();
      {
	ZuFixed skip;
	while (reader.count() < offset) reader.read(skip);
      }
      return reader;
    }
  null:
    buf = nullptr;
    return Reader();
  }
  template <typename Reader>
  Reader firstIndex_(
      ZmRef<Buf> &buf, unsigned search, const ZuFixed &value) const {
    blkIndex = ZuSearchPos(search);
    if (blkIndex >= m_blks.length()) return Reader();
    if (!(buf = loadBuf(blkIndex))) goto null;
    {
      Reader reader = buf->reader<BufReader>();
      {
	ZuFixed skip;
	Reader next = reader;
	while (next.read(skip) && value < skip) reader = next;
      }
      return reader;
    }
  null:
    buf = nullptr;
    blkIndex = 0;
    return Reader();
  }

  auto offsetSearch(uint64_t offset) const {
    return [offset](const Blk &blk) {
      auto hdr = Series::hdr(blk);
      auto hdrOffset = hdr->offset();
      if (offset < hdrOffset) return -1;
      if (offset >= (hdrOffset + hdr->count())) return 1;
      return 0;
    };
  }
  auto indexSearch(const ZuFixed &value) const {
    return [this, value](const Blk &blk) {
      unsigned blkIndex = &const_cast<Blk &>(blk) - &m_blks[0];
      auto buf = loadBuf(blkIndex);
      if (!buf) return -1;
      auto reader = buf->reader<BufReader>();
      ZuFixed value_;
      if (!reader.read(value_)) return -1;
      if (value < value_) return -1;
      return 1;
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
	  &m_blks[blkIndex], m_blks.length() - blkIndex,
	  offsetSearch(offset)),
	offset);
  }
  template <typename BufReader>
  BufReader firstRdrRev(ZmRef<Buf> &buf, uint64_t offset) const {
    return firstReader_<BufReader>(buf,
	ZuInterSearch(&m_blks[0], blkIndex + 1, offsetSearch(offset)),
	offset);
  }

  template <typename BufReader>
  BufReader firstIndex(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(&m_blks[0], m_blks.length(), indexSearch(value)),
	value);
  }
  template <typename BufReader>
  BufReader firstIdxFwd(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(
	  &m_blks[buf->blkIndex], m_blks.length() - buf->blkIndex,
	  indexSearch(value)),
	value);
  }
  template <typename BufReader>
  BufReader firstIdxRev(ZmRef<Buf> &buf, const ZuFixed &value) const {
    return firstIndex_<BufReader>(buf,
	ZuInterSearch(&m_blks[0], buf->blkIndex + 1, indexSearch(value)),
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
    return BufReader();
  }

  template <typename BufWriter>
  Writer firstWriter(ZmRef<Buf> &buf) {
    if (unsigned n = m_blks.length()) {
      if (!(buf = loadBuf(n - 1))) return BufWriter();
      if (buf->space()) return buf->writer<BufWriter>();
    }
    return nextWriter<BufWriter>(buf);
  }
  template <typename BufWriter>
  Writer nextWriter(ZmRef<Buf> &buf) {
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
    new (buf->hdr()) Hdr{offset, 0, 0};
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

class FileLRU =
  ZmList<ZuNull,
    ZmListObject<ZuNull,
      ZmListNodeIsItem<true,
	ZmListHeapID<ZuNull,
	  ZmListLock<ZmNoLock> > > > >;
using FileLRUNode = FileLRU::Node;

ZuDeclTuple(FileID, (unsigned, seriesID), (unsigned, fileIndex));
ZuDeclTuple(FilePos, (unsigned, index), (unsigned, offset));

struct File_ : public ZuObject, public ZiFile, public FileLRUNode {
  FileID	id;
};
struct File_IDAccessor : public ZuAccessor<File_, FileID> {
  ZuInline static unsigned value(const File_ &file) {
    return file.id;
  }
};

struct File_HeapID {
  static const char *id() { return "ZdfSeries.File"; }
};
using FileHash =
  ZmHash<File_,
    ZmHashObject<ZuObject,
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

  const ZiFile::Path &dir() const { return m_dir; }
  const ZiFile::Path &coldDir() const { return m_coldDir; }

  bool open(
      unsigned seriesID, ZuString parent, ZuString name, OpenFn openFn);
  void close(unsigned seriesID);

  ZuRef<File> getFile(FileID fileID, bool create);
  ZuRef<File> openFile(FileID fileID, bool create);
  void archiveFile(FileID fileID);

  bool loadHdr(
      unsigned seriesID, unsigned blkIndex, Hdr &hdr);
  bool load(
      unsigned seriesID, unsigned blkIndex, void *buf);
  template <typename Buf>
  void save(ZmRef<Buf> buf) {
    m_sched->run(m_writeTID, ZmFn<>{ZuMv(buf), [](Buf *buf) {
      static_cast<FileMgr *>(buf->mgr)->save_(
	  buf->seriesID, buf->blkIndex, Buf::Size, buf->data());
    }});
  }

  void purge(unsigned seriesID, unsigned blkIndex);

private:
  void save_(unsigned seriesID, unsigned blkIndex, const void *buf);

  struct SeriesData {
    ZiFile::Path	dir;
    unsigned		minFileIndex = 0;
    unsigned		fileBlks = 0;

    unsigned fileSize() const { return fileBlks * BufSize; }
  };

  const ZiFile::Path &pathName(unsigned seriesID) {
    return m_series[seriesID].path;
  }
  ZiFile::Path fileName(const FileID &fileID) {
    return fileName(pathName(fileID.seriesID()), fileID.index());
  }
  static ZiFile::Path fileName(const ZiFile::Path &path, unsigned fileIndex) {
    return ZiFile::append(path, ZuStringN<20>() <<
	ZuBox<unsigned>(fileIndex).hex(ZuFmt::Right<8>()) << ".sdb");
  }
  FilePos pos(unsigned seriesID, unsigned blkIndex) {
    auto fileBlks = m_series[seriesID].fileBlks;
    return FilePos{
      .index = blkIndex / fileBlks;
      .offset = (blkIndex % fileBlks) * BufSize
    };
  }

  FileLRUNode *shift() {
    if (m_lru.count() >= m_maxOpenFiles) return m_lru.shiftNode();
    return nullptr;
  }
  void push(FileLRUNode *node) { m_lru.push(node); }
  void use(FileLRUNode *node) { m_lru.push(m_lru.del(node)); }
  void del(FileLRUNode *node) { m_lru.del(node); }

  // FIXME - telemetry

  ZtArray<SeriesData>	m_series;	// indexed by seriesID
  FileHash		m_files;
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

class FileSeries : public Series<FileSeries> {
public:
  using Base = Series<FileSeries>;
friend Base;

  FileMgr *mgr() const { return static_cast<FileMgr *>(Base::mgr()); }

  void init(FileMgr *mgr) { Base::init(mgr); }
  void final() { mgr()->final(); }

  bool open(ZuString parent, ZuString name) {
    return mgr()->open(this->seriesID(), parent, name, OpenFn{
      this, [](Series *this_, unsigned blkOffset) {
	this_->open_(blkOffset);
      }});
  }
  void close() {
    mgr()->close(this->seriesID());
    Base::close();
  }

  bool loadHdr(unsigned i, Hdr &hdr) {
    return mgr()->loadHdr(this->seriesID(), i, hdr);
  }
  ZmRef<Buf> load(unsigned i) {
    ZmRef<Buf> buf = new Buf{mgr(), this->seriesID(), i};
    if (mgr()->load(this->seriesID(), i, buf->data()))
      return buf;
    return nullptr;
  }
  void save(ZmRef<Buf> buf) {
    return mgr()->save(ZuMv(buf));
  }
  void purge(unsigned i) {
    mgr()->purge(this->seriesID(), i);
  }
};

} // namespace Zdf

#endif /* ZdfSeries_HPP */
