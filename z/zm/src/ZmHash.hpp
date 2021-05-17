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

// hash table (policy-based, separately chained (linked lists), lock striping)

#ifndef ZmHash_HPP
#define ZmHash_HPP

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
#include <zlib/ZuIf.hpp>
#include <zlib/ZuConversion.hpp>

#include <zlib/ZmAtomic.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmHeap.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmLockTraits.hpp>
#include <zlib/ZmNode.hpp>

#include <zlib/ZmHashMgr.hpp>

// hash bits function

struct ZmHash_Bits {
  static uint32_t hashBits(uint32_t code, int bits) {
    if (!bits) return 0;
    return code & ((1U<<bits) - 1);
  }
};

// hash table lock manager

template <class Lock> class ZmHash_LockMgr {
  using LockTraits = ZmLockTraits<Lock>;

  enum { CacheLineSize = ZmPlatform::CacheLineSize };

  ZuAssert(sizeof(Lock) <= CacheLineSize);

  Lock &lock_(unsigned i) const {
    return *(Lock *)((char *)m_locks + (i * CacheLineSize));
  }

protected:
  ZmHash_LockMgr(const ZmHashParams &p) :
      m_bits(p.bits()   < 2 ? 2 : p.bits()  > 28 ? 28 : p.bits()),
      m_cBits(p.cBits() < 0 ? 0 : p.cBits() > 12 ? 12 : p.cBits()) {
    if (m_cBits > m_bits) m_cBits = m_bits;
    unsigned n = 1U<<m_cBits;
    unsigned z = n * CacheLineSize;
#ifndef _WIN32
    if (posix_memalign(&m_locks, CacheLineSize, z)) m_locks = 0;
#else
    m_locks = _aligned_malloc(z, CacheLineSize);
#endif
    if (!m_locks) throw std::bad_alloc();
    for (unsigned i = 0; i < n; ++i) new (&lock_(i)) Lock();
  }
  ~ZmHash_LockMgr() {
    if (ZuUnlikely(!m_locks)) return;
    unsigned n = 1U<<m_cBits;
    for (unsigned i = 0; i < n; ++i) lock_(i).~Lock();
#ifndef _WIN32
    ::free(m_locks);
#else
    _aligned_free(m_locks);
#endif
  }


public:
  unsigned cBits() const { return m_cBits; }

protected:
  Lock &lockCode(uint32_t code) const {
    return lockSlot(ZmHash_Bits::hashBits(code, m_bits));
  }
  Lock &lockSlot(int slot) const {
    return lock_(slot>>(m_bits - m_cBits));
  }

  int lockAllResize(unsigned bits) {
    for (unsigned i = 0; i < (1U<<m_cBits); i++) {
      LockTraits::lock(lock_(i));
      if (m_bits != bits) {
	for (int j = i; j >= 0; --j) LockTraits::unlock(lock_(j));
	return 1;
      }
    }
    return 0;
  }
  void lockAll() {
    unsigned n = (1U<<m_cBits);
    for (unsigned i = 0; i < n; i++) LockTraits::lock(lock_(i));
  }
  void unlockAll() {
    unsigned n = (1U<<m_cBits);
    for (int i = n; --i >= 0; ) LockTraits::unlock(lock_(i));
  }

protected:
  unsigned	m_bits;
private:
  unsigned	m_cBits;
  mutable void	*m_locks;
};

template <> class ZmHash_LockMgr<ZmNoLock> {
protected:
  ZmHash_LockMgr(const ZmHashParams &p) :
      m_bits(p.bits() < 2 ? 2 : p.bits() > 28 ? 28 : p.bits()) { }
  ~ZmHash_LockMgr() { }

public:
  unsigned bits() const { return m_bits; }
  static constexpr unsigned cBits() { return 0; }

protected:
  void bits(unsigned u) { m_bits = u; }

  ZmNoLock &lockCode(uint32_t code) const {
    return const_cast<ZmNoLock &>(m_noLock);
  }
  ZmNoLock &lockSlot(int slot) const {
    return const_cast<ZmNoLock &>(m_noLock);
  }

  int lockAllResize(unsigned bits) { return 0; }
  void lockAll() { }
  void unlockAll() { }

protected:
  unsigned	m_bits;
private:
  ZmNoLock	m_noLock;
};

