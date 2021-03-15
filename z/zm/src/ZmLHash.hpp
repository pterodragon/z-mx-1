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

// hash table (policy-based)
//
// open addressing, linear probing, fast chained lookup, optionally locked
//
// nodes are stored by value - avoids run-time heap usage and improves
// cache coherence (except during initialization/resizing)
//
// use ZmHash for high-contention high-throughput read/write data
// use ZmLHash for unlocked or mostly uncontended reference data

#ifndef ZmLHash_HPP
#define ZmLHash_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuIndex.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuArrayFn.hpp>

#include <zlib/ZmAtomic.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmLockTraits.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmAssert.hpp>

#include <zlib/ZmHashMgr.hpp>

// NTP (named template parameters):
//
// ZmLHash<ZtString,			// keys are ZtStrings
//   ZmLHashVal<ZtString,		// values are ZtStrings
//     ZmLHashValCmp<ZtICmp> > > >	// case-insensitive comparison

// NTP defaults
struct ZmLHash_Defaults {
  using Val = ZuNull;
  template <typename T> struct CmpT	{ using Cmp	= ZuCmp<T>; };
  template <typename T> struct ICmpT	{ using ICmp	= ZuCmp<T>; };
  template <typename T> struct HashFnT	{ using HashFn	= ZuHash<T>; };
  template <typename T> struct IHashFnT	{ using IHashFn	= ZuHash<T>; };
  template <typename T> struct IndexT	{ using Index	= T; };
  template <typename T> struct ValCmpT	{ using ValCmp	= ZuCmp<T>; };
  using Lock = ZmLock;
  struct ID { static constexpr const char *id() { return "ZmLHash"; } };
  enum { Static = 0 };
};

// ZmLHashCmp - the key comparator
template <class Cmp_, class NTP = ZmLHash_Defaults>
struct ZmLHashCmp : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
  template <typename> struct ICmpT { using ICmp = Cmp_; };
};

// ZmLHashCmp_ - directly override the key comparator
// (used by other templates to forward NTP parameters to ZmLHash)
template <class Cmp_, class NTP = ZmLHash_Defaults>
struct ZmLHashCmp_ : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
};

// ZmLHashICmp - the index comparator
template <class ICmp_, class NTP = ZmLHash_Defaults>
struct ZmLHashICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

// ZmLHashFn - the hash function
template <class HashFn_, class NTP = ZmLHash_Defaults>
struct ZmLHashFn : public NTP {
  template <typename> struct HashFnT { using HashFn = HashFn_; };
  template <typename> struct IHashFnT { using IHashFn = HashFn_; };
};

// ZmLHashIFn - the index hash function
template <class IHashFn_, class NTP = ZmLHash_Defaults>
struct ZmLHashIFn : public NTP {
  template <typename> struct IHashFnT { using IHashFn = IHashFn_; };
};

// ZmLHashIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator, hash function and index type
template <class Accessor, class NTP = ZmLHash_Defaults>
struct ZmLHashIndex : public NTP {
  template <typename T> struct CmpT {
    using Cmp = typename ZuIndex<Accessor>::template CmpT<T>;
  };
  template <typename> struct ICmpT {
    using ICmp = typename ZuIndex<Accessor>::ICmp;
  };
  template <typename> struct HashFnT {
    using HashFn = typename ZuIndex<Accessor>::Hash;
  };
  template <typename> struct IHashFnT {
    using IHashFn = typename ZuIndex<Accessor>::IHash;
  };
  template <typename> struct IndexT {
    using Index = typename ZuIndex<Accessor>::I;
  };
};

// ZmLHashIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmLHash)
template <class Index_, class NTP = ZmLHash_Defaults>
struct ZmLHashIndex_ : public NTP {
  template <typename> struct IndexT { using Index = Index_; };
};

// ZmLHashVal - the value type
template <class Val_, class NTP = ZmLHash_Defaults>
struct ZmLHashVal : public NTP {
  using Val = Val_;
};

// ZmLHashValCmp - the value comparator
template <class ValCmp_, class NTP = ZmLHash_Defaults>
struct ZmLHashValCmp : public NTP {
  template <typename> struct ValCmpT { using ValCmp = ValCmp_; };
};

// ZmLHashLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmLHash_Defaults>
struct ZmLHashLock : public NTP {
  using Lock = Lock_;
};

// ZmLHashID - the hash ID
template <class ID_, class NTP = ZmLHash_Defaults>
struct ZmLHashID : public NTP {
  using ID = ID_;
};

// ZmLHashStatic<Bits> - static/non-resizable vs dynamic/resizable allocation
template <unsigned Static_, class NTP = ZmLHash_Defaults>
struct ZmLHashStatic : public NTP {
  enum { Static = Static_ };
};

