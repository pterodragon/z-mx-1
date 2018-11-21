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
#include <ZdbLib.hpp>
#endif

#include <lz4.h>

#include <ZuTraits.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuPrint.hpp>

#include <ZmAssert.hpp>
#include <ZmBackTrace.hpp>
#include <ZmRef.hpp>
#include <ZmGuard.hpp>
#include <ZmSpecific.hpp>
#include <ZmFn.hpp>
#include <ZmHeap.hpp>
#include <ZmSemaphore.hpp>
#include <ZmPLock.hpp>

#include <ZtString.hpp>

#include <ZePlatform.hpp>
#include <ZeLog.hpp>

#include <ZiFile.hpp>
#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

#ifdef ZDEBUG
#define ZdbRep_DEBUG
#endif

#ifdef ZdbRep_DEBUG
#define ZdbDEBUG(env, e) do { if ((env)->debug()) ZeLOG(Debug, (e)); } while (0)
#else
#define ZdbDEBUG(env, e) ((void)0)
#endif

typedef uint32_t ZdbID;		// database ID
typedef uint64_t ZdbRN;		// record ID

namespace ZdbOp {
  enum { New = 0, Update, Delete };
  inline static const char *name(int op) {
    return op == New ? "New" : op == Update ? "Update" : "Del";
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
  uint16_t	op;			// ZdbOp
  uint16_t	clen;
};
struct Zdb_Msg_Hdr {	// header
  uint8_t		type;
  union {
    struct Zdb_Msg_HB	  hb;
    struct Zdb_Msg_Rep	  rep;
  }			u;
};
#pragma pack(pop)

class ZdbEnv;				// database environment
class Zdb_;				// individual database (generic)
template <typename> class Zdb;		// individual database (type-specific)
class ZdbAnyPOD;			// in-memory record (generic)
class ZdbGuard;				// record sequence guard
class Zdb_Host;				// host
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

struct Zdb_File_IndexAccessor;

class Zdb_File_ : public ZmPolymorph, public ZiFile {
friend struct Zdb_File_IndexAccessor;

public:
  inline Zdb_File_(unsigned index, unsigned unallocated) :
    m_index(index), m_unallocated(unallocated) { }

  ZuInline unsigned index() const { return m_index; }

  ZuInline unsigned unallocated() const { return m_unallocated; }
  ZuInline unsigned allocated() const { return m_allocated; }
  ZuInline unsigned deleted() const { return m_deleted; }

  ZuInline unsigned alloc() { --m_unallocated; return ++m_allocated; }
  ZuInline unsigned del() { --m_allocated; return ++m_deleted; }
  ZuInline unsigned skip() { --m_unallocated; return ++m_deleted; }

  void checkpoint() { sync(); }

private:
  unsigned	m_index = 0;
  unsigned	m_unallocated = 0;
  unsigned	m_allocated = 0;
  unsigned	m_deleted = 0;
};

typedef ZmList<Zdb_File_,
	  ZmListObject<ZmPolymorph,
	    ZmListNodeIsItem<true,
	      ZmListHeapID<ZmNoHeap,
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
		  ZmHashLock<ZmNoLock,
		    ZmHashBase<ZmObject> > > > > > > Zdb_FileHash;
typedef Zdb_FileHash::Node Zdb_File;

class Zdb_FileRec {
public:
  ZuInline Zdb_FileRec() : m_file(0), m_off(0) { }
  ZuInline Zdb_FileRec(ZmRef<Zdb_File> file, ZiFile::Offset off) :
    m_file(ZuMv(file)), m_off(off) { }

  ZuInline bool operator !() const { return !m_file; }
  ZuOpBool

  ZuInline Zdb_File *file() const { return m_file; }
  ZuInline ZiFile::Offset off() const { return m_off; }

private:
  ZmRef<Zdb_File>	m_file;
  ZiFile::Offset	m_off;
};

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

class ZdbAnyPOD_ : public ZmPolymorph {
template <typename, typename> friend struct ZuConversionFriend;

protected:
  template <typename P>
  inline ZdbAnyPOD_(const P &p) : m_ptr(p.p1()), m_size(p.p2()) { }

public:
  inline const void *ptr() const { return m_ptr; }
  inline void *ptr() { return m_ptr; }

