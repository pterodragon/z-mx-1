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

// hash table (separately chained (linked lists), lock striping)

#ifndef ZmHash_HPP
#define ZmHash_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuHash.hpp>
#include <ZuIndex.hpp>
#include <ZuTuple.hpp>
#include <ZuMixin.hpp>
#include <ZuIf.hpp>
#include <ZuConversion.hpp>

#include <ZmAtomic.hpp>
#include <ZmGuard.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmHeap.hpp>
#include <ZmLock.hpp>
#include <ZmNoLock.hpp>
#include <ZmLockTraits.hpp>
#include <ZmKVNode.hpp>

#include <ZmHashMgr.hpp>

// hash bits function

struct ZmHash_Bits {
  inline static uint32_t hashBits(uint32_t code, int bits) {
    if (!bits) return 0;
    return code & ((1U<<bits) - 1);
  }
};

// hash table lock manager

template <class Lock> class ZmHash_LockMgr {
  typedef ZmLockTraits<Lock> LockTraits;

  enum { CacheLineSize = ZmPlatform::CacheLineSize };

  ZuAssert(sizeof(Lock) <= CacheLineSize);

  ZuInline Lock &lock_(unsigned i) const {
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
  inline unsigned bits() const { return m_bits; }
  inline unsigned cBits() const { return m_cBits; }

protected:
  ZuInline void bits(unsigned u) { m_bits = u; }

  ZuInline Lock &lockCode(uint32_t code) const {
    return lockSlot(ZmHash_Bits::hashBits(code, m_bits));
  }
  ZuInline Lock &lockSlot(int slot) const {
    return lock_(slot>>(m_bits - m_cBits));
  }

  inline int lockAllResize(unsigned bits) {
    for (unsigned i = 0; i < (1U<<m_cBits); i++) {
      LockTraits::lock(lock_(i));
      if (m_bits != bits) {
	for (int j = i; j >= 0; --j) LockTraits::unlock(lock_(j));
	return 1;
      }
    }
    return 0;
  }
  inline void lockAll() {
    unsigned n = (1U<<m_cBits);
    for (unsigned i = 0; i < n; i++) LockTraits::lock(lock_(i));
  }
  inline void unlockAll() {
    unsigned n = (1U<<m_cBits);
    for (int i = n; --i >= 0; ) LockTraits::unlock(lock_(i));
  }

private:
  unsigned		m_bits;
  unsigned		m_cBits;
  mutable void		*m_locks;
};

template <> class ZmHash_LockMgr<ZmNoLock> {
protected:
  ZmHash_LockMgr(const ZmHashParams &p) :
      m_bits(p.bits()   < 2 ? 2 : p.bits() >  28 ? 28 : p.bits()) { }
  ~ZmHash_LockMgr() { }

public:
  inline unsigned bits() const { return m_bits; }
  inline unsigned cBits() const { return 0; }

protected:
  inline void bits(unsigned u) { m_bits = u; }

  inline ZmNoLock &lockCode(uint32_t code) const {
    return const_cast<ZmNoLock &>(m_noLock);
  }
  inline ZmNoLock &lockSlot(int slot) const {
    return const_cast<ZmNoLock &>(m_noLock);
  }

  inline int lockAllResize(unsigned bits) { return 0; }
  inline void lockAll() { }
  inline void unlockAll() { }

private:
  volatile unsigned	m_bits;
  ZmNoLock		m_noLock;
};

// NTP (named template parameters):
//
// ZmHash<ZtString,			// keys are ZtStrings
//   ZmHashBase<ZmObject,		// base of ZmObject
//     ZmHashVal<ZtString,		// values are ZtStrings
//	 ZmHashValCmp<ZtICmp> > > >	// case-insensitive comparison

// NTP defaults
struct ZmHash_Defaults {
  typedef ZuNull Val;
  template <typename T> struct CmpT { typedef ZuCmp<T> Cmp; };
  template <typename T> struct ICmpT { typedef ZuCmp<T> ICmp; };
  template <typename T> struct HashFnT { typedef ZuHash<T> HashFn; };
  template <typename T> struct IHashFnT { typedef ZuHash<T> IHashFn; };
  template <typename T> struct IndexT { typedef T Index; };
  template <typename T> struct ValCmpT { typedef ZuCmp<T> ValCmp; };
  enum { NodeIsKey = 0 };
  enum { NodeIsVal = 0 };
  typedef ZmLock Lock;
  typedef ZmObject Object;
  struct HeapID { inline static const char *id() { return "ZmHash"; } };
  typedef HeapID ID;
  struct Base { };
};

// ZmHashCmp - the key comparator
template <class Cmp_, class NTP = ZmHash_Defaults>
struct ZmHashCmp : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
  template <typename> struct ICmpT { typedef Cmp_ ICmp; };
};