template <typename T>
struct ZmLHash_Ops : public ZuArrayFn<T, ZuCmp<T> > {
  static T *alloc(unsigned size) {
    T *ptr = (T *)::malloc(size * sizeof(T));
    if (!ptr) throw std::bad_alloc();
    return ptr;
  }
  static void free(T *ptr) {
    ::free(ptr);
  }
};

template <typename Key, typename Cmp, typename Val, typename ValCmp>
class ZmLHash_Node {
template <typename, class> friend class ZmLHash;
template <class, typename, class, unsigned> friend class ZmLHash_;

public:
  struct KV : public ZuPair<Key, Val> {
    using Pair = ZuPair<Key, Val>;
    friend ZuTraits<Pair> ZuTraitsType(KV *);
    KV() = default;
    KV(const KV &) = default;
    KV &operator =(const KV &) = default;
    KV(KV &&) = default;
    KV &operator =(KV &&) = default;
    ~KV() = default;
    template <typename ...Args>
    KV(Args &&... args) : Pair{ZuFwd<Args>(args)...} { }
    template <typename V>
    KV &operator =(V &&v) {
      Pair::operator =(ZuFwd<V>(v));
      return *this;
    }

    int cmp(const KV &d) const {
      int i;
      if (i = Cmp::cmp(key(), d.key())) return i;
      return ValCmp::cmp(val(), d.val());
    }
    bool less(const KV &d) const {
      return
	!Cmp::less(d.key(), key()) &&
	ValCmp::less(val(), d.val());
    }
    bool equals(const KV &d) const {
      return
	Cmp::equals(key(), d.key()) &&
	ValCmp::equals(val(), d.val());
    }
    bool operator ==(const KV &d) const { return equals(d); }
    bool operator !=(const KV &d) const { return !equals(d); }
    bool operator >(const KV &d) const { return d.less(*this); }
    bool operator >=(const KV &d) const { return !less(d); }
    bool operator <(const KV &d) const { return less(d); }
    bool operator <=(const KV &d) const { return !d.less(*this); }

    const auto &key() const { return this->template p<0>(); }
    auto &key() { return this->template p<0>(); }
    const auto &val() const { return this->template p<1>(); }
    auto &val() { return this->template p<1>(); }
  };

  ZuInline ZmLHash_Node() { }
  ZuInline ~ZmLHash_Node() { if (m_u) kv().~KV(); }

  ZuInline ZmLHash_Node(const ZmLHash_Node &n) {
    if (m_u = n.m_u) new (m_kv) KV{n.kv()};
  }
  ZuInline ZmLHash_Node &operator =(const ZmLHash_Node &n) {
    if (ZuLikely(this != &n)) {
      if (m_u) kv().~KV();
      if (m_u = n.m_u) new (m_kv) KV{n.kv()};
    }
    return *this;
  }

  ZuInline ZmLHash_Node(ZmLHash_Node &&n) {
    if (m_u = n.m_u) new (m_kv) KV{ZuMv(n.kv())};
  }
  ZuInline ZmLHash_Node &operator =(ZmLHash_Node &&n) {
    if (ZuLikely(this != &n)) {
      if (m_u) kv().~KV();
      if (m_u = n.m_u) new (m_kv) KV{ZuMv(n.kv())};
    }
    return *this;
  }

private:
  template <typename Key_, typename Val_>
  void init(
      unsigned head, unsigned tail, unsigned next, Key_ &&key, Val_ &&value) {
    if (!m_u)
      new (m_kv) KV{ZuFwd<Key_>(key), ZuFwd<Val_>(value)};
    else
      kv().key() = ZuFwd<Key_>(key), kv().val() = ZuFwd<Val_>(value);
    m_u = (next<<3U) | (head<<2U) | (tail<<1U) | 1U;
  }
  void null() {
    if (m_u) {
      kv().~KV();
      m_u = 0;
    }
  }

public:
  ZuInline bool operator !() const { return !m_u; }
  ZuOpBool