  inline unsigned size() const { return m_size; }

private:
  void		*m_ptr;
  unsigned	m_size;
};

typedef ZmList<ZdbAnyPOD_,
	  ZmListObject<ZmPolymorph,
	    ZmListNodeIsItem<true,
	      ZmListHeapID<ZmNoHeap,
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
		ZmHashHeapID<ZmNoHeap,
		  ZmHashID<Zdb_Cache_ID,
		    ZmHashLock<ZmNoLock,
		      ZmHashBase<ZmObject> > > > > > > > Zdb_Cache;
typedef Zdb_Cache::Node Zdb_CacheNode;

class ZdbAnyPOD_Compressed;
class ZdbAnyPOD_Send__;

class ZdbAPI ZdbAnyPOD : public Zdb_CacheNode {
friend class Zdb_;
friend class Zdb_Cxn;
friend class ZdbAnyPOD_Send__;
friend class ZdbEnv;
friend class ZdbGuard;

protected:
  inline ZdbAnyPOD(void *ptr, unsigned size, Zdb_ *db) :
    Zdb_CacheNode(ZuMkPair(ptr, size)), m_db(db), m_writeCount(0) { }

public:
  inline ~ZdbAnyPOD() { }

private:
  inline const ZdbTrailer *trailer() const {
    return (const ZdbTrailer *)
      ((const char *)ptr() + size() - sizeof(ZdbTrailer));
  }
  inline ZdbTrailer *trailer() {
    return (ZdbTrailer *)((char *)ptr() + size() - sizeof(ZdbTrailer));
  }

public:
  inline Zdb_ *db() const { return m_db; }

  inline ZdbRN rn() const { return trailer()->rn; }
  inline ZdbRN prevRN() const { return trailer()->prevRN; }
  inline uint32_t magic() const { return trailer()->magic; }

  inline bool committed() const {
    return trailer()->magic == ZdbCommitted;
  }

private:
  inline void init(ZdbRN rn, uint32_t magic = ZdbAllocated) {
    ZdbTrailer *trailer = this->trailer();
    trailer->rn = trailer->prevRN = rn;
    trailer->magic = magic;
  }

  inline void update(ZdbRN rn, ZdbRN prevRN) {
    ZdbTrailer *trailer = this->trailer();
    trailer->rn = rn;
    trailer->prevRN = prevRN;
  }

  inline void commit() { trailer()->magic = ZdbCommitted; }
  inline void del() { trailer()->magic = ZdbDeleted; }

  virtual ZmRef<ZdbAnyPOD_Compressed> compress() { return 0; }

  Zdb_			*m_db;
  unsigned		m_writeCount;	// guarded by Zdb_::m_lock
};
template <> struct ZuPrint<ZdbAnyPOD> : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const ZdbAnyPOD &v) {
    s << ZtHexDump("", v.ptr(), v.size());
  }
};

inline ZdbRN ZdbLRUNode_RNAccessor::value(const ZdbLRUNode &pod)
{
  return static_cast<const ZdbAnyPOD &>(pod).rn();
}

class ZdbAnyPOD_Compressed : public ZuObject {
public:
  inline ZdbAnyPOD_Compressed(void *ptr, unsigned size) :
    m_ptr(ptr), m_size(size) { }

  inline void *ptr() const { return m_ptr; }
  inline unsigned size() const { return m_size; }

  int compress(const char *src, unsigned size);

private:
  void		*m_ptr;
  unsigned	m_size;
};

class ZdbAnyPOD_Send__ : public ZmPolymorph {
public:
  inline ZdbAnyPOD_Send__(ZdbAnyPOD *pod,
      int type, ZdbRange range, int op, bool compress) :
	m_pod(pod), m_vec(0) {
    init(type, range, op, compress);
  }

  void send(Zdb_Cxn *cxn);

private:
  void init(int type, ZdbRange range, int op, bool compress);

