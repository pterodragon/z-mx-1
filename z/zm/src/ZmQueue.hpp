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
// ZmQueue<ZtString,				// keys are ZtStrings
//   ZmQueueBase<ZmObject,			// base of ZmObject
//     ZmQueueValCmp<ZtICmp> > >		// case-insensitive comparison

// NTP defaults
struct ZmQueue_Defaults {
  typedef uint64_t ID;
  template <typename T> struct CmpT { typedef ZuCmp<T> Cmp; };
  template <typename T> struct ICmpT { typedef ZuCmp<T> ICmp; };
  template <typename T> struct HashFnT { typedef ZuHash<T> HashFn; };
  template <typename T> struct IHashFnT { typedef ZuHash<T> IHashFn; };
  template <typename T> struct IndexT { typedef T Index; };
  typedef ZmLock Lock;
  struct HeapID { inline static const char *id() { return "ZmQueue"; } };
  struct Base { };
};

// ZmQueueID - define internal queue ID type (default: uint64_t)
template <typename ID_, class NTP = ZmQueue_Defaults>
struct ZmQueueID : public NTP {
  ZuAssert(ZuTraits<ID_>::IsPrimitive &&
	   ZuTraits<ID_>::IsIntegral &&
	   !ZuTraits<ID_>::IsSigned);
  typedef ID_ ID;
};

// ZmQueueIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
template <class Accessor, class NTP = ZmQueue_Defaults>
struct ZmQueueIndex : public NTP {
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

// ZmQueueCmp - the key comparator
template <class Cmp_, class NTP = ZmQueue_Defaults>
struct ZmQueueCmp : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
  template <typename> struct ICmpT { typedef Cmp_ ICmp; };
};

// ZmQueueICmp - the index comparator
template <class ICmp_, class NTP = ZmQueue_Defaults>
struct ZmQueueICmp : public NTP {
  template <typename> struct ICmpT { typedef ICmp_ ICmp; };
};

// ZmQueueHashFn - the hash function
template <class HashFn_, class NTP = ZmQueue_Defaults>
struct ZmQueueHashFn : public NTP {
  template <typename> struct HashFnT { typedef HashFn_ HashFn; };
};

// ZmQueueIHashFn - the index hash function
template <class IHashFn_, class NTP = ZmQueue_Defaults>
struct ZmQueueIHashFn : public NTP {
  template <typename> struct IHashFnT { typedef IHashFn_ IHashFn; };
};

// ZmQueueLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmQueue_Defaults>
struct ZmQueueLock : public NTP {
  typedef Lock_ Lock;
};

// ZmQueueHeapID - the heap ID
template <class HeapID_, class NTP = ZmQueue_Defaults>
struct ZmQueueHeapID : public NTP {
  typedef HeapID_ HeapID;
};

// ZmQueueBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmQueue_Defaults>
struct ZmQueueBase : public NTP {
  typedef Base_ Base;
};

template <typename Key, class NTP = ZmQueue_Defaults>
class ZmQueue : public NTP::Base {
  ZmQueue(const ZmQueue &);
  ZmQueue &operator =(const ZmQueue &);	// prevent mis-use

public:
  typedef typename NTP::ID ID;
  typedef typename NTP::template CmpT<Key>::Cmp Cmp;
  typedef typename NTP::template ICmpT<Key>::ICmp ICmp;
  typedef typename NTP::template HashFnT<Key>::HashFn HashFn;
  typedef typename NTP::template IHashFnT<Key>::IHashFn IHashFn;
  typedef typename NTP::template IndexT<Key>::Index Index;
  typedef typename NTP::Lock Lock;
  typedef typename NTP::HeapID HeapID;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  ZuAssert(ZuTraits<ID>::IsPrimitive && ZuTraits<ID>::IsIntegral);

  typedef ZmHash<Key,
	    ZmHashCmp_<Cmp,
	      ZmHashICmp<ICmp,
		ZmHashFn<HashFn,
		  ZmHashIFn<IHashFn,
		    ZmHashIndex_<Index,
		      ZmHashVal<ID,
			ZmHashLock<ZmNoLock,
			  ZmHashHeapID<HeapID> > > > > > > > > Key2ID;
  typedef ZmHash<ID,
	    ZmHashVal<Key,
	      ZmHashValCmp<Cmp,
		ZmHashLock<ZmNoLock,
		  ZmHashHeapID<HeapID> > > > > ID2Key;