  int cmp(const ZmLHash_Node &n) const {
    if (!n.m_u) return !m_u ? 0 : 1;
    if (!m_u) return -1;
    return kv().cmp(n.kv());
  }
  bool less(const ZmLHash_Node &n) const {
    if (!n.m_u) return false;
    if (!m_u) return true;
    return kv().less(n.kv());
  }
  bool equals(const ZmLHash_Node &n) const {
    if (!n.m_u) return !m_u;
    if (!m_u) return false;
    return kv().equals(n.kv());
  }
  bool operator ==(const ZmLHash_Node &n) const { return equals(n); }
  bool operator !=(const ZmLHash_Node &n) const { return !equals(n); }
  bool operator >(const ZmLHash_Node &n) const { return cmp(n) > 0; }
  bool operator >=(const ZmLHash_Node &n) const { return cmp(n) >= 0; }
  bool operator <(const ZmLHash_Node &n) const { return cmp(n) < 0; }
  bool operator <=(const ZmLHash_Node &n) const { return cmp(n) <= 0; }

private:
  ZuInline bool head() const { return m_u & 4U; }
  ZuInline void setHead() { m_u |= 4U; }
  ZuInline void clrHead() { m_u &= ~4U; }
  ZuInline bool tail() const { return m_u & 2U; }
  ZuInline void setTail() { m_u |= 2U; }
  ZuInline void clrTail() { m_u &= ~2U; }
  ZuInline unsigned next() const { return m_u>>3U; }
  ZuInline void next(unsigned n) { m_u = (n<<3U) | (m_u & 7U); }

  const KV &kv() const {
    ZmAssert(m_u);
    const KV *ZuMayAlias(kv_) = reinterpret_cast<const KV *>(m_kv);
    return *kv_;
  }
  KV &kv() {
    ZmAssert(m_u);
    KV *ZuMayAlias(kv_) = reinterpret_cast<KV *>(m_kv);
    return *kv_;
  }

  const Key &key() const { return kv().key(); }
  Key &key() { return kv().key(); }
  const Val &val() const { return kv().val(); }
  Val &val() { return kv().val(); }

public:
  struct Traits : public ZuBaseTraits<ZmLHash_Node> {
    enum { IsPOD = ZuTraits<Key>::IsPOD && ZuTraits<Val>::IsPOD };
  };
  friend Traits ZuTraitsType(ZmLHash_Node *);

private:
  uint32_t	m_u = 0;
  char		m_kv[sizeof(KV)];
};

// common base class for both static and dynamic tables
template <typename Key, typename NTP> class ZmLHash__ : public ZmAnyHash {
  using Lock = typename NTP::Lock;

public:
  ZuInline unsigned loadFactor_() const { return m_loadFactor; }
  ZuInline double loadFactor() const { return (double)m_loadFactor / 16.0; }

  ZuInline unsigned count_() const { return m_count.load_(); }

protected:
  ZmLHash__(const ZmHashParams &params) {
    double loadFactor = params.loadFactor();
    if (loadFactor < 0.5) loadFactor = 0.5;
    else if (loadFactor > 1.0) loadFactor = 1.0;
    m_loadFactor = (unsigned)(loadFactor * 16.0);
  }

  unsigned		m_loadFactor = 0;
  ZmAtomic<unsigned>	m_count = 0;
  Lock			m_lock;
};

// statically allocated hash table base class
template <class Hash, typename Key, class NTP, unsigned Static>
class ZmLHash_ : public ZmLHash__<Key, NTP> {
  using Base = ZmLHash__<Key, NTP>;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using Val = typename NTP::Val;
  using ValCmp = typename NTP::template ValCmpT<Val>::ValCmp;
  using Node = ZmLHash_Node<Key, Cmp, Val, ValCmp>;
  using Ops = ZmLHash_Ops<Node>;

public:
  ZuInline static constexpr unsigned bits() { return Static; }

protected:
  ZmLHash_(const ZmHashParams &params) : Base(params) { }

  void init() { Ops::initItems(m_table, 1U<<Static); }
  void final() { Ops::destroyItems(m_table, 1U<<Static); }
  void resize() { }
  static constexpr unsigned resized() { return 0; }

  Node		m_table[1U<<Static];
};

// dynamically allocated hash table base class
template <class Hash, typename Key, class NTP>
class ZmLHash_<Hash, Key, NTP, 0> : public ZmLHash__<Key, NTP> {
  using Base = ZmLHash__<Key, NTP>;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using Val = typename NTP::Val;
  using ValCmp = typename NTP::template ValCmpT<Val>::ValCmp;
  using Node = ZmLHash_Node<Key, Cmp, Val, ValCmp>;
  using Ops = ZmLHash_Ops<Node>;
  using HashFn = typename NTP::template HashFnT<Key>::HashFn;

public:
  ZuInline unsigned bits() const { return m_bits; }

protected:
  ZmLHash_(const ZmHashParams &params) : Base(params),
    m_bits(params.bits()) { }

  void init() {
    unsigned size = 1U<<m_bits;
    m_table = Ops::alloc(size);
    Ops::initItems(m_table, size);
    ZmHashMgr::add(this);
  }