// ZmHashCmp_ - directly override the key comparator
// (used by other templates to forward NTP parameters to ZmHash)
template <class Cmp_, class NTP = ZmHash_Defaults>
struct ZmHashCmp_ : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
};

// ZmHashICmp - the index comparator
template <class ICmp_, class NTP = ZmHash_Defaults>
struct ZmHashICmp : public NTP {
  template <typename> struct ICmpT { typedef ICmp_ ICmp; };
};

// ZmHashFn - the hash function
template <class HashFn_, class NTP = ZmHash_Defaults>
struct ZmHashFn : public NTP {
  template <typename> struct HashFnT {
    typedef HashFn_ HashFn;
  };
  template <typename> struct IHashFnT {
    typedef HashFn_ IHashFn;
  };
};

// ZmHashIFn - the index hash function
template <class IHashFn_, class NTP = ZmHash_Defaults>
struct ZmHashIFn : public NTP {
  template <typename> struct IHashFnT { typedef IHashFn_ IHashFn; };
};

// ZmHashIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator, hash function and index type
template <class Accessor, class NTP = ZmHash_Defaults>
struct ZmHashIndex : public NTP {
  template <typename T> struct CmpT {
    typedef typename ZuIndex<Accessor>::template CmpT<T> Cmp;
  };
  template <typename> struct ICmpT {
    typedef typename ZuIndex<Accessor>::ICmp ICmp;
  };
  template <typename> struct HashFnT {
    typedef typename ZuIndex<Accessor>::Hash HashFn;
  };
  template <typename> struct IHashFnT {
    typedef typename ZuIndex<Accessor>::IHash IHashFn;
  };
  template <typename> struct IndexT {
    typedef typename ZuIndex<Accessor>::I Index;
  };
};

// ZmHashIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmHash)
template <class Index_, class NTP = ZmHash_Defaults>
struct ZmHashIndex_ : public NTP {
  template <typename> struct IndexT {
    typedef Index_ Index;
  };
};

// ZmHashVal - the value type
template <class Val_, class NTP = ZmHash_Defaults>
struct ZmHashVal : public NTP {
  typedef Val_ Val;
};

// ZmHashValCmp - the value comparator
template <class ValCmp_, class NTP = ZmHash_Defaults>
struct ZmHashValCmp : public NTP {
  template <typename> struct ValCmpT { typedef ValCmp_ ValCmp; };
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
  typedef Lock_ Lock;
};

// ZmHashObject - the reference-counted object type used for nodes
template <class Object_, class NTP = ZmHash_Defaults>
struct ZmHashObject : public NTP {
  typedef Object_ Object;
};

// ZmHashHeapID - the heap ID
template <class HeapID_, class NTP = ZmHash_Defaults>
struct ZmHashHeapID : public NTP {
  typedef HeapID_ HeapID;
  typedef HeapID_ ID;
};
template <class NTP>
struct ZmHashHeapID<ZmNoHeap, NTP> : public NTP {
  typedef ZmNoHeap HeapID;
};

// ZmHashID - the hash ID (if the heap ID is shared)
template <class ID_, class NTP = ZmHash_Defaults>
struct ZmHashID : public NTP {
  typedef ID_ ID;
};

// ZmHashBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmHash_Defaults>
struct ZmHashBase : public NTP {
  typedef Base_ Base;
};

template <typename Key_, class NTP = ZmHash_Defaults>
class ZmHash : public NTP::Base, public ZmHash_LockMgr<typename NTP::Lock> {
  ZmHash(const ZmHash &);
  ZmHash &operator =(const ZmHash &);	// prevent mis-use

public:
  typedef Key_ Key;
  typedef typename NTP::Val Val;
  typedef typename NTP::template CmpT<Key>::Cmp Cmp;
  typedef typename NTP::template ICmpT<Key>::ICmp ICmp;
  typedef typename NTP::template HashFnT<Key>::HashFn HashFn;
  typedef typename NTP::template IHashFnT<Key>::IHashFn IHashFn;
  typedef typename NTP::template IndexT<Key>::Index Index;
  typedef typename NTP::template ValCmpT<Val>::ValCmp ValCmp;
  enum { NodeIsKey = NTP::NodeIsKey };
  enum { NodeIsVal = NTP::NodeIsVal };
  typedef typename NTP::Lock Lock;
  typedef typename NTP::Object Object;
  typedef typename NTP::ID ID;
  typedef typename NTP::HeapID HeapID;

  typedef ZmLockTraits<Lock> LockTraits;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

private:
  // CheckHashFn ensures that legacy hash functions returning int
  // trigger a compile-time assertion failure; hash() must return uint32_t
  class CheckHashFn {
    typedef char Small;
    struct Big { char _[2]; };
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

protected:
  template <typename I> class Iterator_;
template <typename> friend class Iterator_;

public:
  // node in a hash table

  struct NullObject { }; // deconflict with ZuNull
  template <typename Node, typename Heap,
    bool NodeIsKey, bool NodeIsVal> class NodeFn :
      public ZuIf<NullObject, Object,
	ZuConversion<Object, ZuNull>::Is ||
	(NodeIsKey && ZuConversion<Object, Key>::Is) ||
	(NodeIsVal && ZuConversion<Object, Val>::Is)>::T,
      public Heap {
    NodeFn(const NodeFn &);
    NodeFn &operator =(const NodeFn &);	// prevent mis-use

  friend class ZmHash<Key, NTP>;
  template <typename> friend class ZmHash<Key, NTP>::Iterator_;

  protected:
    inline NodeFn() : m_next(0) { }

  private:
    // access to these methods is always guarded, so no need to protect
    // the returned object against concurrent deletion; these are private
    ZuInline Node *next() const { return m_next; }

    ZuInline void next(Node *n) { m_next = n; }

    Node		*m_next;
  };

  template <typename Heap>
  using Node_ = ZmKVNode<Heap, NodeIsKey, NodeIsVal, NodeFn, Key, Val>;
  struct NullHeap { }; // deconflict with ZuNull
  typedef ZmHeap<HeapID, sizeof(Node_<NullHeap>)> NodeHeap;
  typedef Node_<NodeHeap> Node;
  typedef typename Node::Fn Fn;

  typedef typename ZuIf<ZmRef<Node>, Node *, ZuIsObject_<Object>::OK>::T
    NodeRef;
  typedef Node *NodePtr;

private:
  // in order to support both intrusively reference-counted and plain node
  // objects, some overloading is required for ref/deref/delete
  template <typename O>
  ZuInline void nodeRef(const ZmRef<O> &o) { ZmREF(o); }
  template <typename O>
  ZuInline typename ZuIsObject<O>::T nodeRef(const O *o) { ZmREF(o); }
  template <typename O>
  ZuInline void nodeDeref(const ZmRef<O> &o) { ZmDEREF(o); }
  template <typename O>
  ZuInline typename ZuIsObject<O>::T nodeDeref(const O *o) { ZmDEREF(o); }
  template <typename O>
  ZuInline void nodeDelete(const ZmRef<O> &o) { }
  template <typename O>
  ZuInline typename ZuIsObject<O>::T nodeDelete(const O *) { }

