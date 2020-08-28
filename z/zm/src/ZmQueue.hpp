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

// queue with fast indexed on-queue find and delete

#ifndef ZmQueue_HPP
#define ZmQueue_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuAssert.hpp>
#include <zlib/ZuTraits.hpp>

#include <zlib/ZmHash.hpp>

// NTP (named template parameters):
//
// ZmQueue<ZtString,			// keys are ZtStrings
//   ZmQueueValCmp<ZtICmp> >		// case-insensitive comparison

// NTP defaults
struct ZmQueue_Defaults {
  using ID = uint64_t;
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct HashFnT { using HashFn = ZuHash<T>; };
  template <typename T> struct IHashFnT { using IHashFn = ZuHash<T>; };
  template <typename T> struct IndexT { using Index = T; };
  using Lock = ZmLock;
  struct HeapID { static constexpr const char *id() { return "ZmQueue"; } };
};

// ZmQueueID - define internal queue ID type (default: uint64_t)
template <typename ID_, class NTP = ZmQueue_Defaults>
struct ZmQueueID : public NTP {
  ZuAssert(ZuTraits<ID_>::IsPrimitive &&
	   ZuTraits<ID_>::IsIntegral &&
	   !ZuTraits<ID_>::IsSigned);
  using ID = ID_;
};

// ZmQueueIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
template <class Accessor, class NTP = ZmQueue_Defaults>
struct ZmQueueIndex : public NTP {
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

// ZmQueueCmp - the key comparator
template <class Cmp_, class NTP = ZmQueue_Defaults>
struct ZmQueueCmp : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
  template <typename> struct ICmpT { using ICmp = Cmp_; };
};

// ZmQueueICmp - the index comparator
template <class ICmp_, class NTP = ZmQueue_Defaults>
struct ZmQueueICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

// ZmQueueHashFn - the hash function
template <class HashFn_, class NTP = ZmQueue_Defaults>
struct ZmQueueHashFn : public NTP {
  template <typename> struct HashFnT { using HashFn = HashFn_; };
};

// ZmQueueIHashFn - the index hash function
template <class IHashFn_, class NTP = ZmQueue_Defaults>
struct ZmQueueIHashFn : public NTP {
  template <typename> struct IHashFnT { using IHashFn = IHashFn_; };
};

// ZmQueueLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmQueue_Defaults>
struct ZmQueueLock : public NTP {
  using Lock = Lock_;
};

// ZmQueueHeapID - the heap ID
template <class HeapID_, class NTP = ZmQueue_Defaults>
struct ZmQueueHeapID : public NTP {
  using HeapID = HeapID_;
};

template <typename Key, class NTP = ZmQueue_Defaults>
class ZmQueue {
  ZmQueue(const ZmQueue &);
  ZmQueue &operator =(const ZmQueue &);	// prevent mis-use

public:
  using ID = typename NTP::ID;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using ICmp = typename NTP::template ICmpT<Key>::ICmp;
  using HashFn = typename NTP::template HashFnT<Key>::HashFn;
  using IHashFn = typename NTP::template IHashFnT<Key>::IHashFn;
  using Index = typename NTP::template IndexT<Key>::Index;
  using Lock = typename NTP::Lock;
  using HeapID = typename NTP::HeapID;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  ZuAssert(ZuTraits<ID>::IsPrimitive && ZuTraits<ID>::IsIntegral);

  using Key2ID =
    ZmHash<Key,
      ZmHashCmp_<Cmp,
	ZmHashICmp<ICmp,
	  ZmHashFn<HashFn,
	    ZmHashIFn<IHashFn,
	      ZmHashIndex_<Index,
		ZmHashVal<ID,
		  ZmHashLock<ZmNoLock,
		    ZmHashHeapID<HeapID> > > > > > > > >;
  using ID2Key =
    ZmHash<ID,
      ZmHashVal<Key,
	ZmHashValCmp<Cmp,
	  ZmHashLock<ZmNoLock,
	    ZmHashHeapID<HeapID> > > > >;

  ZmQueue() : m_head(0), m_tail(0) { }

  template <typename ...Args>
  ZmQueue(ID initialID, const ZmHashParams &params, Args &&... args) :
      m_head(initialID), m_tail(initialID) {
    m_key2id = new Key2ID(params);
    m_id2key = new ID2Key(params);
  }

  virtual ~ZmQueue() { }

  class Iterator;
friend Iterator;
  class Iterator : private Guard {
    Iterator(const Iterator &);
    Iterator &operator =(const Iterator &);	// prevent mis-use