  void final() {
    ZmHashMgr::del(this);
    Ops::destroyItems(m_table, 1U<<m_bits);
    Ops::free(m_table);
  }

  void resize() {
    ++m_resized;
    ++m_bits;
    unsigned size = 1U<<m_bits;
    Node *oldTable = m_table;
    m_table = Ops::alloc(size);
    Ops::initItems(m_table, size);
    for (unsigned i = 0; i < (size>>1U); i++)
      if (!!oldTable[i])
	static_cast<Hash *>(this)->add__(
	    ZuMv(oldTable[i].key()), ZuMv(oldTable[i].val()),
	    HashFn::hash(oldTable[i].key()));
    Ops::destroyItems(oldTable, (size>>1U));
    Ops::free(oldTable);
  }

  unsigned resized() const { return m_resized.load_(); }

  ZmAtomic<unsigned>	m_resized = 0;
  unsigned		m_bits;
  Node	 		*m_table = nullptr;
};

template <typename Key_, class NTP = ZmLHash_Defaults>
class ZmLHash : public ZmLHash_<ZmLHash<Key_, NTP>, Key_, NTP, NTP::Static> {
  ZmLHash(const ZmLHash &) = delete;
  ZmLHash &operator =(const ZmLHash &) = delete; // prevent mis-use

template <class, typename, class, unsigned> friend class ZmLHash_;

  using Base = ZmLHash_<ZmLHash<Key_, NTP>, Key_, NTP, NTP::Static>;

public:
  using Key = Key_;
  using Val = typename NTP::Val;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using ICmp = typename NTP::template ICmpT<Key>::ICmp;
  using HashFn = typename NTP::template HashFnT<Key>::HashFn;
  using IHashFn = typename NTP::template IHashFnT<Key>::IHashFn;
  using Index = typename NTP::template IndexT<Key>::Index;
  using ValCmp = typename NTP::template ValCmpT<Val>::ValCmp;
  using Lock = typename NTP::Lock;
  using ID = typename NTP::ID;
  using LockTraits = ZmLockTraits<Lock>;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;
  using Node = ZmLHash_Node<Key, Cmp, Val, ValCmp>;
  using KV = typename Node::KV;
  using Ops = ZmLHash_Ops<Node>;
  enum { Static = NTP::Static };
 
private:
  // CheckHashFn ensures that legacy hash functions returning int
  // trigger a compile-time assertion failure; hash() must return uint32_t
  class CheckHashFn {
    using Small = char;
    class	 Big { char _[2]; };
    static Small test(const uint32_t &_); // named parameter due to VS2010 bug
    static Big	 test(const int &_);
    static Big	 test(...);
    static Key	 key();
    static Index index();
  public:
    CheckHashFn(); // keep gcc quiet
    enum _ {
      IsUInt32 = sizeof(test(HashFn::hash(key()))) == sizeof(Small) &&
		 sizeof(test(IHashFn::hash(index()))) == sizeof(Small)
    };
  };
  ZuAssert(CheckHashFn::IsUInt32);

  using Base::m_count;
  using Base::m_lock;
  using Base::m_table;

public:
  using Base::bits;
  using Base::loadFactor_;
  using Base::loadFactor;
  using Base::resized;

protected:
  class Iterator_;
friend Iterator_;
  class Iterator_ {			// hash iterator
    Iterator_(const Iterator_ &) = delete;
    Iterator_ &operator =(const Iterator_ &) = delete;

    using Hash = ZmLHash<Key, NTP>;

  friend Hash;

  protected:
    Iterator_(Iterator_ &&) = default;
    Iterator_ &operator =(Iterator_ &&) = default;

    Iterator_(Hash &hash) : m_hash(hash), m_slot(-1), m_next(-1) { }

    virtual void lock(Lock &l) = 0;
    virtual void unlock(Lock &l) = 0;

  public:
    void reset() { m_hash.startIterate(*this); }
    const KV *iterate() { return m_hash.iterate(*this); }
    const Key &iterateKey() { return m_hash.iterateKey(*this); }
    const Val &iterateVal() { return m_hash.iterateVal(*this); }

    ZuInline unsigned count() const { return m_hash.count_(); }

    ZuInline bool operator !() const { return m_slot < 0; }
    ZuOpBool

  protected:
    Hash	&m_hash;
    int		m_slot;
    int		m_next;
  };

  class IndexIterator_;
friend IndexIterator_;
  class IndexIterator_ : protected Iterator_ {
    IndexIterator_(const IndexIterator_ &) = delete;
    IndexIterator_ &operator =(const IndexIterator_ &) = delete;

    using Hash = ZmLHash<Key, NTP>;
  friend Hash;