  template <typename O>
  ZuInline typename ZuNotObject<O>::T nodeRef(const O *) { }
  template <typename O>
  ZuInline typename ZuNotObject<O>::T nodeDeref(const O *) { }
  template <typename O>
  ZuInline typename ZuNotObject<O>::T nodeDelete(const O *o) { delete o; }

protected:
  template <typename I> struct Iterator__ {
    inline const Key &iterateKey() {
      NodeRef node = static_cast<I *>(this)->iterate();
      if (ZuLikely(node)) return node->Node::key();
      return Cmp::null();
    }
    inline const Val &iterateVal() {
      NodeRef node = static_cast<I *>(this)->iterate();
      if (ZuLikely(node)) return node->Node::val();
      return ValCmp::null();
    }
  };

  template <typename I> class Iterator_ : public Iterator__<I> {
    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;

  protected:
    ZuInline Iterator_(Hash &hash) : m_hash(hash) { }

  public:
    ZuInline void reset() {
      m_hash.startIterate(static_cast<I &>(*this));
    }
    ZuInline NodeRef iterate() {
      return m_hash.iterate(static_cast<I &>(*this));
    }

  protected:
    Hash			&m_hash;
    int				m_slot;
    typename Hash::NodeRef	m_node;
    typename Hash::NodeRef	m_prev;
  };

  template <typename I> class IndexIterator_ : public Iterator_<I> {
    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;

  protected:
    template <typename Index_>
    ZuInline IndexIterator_(Hash &hash, Index_ &&index) :
	Iterator_<I>(hash), m_index(ZuFwd<Index_>(index)) { }

  public:
    ZuInline void reset() {
      this->m_hash.startIndexIterate(static_cast<I &>(*this));
    }
    ZuInline NodeRef iterate() {
      return this->m_hash.indexIterate(static_cast<I &>(*this));
    }

  protected:
    Index			m_index;
  };

public:
  class Iterator : public Iterator_<Iterator> {
    Iterator(const Iterator &) = delete;
    Iterator &operator =(const Iterator &) = delete;

    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;
    typedef Iterator_<Iterator> Base;

    void lock(Lock &l) { LockTraits::lock(l); }
    void unlock(Lock &l) { LockTraits::unlock(l); }

  public:
    inline Iterator(Hash &hash) : Base(hash) { hash.startIterate(*this); }
    virtual ~Iterator() { this->m_hash.endIterate(*this); }
    inline void del() { this->m_hash.delIterate(*this); }
  };

  class ReadIterator : public Iterator_<ReadIterator> {
    ReadIterator(const ReadIterator &) = delete;
    ReadIterator &operator =(const ReadIterator &) = delete;

    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;
    typedef Iterator_<ReadIterator> Base;

    ZuInline void lock(Lock &l) { LockTraits::readlock(l); }
    ZuInline void unlock(Lock &l) { LockTraits::readunlock(l); }

  public:
    ZuInline ReadIterator(const Hash &hash) : Base(const_cast<Hash &>(hash)) {
      const_cast<Hash &>(hash).startIterate(*this);
    }
    ZuInline ~ReadIterator() { this->m_hash.endIterate(*this); }
  };

  class IndexIterator : public IndexIterator_<IndexIterator> {
    IndexIterator(const IndexIterator &) = delete;
    IndexIterator &operator =(const IndexIterator &) = delete;

    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;
    typedef IndexIterator_<IndexIterator> Base;

    ZuInline void lock(Lock &l) { LockTraits::lock(l); }
    ZuInline void unlock(Lock &l) { LockTraits::unlock(l); }

  public:
    template <typename Index_>
    ZuInline IndexIterator(Hash &hash, Index_ &&index) :
	Base(hash, ZuFwd<Index_>(index)) {
      hash.startIndexIterate(*this);
    }
    ZuInline ~IndexIterator() { this->m_hash.endIterate(*this); }
    ZuInline void del() { this->m_hash.delIterate(*this); }
  };

