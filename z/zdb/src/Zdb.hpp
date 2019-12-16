//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

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

// Z Database

#ifndef Zdb_HPP
#define Zdb_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdbLib_HPP
#include <zlib/ZdbLib.hpp>
#endif

#include <lz4.h>

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuPOD.hpp>

#include <zlib/ZmAssert.hpp>
#include <zlib/ZmBackTrace.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmSpecific.hpp>
#include <zlib/ZmFn.hpp>
#include <zlib/ZmHeap.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmPLock.hpp>

#include <zlib/ZtString.hpp>
#include <zlib/ZtEnum.hpp>

#include <zlib/ZePlatform.hpp>
#include <zlib/ZeLog.hpp>

#include <zlib/ZiFile.hpp>
#include <zlib/ZiMultiplex.hpp>

#include <zlib/Zfb.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvTelemetry.hpp>

#include <zlib/telemetry_fbs.h>

#if defined(ZDEBUG) && !defined(ZdbRep_DEBUG)
#define ZdbRep_DEBUG
#endif

#ifdef ZdbRep_DEBUG
#define ZdbDEBUG(env, e) do { if ((env)->debug()) ZeLOG(Debug, (e)); } while (0)
#else
#define ZdbDEBUG(env, e) ((void)0)
#endif

typedef uint32_t ZdbID;		// database ID
typedef uint64_t ZdbRN;		// record ID
#define ZdbNullRN (~((uint64_t)0))
#define ZdbMaxRN ZdbNullRN

#define ZdbFileRecs	16384
#define ZdbFileShift	14
#define ZdbFileMask	0x3fffU

namespace ZdbOp {
  enum { Add = 0, Upd, Del };
  inline static const char *name(int op) {
    static const char *names[] = { "Add", "Upd", "Del" };
    if (ZuUnlikely(op < 0 || op >= (int)(sizeof(names) / sizeof(names[0]))))
      return "Unk";
    return names[op];
  }
};

// replication protocol messages

namespace Zdb_Msg {
  enum { HB = 0, Rep, Rec };
};
#pragma pack(push, 1)
struct Zdb_Msg_HB {	// heartbeat
  uint16_t	hostID;
  uint16_t	state;
  uint16_t	dbCount;		// followed by RNs
};
struct Zdb_Msg_Rep {	// replication
  ZdbID		db;
  ZdbRN		rn;			// followed by record
  ZdbRN		prevRN;
  uint32_t	range;
  uint16_t	clen;
  uint8_t	op;			// ZdbOp
};
struct Zdb_Msg_Hdr {	// header
  union {
    struct Zdb_Msg_HB	  hb;
    struct Zdb_Msg_Rep	  rep;
  }			u;
  uint8_t		type;
};
#pragma pack(pop)

class ZdbEnv;				// database environment
class ZdbAny;				// individual database (generic)
template <typename> class Zdb;		// individual database (type-specific)
class ZdbAnyPOD;			// in-memory record (generic)
class ZdbHost;				// host
class Zdb_Cxn;				// cxn

class ZdbRange {
public:
  ZuInline ZdbRange() : m_val(0) { }

  ZuInline ZdbRange(const ZdbRange &r) : m_val(r.m_val) { }
  ZuInline ZdbRange &operator =(const ZdbRange &r)
    { if (this != &r) m_val = r.m_val; return *this; }

  template <typename V> ZuInline ZdbRange(const V &v) : m_val(v) { }
  template <typename V> ZuInline ZdbRange &operator =(const V &v)
    { m_val = v; return *this; }

  ZuInline ZdbRange(unsigned off, unsigned len) : m_val((off<<16) | len) { }

  ZuInline bool operator !() const { return !m_val; }

  ZuInline operator uint32_t() const { return m_val; }

  ZuInline unsigned off() const { return m_val>>16; }
  ZuInline unsigned len() const { return m_val & 0xffff; }

  ZuInline void init(unsigned off, unsigned len) {
    m_val = (off<<16) | len;
  }

  inline void merge(unsigned off, unsigned len) {
    unsigned old = m_val;
    unsigned newOff, newLen;
    if (!old) {
      newOff = off;
      newLen = len;
    } else {
      unsigned oldOff = old>>16;
      unsigned oldLen = old & 0xffff;
      newOff = (off > oldOff) ? oldOff : off;
      newLen =
	(off + len > oldOff + oldLen) ?
	(off + len) - newOff :
	(oldOff + oldLen) - newOff;
    }
    m_val = (newOff<<16) | newLen;
  }

  template <typename S> inline void print(S &s) const {
    if (!m_val)
      s << "null";
    else
      s << off() << ':' << len();
  }

private:
  uint32_t	m_val;
};
template <> struct ZuPrint<ZdbRange> : public ZuPrintFn { };

struct Zdb_File_IndexAccessor;

class Zdb_File_ : public ZmPolymorph, public ZiFile {
friend struct Zdb_File_IndexAccessor;

public:
  inline Zdb_File_(unsigned index) : m_index(index) {
    memset((void *)m_undeleted, 0xff, ZdbFileRecs>>3);
  }