    using Iterator_::m_hash;

  protected:
    IndexIterator_(IndexIterator_ &&) = default;
    IndexIterator_ &operator =(IndexIterator_ &&) = default;

    template <typename Index_>
    IndexIterator_(Hash &hash, const Index_ &index) :
	Iterator_(hash), m_index(index), m_prev(-1) { }

  public:
    void reset() { m_hash.startIterate(*this); }
    const KV *iterate() { return m_hash.iterate(*this); }
    const Key &iterateKey() { return m_hash.iterateKey(*this); }
    const Val &iterateVal() { return m_hash.iterateVal(*this); }

  protected:
    Index	m_index;
    int		m_prev;
  };

public:
  class Iterator : public Iterator_ {
    Iterator(const Iterator &) = delete;
    Iterator &operator =(const Iterator &) = delete;

    using Hash = ZmLHash<Key, NTP>;
    void lock(Lock &l) { LockTraits::lock(l); }
    void unlock(Lock &l) { LockTraits::unlock(l); }

    using Iterator_::m_hash;

  public:
    Iterator(Iterator &&) = default;
    Iterator &operator =(Iterator &&) = default;

    Iterator(Hash &hash) : Iterator_(hash) { hash.startIterate(*this); }
    ~Iterator() { m_hash.endIterate(*this); }
    void del() { m_hash.delIterate(*this); }
  };

  class ReadIterator : public Iterator_ {
    ReadIterator(const ReadIterator &) = delete;
    ReadIterator &operator =(const ReadIterator &) = delete;

    using Hash = ZmLHash<Key, NTP>;
    void lock(Lock &l) { LockTraits::readlock(l); }
    void unlock(Lock &l) { LockTraits::readunlock(l); }

    using Iterator_::m_hash;

  public:
    ReadIterator(ReadIterator &&) = default;
    ReadIterator &operator =(ReadIterator &&) = default;

    ReadIterator(const Hash &hash) : Iterator_(const_cast<Hash &>(hash))
      { const_cast<Hash &>(hash).startIterate(*this); }
    ~ReadIterator() { m_hash.endIterate(*this); }
  };

  class IndexIterator : public IndexIterator_ {
    IndexIterator(const IndexIterator &) = delete;
    IndexIterator &operator =(const IndexIterator &) = delete;

    using Hash = ZmLHash<Key, NTP>;
    void lock(Lock &l) { LockTraits::lock(l); }
    void unlock(Lock &l) { LockTraits::unlock(l); }

    using IndexIterator_::m_hash;

  public:
    IndexIterator(IndexIterator &&) = default;
    IndexIterator &operator =(IndexIterator &&) = default;

    template <typename Index_>
    IndexIterator(Hash &hash, Index_ &&index) :
	IndexIterator_(hash, ZuFwd<Index_>(index)) { hash.startIterate(*this); }
    ~IndexIterator() { m_hash.endIterate(*this); }
    void del() { m_hash.delIterate(*this); }
  };

  class ReadIndexIterator : public IndexIterator_ {
    ReadIndexIterator(const ReadIndexIterator &) = delete;
    ReadIndexIterator &operator =(const ReadIndexIterator &) = delete;

    using Hash = ZmLHash<Key, NTP>;
    void lock(Lock &l) { LockTraits::readlock(l); }
    void unlock(Lock &l) { LockTraits::readunlock(l); }

    using IndexIterator_::m_hash;

  public:
    ReadIndexIterator(ReadIndexIterator &&) = default;
    ReadIndexIterator &operator =(ReadIndexIterator &&) = default;

    template <typename Index_>
    ReadIndexIterator(Hash &hash, Index_ &&index) :
	IndexIterator_(const_cast<Hash &>(hash), ZuFwd<Index_>(index))
      { const_cast<Hash &>(hash).startIterate(*this); }
    ~ReadIndexIterator() { m_hash.endIterate(*this); }
  };

  template <typename ...Args>
  ZmLHash(ZmHashParams params = ZmHashParams(ID::id())) : Base(params) {
    Base::init();
  }

  ~ZmLHash() { Base::final(); }

  unsigned size() const {
    return (double)(((uint64_t)1)<<bits()) * loadFactor();
  }