  class ReadIndexIterator : public IndexIterator_<ReadIndexIterator> {
    ReadIndexIterator(const ReadIndexIterator &) = delete;
    ReadIndexIterator &operator =(const ReadIndexIterator &) = delete;

    typedef ZmHash<Key, NTP> Hash;
  friend class ZmHash<Key, NTP>;
    typedef IndexIterator_<ReadIndexIterator> Base;

    ZuInline void lock(Lock &l) { LockTraits::readlock(l); }
    ZuInline void unlock(Lock &l) { LockTraits::readunlock(l); }

  public:
    template <typename Index_>
    ZuInline ReadIndexIterator(const Hash &hash, Index_ &&index) :
	Base(const_cast<Hash &>(hash), ZuFwd<Index_>(index)) {
      const_cast<Hash &>(hash).startIndexIterate(*this);
    }
    ZuInline ~ReadIndexIterator() { this->m_hash.endIterate(*this); }
  };

private:
  inline void init(const ZmHashParams &params) {
    double loadFactor = params.loadFactor();
    if (loadFactor < 1.0) loadFactor = 1.0;
    m_loadFactor = (unsigned)(loadFactor * (1<<4));
    m_table = new NodePtr[1<<this->bits()];
    memset(m_table, 0, sizeof(NodePtr)<<(this->bits()));
    ZmHashMgr::add(this,
	ZmHashMgr::ReportFn::Member<&ZmHash::report>::fn(this));
  }

public:
  template <typename ...Args>
  inline ZmHash(ZmHashParams params = ZmHashParams(ID::id()), Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      ZmHash_LockMgr<Lock>(params),
      m_count(0), m_resized(0) {
    init(params);
  }

  ~ZmHash() {
    ZmHashMgr::del(this);
    clean();
    delete [] m_table;
  }

  inline unsigned count() const { return m_count; }

  template <typename Key__>
  inline typename ZuNotConvertible<
	typename ZuDeref<Key__>::T, NodeRef, NodeRef>::T
      add(Key__ &&key) {
    NodeRef node = new Node(ZuFwd<Key__>(key));
    this->add(node);
    return node;
  }
  template <typename Key__, typename Val_>
  inline NodeRef add(Key__ &&key, Val_ &&val) {
    NodeRef node = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    this->add(node);
    return node;
  }
  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef_, NodeRef>::T
      add(const NodeRef_ &node_) {
    const NodeRef &node = node_;
    uint32_t code = HashFn::hash(node->Node::key());
    Guard guard(this->lockCode(code));
    addNode_(node, code);
  }
private:
  template <typename NodeRef_>
  inline void addNode_(NodeRef_ &&node, uint32_t code) {
    {
      unsigned bits = this->bits();

      if (m_count < (1U<<28) && ((m_count<<4)>>bits) >= m_loadFactor) {
	Lock &lock = this->lockCode(code);

	LockTraits::unlock(lock);
	resize(bits);
	LockTraits::lock(lock);
      }
    }

    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    nodeRef(node);
    node->Fn::next(m_table[slot]);
    m_table[slot] = ZuFwd<NodeRef_>(node);
    ++m_count;
  }

public:
  template <typename Index_>
  inline NodeRef find(const Index_ &index) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(this->lockCode(code));

    return find_(index, code);
  }
  template <typename Index_>
  inline Key findKey(const Index_ &index) const {
    NodeRef node = find(index);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_>
  inline Val findVal(const Index_ &index) const {
    NodeRef node = find(index);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Index_>
  inline NodeRef find_(const Index_ &index, uint32_t code) const {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    for (node = m_table[slot];
	 node && !ICmp::equals(node->Node::key(), index);
	 node = node->Fn::next());

    return node;
  }

public:
  template <typename Index_, typename Val_>
  inline NodeRef find(const Index_ &index, const Val_ &val) const {
    uint32_t code = IHashFn::hash(index);
    ReadGuard guard(this->lockCode(code));

    return findKeyVal_(index, val, code);
  }
  template <typename Index_, typename Val_>
  inline Key findKey(const Index_ &index, const Val_ &val) const {
    Node *node = find(index, val);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_, typename Val_>
  inline Val findVal(const Index_ &index, const Val_ &val) const {
    Node *node = find(index, val);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Index_, typename Val_>
  inline NodeRef findKeyVal_(
      const Index_ &index, const Val_ &val, uint32_t code) const {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    for (node = m_table[slot];
	 node && (!ICmp::equals(node->Node::key(), index) ||
		  !ValCmp::equals(node->Node::val(), val));
	 node = node->Fn::next());

    return node;
  }

public:
  template <typename Key__>
  inline NodeRef findAdd(Key__ &&key) {
    uint32_t code = HashFn::hash(key);
    Guard guard(this->lockCode(code));

    return findAdd_(ZuFwd<Key__>(key), Val(), code);
  }
  template <typename Key__, typename Val_>
  inline NodeRef findAdd(Key__ &&key, Val_ &&val) {
    uint32_t code = HashFn::hash(key);
    Guard guard(this->lockCode(code));

    return findAdd_(ZuFwd<Key__>(key), ZuFwd<Val_>(val), code);
  }
  template <typename Key__>
  inline Key findAddKey(Key__ &&key) {
    NodeRef node = findAdd(ZuFwd<Key__>(key), Val());
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Key__, typename Val_>
  inline Key findAddKey(Key__ &&key, Val_ &&val) {
    NodeRef node = findAdd(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Key__, typename Val_>
  inline Val findAddVal(Key__ &&key, Val_ &&val) {
    NodeRef node = findAdd(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }
private:
  template <typename Key__, typename Val_>
  inline NodeRef findAdd_(Key__ &&key, Val_ &&val, uint32_t code) {
    Node *node;
    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    for (node = m_table[slot];
	 node && !Cmp::equals(node->Node::key(), key);
	 node = node->Fn::next());

    if (node) return node;

    {
      NodeRef node = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
      addNode_(node, code);
      return node;
    }
  }

public:
  template <typename Index__>
  inline typename ZuNotConvertible<Index__, NodeRef, NodeRef>::T
      del(const Index__ &index) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(this->lockCode(code));
    return del_(index, code);
  }
  template <typename Index_>
  inline Key delKey(const Index_ &index) {
    NodeRef node = del(index);
    if (ZuUnlikely(!node)) return Cmp::null();
    Key key = ZuMv(node->Node::key());
    nodeDelete(node);
    return key;
  }
  template <typename Index_>
  inline Val delVal(const Index_ &index) {
    NodeRef node = del(index);
    if (ZuUnlikely(!node)) return ValCmp::null();
    Val val = ZuMv(node->Node::val());
    nodeDelete(node);
    return val;
  }
private:
  template <typename Index_>
  inline NodeRef del_(const Index_ &index, uint32_t code) {
    if (!m_count) return 0;

    Node *node, *prevNode = 0;
    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    for (node = m_table[slot];
	 node && !ICmp::equals(node->Node::key(), index);
	 prevNode = node, node = node->Fn::next());

    if (!node) return 0;

    if (!prevNode)
      m_table[slot] = node->Fn::next();
    else
      prevNode->Fn::next(node->Fn::next());

    NodeRef ret = node;

    nodeDeref(node);
    --m_count;

    return ret;
  }

public:
  template <typename Index_, typename Val_>
  inline NodeRef del(const Index_ &index, const Val_ &val) {
    uint32_t code = IHashFn::hash(index);
    Guard guard(this->lockCode(code));
    return delKeyVal_(index, val, code);
  }
private:
  template <typename Index_, typename Val_>
  inline NodeRef delKeyVal_(
      const Index_ &index, const Val_ &val, uint32_t code) {
    if (!m_count) return 0;

    Node *node, *prevNode = 0;
    unsigned slot = ZmHash_Bits::hashBits(code, this->bits());

    for (node = m_table[slot];
	 node && (!ICmp::equals(node->Node::key(), index) ||
		  !ValCmp::equals(node->Node::val(), val));
	 prevNode = node, node = node->Fn::next());

    if (!node) return 0;

    if (!prevNode)
      m_table[slot] = node->Fn::next();
    else
      prevNode->Fn::next(node->Fn::next());

    NodeRef ret = node;

    nodeDeref(node);
    --m_count;

    return ret;
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
  inline void startIterate(I &iterator) {
    iterator.lock(this->lockSlot(0));
    iterator.m_slot = 0;
    iterator.m_node = 0;
    iterator.m_prev = 0;
  }
  template <typename I>
  inline void startIndexIterate(I &iterator) {
    uint32_t code = IHashFn::hash(iterator.m_index);

    iterator.lock(this->lockCode(code));
    iterator.m_slot = ZmHash_Bits::hashBits(code, this->bits());
    iterator.m_node = 0;
    iterator.m_prev = 0;
  }
  template <typename I>
  inline NodeRef iterate(I &iterator) {
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
	iterator.unlock(this->lockSlot(slot));
	if (++slot >= (1<<this->bits())) {
	  iterator.m_slot = -1;
	  return 0;
	}
	iterator.lock(this->lockSlot(slot));
	iterator.m_slot = slot;
      } while (!(node = m_table[slot]));
    }

    iterator.m_prev = prevNode;
    return iterator.m_node = node;
  }
  template <typename I>
  inline NodeRef indexIterate(I &iterator) {
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
      iterator.unlock(this->lockSlot(slot));
      iterator.m_slot = -1;
      return 0;
    }

    iterator.m_prev = prevNode;
    return iterator.m_node = node;
  }
  template <typename I>
  void endIterate(I &iterator) {
    if (iterator.m_slot < 0) return;

    iterator.unlock(this->lockSlot(iterator.m_slot));
  }
  template <typename I>
  void delIterate(I &iterator) {
    Node *node = iterator.m_node, *prevNode = iterator.m_prev;

    if (!m_count || !node) return;

    if (!prevNode) {
      m_table[iterator.m_slot] = node->Fn::next();
      iterator.m_node = 0;
    } else
      prevNode->Fn::next(node->Fn::next());

    nodeDeref(node);
    nodeDelete(node);
    --m_count;
  }

public:
  void clean() {
    this->lockAll();

    for (unsigned i = 0, n = (1U<<this->bits()); i < n; i++) {
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

    this->unlockAll();
  }

  template <typename Index_>
  inline Lock &lock(Index_ &&index) {
    return lockCode(IHashFn::hash(ZuFwd<Index_>(index)));
  }

  void report(ZmHashStats &s) {
    s.id = ID::id();
    s.linear = false;
    s.nodeSize = sizeof(Node);
    s.bits = this->bits();
    s.loadFactor = ((double)m_loadFactor) / 16.0;
    s.cBits = this->cBits();
    s.count = m_count; // deliberately unsafe
    s.effLoadFactor = ((double)s.count) / ((double)(1<<s.bits));
    s.resized = m_resized; // deliberately unsafe
  }

private:
  void resize(unsigned bits) {
    if (this->lockAllResize(bits)) return;

    ++m_resized;

    unsigned i, j, n = (1U<<bits);

    this->bits(++bits);

    NodePtr *table = new NodePtr[1<<bits];
    memset(table, 0, sizeof(NodePtr)<<bits);
    Node *node, *nextNode;

    for (i = 0; i < n; i++) {
      for (node = m_table[i];
	   node;
	   node = nextNode) {
	nextNode = node->Fn::next();
	j = ZmHash_Bits::hashBits(HashFn::hash(node->Node::key()), bits);
	node->Fn::next(table[j]);
	table[j] = node;
      }
    }
    delete [] m_table;
    m_table = table;

    this->unlockAll();
  }

  unsigned	m_loadFactor;
  unsigned	m_count;
  unsigned	m_resized;
  NodePtr	*m_table;
};

#endif /* ZmHash_HPP */