  ZuInline unsigned index() const { return m_index; }

  ZuInline bool del(unsigned i) {
    uint64_t m = (((uint64_t)1)<<(i & 63U));
    unsigned o = i>>6U;
    if (m_undeleted[o] & m) {
      --m_undelCount;
      m_undeleted[o] &= ~m;
    }
    return !m_undelCount;
  }

  void checkpoint() { sync(); }

private:
  unsigned	m_index = 0;
  unsigned	m_undelCount = ZdbFileRecs;
  uint64_t	m_undeleted[ZdbFileRecs>>6];
};

typedef ZmList<Zdb_File_,
	  ZmListObject<ZmPolymorph,
	    ZmListNodeIsItem<true,
	      ZmListHeapID<ZuNull,
		ZmListLock<ZmNoLock> > > > > Zdb_FileLRU;
typedef Zdb_FileLRU::Node Zdb_FileLRUNode;

struct Zdb_FileIndexAccessor : public ZuAccessor<Zdb_FileLRUNode, unsigned> {
  inline static unsigned value(const Zdb_FileLRUNode &node) {
    return node.index();
  }
};

struct Zdb_FileHeapID {
  inline static const char *id() { return "Zdb_File"; }
};
typedef ZmHash<Zdb_FileLRUNode,
	  ZmHashObject<ZmPolymorph,
	    ZmHashNodeIsKey<true,
	      ZmHashIndex<Zdb_FileIndexAccessor,
		ZmHashHeapID<Zdb_FileHeapID,
		  ZmHashLock<ZmNoLock> > > > > > Zdb_FileHash;
typedef Zdb_FileHash::Node Zdb_File;

class Zdb_FileRec {
public:
  ZuInline Zdb_FileRec() : m_file(0), m_offRN(0) { }
  ZuInline Zdb_FileRec(ZmRef<Zdb_File> file, unsigned offRN) :
    m_file(ZuMv(file)), m_offRN(offRN) { }

  ZuInline bool operator !() const { return !m_file; }
  ZuOpBool

  ZuInline Zdb_File *file() const { return m_file; }
  ZuInline unsigned offRN() const { return m_offRN; }

private:
  ZmRef<Zdb_File>	m_file;
  unsigned		m_offRN;
};

#define ZdbSchema    0x2db5ce3a // "Zdb schema"
#define ZdbAllocated 0xa110c8ed // "allocated"
#define ZdbCommitted 0xc001da7a // "cool data"
#define ZdbDeleted   0xdeadda7a	// "dead data"

#pragma pack(push, 1)
struct ZdbTrailer {
  ZdbRN		rn;
  ZdbRN		prevRN;
  uint32_t	magic;
};
#pragma pack(pop)

typedef ZmList<ZmPolymorph,
	  ZmListObject<ZmPolymorph,
	    ZmListNodeIsItem<true,
	      ZmListHeapID<ZuNull,
		ZmListLock<ZmNoLock> > > > > ZdbLRU;
typedef ZdbLRU::Node ZdbLRUNode;

struct ZdbLRUNode_RNAccessor : public ZuAccessor<ZdbLRUNode, ZdbRN> {
  static ZdbRN value(const ZdbLRUNode &pod);
};

struct Zdb_Cache_ID {
  inline static const char *id() { return "Zdb.Cache"; }
};

typedef ZmHash<ZdbLRUNode,
	  ZmHashObject<ZmPolymorph,
	    ZmHashNodeIsKey<true,
	      ZmHashIndex<ZdbLRUNode_RNAccessor,
		ZmHashHeapID<ZuNull,
		  ZmHashID<Zdb_Cache_ID,
		    ZmHashLock<ZmNoLock> > > > > > > Zdb_Cache;
typedef Zdb_Cache::Node Zdb_CacheNode;

class ZdbAnyPOD_Cmpr;
class ZdbAnyPOD_Send__;

class ZdbAPI ZdbAnyPOD : public Zdb_CacheNode, public ZuPrintable {
friend class ZdbAny;
friend class Zdb_Cxn;
friend class ZdbAnyPOD_Send__;
friend class ZdbEnv;

protected:
  ZuInline ZdbAnyPOD(ZdbAny *db) : m_db(db) { }

public:
  template <typename T = void> ZuInline const T *ptr() const {
    return static_cast<const ZuAnyPOD_<ZdbAnyPOD> *>(this)->ptr<T>();
  }
  template <typename T = void> ZuInline T *ptr() {
    return static_cast<ZuAnyPOD_<ZdbAnyPOD> *>(this)->ptr<T>();
  }
  ZuInline unsigned size() const {
    return static_cast<const ZuAnyPOD_<ZdbAnyPOD> *>(this)->size();
  }

private:
  ZuInline const ZdbTrailer *trailer() const {
    return const_cast<ZdbAnyPOD *>(this)->trailer();
  }
  ZuInline ZdbTrailer *trailer() {
    auto pod = static_cast<ZuAnyPOD_<ZdbAnyPOD> *>(this);
    return (ZdbTrailer *)
      ((char *)pod->ptr() + pod->size() - sizeof(ZdbTrailer));
  }

public:
  ZuInline ZdbAny *db() const { return m_db; }