  template <typename Key__>
  int add(Key__ &&key) { return add(ZuFwd<Key__>(key), Val()); }
  template <typename Key__, typename Val_>
  int add(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return add_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }
  template <typename Key__>
  int add_(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    return add_(ZuFwd<Key__>(key), Val(), code);
  }
  template <typename Key__, typename Val_>
  int add_(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    return add_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }
private:
  int alloc(unsigned slot) {
    unsigned size = 1U<<bits();
    for (unsigned i = 1; i < size; i++) {
      unsigned probe = (slot + i) & (size - 1);
      if (!m_table[probe]) return probe;
    }
    return -1;
  }
  int prev(unsigned slot) {
    unsigned size = 1U<<bits();
    for (unsigned i = 1; i < size; i++) {
      unsigned prev = (slot + size - i) & (size - 1);
      if (m_table[prev] &&
	  !m_table[prev].tail() &&
	  m_table[prev].next() == slot)
	return prev;
    }
    return -1;
  }
  template <typename Key__, typename Val_>
  int add_(Key__ &&key, Val_ &&val, uint32_t code) {
    unsigned size = 1U<<bits();

    unsigned count = m_count.load_();
    if (count < (1U<<28) && ((count<<4)>>bits()) >= loadFactor_()) {
      Base::resize();
      size = 1U<<bits();
    }

    if (count >= size) return -1;

    m_count.store_(count + 1);

    return add__(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }

  template <typename Key__, typename Val_>
  int add__(Key__ &&key, Val_ &&val, uint32_t code) {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);

    if (!m_table[slot]) {
      m_table[slot].init(1, 1, 0, ZuFwd<Key__>(key), ZuFwd<Val_>(val));
      return slot;
    }

    int move = alloc(slot);
    if (move < 0) return -1;

    if (m_table[slot].head()) {
      (m_table[move] = ZuMv(m_table[slot])).clrHead();
      m_table[slot].init(1, 0, move, ZuFwd<Key__>(key), ZuFwd<Val_>(val));
      return slot;
    }

    int prev = this->prev(slot);
    if (prev < 0) return -1;

    m_table[move] = ZuMv(m_table[slot]);
    m_table[prev].next(move);
    m_table[slot].init(1, 1, 0, ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    return slot;
  }

public:
  template <typename Index_>
  bool exists(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(const_cast<Lock &>(m_lock));

    return find__(index, code) >= 0;
  }
  template <typename Index_>
  bool exists_(const Index_ &index) const {
    return find__(index, IHashFn::hash(index)) >= 0;
  }
  template <typename Index_>
  const KV *find(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(const_cast<Lock &>(m_lock));

    return kv__(find__(index, code));
  }
  template <typename Index_>
  const KV *find_(const Index_ &index) const {
    return kv__(find__(index, IHashFn::hash(index)));
  }
  template <typename Index_>
  Key findKey(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(const_cast<Lock &>(m_lock));

    return key__(find__(index, code));
  }
  template <typename Index_>
  Val findVal(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(const_cast<Lock &>(m_lock));

    return val__(find__(index, code));
  }
private:
  template <typename Index_>
  int find__(const Index_ &index, uint32_t code) const {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);
    if (!m_table[slot] || !m_table[slot].head()) return -1;
    for (;;) {
      if (ICmp::equals(m_table[slot].key(), index)) return slot;
      if (m_table[slot].tail()) return -1;
      slot = m_table[slot].next();
    }
  }
  template <typename Index_>
  int findPrev__(const Index_ &index, uint32_t code) const {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);
    if (!m_table[slot] || !m_table[slot].head()) return -1;
    int prev = -1;
    for (;;) {
      if (ICmp::equals(m_table[slot].key(), index))
	return prev < 0 ? (-((int)slot) - 2) : prev;
      if (m_table[slot].tail()) return -1;
      prev = slot, slot = m_table[slot].next();
    }
  }

  const KV *kv__(int slot) const {
    if (ZuUnlikely(slot < 0)) return nullptr;
    return &m_table[slot].kv();
  }
  const Key &key__(int slot) const {
    if (ZuUnlikely(slot < 0)) return Cmp::null();
    return m_table[slot].key();
  }
  const Val &val__(int slot) const {
    if (ZuUnlikely(slot < 0)) return ValCmp::null();
    return m_table[slot].val();
  }

public:
  template <typename Index_, typename Val_>
  const KV *find(
      const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(m_lock);

    return kv__(find__(index, val, code));
  }
  template <typename Index_, typename Val_>
  const KV *find_(
      const Index_ &index, const Val_ &val) const {
    return kv__(find__(index, val, IHashFn::hash(index)));
  }
  template <typename Index_, typename Val_>
  Key findKey(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(m_lock);

    return key__(find__(index, val, code));
  }
  template <typename Index_, typename Val_>
  Val findVal(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(m_lock);

    return val__(find__(index, val, code));
  }