  void send_(ZiIOContext &io);
  void sent(ZiIOContext &io);

  ZmRef<ZdbAnyPOD>		m_pod;
  ZmRef<ZdbAnyPOD_Compressed>	m_compressed;
  unsigned			m_vec;
  ZiVec				m_vecs[2];
  Zdb_Msg_Hdr			m_hdr;
};
struct ZdbAnyPOD_Send_HeapID {
  inline static const char *id() { return "ZdbAnyPOD_Send"; }
};
template <class Heap>
class ZdbAnyPOD_Send_ : public ZdbAnyPOD_Send__, public Heap {
public:
  template <typename ...Args>
  inline ZdbAnyPOD_Send_(Args &&... args) :
    ZdbAnyPOD_Send__(ZuFwd<Args>(args)...) { }
};
typedef ZdbAnyPOD_Send_<ZmHeap<ZdbAnyPOD_Send_HeapID,
	sizeof(ZdbAnyPOD_Send_<ZuNull>)> > ZdbAnyPOD_Send;

class ZdbAPI ZdbAnyPOD_Write__ : public ZmPolymorph {
public:
  inline ZdbAnyPOD_Write__(ZdbAnyPOD *pod, ZdbRange range, int op) :
    m_pod(pod), m_range(range), m_op(op) { }

  inline ZmRef<ZdbAnyPOD> pod() const { return m_pod; }
  inline const ZdbRange &range() const { return m_range; }
  inline int op() const { return m_op; }

  void write();

private:
  ZmRef<ZdbAnyPOD>	m_pod;
  ZdbRange		m_range;
  int			m_op;	// ZdbOp
};
template <class Heap>
class ZdbAnyPOD_Write_ : public ZdbAnyPOD_Write__, public Heap {
public:
  template <typename ...Args>
  inline ZdbAnyPOD_Write_(Args &&... args) :
    ZdbAnyPOD_Write__(ZuFwd<Args>(args)...) { }
};
struct ZdbAnyPOD_Write_HeapID {
  inline static const char *id() { return "ZdbAnyPOD_Write"; }
};
typedef ZdbAnyPOD_Write_<ZmHeap<ZdbAnyPOD_Write_HeapID,
	sizeof(ZdbAnyPOD_Write_<ZuNull>)> > ZdbAnyPOD_Write;

// AllocFn - called to allocate/initialize new record from memory
typedef ZmFn<Zdb_ *, ZmRef<ZdbAnyPOD> &> ZdbAllocFn;
// RecoverFn(pod) - (optional) called when record is recovered
typedef ZmFn<ZdbAnyPOD *> ZdbRecoverFn;
// ReplicateFn(pod, ptr, range, op) - (optional) '' when replicated (rcvd)
typedef ZmFn<ZdbAnyPOD *, void *, ZdbRange, int> ZdbReplicateFn;
// CopyFn(pod, range, op) - (optional) called for app drop copy
typedef ZmFn<ZdbAnyPOD *, ZdbRange, int> ZdbCopyFn;

struct ZdbPOD_HeapID {
  inline static const char *id() { return "ZdbPOD"; }
};
// heap ID can be specialized by app
template <typename T> struct ZdbPOD_Compressed_HeapID {
  inline static const char *id() { return "ZdbPOD_Compressed"; }
};

template <typename T_, class Heap>
class ZdbPOD_ : public Heap, public ZdbAnyPOD {
friend class Zdb<T_>;
friend class ZdbAnyPOD_Send__;

public:
  typedef T_ T;

private:
  struct Data : public T, public ZdbTrailer { };

  template <class Heap_>
  class Compressed_ : public ZdbAnyPOD_Compressed, public Heap_ {
  public:
    inline Compressed_() : 
      ZdbAnyPOD_Compressed(m_data, LZ4_compressBound(sizeof(Data))) { }

  private:
    char	m_data[LZ4_compressBound(sizeof(Data))];
  };
  typedef Compressed_<ZmHeap<ZdbPOD_Compressed_HeapID<T_>,
	  sizeof(Compressed_<ZuNull>)> > Compressed;