  inline ZmQueue() : m_head(0), m_tail(0) { }

  template <typename ...Args>
  inline ZmQueue(ID initialID, const ZmHashParams &params, Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      m_head(initialID), m_tail(initialID) {
    m_key2id = new Key2ID(params);
    m_id2key = new ID2Key(params);
  }

  virtual ~ZmQueue() { }

  class Iterator;
friend class Iterator;
  class Iterator : private Guard {
    Iterator(const Iterator &);
    Iterator &operator =(const Iterator &);	// prevent mis-use

    typedef ZmQueue<Key, NTP> Queue;

  public:
    inline Iterator(Queue &queue) :
	Guard(queue.m_lock), m_queue(queue), m_id(queue.m_head) { }

    inline void reset() { m_id = m_queue.m_head; }

    inline const Key &iterate() {
      if (!m_queue.m_key2id->count_() || m_id == m_queue.m_tail)
	return Cmp::null();
      typename ID2Key::NodeRef node;
      do {
	node = m_queue.m_id2key->find(m_id++);
      } while (!node && m_id != m_queue.m_tail);
      if (!node) return Cmp::null();
      return node->val();
    }

    inline void del() {
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

    inline ID id() const { return m_id; }

  private:
    Queue		&m_queue;
    ID			m_id;
  };

  inline void push(const Key &key, bool possDup = true) {
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
  inline void push(ID id, const Key &key, bool possDup) {
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

  inline Key shift() {
    Guard guard(m_lock);
    if (!m_key2id->count_() || m_head == m_tail) return Cmp::null();
    typename ID2Key::NodeRef node;
    do { node = m_id2key->del(m_head++); } while (!node && m_head != m_tail);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    return ZuMv(node->val());
  }
  inline Key shift(ID &id) {
    Guard guard(m_lock);
    if (!m_key2id->count_() || m_head == m_tail) return Cmp::null();
    typename ID2Key::NodeRef node;
    do { node = m_id2key->del(m_head++); } while (!node && m_head != m_tail);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    id = node->key();
    return ZuMv(node->val());
  }

  inline void unshift(const Key &key) {
    Guard guard(m_lock);
    ID id = --m_head;
    m_key2id->add(key, id);
    m_id2key->add(id, key);
  }

  inline bool find(const Key &key, ID &id) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->find(key);
    if (!node) return false;
    id = node->val();
    return true;
  }
  inline const Key &findID(ID id) {
    Guard guard(m_lock);
    typename ID2Key::NodeRef node = m_id2key->find(id);
    if (!node) return Cmp::null();
    return node->val();
  }

  inline bool del(const Key &key) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->del(key);
    if (!node) return false;
    ID id = node->val();
    if (m_id2key->del(id)) del_(id);
    return true;
  }
  inline bool del(const Key &key, ID &id) {
    Guard guard(m_lock);
    typename Key2ID::NodeRef node = m_key2id->del(key);
    if (!node) return false;
    id = node->val();
    if (m_id2key->del(id)) del_(id);
    return true;
  }
  inline Key delID(ID id) {
    Guard guard(m_lock);
    typename ID2Key::NodeRef node = m_id2key->del(id);
    if (!node) return Cmp::null();
    m_key2id->del(node->val());
    del_(id);
    return ZuMv(node->val());
  }

private:
  inline void del_(ID id) {
    if (id == m_tail - 1) {
      if (id != m_head - 1) m_tail = id;
    } else if (id == m_head && id != m_tail) {
      m_head = id + 1;
    }
  }

public:
  inline void clean(ID initialID = 0) {
    Guard guard(m_lock);
    m_head = m_tail = initialID;
    m_key2id->clean();
    m_id2key->clean();
  }

  inline unsigned count() const { return m_key2id->count_(); }

  inline ID head() const { return m_head; }
  inline ID tail() const { return m_tail; }

private:
  Lock			m_lock;
  ID			m_head;
  ID			m_tail;
  ZmRef<Key2ID>		m_key2id;
  ZmRef<ID2Key>		m_id2key;
};

#endif /* ZmQueue_HPP */