// NTP (named template parameters):
//
// ZmHash<ZtString,			// keys are ZtStrings
//   ZmHashVal<ZtString,		// values are ZtStrings
//     ZmHashValCmp<ZtICmp> > >		// case-insensitive comparison

// NTP defaults
struct ZmHash_Defaults {
  using Val = ZuNull;
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct HashFnT { using HashFn = ZuHash<T>; };
  template <typename T> struct IHashFnT { using IHashFn = ZuHash<T>; };
  template <typename T> struct IndexT { using Index = T; };
  template <typename T> struct ValCmpT { using ValCmp = ZuCmp<T>; };
  enum { NodeIsKey = 0 };
  enum { NodeIsVal = 0 };
  using Lock = ZmLock;
  using Object = ZmObject;
  struct HeapID { static constexpr const char *id() { return "ZmHash"; } };
  using ID = HeapID;
};

// ZmHashCmp - the key comparator
template <class Cmp_, class NTP = ZmHash_Defaults>
struct ZmHashCmp : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
  template <typename> struct ICmpT { using ICmp = Cmp_; };
};

// ZmHashCmp_ - directly override the key comparator
// (used by other templates to forward NTP parameters to ZmHash)
template <class Cmp_, class NTP = ZmHash_Defaults>
struct ZmHashCmp_ : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
};

// ZmHashICmp - the index comparator
template <class ICmp_, class NTP = ZmHash_Defaults>
struct ZmHashICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

// ZmHashFn - the hash function
template <class HashFn_, class NTP = ZmHash_Defaults>
struct ZmHashFn : public NTP {
  template <typename> struct HashFnT {
    using HashFn = HashFn_;
  };
  template <typename> struct IHashFnT {
    using IHashFn = HashFn_;
  };
};

// ZmHashIFn - the index hash function
template <class IHashFn_, class NTP = ZmHash_Defaults>
struct ZmHashIFn : public NTP {
  template <typename> struct IHashFnT { using IHashFn = IHashFn_; };
};

// ZmHashIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator, hash function and index type
template <class Accessor, class NTP = ZmHash_Defaults>
struct ZmHashIndex : public NTP {
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

// ZmHashIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmHash)
template <class Index_, class NTP = ZmHash_Defaults>
struct ZmHashIndex_ : public NTP {
  template <typename> struct IndexT {
    using Index = Index_;
  };
};

// ZmHashVal - the value type
template <class Val_, class NTP = ZmHash_Defaults>
struct ZmHashVal : public NTP {
  using Val = Val_;
};

// ZmHashValCmp - the value comparator
template <class ValCmp_, class NTP = ZmHash_Defaults>
struct ZmHashValCmp : public NTP {
  template <typename> struct ValCmpT { using ValCmp = ValCmp_; };
};

// ZmHashNodeIsKey - derive ZmHash::Node from Key instead of containing it
template <bool NodeIsKey_, class NTP = ZmHash_Defaults>
struct ZmHashNodeIsKey : public NTP {
  enum { NodeIsKey = NodeIsKey_ };
};

// ZmHashNodeIsVal - derive ZmHash::Node from Val instead of containing it
template <bool NodeIsVal_, class NTP = ZmHash_Defaults>
struct ZmHashNodeIsVal : public NTP {
  enum { NodeIsVal = NodeIsVal_ };
};

// ZmHashLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmHash_Defaults>
struct ZmHashLock : public NTP {
  using Lock = Lock_;
};

// ZmHashObject - the reference-counted object type used for nodes
template <class Object_, class NTP = ZmHash_Defaults>
struct ZmHashObject : public NTP {
  using Object = Object_;
};

// ZmHashHeapID - the heap ID
template <class HeapID_, class NTP = ZmHash_Defaults>
struct ZmHashHeapID : public NTP {
  using HeapID = HeapID_;
  using ID = HeapID_;
};
template <class NTP>
struct ZmHashHeapID<ZuNull, NTP> : public NTP {
  using HeapID = ZuNull;
};

// ZmHashID - the hash ID (if the heap ID is shared)
template <class ID_, class NTP = ZmHash_Defaults>
struct ZmHashID : public NTP {
  using ID = ID_;
};

template <typename Key_, class NTP = ZmHash_Defaults>
class ZmHash :
    public ZmAnyHash,
    public ZmHash_LockMgr<typename NTP::Lock>,
    public ZmNodePolicy<typename NTP::Object> {
  using LockMgr = ZmHash_LockMgr<typename NTP::Lock>;
  using NodePolicy = ZmNodePolicy<typename NTP::Object>;

public:
  using Key = Key_;
  using Val = typename NTP::Val;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using ICmp = typename NTP::template ICmpT<Key>::ICmp;
  using HashFn = typename NTP::template HashFnT<Key>::HashFn;
  using IHashFn = typename NTP::template IHashFnT<Key>::IHashFn;
  using Index = typename NTP::template IndexT<Key>::Index;
  using ValCmp = typename NTP::template ValCmpT<Val>::ValCmp;
  enum { NodeIsKey = NTP::NodeIsKey };
  enum { NodeIsVal = NTP::NodeIsVal };
  using Lock = typename NTP::Lock;
  using Object = typename NodePolicy::Object;
  using ID = typename NTP::ID;
  using HeapID = typename NTP::HeapID;

  using LockTraits = ZmLockTraits<Lock>;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

private:
  using LockMgr::lockCode;
  using LockMgr::lockSlot;
  using LockMgr::lockAllResize;
  using LockMgr::lockAll;
  using LockMgr::unlockAll;

  using LockMgr::m_bits;

public:
  unsigned bits() const { return m_bits; }
  using LockMgr::cBits;

protected:
  template <typename I> class Iterator_;
template <typename> friend class Iterator_;

public:
  // node in a hash table

  struct NullObject { }; // deconflict with ZuNull
  template <typename Node, typename Heap,
    bool NodeIsKey, bool NodeIsVal> class NodeFn :
      public ZuIf<NullObject, Object,
	ZuConversion<ZuNull, Object>::Is ||
	ZuConversion<ZuShadow, Object>::Is ||
	(NodeIsKey && ZuConversion<Object, Key>::Is) ||
	(NodeIsVal && ZuConversion<Object, Val>::Is)>,
      public Heap {
    NodeFn(const NodeFn &);
    NodeFn &operator =(const NodeFn &);	// prevent mis-use

  friend ZmHash<Key, NTP>;
  template <typename> friend class ZmHash<Key, NTP>::Iterator_;

  protected:
    NodeFn() { }

  private:
    void init() { m_next = 0; }

    // access to these methods is always guarded, so no need to protect
    // the returned object against concurrent deletion; these are private
    Node *next() const { return m_next; }

    void next(Node *n) { m_next = n; }

    Node		*m_next;
  };

  template <typename Heap>
  using Node_ = ZmKVNode<Heap, NodeIsKey, NodeIsVal, NodeFn, Key, Val>;
  struct NullHeap { }; // deconflict with ZuNull
  using NodeHeap = ZmHeap<HeapID, sizeof(Node_<NullHeap>)>;
  using Node = Node_<NodeHeap>;
  using Fn = typename Node::Fn;

  using NodeRef = typename NodePolicy::template Ref<Node>::T;
  using NodePtr = Node *;

private:
  using NodePolicy::nodeRef;
  using NodePolicy::nodeDeref;
  using NodePolicy::nodeDelete;

protected:
  template <typename I> struct Iterator__ {
    const Key &iterateKey() {
      Node *node = static_cast<I *>(this)->iterate();
      if (ZuLikely(node)) return node->Node::key();
      return Cmp::null();
    }
    const Val &iterateVal() {
      Node *node = static_cast<I *>(this)->iterate();
      if (ZuLikely(node)) return node->Node::val();
      return ValCmp::null();
    }
  };

  template <typename I> class Iterator_ : public Iterator__<I> {
    Iterator_(const Iterator_ &) = delete;
    Iterator_ &operator =(const Iterator_ &) = delete;

    using Hash = ZmHash<Key, NTP>;
  friend Hash;

  protected:
    Iterator_(Iterator_ &&) = default;
    Iterator_ &operator =(Iterator_ &&) = default;

    Iterator_(Hash &hash) : m_hash(hash) { }

  public:
    void reset() {
      m_hash.startIterate(static_cast<I &>(*this));
    }
    Node *iterate() {
      return m_hash.iterate(static_cast<I &>(*this));
    }

    unsigned count() const { return m_hash.count_(); }

  protected:
    Hash			&m_hash;
    int				m_slot;
    typename Hash::Node		*m_node;
    typename Hash::Node		*m_prev;
  };

  template <typename I> class IndexIterator_ : public Iterator_<I> {
    using Hash = ZmHash<Key, NTP>;
  friend class ZmHash<Key, NTP>;

    using Iterator_<I>::m_hash;

    IndexIterator_(const IndexIterator_ &) = delete;
    IndexIterator_ &operator =(const IndexIterator_ &) = delete;

  protected:
    IndexIterator_(IndexIterator_ &&) = default;
    IndexIterator_ &operator =(IndexIterator_ &&) = default;

    template <typename Index_>
    IndexIterator_(Hash &hash, Index_ &&index) :
	Iterator_<I>(hash), m_index(ZuFwd<Index_>(index)) { }

  public:
    void reset() {
      m_hash.startIndexIterate(static_cast<I &>(*this));
    }
    Node *iterate() {
      return m_hash.indexIterate(static_cast<I &>(*this));
    }

  protected:
    Index			m_index;
  };

public:
  class Iterator : public Iterator_<Iterator> {
    Iterator(const Iterator &) = delete;
    Iterator &operator =(const Iterator &) = delete;

    using Hash = ZmHash<Key, NTP>;
  friend Hash;
    using Base = Iterator_<Iterator>;

    using Base::m_hash;

    void lock(Lock &l) { LockTraits::lock(l); }
    void unlock(Lock &l) { LockTraits::unlock(l); }

  public:
    Iterator(Iterator &&) = default;
    Iterator &operator =(Iterator &&) = default;

    Iterator(Hash &hash) : Base(hash) { hash.startIterate(*this); }
    virtual ~Iterator() { m_hash.endIterate(*this); }
    void del() { m_hash.delIterate(*this); }
  };

  class ReadIterator : public Iterator_<ReadIterator> {
    ReadIterator(const ReadIterator &) = delete;
    ReadIterator &operator =(const ReadIterator &) = delete;

    using Hash = ZmHash<Key, NTP>;
  friend Hash;
    using Base = Iterator_<ReadIterator>;

    using Base::m_hash;

    void lock(Lock &l) { LockTraits::readlock(l); }
    void unlock(Lock &l) { LockTraits::readunlock(l); }

  public:
    ReadIterator(ReadIterator &&) = default;
    ReadIterator &operator =(ReadIterator &&) = default;

    ReadIterator(const Hash &hash) : Base(const_cast<Hash &>(hash)) {
      const_cast<Hash &>(hash).startIterate(*this);
    }
    ~ReadIterator() { m_hash.endIterate(*this); }
  };

  class IndexIterator : public IndexIterator_<IndexIterator> {
    IndexIterator(const IndexIterator &) = delete;
    IndexIterator &operator =(const IndexIterator &) = delete;

    using Hash = ZmHash<Key, NTP>;
  friend Hash;
    using Base = IndexIterator_<IndexIterator>;

    using Base::m_hash;

    void lock(Lock &l) { LockTraits::lock(l); }
    void unlock(Lock &l) { LockTraits::unlock(l); }

  public:
    IndexIterator(IndexIterator &&) = default;
    IndexIterator &operator =(IndexIterator &&) = default;

    template <typename Index_>
    IndexIterator(Hash &hash, Index_ &&index) :
	Base(hash, ZuFwd<Index_>(index)) {
      hash.startIndexIterate(*this);
    }
    ~IndexIterator() { m_hash.endIterate(*this); }
    void del() { m_hash.delIterate(*this); }
  };

  class ReadIndexIterator : public IndexIterator_<ReadIndexIterator> {
    ReadIndexIterator(const ReadIndexIterator &) = delete;
    ReadIndexIterator &operator =(const ReadIndexIterator &) = delete;

    using Hash = ZmHash<Key, NTP>;
  friend Hash;
    using Base = IndexIterator_<ReadIndexIterator>;

    using Base::m_hash;

    void lock(Lock &l) { LockTraits::readlock(l); }
    void unlock(Lock &l) { LockTraits::readunlock(l); }

  public:
    ReadIndexIterator(ReadIndexIterator &&) = default;
    ReadIndexIterator &operator =(ReadIndexIterator &&) = default;

    template <typename Index_>
    ReadIndexIterator(const Hash &hash, Index_ &&index) :
	Base(const_cast<Hash &>(hash), ZuFwd<Index_>(index)) {
      const_cast<Hash &>(hash).startIndexIterate(*this);
    }
    ~ReadIndexIterator() { m_hash.endIterate(*this); }
  };

private:
  void init(const ZmHashParams &params) {
    double loadFactor = params.loadFactor();
    if (loadFactor < 1.0) loadFactor = 1.0;
    m_loadFactor = (unsigned)(loadFactor * (1<<4));
    m_table = new NodePtr[1U<<m_bits];
    memset(m_table, 0, sizeof(NodePtr)<<m_bits);
    ZmHashMgr::add(this);
  }

public:
  ZmHash(ZmHashParams params = ZmHashParams(ID::id())) :
      ZmHash_LockMgr<Lock>(params) {
    init(params);
  }
  ZmHash(const ZmHash &) = delete;
  ZmHash &operator =(const ZmHash &) = delete;
  ZmHash(ZmHash &&) = delete;
  ZmHash &operator =(ZmHash &&) = delete;

  ~ZmHash() {
    ZmHashMgr::del(this);
    clean();
    delete [] m_table;
  }

  unsigned loadFactor_() const { return m_loadFactor; }
  double loadFactor() const { return (double)m_loadFactor / 16.0; }
  unsigned size() const {
    return (double)(((uint64_t)1)<<m_bits) * loadFactor();
  }

  unsigned count_() const { return m_count.load_(); }

  template <typename Key__>
  ZuNotConvertible<
	ZuDeref<Key__>, NodeRef, NodeRef>
      add(Key__ &&key) {
    NodeRef node = new Node(ZuFwd<Key__>(key));
    this->add(node);
    return node;
  }
  template <typename Key__, typename Val_>
  NodeRef add(Key__ &&key, Val_ &&val) {
    NodeRef node = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    this->add(node);
    return node;
  }
  template <typename NodeRef_>
  ZuConvertible<NodeRef_, NodeRef>
      add(const NodeRef_ &node_) {
    const NodeRef &node = node_;
    uint32_t code = HashFn::hash(node->Node::key());
    Guard guard(lockCode(code));
    addNode_(node, code);
  }
private:
  template <typename NodeRef_>
  void addNode_(NodeRef_ &&node, uint32_t code) {
    unsigned count = m_count.load_();

    node->Fn::init();
    {
      unsigned bits = m_bits;

      if (count < (1U<<28) && ((count<<4)>>bits) >= m_loadFactor) {
	Lock &lock = lockCode(code);

	LockTraits::unlock(lock);
	resize(bits);
	LockTraits::lock(lock);
      }
    }

    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    nodeRef(node);
    node->Fn::next(m_table[slot]);
    m_table[slot] = ZuFwd<NodeRef_>(node);
    m_count.store_(count + 1);
  }

public:
  template <typename Index_>
  NodeRef find(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    return find_(index, code);
  }
  template <typename Index_>
  Node *findPtr(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    return find_(index, code);
  }
  template <typename Index_>
  Key findKey(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    Node *node = find_(index, code);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_>
  Val findVal(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    Node *node = find_(index, code);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Index_>
  Node *find_(const Index_ &index, uint32_t code) const {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    for (node = m_table[slot];
	 node && !ICmp::equals(node->Node::key(), index);
	 node = node->Fn::next());

    return node;
  }

public:
  template <typename Index_, typename Val_>
  NodeRef find(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    return findKeyVal_(index, val, code);
  }
  template <typename Index_, typename Val_>
  Node *findPtr(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    return findKeyVal_(index, val, code);
  }
  template <typename Index_, typename Val_>
  Key findKey(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    Node *node = findKeyVal_(index, val, code);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_, typename Val_>
  Val findVal(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(lockCode(code));
    Node *node = findKeyVal_(index, val, code);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Index_, typename Val_>
  Node *findKeyVal_(
      const Index_ &index, const Val_ &val, uint32_t code) const {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    for (node = m_table[slot];
	 node && (!ICmp::equals(node->Node::key(), index) ||
		  !ValCmp::equals(node->Node::val(), val));
	 node = node->Fn::next());

    return node;
  }

public:
  template <typename Key__>
  NodeRef findAdd(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    return findAdd_(ZuFwd<Key__>(key), Val(), code);
  }
  template <typename Key__, typename Val_>
  NodeRef findAdd(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    return findAdd_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }
  template <typename Key__>
  Node *findAddPtr(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    return findAdd_(ZuFwd<Key__>(key), Val(), code);
  }
  template <typename Key__, typename Val_>
  Node *findAddPtr(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    return findAdd_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }
  template <typename Key__>
  Key findAddKey(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    Node *node = findAdd_(ZuFwd<Key__>(key), Val(), code);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Key__, typename Val_>
  Key findAddKey(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    Node *node = findAdd_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Key__, typename Val_>
  Val findAddVal(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(lockCode(code));
    Node *node = findAdd_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Key__, typename Val_>
  Node *findAdd_(Key__ &&key, Val_ &&val, uint32_t code) {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    for (node = m_table[slot];
	 node && !Cmp::equals(node->Node::key(), key);
	 node = node->Fn::next());

    if (!node) {
      NodeRef nodeRef = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
      node = nodeRef.ptr();
      addNode_(ZuMv(nodeRef), code);
    }
    return node;
  }

public:
  template <typename Index__>
  ZuNotConvertible<Index__, Node *, NodeRef>
      del(const Index__ &index) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(lockCode(code));
    return del_(index, code);
  }
  template <typename Index_>
  Key delKey(const Index_ &index) {
    NodeRef node = del(index);
    if (ZuUnlikely(!node)) return Cmp::null();
    Key key = ZuMv(node->Node::key());
    nodeDelete(node);
    return key;
  }
  template <typename Index_>
  Val delVal(const Index_ &index) {
    NodeRef node = del(index);
    if (ZuUnlikely(!node)) return ValCmp::null();
    Val val = ZuMv(node->Node::val());
    nodeDelete(node);
    return val;
  }
private:
  template <typename Index_>
  NodeRef del_(const Index_ &index, uint32_t code) {
    unsigned count = m_count.load_();
    if (!count) return 0;

    Node *node, *prevNode = 0;
    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    for (node = m_table[slot];
	 node && !ICmp::equals(node->Node::key(), index);
	 prevNode = node, node = node->Fn::next());

    if (!node) return 0;

    if (!prevNode)
      m_table[slot] = node->Fn::next();
    else
      prevNode->Fn::next(node->Fn::next());

    m_count.store_(count - 1);

    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }

public:
  template <typename Index_, typename Val_>
  NodeRef del(const Index_ &index, const Val_ &val) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(lockCode(code));
    return delKeyVal_(index, val, code);
  }
private:
  template <typename Index_, typename Val_>
  NodeRef delKeyVal_(
      const Index_ &index, const Val_ &val, uint32_t code) {
    unsigned count = m_count.load_();
    if (!count) return 0;

    Node *node, *prevNode = 0;
    unsigned slot = ZmHash_Bits::hashBits(code, m_bits);

    for (node = m_table[slot];
	 node && (!ICmp::equals(node->Node::key(), index) ||
		  !ValCmp::equals(node->Node::val(), val));
	 prevNode = node, node = node->Fn::next());

    if (!node) return 0;

    if (!prevNode)
      m_table[slot] = node->Fn::next();
    else
      prevNode->Fn::next(node->Fn::next());

    m_count.store_(count - 1);

    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }

public:
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
  template <typename I>
  void startIterate(I &iterator) {
    iterator.lock(lockSlot(0));
    iterator.m_slot = 0;
    iterator.m_node = 0;
    iterator.m_prev = 0;
  }
  template <typename I>
  void startIndexIterate(I &iterator) {
    uint32_t code = IHashFn::hash(iterator.m_index);

    iterator.lock(lockCode(code));
    iterator.m_slot = ZmHash_Bits::hashBits(code, m_bits);
    iterator.m_node = 0;
    iterator.m_prev = 0;
  }
  template <typename I>
  Node *iterate(I &iterator) {
    int slot = iterator.m_slot;

    if (slot < 0) return 0;

    Node *node = iterator.m_node, *prevNode;

    if (!node) {
      prevNode = 0;
      node = m_table[slot];
    } else {
      prevNode = node;
      node = node->Fn::next();
    }

    if (!node) {
      prevNode = 0;
      do {
	iterator.unlock(lockSlot(slot));
	if (++slot >= (1U<<m_bits)) {
	  iterator.m_slot = -1;
	  return 0;
	}
	iterator.lock(lockSlot(slot));
	iterator.m_slot = slot;
      } while (!(node = m_table[slot]));
    }

    iterator.m_prev = prevNode;
    return iterator.m_node = node;
  }
  template <typename I>
  Node *indexIterate(I &iterator) {
    int slot = iterator.m_slot;

    if (slot < 0) return 0;

    Node *node = iterator.m_node, *prevNode;

    if (!node) {
      prevNode = 0;
      node = m_table[slot];
    } else {
      prevNode = node;
      node = node->Fn::next();
    }

    for (;
	 node && !ICmp::equals(node->Node::key(), iterator.m_index);
	 prevNode = node, node = node->Fn::next());

    if (!node) {
      iterator.unlock(lockSlot(slot));
      iterator.m_slot = -1;
      return 0;
    }

    iterator.m_prev = prevNode;
    return iterator.m_node = node;
  }
  template <typename I>
  void endIterate(I &iterator) {
    if (iterator.m_slot < 0) return;

    iterator.unlock(lockSlot(iterator.m_slot));
  }
  template <typename I>
  void delIterate(I &iterator) {
    Node *node = iterator.m_node, *prevNode = iterator.m_prev;

    unsigned count = m_count.load_();
    if (!count || !node) return;

    if (!prevNode)
      m_table[iterator.m_slot] = node->Fn::next();
    else
      prevNode->Fn::next(node->Fn::next());

    iterator.m_node = prevNode;
    nodeDeref(node);
    nodeDelete(node);
    m_count.store_(count - 1);
  }

public:
  void clean() {
    lockAll();

    for (unsigned i = 0, n = (1U<<m_bits); i < n; i++) {
      Node *node, *prevNode;

      node = m_table[i];

      while (prevNode = node) {
	node = prevNode->Fn::next();
	nodeDeref(prevNode);
	nodeDelete(prevNode);
      }

      m_table[i] = 0;
    }
    m_count = 0;

    unlockAll();
  }

  template <typename Index_>
  Lock &lock(Index_ &&index) {
    return lockCode(IHashFn::hash(ZuFwd<Index_>(index)));
  }

  void telemetry(ZmHashTelemetry &data) const {
    data.id = ID::id();
    data.addr = reinterpret_cast<uintptr_t>(this);
    data.loadFactor = loadFactor();
    unsigned count = m_count.load_();
    unsigned bits = m_bits;
    data.effLoadFactor = static_cast<double>(count) / (1<<bits);
    data.nodeSize = sizeof(Node);
    data.count = count;
    data.resized = m_resized.load_();
    data.bits = bits;
    data.cBits = cBits();
    data.linear = false;
  }

private:
  void resize(unsigned bits) {
    if (lockAllResize(bits)) return;

    m_resized.store_(m_resized.load_() + 1);

    unsigned n = (1U<<bits);

    m_bits = ++bits;

    NodePtr *table = new NodePtr[1<<bits];
    memset(table, 0, sizeof(NodePtr)<<bits);
    Node *node, *nextNode;

    for (unsigned i = 0; i < n; i++)
      for (node = m_table[i]; node; node = nextNode) {
	nextNode = node->Fn::next();
	unsigned j = ZmHash_Bits::hashBits(
	    HashFn::hash(node->Node::key()), bits);
	node->Fn::next(table[j]);
	table[j] = node;
      }
    delete [] m_table;
    m_table = table;

    unlockAll();
  }

  unsigned		m_loadFactor = 0;
  ZmAtomic<unsigned>	m_count = 0;
  ZmAtomic<unsigned>	m_resized = 0;
  NodePtr		*m_table;
};

#endif /* ZmHash_HPP */