  ZmRef<ZdbAnyPOD_Compressed> compress() { return new Compressed(); }

public:
  inline ZdbPOD_(Zdb_ *db) :
    ZdbAnyPOD(&m_data, sizeof(Data), db) { }

  inline const T *ptr() const {
    const T *ZuMayAlias(ptr_) = (const T *)&m_data[0];
    return ptr_;
  }
  inline T *ptr() {
    T *ZuMayAlias(ptr_) = (T *)&m_data[0];
    return ptr_;
  }

  inline const T &data() const { return *ptr(); }
  inline T &data() { return *ptr(); }

private:
  char	m_data[sizeof(Data)];
};
template <typename T, class HeapID = ZdbPOD_HeapID>
using ZdbPOD = ZdbPOD_<T, ZmHeap<HeapID, sizeof(ZdbPOD_<T, ZuNull>)> >;
template <typename T, class HeapID>
struct ZuPrint<ZdbPOD<T, HeapID> > : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const ZdbPOD<T, HeapID> &v) { s << v.data(); }
};

struct ZdbConfig {
  inline ZdbConfig() { }
  inline ZdbConfig(const ZtString &key, ZvCf *cf) {
    id = cf->toInt("ID", key, 0, 1<<30);
    path = cf->get("path", true);
    fileSize = cf->getInt64("fileSize",
	((int64_t)4)<<10, ((int64_t)10)<<30, false, 0);
    preAlloc = cf->getInt("preAlloc", 0, 10<<24, false, 0);
    compress = cf->getInt("compress", 0, 1, false, 0);
    replicate = cf->getInt("replicate", 0, 1, false, 1);
    cache.init(cf->get("cache", false, "Zdb.Cache"));
    fileHash.init(cf->get("fileHash", false, "Zdb.FileHash"));
  }

  unsigned		id = 0;
  ZtString		path;
  uint64_t		fileSize = 0;
  unsigned		preAlloc = 0;	// #records to pre-allocate
  bool			compress = 0;
  bool			replicate = 0;
  ZmHashParams		cache;
  ZmHashParams		fileHash;
};

struct ZdbHandler {
  ZdbAllocFn		allocFn;
  ZdbRecoverFn		recoverFn;
  ZdbReplicateFn	replicateFn;
  ZdbCopyFn		copyFn;
};

class ZdbAPI Zdb_ : public ZmPolymorph {
friend class ZdbEnv;
friend class ZdbAnyPOD_Write__;

  struct IDAccessor;
friend struct IDAccessor;
  struct IDAccessor : public ZuAccessor<Zdb_ *, ZdbID> {
    inline static ZdbID value(const Zdb_ *db) { return db->m_id; }
  };

  typedef Zdb_Cache Cache;
  typedef Zdb_FileHash FileHash;

protected:
  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef ZmLock FSLock;
  typedef ZmGuard<FSLock> FSGuard;

  Zdb_(ZdbEnv *env, ZdbID id, ZdbHandler handler,
      unsigned recSize, unsigned dataSize);

public:
  ~Zdb_();

private:
  void init(ZdbConfig *config);
  void final();

  void open();
  void close();

  void recover();
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

  // next RN that will be allocated
  inline ZdbRN allocRN() { return m_allocRN; }

  // new record
  ZmRef<ZdbAnyPOD> push();

  // commit record following push() - causes replication / sync
  void put(ZdbAnyPOD *, bool copy = true);

  // get record
  ZmRef<ZdbAnyPOD> get(ZdbRN rn);	// use for read-only queries
  ZmRef<ZdbAnyPOD> get_(ZdbRN rn);	// use for RMW - does not update cache

  // update record following get() / get_()
  void update(ZdbAnyPOD *, ZdbRange range = ZdbRange(), bool copy = true);

  // delete record following push() / get() / get_()
  void del(ZdbAnyPOD *, bool copy = true);