  ZuInline ZdbRN rn() const { return trailer()->rn; }
  ZuInline ZdbRN prevRN() const { return trailer()->prevRN; }
  ZuInline uint32_t magic() const { return trailer()->magic; }

  ZuInline bool committed() const {
    return trailer()->magic == ZdbCommitted;
  }

  ZuInline void range(ZdbRange range) {
    Zdb_Msg_Rep &rep = m_hdr.u.rep;
    rep.range = range;
  }
  ZuInline ZdbRange range() const {
    const Zdb_Msg_Rep &rep = m_hdr.u.rep;
    return ZdbRange{rep.range};
  }

  ZuInline void pin() { m_pinned = true; }
  ZuInline void unpin() { m_pinned = false; }
  ZuInline bool pinned() const { return m_pinned; }

private:
  inline void placeholder() {
    ZdbTrailer *trailer = this->trailer();
    trailer->rn = trailer->prevRN = ZdbNullRN;
    trailer->magic = ZdbAllocated;
    this->range(ZdbRange{});
  }

  inline void init(ZdbRN rn, ZdbRange range,
      uint32_t magic = ZdbAllocated) {
    ZdbTrailer *trailer = this->trailer();
    trailer->rn = trailer->prevRN = rn;
    trailer->magic = magic;
    this->range(range);
  }

  inline void update(ZdbRN rn, ZdbRN prevRN, ZdbRange range,
      uint32_t magic = ZdbAllocated) {
    ZdbTrailer *trailer = this->trailer();
    trailer->rn = rn;
    trailer->prevRN = prevRN;
    trailer->magic = magic;
    this->range(range);
  }

  ZuInline void commit() {
    trailer()->magic = ZdbCommitted;
  }
  ZuInline void del() {
    trailer()->magic = ZdbDeleted;
    this->range(ZdbRange{});
  }

public:
  template <typename S> inline void print(S &s) const {
    s << "rn=" << rn()
      << " prevRN=" << prevRN()
      << " magic=" << ZuBoxed(magic()).hex();
  }

private:
  void replicate(int type, int op, bool compress);

  void send(ZiIOContext &io);
  int write();

  virtual ZmRef<ZdbAnyPOD_Cmpr> compress() = 0;

  void sent(ZiIOContext &io);
  void sent2(ZiIOContext &io);
  void sent3(ZiIOContext &io);

private:
  ZdbAny		*m_db;
  ZmRef<ZdbAnyPOD_Cmpr>	m_compressed;
  Zdb_Msg_Hdr		m_hdr;
  bool			m_pinned = false;
};

inline ZdbRN ZdbLRUNode_RNAccessor::value(const ZdbLRUNode &pod)
{
  return static_cast<const ZdbAnyPOD &>(pod).rn();
}

class ZdbAnyPOD_Cmpr : public ZuObject {
public:
  ZuInline ZdbAnyPOD_Cmpr(void *ptr, unsigned size) :
    m_ptr(ptr), m_size(size) { }

  ZuInline void *ptr() const { return m_ptr; }
  ZuInline unsigned size() const { return m_size; }

  int compress(const char *src, unsigned size);

private:
  void		*m_ptr;
  unsigned	m_size;
};

// Note: ptr and range can be null if op is ZdbOp::Delete

// AllocFn - called to allocate/initialize new record from memory
typedef ZmFn<ZdbAny *, ZmRef<ZdbAnyPOD> &> ZdbAllocFn;
// AddFn(pod, op, recovered) - record is recovered, or new/update replicated
typedef ZmFn<ZdbAnyPOD *, int, bool> ZdbAddFn;
// WriteFn(pod, op) - write drop copy
typedef ZmFn<ZdbAnyPOD *, int> ZdbWriteFn;

struct ZdbPOD_HeapID {
  inline static const char *id() { return "ZdbPOD"; }
};
// heap ID can be specialized by app
template <typename T> struct ZdbPOD_Cmpr_HeapID {
  inline static const char *id() { return "ZdbPOD_Cmpr"; }
};

template <typename T_>
struct ZdbData : public T_, public ZdbTrailer {
  using T = T_;
};

template <typename T_, class Heap>
class ZdbPOD_ : public Heap, public ZuPOD_<ZdbData<T_>, ZdbAnyPOD> {
friend class Zdb<T_>;
friend class ZdbAnyPOD_Send__;

  typedef ZuPOD_<ZdbData<T_>, ZdbAnyPOD> Base;

public:
  typedef T_ T;
  typedef ZdbData<T> Data;

  inline ZdbPOD_(ZdbAny *db) : Base(db) { }

  ZuInline static const ZdbPOD_ *pod(const T *data) {
    const Data *ZuMayAlias(ptr) = (const Data *)data;
    return static_cast<const ZdbPOD_ *>(Base::pod(ptr));
  }
  ZuInline static ZdbPOD_ *pod(T *data) {
    Data *ZuMayAlias(ptr) = (Data *)data;
    return static_cast<ZdbPOD_ *>(Base::pod(ptr));
  }