    using Queue = ZmQueue<Key, NTP>;

  public:
    Iterator(Queue &queue) :
	Guard(queue.m_lock), m_queue(queue), m_id(queue.m_head) { }

    void reset() { m_id = m_queue.m_head; }

    const Key &iterate() {
      if (!m_queue.m_key2id->count_() || m_id == m_queue.m_tail)
	return Cmp::null();
      typename ID2Key::NodeRef node;
      do {
	node = m_queue.m_id2key->find(m_id++);
      } while (!node && m_id != m_queue.m_tail);
      if (!node) return Cmp::null();
      return node->val();
    }

    void del() {
      ID prevID = m_id - 1;
      typename ID2Key::NodeRef node = m_queue.m_id2key->del(prevID);
      if (!node) return;
      m_queue.m_key2id->del(node->val());
      if (prevID == m_queue.m_tail - 1) {
	if (prevID != m_queue.m_head - 1) m_id = m_queue.m_tail = prevID;
      } else if (prevID == m_queue.m_head && prevID != m_queue.m_tail) {
	m_queue.m_head = m_id;
      }
    }

    ID id() const { return m_id; }

  private:
    Queue		&m_queue;
    ID			m_id;
  };

  void push(const Key &key, bool possDup = true) {
    Guard guard(m_lock);
    ID id = m_tail++;
    if (possDup) {
      typename Key2ID::NodeRef node = m_key2id->del(key);
      if (node) {
	ID id = node->val();
	if (m_id2key->del(id)) del_(id);
      }
    }
    m_key2id->add(key, id);
    m_id2key->add(id, key);
  }
  void push(ID id, const Key &key, bool possDup) {
    Guard guard(m_lock);
    if (m_tail <= id) m_tail = id + 1;
    if (m_head > id) m_head = id;
    if (possDup) {
      typename Key2ID::NodeRef node = m_key2id->del(key);
      if (node) {
	ID id_ = node->val();
	if (id_ != id && m_id2key->del(id_)) del_(id_);
      }
    }
    m_key2id->add(key, id);
    if (possDup) m_id2key->del(id);
    m_id2key->add(id, key);
  }

  Key shift() {
    Guard guard(m_lock);
    if (!m_key2id->count_() || m_head == m_tail) return Cmp::null();
    typename ID2Key::NodeRef node;
    do { node = m_id2key->del(m_head++); } while (!node && m_head != m_tail);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    return ZuMv(node->val());
  }
  Key shift(ID &id) {
    Guard guard(m_lock);
    if (!m_key2id->count_() || m_head == m_tail) return Cmp::null();
    typename ID2Key::NodeRef node;
    do { node = m_id2key->del(m_head++); } while (!node && m_head != m_tail);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    id = node->key();
    return ZuMv(node->val());
  }

  void unshift(const Key &key) {
    Guard guard(m_lock);
    ID id = --m_head;
    m_key2id->add(key, id);
    m_id2key->add(id, key);
  }

  bool find(const Key &key, ID &id) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->find(key);
    if (!node) return false;
    id = node->val();
    return true;
  }
  const Key &findID(ID id) {
    Guard guard(m_lock);
    typename ID2Key::NodeRef node = m_id2key->find(id);
    if (!node) return Cmp::null();
    return node->val();
  }

  bool del(const Key &key) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->del(key);
    if (!node) return false;
    ID id = node->val();
    if (m_id2key->del(id)) del_(id);
    return true;
  }
  bool del(const Key &key, ID &id) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->del(key);
    if (!node) return false;
    id = node->val();
    if (m_id2key->del(id)) del_(id);
    return true;
  }
  Key delID(ID id) {
    Guard guard(m_lock);
    typename ID2Key::NodeRef node = m_id2key->del(id);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    del_(id);
    return ZuMv(node->val());
  }

private:
  void del_(ID id) {
    if (id == m_tail - 1) {
      if (id != m_head - 1) m_tail = id;
    } else if (id == m_head && id != m_tail) {
      m_head = id + 1;
    }
  }

public:
  void clean(ID initialID = 0) {
    Guard guard(m_lock);
    m_head = m_tail = initialID;
    m_key2id->clean();
    m_id2key->clean();
  }

  unsigned count() const { return m_key2id->count_(); }

  ID head() const { return m_head; }
  ID tail() const { return m_tail; }

private:
  Lock			m_lock;
  ID			m_head;
  ID			m_tail;
  ZmRef<Key2ID>		m_key2id;
  ZmRef<ID2Key>		m_id2key;
};

#endif /* ZmQueue_HPP */