  inline void replicate(bool v) { m_config->replicate = v; }

private:
  // application call handlers
  inline void alloc(ZmRef<ZdbAnyPOD> &pod) { m_handler.allocFn(this, pod); }
  inline void recover(ZdbAnyPOD *pod) { m_handler.recoverFn(pod); }
  inline void replicate(ZdbAnyPOD *pod,
      void *ptr, ZdbRange range, int op) {
#ifdef ZdbRep_DEBUG
    ZmAssert(range.len() > 0);
    ZmAssert((range.off() + range.len()) <= pod->size());
#endif
    if (m_handler.replicateFn)
      m_handler.replicateFn(pod, ptr, range, op);
    else
      memcpy((char *)pod->ptr() + range.off(), ptr, range.len());
  }
  inline void copy(ZdbAnyPOD *pod, ZdbRange range, int op) {
    m_handler.copyFn(pod, range, op);
  }

  // push initial record
  ZmRef<ZdbAnyPOD> push_();

  // low-level get, does not filter deleted records
  ZmRef<ZdbAnyPOD> get__(ZdbRN rn);

  // replication data rcvd (copy/commit, called while env is unlocked)
  ZmRef<ZdbAnyPOD> replicated(
      ZdbRN rn, ZdbRN prevRN, void *ptr, ZdbRange range, int op);

  // apply received replication data
  ZmRef<ZdbAnyPOD> replicated_(ZdbRN rn, ZdbRN prevRN, int op);

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

  ZmRef<ZdbAnyPOD> read_(const Zdb_FileRec &);

  void write(ZdbAnyPOD *pod, ZdbRange range, int op);
  void write_(ZdbRN rn, ZdbRN prevRN, const void *ptr, ZdbRange range, int op);
  void fileError_(const Zdb_FileRec &, ZeError);

  void cache(ZdbAnyPOD *pod);
  void cache_(ZdbAnyPOD *pod);
  void cacheDel_(ZdbAnyPOD *pod);

  ZdbEnv			*m_env;
  ZdbConfig			*m_config;
  ZdbID				m_id;
  ZdbHandler			m_handler;
  unsigned			m_recSize = 0;
  unsigned			m_dataSize = 0;
  Lock				m_lock;
    unsigned			  m_fileRecs;
    ZdbRN			  m_allocRN = 0;
    ZdbRN			  m_fileRN = 0;
    ZdbLRU			  m_lru;
    ZmRef<Zdb_Cache>		  m_cache;
    unsigned			  m_cacheSize = 0;
  FSLock			m_fsLock;	// guards files
    Zdb_FileLRU			  m_filesLRU;
    ZmRef<Zdb_FileHash>		  m_files;
    unsigned			  m_filesMax = 0;
};

template <typename T_>
class Zdb : public Zdb_ {
public:
  typedef T_ T;