  template <typename S>
  inline void print(S &s) const {
    ZdbAnyPOD::print(s);
    s << ' ' << this->data();
  }

private:
  template <class Heap_> class Cmpr_ : public Heap_, public ZdbAnyPOD_Cmpr {
  public:
    inline Cmpr_() : 
      ZdbAnyPOD_Cmpr(m_data, LZ4_COMPRESSBOUND(sizeof(Data))) { }

  private:
    char	m_data[LZ4_COMPRESSBOUND(sizeof(Data))];
  };
  using Cmpr_Heap = ZmHeap<ZdbPOD_Cmpr_HeapID<T_>, sizeof(Cmpr_<ZuNull>)>;
  typedef Cmpr_<Cmpr_Heap> Cmpr;

  ZmRef<ZdbAnyPOD_Cmpr> compress() { return new Cmpr(); }
};
template <typename T, class HeapID = ZdbPOD_HeapID>
using ZdbPOD = ZdbPOD_<T, ZmHeap<HeapID, sizeof(ZdbPOD_<T, ZuNull>)> >;

struct ZdbConfig {
  inline ZdbConfig() { }
  inline ZdbConfig(ZuString name_, ZvCf *cf) {
    name = name_;
    path = cf->get("path", true);
#if 0
    fileSize = cf->getInt("fileSize",
	((int32_t)4)<<10, ((int32_t)1)<<30, false, 0); // range: 4K to 1G
#endif
    preAlloc = cf->getInt("preAlloc", 0, 10<<24, false, 0);
    repMode = cf->getInt("repMode", 0, 1, false, 0);
    compress = cf->getInt("compress", 0, 1, false, 0);
    cache.init(cf->get("cache", false, "Zdb.Cache"));
    fileHash.init(cf->get("fileHash", false, "Zdb.FileHash"));
  }

  ZtString		name;
  ZtString		path;
  // unsigned		fileSize = 0;
  unsigned		preAlloc = 0;	// #records to pre-allocate
  uint8_t		repMode = 0;	// 0 - deferred, 1 - in put()
  bool			compress = false;
  ZmHashParams		cache;
  ZmHashParams		fileHash;
};

namespace ZdbCacheMode {
  using namespace ZvTelemetry::DBCacheMode;
}

struct ZdbHandler {
  ZdbAllocFn	allocFn;
  ZdbAddFn	addFn;
  ZdbWriteFn	writeFn;
};

class ZdbAPI ZdbAny : public ZmPolymorph {
friend class ZdbEnv;
friend class ZdbAnyPOD;

  struct IDAccessor;
friend struct IDAccessor;
  struct IDAccessor : public ZuAccessor<ZdbAny *, ZdbID> {
    inline static ZdbID value(const ZdbAny *db) { return db->m_id; }
  };

  typedef Zdb_Cache Cache;
  typedef Zdb_FileHash FileHash;

protected:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef ZmLock FSLock;
  typedef ZmGuard<FSLock> FSGuard;
  typedef ZmReadGuard<FSLock> FSReadGuard;

  ZdbAny(ZdbEnv *env, ZuString name, uint32_t version, int cacheMode,
      ZdbHandler handler, unsigned recSize, unsigned dataSize);

public:
  ~ZdbAny();

private:
  void init(ZdbConfig *config, ZdbID);
  void final();

  bool open();
  void close();

  bool recover();
  void checkpoint();
  void checkpoint_();

public:
  inline const ZdbConfig &config() const { return *m_config; }

  inline ZdbEnv *env() const { return m_env; }
  inline ZdbID id() const { return m_id; }
  inline unsigned recSize() { return m_recSize; }
  inline unsigned dataSize() { return m_dataSize; }
  inline unsigned cacheSize() { return m_cacheSize; } // unclean read
  inline unsigned filesMax() { return m_filesMax; }

  // first RN that is committed (will be ZdbMaxRN if DB is empty)
  inline ZdbRN minRN() { return m_minRN; }
  // next RN that will be allocated
  inline ZdbRN nextRN() { return m_nextRN; }

  // create new placeholder record (null RN, in-memory only, never in DB)
  ZmRef<ZdbAnyPOD> placeholder();

  // create new record
  ZmRef<ZdbAnyPOD> push();
  // create new record (idempotent)
  ZmRef<ZdbAnyPOD> push(ZdbRN rn);
  // allocate RN only for new record, for later use with push(rn)
  ZdbRN pushRN();
  // commit record following push() - causes replication / sync
  void put(ZdbAnyPOD *);
  // abort push()
  void abort(ZdbAnyPOD *);

  // get record
  ZmRef<ZdbAnyPOD> get(ZdbRN rn);	// use for read-only queries
  ZmRef<ZdbAnyPOD> get_(ZdbRN rn);	// use for RMW - does not update cache