private:
  template <typename Index_, typename Val_>
  int find__(
      const Index_ &index, const Val_ &val, uint32_t code) const {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);
    if (!m_table[slot] || !m_table[slot].head()) return -1;
    for (;;) {
      if (ICmp::equals(m_table[slot].key(), index) &&
	  ValCmp::equals(m_table[slot].val(), val)) return slot;
      if (m_table[slot].tail()) return -1;
      slot = m_table[slot].next();
    }
  }
  template <typename Index_, typename Val_>
  int findPrev__(
      const Index_ &index, const Val_ &val, uint32_t code) const {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);
    if (!m_table[slot] || !m_table[slot].head()) return -1;
    int prev = -1;
    for (;;) {
      if (ICmp::equals(m_table[slot].key(), index) &&
	  ValCmp::equals(m_table[slot].val(), val))
	return prev < 0 ? (-slot - 2) : prev;
      if (m_table[slot].tail()) return -1;
      prev = slot, slot = m_table[slot].next();
    }
  }

public:
  template <typename Key__>
  const KV *findAdd(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return kv__(findAdd__(ZuFwd<Key__>(key), Val(), code));
  }
  template <typename Key__, typename Val_>
  const KV *findAdd(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return kv__(findAdd__(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code));
  }
  template <typename Key__>
  const KV *findAdd_(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    return kv__(findAdd__(ZuFwd<Key__>(key), Val(), code));
  }
  template <typename Key__, typename Val_>
  const KV *findAdd_(
      Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    return kv__(findAdd__(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code));
  }
  template <typename Key__>
  Key findAddKey(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return key__(findAdd__(ZuFwd<Key__>(key), Val(), code));
  }
  template <typename Key__, typename Val_>
  Key findAddKey(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return key__(findAdd__(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code));
  }
  template <typename Key__, typename Val_>
  Val findAddVal(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(m_lock);

    return val__(findAdd__(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code));
  }
private:
  template <typename Key__, typename Val_>
  int findAdd__(Key__ &&key, Val_ &&val, uint32_t code) {
    unsigned size = 1U<<bits();
    unsigned slot = code & (size - 1);
    if (!!m_table[slot] && m_table[slot].head())
      for (;;) {
	if (Cmp::equals(m_table[slot].key(), key)) return slot;
	if (m_table[slot].tail()) break;
	slot = m_table[slot].next();
      }
    return add_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }

public:
  template <typename Index_>
  void del(const Index_ &index) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    del__(findPrev__(index, code));
  }
  template <typename Index_>
  void del_(const Index_ &index) {
    del__(findPrev__(index, IHashFn::hash(index)));
  }
  template <typename Index_>
  Key delKey(const Index_ &index) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    return delKey__(findPrev__(index, code));
  }
  template <typename Index_>
  Val delVal(const Index_ &index) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    return delVal__(findPrev__(index, code));
  }

private:
  void del___(int prev) {
    int slot;

    if (prev < 0)
      slot = (-prev - 2), prev = -1;
    else
      slot = m_table[prev].next();

    if (!m_table[slot]) return;

    if (unsigned count = m_count.load_())
      m_count.store_(count - 1);

    if (m_table[slot].head()) {
      ZmAssert(prev < 0);
      if (m_table[slot].tail()) {
	m_table[slot].null();
	return;
      }
      unsigned next = m_table[slot].next();
      (m_table[slot] = ZuMv(m_table[next])).setHead();
      m_table[next].null();
      return;
    }

    if (m_table[slot].tail()) {
      ZmAssert(prev >= 0);
      if (prev >= 0) m_table[prev].setTail();
      m_table[slot].null();
      return;
    }

    unsigned next = m_table[slot].next();
    m_table[slot] = ZuMv(m_table[next]);
    m_table[next].null();
  }

  void del__(int prev) {
    if (prev == -1) return;
    del___(prev);
  }
  Key delKey__(int prev) {
    if (prev == -1) return Cmp::null();
    int slot = prev < 0 ? (-prev - 2) : m_table[prev].next();
    Key key{ZuMv(m_table[slot].key())};
    del___(prev);
    return key;
  }
  Key delVal__(int prev) {
    if (prev == -1) return Cmp::null();
    int slot = prev < 0 ? (-prev - 2) : m_table[prev].next();
    Val val{ZuMv(m_table[slot].val())};
    del___(prev);
    return val;
  }