  template <typename Handler>
  inline Zdb(ZdbEnv *env, ZdbID id, Handler &&handler) :
    Zdb_(env, id, ZuFwd<Handler>(handler),
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
    id = cf->toInt("ID", key, 0, 1<<30);
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

class Zdb_Host : public ZmPolymorph {
friend class ZdbEnv;
friend class Zdb_Cxn;
template <typename> friend struct ZuPrint;

  typedef ZmPLock Lock;
  typedef ZmGuard<Lock> Guard;

  enum State {
    Instantiated = 0,	// instantiated, init() not yet called
    Initialized,	// init() called
    Stopped,		// open() called
    Electing,		// start() called, determining active/inactive
    Activating,		// activating application
    Active,		// active (master)
    Deactivating,	// deactivating application
    Inactive,		// inactive (client)
    Stopping		// stop() called - stopping
  };

  struct IDAccessor;
friend struct IDAccessor;
  struct IDAccessor : public ZuAccessor<Zdb_Host *, int> {
    inline static int value(Zdb_Host *h) { return h->id(); }
  };

  Zdb_Host(ZdbEnv *env, const ZdbHostConfig *config);

  inline const ZdbHostConfig &config() const { return *m_config; }

  inline unsigned id() const { return m_config->id; }
  inline unsigned priority() const { return m_config->priority; }
  inline ZiIP ip() const { return m_config->ip; }
  inline uint16_t port() const { return m_config->port; }

  inline ZmRef<Zdb_Cxn> cxn() const { return m_cxn; }

  inline int state() const { return m_state; }
  inline void state(int s) { m_state = s; }

  inline const Zdb_DBState &dbState() const { return m_dbState; }
  inline Zdb_DBState &dbState() { return m_dbState; }

  inline bool active() const {
    switch (m_state) {
      case Zdb_Host::Activating:
      case Zdb_Host::Active:
	return true;
    }
    return false;
  }

  inline int cmp(const Zdb_Host *host) const {
    int i = m_dbState.cmp(host->m_dbState); if (i) return i;
    if (i = ZuCmp<bool>::cmp(active(), host->active())) return i;
    return ZuCmp<int>::cmp(priority(), host->priority());
  }

#if 0
  inline int cmp(const Zdb_Host *host) {
    int i = cmp_(host);

    printf("cmp(host %d priority %d dbState %d, "
	   "host %d priority %d dbState %d): %d\n",
	   (int)this->id(),
	   (int)this->config().m_priority, (int)this->dbState()[0],
	   (int)host->id(),
	   (int)host->config().m_priority,
	   (int)((Zdb_Host *)host)->dbState()[0], i);
    return i;
  }
#endif

  inline void voted(bool v) { m_voted = v; }
  inline bool voted() const { return m_voted; }

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
template <> struct ZuPrint<Zdb_Host> : public ZuPrintDelegate {
  template <typename S>
  inline static void print(S &s, const Zdb_Host &v) {
    s << "[ID:" << v.id() << " PRI:" << v.priority() << " V:" << v.voted() <<
      " S:" << v.state() << "] " << v.dbState();
  }
};
template <> struct ZuPrint<Zdb_Host *> : public ZuPrintDelegate {
  typedef Zdb_Host *Zdb_HostPtr;
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
friend class Zdb_Host;
friend class ZdbAnyPOD_Send__;

  Zdb_Cxn(ZdbEnv *env, Zdb_Host *host, const ZiCxnInfo &ci);

  inline ZdbEnv *env() const { return m_env; }
  inline void host(Zdb_Host *host) { m_host = host; }
  inline Zdb_Host *host() const { return m_host; }
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

  void repRcvd(ZiIOContext &);
  void repDataRead(ZiIOContext &);
  void repDataRcvd(ZiIOContext &);

  void repSend(ZdbAnyPOD *pod, int type, ZdbRange range, int op, bool compress);

  void ackRcvd();
  void ackSend(int type, ZdbAnyPOD *pod);

  ZdbEnv		*m_env;
  ZiMultiplex		*m_mx;
  Zdb_Host		*m_host;	// 0 if not yet associated

  Zdb_Msg_Hdr		m_readHdr;
  ZtArray<char>		m_readData;
  ZtArray<char>		m_readData2;

  unsigned		m_hbSendVec;
  ZiVec			m_hbSendVecs[2];
  Zdb_Msg_Hdr		m_hbSendHdr;

  Zdb_Msg_Hdr		m_ackSendHdr;

  ZmScheduler::Timer	m_hbTimer;
};

// ZdbEnv configuration
struct ZdbEnvConfig {
  inline ZdbEnvConfig() { }
  inline ZdbEnvConfig(ZvCf *cf) {
    writeThread = cf->getInt("writeThread", 1, INT_MAX, true);
    {
      ZvCf::Iterator i(cf->subset("dbs", false, true));
      ZuString key;
      while (ZmRef<ZvCf> dbCf = i.subset(key)) {
	ZdbConfig db(key, dbCf); // might throw, do not push() here
	new (dbCfs.push()) ZdbConfig(db);
      }
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

  unsigned			writeThread = 0;
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

friend class Zdb_;
friend class Zdb_Host;
friend class Zdb_Cxn;
friend class ZdbAnyPOD;
friend class ZdbAnyPOD_Send__;

  struct HostTree_HeapID {
    inline static const char *id() { return "ZdbEnv.HostTree"; }
  };
  typedef ZmRBTree<ZmRef<Zdb_Host>,
	    ZmRBTreeIndex<Zdb_Host::IDAccessor,
	      ZmRBTreeObject<ZuNull,
		ZmRBTreeLock<ZmNoLock,
		  ZmRBTreeHeapID<HostTree_HeapID> > > > > HostTree;

  struct CxnHash_HeapID {
    inline static const char *id() { return "ZdbEnv.CxnHash"; }
  };
  typedef ZmHash<ZmRef<Zdb_Cxn>,
	    ZmHashLock<ZmPLock,
	      ZmHashObject<ZuNull,
		ZmHashBase<ZmObject,
		  ZmHashHeapID<CxnHash_HeapID> > > > > CxnHash;

  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmCondition<Lock> StateCond;

#ifdef ZdbRep_DEBUG
  inline bool debug() const { return m_config.debug; }
#endif

public:
  ZdbEnv(unsigned maxDBID);
  ~ZdbEnv();

  void init(const ZdbEnvConfig &config, ZiMultiplex *mx,
      ZmFn<> activeFn, ZmFn<> inactiveFn);
  void final();

  void open();
  void close();

  void start();
  void stop();

  void checkpoint();

  inline const ZdbEnvConfig &config() const { return m_config; }
  inline ZiMultiplex *mx() const { return m_mx; }

  inline int state() const {
    return m_self ? m_self->state() : Zdb_Host::Instantiated;
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
    switch (state()) {
      case Zdb_Host::Electing:
      case Zdb_Host::Activating:
      case Zdb_Host::Active:
      case Zdb_Host::Deactivating:
      case Zdb_Host::Inactive:
	return true;
    }
    return false;
  }
  inline bool active() {
    switch (state()) {
      case Zdb_Host::Activating:
      case Zdb_Host::Active:
	return true;
    }
    return false;
  }

private:
  inline Zdb_Host *self() const { return m_self; }

  void add(Zdb_ *db);	// adds database, finds cf, calls db->init(cf)
  inline Zdb_ *db(ZdbID id) {
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
  void reactivate(Zdb_Host *host);	// re-assert master

  ZiConnection *accepted(const ZiCxnInfo &ci);
  void connected(Zdb_Cxn *cxn);
  void associate(Zdb_Cxn *cxn, int hostID);
  void associate(Zdb_Cxn *cxn, Zdb_Host *host);
  void disconnected(Zdb_Cxn *cxn);

  void hbDataRcvd(
      Zdb_Host *host, const Zdb_Msg_HB &hb, ZdbRN *dbState);
  void vote(Zdb_Host *host);

  void hbStart();
  void hbSend();		// send heartbeat and reschedule self
  void hbSend_(Zdb_Cxn *cxn);	// send heartbeat (once)

  void dbStateRefresh();	// refresh m_self->dbState() (with guard)
  void dbStateRefresh_();	// '' (unlocked)

  Zdb_Host *setMaster();	// returns old master
  void setNext(Zdb_Host *host);
  void setNext();

  void startReplication();
  void stopReplication();

  void repDataRcvd(
      Zdb_Host *host, Zdb_Cxn *cxn, int type,
      const Zdb_Msg_Rep &rep, void *ptr);

  void repSend(ZdbAnyPOD *pod, int type, ZdbRange range, int op, bool compress);
  void recSend();

  void ackRcvd(Zdb_Host *host, bool positive, ZdbID db, ZdbRN rn);

  void replicate(ZdbAnyPOD *pod);

  void write(ZdbAnyPOD *pod, ZdbRange range, int op);

  ZdbEnvConfig		m_config;
  ZiMultiplex		*m_mx;

  ZmFn<>		m_activeFn;
  ZmFn<>		m_inactiveFn;

  Lock			m_lock;
    StateCond		m_stateCond;
    bool		m_appActive;
    Zdb_Host		*m_self;
    Zdb_Host		*m_master;	// == m_self if Active
    Zdb_Host		*m_prev;	// previous-ranked host
    Zdb_Host		*m_next;	// next-ranked host
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
  ZtArray<ZmRef<Zdb_> >	m_dbs;
  HostTree		m_hosts;
  ZmRef<CxnHash>	m_cxns;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* Zdb_HPP */