  // update record
  ZmRef<ZdbAnyPOD> update(ZdbAnyPOD *prev);
  // update record (idempotent)
  ZmRef<ZdbAnyPOD> update(ZdbAnyPOD *prev, ZdbRN rn);
  // update record (with prevRN, without prev POD)
  ZmRef<ZdbAnyPOD> update_(ZdbRN prevRN);
  // update record (idempotent) (with prevRN, without prev POD)
  ZmRef<ZdbAnyPOD> update_(ZdbRN prevRN, ZdbRN rn);
  // commit record following update(), potentially a partial update
  void putUpdate(ZdbAnyPOD *, bool replace = true);

  // delete record following get() / get_()
  void del(ZdbAnyPOD *);

  // delete all records < minRN
  void purge(ZdbRN minRN);

  using Telemetry = ZvTelemetry::DB;

  void telemetry(Telemetry &data) const;

private:
  // application call handlers
  inline void alloc(ZmRef<ZdbAnyPOD> &pod) { m_handler.allocFn(this, pod); }
  void recover(ZmRef<ZdbAnyPOD> pod, int op);
  void replicate(ZdbAnyPOD *pod, void *ptr, int op);

  // push initial record
  ZmRef<ZdbAnyPOD> push_();
  // idempotent push
  ZmRef<ZdbAnyPOD> push_(ZdbRN rn);

  // low-level get, does not filter deleted records
  ZmRef<ZdbAnyPOD> get__(ZdbRN rn);

  // replication data rcvd (copy/commit, called while env is unlocked)
  ZmRef<ZdbAnyPOD> replicated(
      ZdbRN rn, ZdbRN prevRN, void *ptr, ZdbRange range, int op);

  // apply received replication data
  ZmRef<ZdbAnyPOD> replicated_(ZdbRN rn, ZdbRN prevRN, ZdbRange range, int op);

  inline ZiFile::Path dirName(unsigned i) const {
    return ZiFile::append(m_config->path, ZuStringN<8>() <<
	ZuBox<unsigned>(i>>20).hex(ZuFmt::Right<5>()));
  }
  inline ZiFile::Path fileName(ZiFile::Path dir, unsigned i) const {
    return ZiFile::append(dir, ZuStringN<12>() <<
	ZuBox<unsigned>(i & 0xfffffU).hex(ZuFmt::Right<5>()) << ".zdb");
  }
  inline ZiFile::Path fileName(unsigned i) const {
    return fileName(dirName(i), i);
  }

  Zdb_FileRec rn2file(ZdbRN rn, bool write);

  ZmRef<Zdb_File> getFile(unsigned index, bool create);
  ZmRef<Zdb_File> openFile(unsigned i, bool create);
  void delFile(Zdb_File *file);
  void recover(Zdb_File *file);
  void scan(Zdb_File *file);

  ZmRef<ZdbAnyPOD> read_(const Zdb_FileRec &);

  void write(ZmRef<ZdbAnyPOD> pod);
  void write_(ZdbRN rn, ZdbRN prevRN, const void *ptr, int op);

  void fileReadError_(Zdb_File *, ZiFile::Offset, int, ZeError e);
  void fileWriteError_(Zdb_File *, ZiFile::Offset, ZeError e);

  void cache(ZdbAnyPOD *pod);
  void cache_(ZdbAnyPOD *pod);
  void cacheDel_(ZdbRN rn);

  ZdbEnv			*m_env;
  ZdbConfig			*m_config = nullptr;
  ZdbID				m_id = 0;
  uint32_t			m_version;
  int				m_cacheMode = ZdbCacheMode::Normal;
  ZdbHandler			m_handler;
  unsigned			m_recSize = 0;
  unsigned			m_dataSize = 0;
  unsigned			m_fileSize = 0;
  Lock				m_lock;
    ZdbRN			  m_minRN = ZdbMaxRN;
    ZdbRN			  m_nextRN = 0;
    ZdbRN			  m_fileRN = 0;
    ZdbLRU			  m_lru;
    ZmRef<Zdb_Cache>		  m_cache;
    unsigned			  m_cacheSize = 0;
    uint64_t			  m_cacheLoads = 0;
    uint64_t			  m_cacheMisses = 0;
  FSLock			m_fsLock;	// guards files
    Zdb_FileLRU			  m_filesLRU;
    ZmRef<Zdb_FileHash>		  m_files;
    unsigned			  m_filesMax = 0;
    unsigned			  m_lastFile = 0;
    uint64_t			  m_fileLoads = 0;
    uint64_t			  m_fileMisses = 0;
};

template <typename T_>
class Zdb : public ZdbAny {
public:
  typedef T_ T;

  template <typename Handler>
  inline Zdb(ZdbEnv *env, ZuString name, uint32_t version, int cacheMode,
      Handler &&handler) :
    ZdbAny(env, name, version, cacheMode, ZuFwd<Handler>(handler),
	sizeof(typename ZdbPOD<T, ZuNull>::Data), sizeof(T)) { }
};

typedef ZtArray<ZdbRN> Zdb_DBState;

template <> struct ZuPrint<Zdb_DBState> : public ZuPrintDelegate {
  template <typename P, typename S>
  inline static void print(S &s, const P &p) {
    unsigned i = 0, n = p.length();
    if (ZuUnlikely(!n)) return;
    s << ZuBoxed(p[0]);
    while (++i < n) s << ',' << ZuBoxed(p[i]);
  }
};