public:
  template <typename Index_, typename Val_>
  void del(const Index_ &index, const Val_ &val) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    del__(findPrev__(index, val, code));
  }
  template <typename Index_, typename Val_>
  void del_(const Index_ &index, const Val_ &val) {
    del__(findPrev__(index, val, IHashFn::hash(index)));
  }
  template <typename Index_, typename Val_>
  Key delKey(const Index_ &index, const Val_ &val) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    return delKey__(findPrev__(index, val, code));
  }
  template <typename Index_, typename Val_>
  Val delVal(const Index_ &index, const Val_ &val) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(m_lock);

    return delVal__(findPrev__(index, val, code));
  }

  void clean() {
    unsigned size = 1U<<bits();
    Guard guard(m_lock);

    for (unsigned i = 0; i < size; i++) m_table[i].null();
    m_count = 0;
  }

  void telemetry(ZmHashTelemetry &data) const {
    data.id = ID::id();
    data.addr = (uintptr_t)this;
    data.loadFactor = loadFactor();
    unsigned count = m_count.load_();
    unsigned bits = this->bits();
    data.effLoadFactor = ((double)count) / ((double)(1<<bits));
    data.nodeSize = sizeof(Node);
    data.count = count;
    data.resized = resized();
    data.bits = bits;
    data.cBits = 0;
    data.linear = true;
  }

  auto iterator() { return Iterator(*this); }
  template <typename Index_>
  auto iterator(Index_ &&index) {
    return IndexIterator(*this, ZuFwd<Index_>(index));
  }

  auto readIterator() const { return ReadIterator(*this); }
  template <typename Index_>
  auto readIterator(Index_ &&index) const {
    return ReadIndexIterator(*this, ZuFwd<Index_>(index));
  }

private:
  void startIterate(Iterator_ &iterator) {
    iterator.lock(m_lock);
    iterator.m_slot = -1;
    int next = -1;
    int size = 1<<bits();
    while (++next < size)
      if (!!m_table[next]) {
	iterator.m_next = next;
	return;
      }
    iterator.m_next = -1;
  }
  void startIterate(IndexIterator_ &iterator) {
    iterator.lock(m_lock);
    iterator.m_slot = -1;
    int prev =
      findPrev__(iterator.m_index, IHashFn::hash(iterator.m_index));
    if (prev == -1) {
      iterator.m_next = iterator.m_prev = -1;
      return;
    }
    if (prev < 0)
      iterator.m_next = -prev - 2, iterator.m_prev = -1;
    else
      iterator.m_next = m_table[iterator.m_prev = prev].next();
  }
  void iterate_(Iterator_ &iterator) {
    int next = iterator.m_next;
    if (next < 0) {
      iterator.m_slot = -1;
      return;
    }
    iterator.m_slot = next;
    int size = 1<<bits();
    while (++next < size)
      if (!!m_table[next]) {
	iterator.m_next = next;
	return;
      }
    iterator.m_next = -1;
  }
  void iterate_(IndexIterator_ &iterator) {
    int next = iterator.m_next;
    if (next < 0) {
      iterator.m_slot = -1;
      return;
    }
    if (iterator.m_slot >= 0) iterator.m_prev = iterator.m_slot;
    iterator.m_slot = next;
    while (!m_table[next].tail()) {
      next = m_table[next].next();
      if (ICmp::equals(m_table[next].key(), iterator.m_index)) {
	iterator.m_next = next;
	return;
      }
    }
    iterator.m_next = -1;
  }
  template <typename I>
  const KV *iterate(I &iterator) {
    iterate_(iterator);
    return kv__(iterator.m_slot);
  }
  template <typename I>
  const Key &iterateKey(I &iterator) {
    iterate_(iterator);
    return key__(iterator.m_slot);
  }
  template <typename I>
  const Val &iterateVal(I &iterator) {
    iterate_(iterator);
    return val__(iterator.m_slot);
  }
  void endIterate(Iterator_ &iterator) {
    iterator.unlock(m_lock);
    iterator.m_slot = iterator.m_next = -1;
  }
  void delIterate(Iterator_ &iterator) {
    int slot = iterator.m_slot;
    if (slot < 0) return;
    bool advanceRegardless =
      !m_table[slot].tail() && (int)m_table[slot].next() < slot;
    del___(m_table[slot].head() ? (-slot - 2) : prev(slot));
    if (!advanceRegardless && !!m_table[slot]) iterator.m_next = slot;
    iterator.m_slot = -1;
  }
  void delIterate(IndexIterator_ &iterator) {
    int slot = iterator.m_slot;
    if (slot < 0) return;
    del___(iterator.m_prev < 0 ? (-slot - 2) : iterator.m_prev);
    iterator.m_slot = -1;
    if (!m_table[slot]) return;
    for (;;) {
      if (ICmp::equals(m_table[slot].key(), iterator.m_index)) {
	iterator.m_next = slot;
	return;
      }
      if (m_table[slot].tail()) break;
      slot = m_table[slot].next();
    }
    iterator.m_next = -1;
  }
};

#endif /* ZmLHash_HPP */