struct ZdbHostConfig {
  inline ZdbHostConfig(const ZtString &key, ZvCf *cf) {
    id = ZvCf::toInt(cf, "ID", key, 0, 1<<30);
    priority = cf->getInt("priority", 0, 1<<30, true);
    ip = cf->get("IP", true);
    port = cf->getInt("port", 1, (1<<16) - 1, true);
    up = cf->get("up");
    down = cf->get("down");
  }

  unsigned	id = 0;
  unsigned	priority = 0;
  ZiIP		ip;
  uint16_t	port = 0;
  ZtString	up;
  ZtString	down;
};

namespace ZdbHostState {
  using namespace ZvTelemetry::DBHostState;
}

class ZdbAPI ZdbHost : public ZmPolymorph {
friend class ZdbEnv;
friend class Zdb_Cxn;
template <typename> friend struct ZuPrint;

  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  struct IDAccessor;
friend struct IDAccessor;
  struct IDAccessor : public ZuAccessor<ZdbHost *, int> {
    inline static int value(ZdbHost *h) { return h->id(); }
  };

  ZdbHost(ZdbEnv *env, const ZdbHostConfig *config);

public:
  inline const ZdbHostConfig &config() const { return *m_config; }

  inline unsigned id() const { return m_config->id; }
  inline unsigned priority() const { return m_config->priority; }
  inline ZiIP ip() const { return m_config->ip; }
  inline uint16_t port() const { return m_config->port; }

  inline bool voted() const { return m_voted; }
  inline int state() const { return m_state; }

  static const char *stateName(int);

  using Telemetry = ZvTelemetry::DBHost;

  void telemetry(Telemetry &data) const;

private:
  inline ZmRef<Zdb_Cxn> cxn() const { return m_cxn; }

  inline void state(int s) { m_state = s; }

  inline const Zdb_DBState &dbState() const { return m_dbState; }
  inline Zdb_DBState &dbState() { return m_dbState; }

  inline bool active() const {
    using namespace ZdbHostState;
    switch (m_state) {
      case Activating:
      case Active:
	return true;
    }
    return false;
  }

  inline int cmp(const ZdbHost *host) const {
    int i = m_dbState.cmp(host->m_dbState); if (i) return i;
    if (i = ZuCmp<bool>::cmp(active(), host->active())) return i;
    return ZuCmp<int>::cmp(priority(), host->priority());
  }

#if 0
  inline int cmp(const ZdbHost *host) {
    int i = cmp_(host);

    printf("cmp(host %d priority %d dbState %d, "
	   "host %d priority %d dbState %d): %d\n",
	   (int)this->id(),
	   (int)this->config().m_priority, (int)this->dbState()[0],
	   (int)host->id(),
	   (int)host->config().m_priority,
	   (int)((ZdbHost *)host)->dbState()[0], i);
    return i;
  }
#endif

  inline void voted(bool v) { m_voted = v; }

  void connect();
  void connectFailed(bool transient);
  void reconnect();
  void reconnect2();
  void cancelConnect();
  ZiConnection *connected(const ZiCxnInfo &ci);
  void associate(Zdb_Cxn *cxn);
  void disconnected();

  void reactivate();

  ZdbEnv		*m_env;
  const ZdbHostConfig	*m_config;
  ZiMultiplex		*m_mx;

  Lock			m_lock;
    ZmRef<Zdb_Cxn>	  m_cxn;

  ZmScheduler::Timer	m_connectTimer;

  int			m_state;	// guarded by ZdbEnv
  Zdb_DBState		m_dbState;	// ''
  bool			m_voted;	// ''
};
template <> struct ZuPrint<ZdbHost> : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const ZdbHost &v) {
    s << "[ID:" << v.id() << " PRI:" << v.priority() << " V:" << v.voted() <<
      " S:" << v.state() << "] " << v.dbState();
  }
};
template <> struct ZuPrint<ZdbHost *> : public ZuPrintDelegate {
  typedef ZdbHost *Zdb_HostPtr;
  template <typename S>
  inline static void print(S &s, const Zdb_HostPtr &v) {
    if (!v)
      s << "null";
    else
      s << *v;
  }
};

class Zdb_Cxn : public ZiConnection {
  Zdb_Cxn(const Zdb_Cxn &);
  Zdb_Cxn &operator =(const Zdb_Cxn &);	// prevent mis-use

friend class ZiConnection;
friend class ZiMultiplex;
friend class ZdbEnv;
friend class ZdbHost;
friend class ZdbAnyPOD_Send__;

  Zdb_Cxn(ZdbEnv *env, ZdbHost *host, const ZiCxnInfo &ci);

  inline ZdbEnv *env() const { return m_env; }
  inline void host(ZdbHost *host) { m_host = host; }
  inline ZdbHost *host() const { return m_host; }
  inline ZiMultiplex *mx() const { return m_mx; }

  void connected(ZiIOContext &);
  void disconnected();

  void msgRead(ZiIOContext &);
  void msgRcvd(ZiIOContext &);

  void hbRcvd(ZiIOContext &);
  void hbDataRead(ZiIOContext &);
  void hbDataRcvd(ZiIOContext &);
  void hbTimeout();
  void hbSend();
  void hbSend_(ZiIOContext &);
  void hbSent(ZiIOContext &);
  void hbSent2(ZiIOContext &);

  void repRcvd(ZiIOContext &);
  void repDataRead(ZiIOContext &);
  void repDataRcvd(ZiIOContext &);

  void repSend(ZmRef<ZdbAnyPOD> pod, int type, int op, bool compress);
  void repSend(ZmRef<ZdbAnyPOD> pod);

  void ackRcvd();
  void ackSend(int type, ZdbAnyPOD *pod);

  ZdbEnv		*m_env;
  ZiMultiplex		*m_mx;
  ZdbHost		*m_host;	// 0 if not yet associated

  Zdb_Msg_Hdr		m_recvHdr;
  ZtArray<char>		m_recvData;
  ZtArray<char>		m_recvData2;

  Zdb_Msg_Hdr		m_hbSendHdr;

  Zdb_Msg_Hdr		m_ackSendHdr;

  ZmScheduler::Timer	m_hbTimer;
};

// ZdbEnv configuration
struct ZdbEnvConfig {
  ZdbEnvConfig(const ZdbEnvConfig &) = delete;
  ZdbEnvConfig &operator =(const ZdbEnvConfig &) = delete;
  ZdbEnvConfig() = default;
  ZdbEnvConfig(ZdbEnvConfig &&) = default;
  ZdbEnvConfig &operator =(ZdbEnvConfig &&) = default;

  inline ZdbEnvConfig(ZvCf *cf) {
    writeThread = cf->get("writeThread", true);
    const ZtArray<ZtString> *names = cf->getMultiple("dbs", 0, 100, true);
    dbCfs.size(names->length());
    for (unsigned i = 0; i < names->length(); i++) {
      ZmRef<ZvCf> dbCf = cf->subset((*names)[i], false, true);
      ZdbConfig db((*names)[i], dbCf); // might throw, do not push() here
      new (dbCfs.push()) ZdbConfig(db);
    }
    hostID = cf->getInt("hostID", 0, 1<<30, true);
    {
      ZvCf::Iterator i(cf->subset("hosts", false, true));
      ZuString key;
      while (ZmRef<ZvCf> hostCf = i.subset(key)) {
	ZdbHostConfig host(key, hostCf); // might throw, do not push() here
	new (hostCfs.push()) ZdbHostConfig(host);
      }
    }
    nAccepts = cf->getInt("nAccepts", 1, 1<<10, false, 8);
    heartbeatFreq = cf->getInt("heartbeatFreq", 1, 3600, false, 1);
    heartbeatTimeout = cf->getInt("heartbeatTimeout", 1, 14400, false, 4);
    reconnectFreq = cf->getInt("reconnectFreq", 1, 3600, false, 1);
    electionTimeout = cf->getInt("electionTimeout", 1, 3600, false, 8);
    cxnHash.init(cf->get("cxnHash", false, "Zdb.CxnHash"));
#ifdef ZdbRep_DEBUG
    debug = cf->getInt("debug", 0, 1, false, 0);
#endif
  }

  ZmThreadName			writeThread;
  mutable unsigned		writeTID = 0;
  ZtArray<ZdbConfig>		dbCfs;
  unsigned			hostID = 0;
  ZtArray<ZdbHostConfig>	hostCfs;
  unsigned			nAccepts = 0;
  unsigned			heartbeatFreq = 0;
  unsigned			heartbeatTimeout = 0;
  unsigned			reconnectFreq = 0;
  unsigned			electionTimeout = 0;
  ZmHashParams			cxnHash;
#ifdef ZdbRep_DEBUG
  bool				debug = 0;
#endif
};

class ZdbAPI ZdbEnv : public ZuObject {
  ZdbEnv(const ZdbEnv &);
  ZdbEnv &operator =(const ZdbEnv &);		// prevent mis-use

friend class ZdbAny;
friend class ZdbHost;
friend class Zdb_Cxn;
friend class ZdbAnyPOD;
friend class ZdbAnyPOD_Send__;

  struct HostTree_HeapID {
    inline static const char *id() { return "ZdbEnv.HostTree"; }
  };
  typedef ZmRBTree<ZmRef<ZdbHost>,
	    ZmRBTreeIndex<ZdbHost::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<HostTree_HeapID> > > > > HostTree;

  struct CxnHash_HeapID {
    inline static const char *id() { return "ZdbEnv.CxnHash"; }
  };
  typedef ZmHash<ZmRef<Zdb_Cxn>,
	    ZmHashLock<ZmPLock,
	      ZmHashObject<ZuNull,
		ZmHashHeapID<CxnHash_HeapID> > > > CxnHash;

  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;
  typedef ZmCondition<Lock> StateCond;

#ifdef ZdbRep_DEBUG
  inline bool debug() const { return m_config.debug; }
#endif

public:
  ZdbEnv();
  ~ZdbEnv();

  void init(ZdbEnvConfig config, ZiMultiplex *mx,
      ZmFn<> activeFn, ZmFn<> inactiveFn);
  void final();

  bool open();
  void close();

  void start();
  void stop();

  void checkpoint();

  inline const ZdbEnvConfig &config() const { return m_config; }
  inline ZiMultiplex *mx() const { return m_mx; }

  inline int state() const {
    return m_self ? m_self->state() : ZdbHostState::Instantiated;
  }
  inline void state(int n) {
    if (!m_self) {
      ZeLOG(Fatal, ZtString() <<
	  "ZdbEnv::state(" << ZuBoxed(n) << ") called out of order");
      return;
    }
    m_self->state(n);
  }
  inline bool running() {
    using namespace ZdbHostState;
    switch (state()) {
      case Electing:
      case Activating:
      case Active:
      case Deactivating:
      case Inactive:
	return true;
    }
    return false;
  }
  inline bool active() {
    using namespace ZdbHostState;
    switch (state()) {
      case Activating:
      case Active:
	return true;
    }
    return false;
  }

  inline ZdbHost *self() const { return m_self; }
  inline ZdbHost *host(unsigned id) const { return m_hosts.findKey(id); }
  template <typename L> inline void allHosts(L l) const {
    auto i = m_hosts.readIterator();
    while (auto node = i.iterate()) { l(node->key()); }
  }

  inline ZdbAny *db(ZdbID id) const {
    if (id >= (ZdbID)m_dbs.length()) return nullptr;
    return m_dbs[id];
  }
  template <typename L> inline void allDBs(L l) const {
    for (unsigned i = 0, n = m_dbs.length(); i < n; i++)
      if (m_dbs[i]) l(m_dbs[i]);
  }

  using Telemetry = ZvTelemetry::DBEnv;

  void telemetry(Telemetry &data) const;

private:
  void add(ZdbAny *db, ZuString name);
  inline ZdbAny *db(ZdbID id) {
    if (id >= (ZdbID)m_dbs.length()) return 0;
    return m_dbs[id];
  }
  inline unsigned dbCount() { return m_dbs.length(); }

  void listen();
  void listening(const ZiListenInfo &);
  void listenFailed(bool transient);
  void stopListening();

  bool disconnectAll();

  void holdElection();	// elect new master
  void deactivate();	// become client (following dup master)
  void reactivate(ZdbHost *host);	// re-assert master

  ZiConnection *accepted(const ZiCxnInfo &ci);
  void connected(Zdb_Cxn *cxn);
  void associate(Zdb_Cxn *cxn, int hostID);
  void associate(Zdb_Cxn *cxn, ZdbHost *host);
  void disconnected(Zdb_Cxn *cxn);

  void hbDataRcvd(
      ZdbHost *host, const Zdb_Msg_HB &hb, ZdbRN *dbState);
  void vote(ZdbHost *host);

  void hbStart();
  void hbSend();		// send heartbeat and reschedule self
  void hbSend_(Zdb_Cxn *cxn);	// send heartbeat (once)

  void dbStateRefresh();	// refresh m_self->dbState() (with guard)
  void dbStateRefresh_();	// '' (unlocked)

  ZdbHost *setMaster();	// returns old master
  void setNext(ZdbHost *host);
  void setNext();

  void startReplication();
  void stopReplication();

  void repDataRcvd(ZdbHost *host, Zdb_Cxn *cxn,
      const Zdb_Msg_Rep &rep, void *ptr);

  void repSend(ZmRef<ZdbAnyPOD> pod, int type, int op, bool compress);
  void repSend(ZmRef<ZdbAnyPOD> pod);
  void recSend();

  void ackRcvd(ZdbHost *host, bool positive, ZdbID db, ZdbRN rn);

  void write(ZmRef<ZdbAnyPOD> pod, int type, int op, bool compress);

  ZdbEnvConfig		m_config;
  ZiMultiplex		*m_mx;

  ZmFn<>		m_activeFn;
  ZmFn<>		m_inactiveFn;

  Lock			m_lock;
    StateCond		m_stateCond;
    bool		m_appActive;
    ZdbHost		*m_self;
    ZdbHost		*m_master;	// == m_self if Active
    ZdbHost		*m_prev;	// previous-ranked host
    ZdbHost		*m_next;	// next-ranked host
    ZmRef<Zdb_Cxn>	m_nextCxn;	// replica peer's cxn
    bool		m_recovering;	// recovering next-ranked host
    Zdb_DBState		m_recover;	// recovery state
    Zdb_DBState		m_recoverEnd;	// recovery end
    int			m_nPeers;	// # up to date peers
					// # votes received (Electing)
					// # pending disconnects (Stopping)
    ZmTime		m_hbSendTime;

  ZmScheduler::Timer	m_hbSendTimer;
  ZmScheduler::Timer	m_electTimer;

  ZtArray<ZmRef<ZdbAny> >	m_dbs;
  HostTree			m_hosts;
  ZmRef<CxnHash>		m_cxns;
};

#endif /* Zdb_HPP */
