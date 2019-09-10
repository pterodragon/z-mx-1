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

// priority queue (optimized for message sequence reassembly)

// internal data structure is a deterministic skip list. The list is a
// sequence of ordered items. Each item contains one or more adjacent
// elements, each of which is individually numbered. This run-length
// encoding allows highly efficient duplicate detection, enqueuing of
// both in- and out- of order items as well as in-order deletion (dequeuing)
// without re-balancing.
//
// if used for sequences of packets with bytecount sequence numbering,
// the elements are chars, the items are packet buffers, and the key
// is the bytecount; other possibilities are elements being individual
// messages within a containing packet as in FIX, OUCH, ITCH and similar
// protocols

#ifndef ZmPQueue_HPP
#define ZmPQueue_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuIndex.hpp>
#include <ZuPair.hpp>
#include <ZuBox.hpp>

#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmHeap.hpp>
#include <ZmINode.hpp>

// the application will normally substitute ZmPQueueDefaultFn with
// a type that is specific to the queued Item; it must conform to the
// specified interface

// Key - the type of the key (the sequence number)
// key() - obtain the key
// length() - obtain the length (key count)

// E.g. for packets/fragments using a bytecount (TCP, etc.)
// the value would be a buffer large enough to hold one packet/fragment
// Key would be uint32_t/uint64_t, key() would return the key in the header
// length() would return the number of bytes in the packet/fragment

template <typename Item> class ZmPQueueDefaultFn {
public:
  // Key is the type of the key (sequence number)
  typedef uint32_t Key;

  ZuInline ZmPQueueDefaultFn(Item &item) : m_item(item) { }

  // key() returns the key for the first element in the item
  ZuInline Key key() const { return m_item.key(); }

  // length() returns the length of the item in units
  ZuInline unsigned length() const { return m_item.length(); }

  // clipHead()/clipTail() remove elements from the item's head or tail
  // clipHead()/clipTail() can just return 1 if item length is always 1,
  // or do nothing and return the unchanged length if items are guaranteed
  // never to overlap; these functions return the length remaining in the item
  ZuInline unsigned clipHead(unsigned n) { return m_item.clipHead(n); }
  ZuInline unsigned clipTail(unsigned n) { return m_item.clipTail(n); }

  // write() overwrites overlapping data from item
  ZuInline void write(const ZmPQueueDefaultFn &item) {
    m_item.write(item.m_item);
  }

  // bytes() returns the size in bytes of the item (for queue statistics)
  ZuInline unsigned bytes() const { return m_item.bytes(); }

private:
  Item	&m_item;
};

// uses NTP (named template parameters)

struct ZmPQueue_Defaults {
  enum { NodeIsItem = 0 };
  typedef ZmLock Lock;
  typedef ZmObject Object;
  struct HeapID {
    inline static const char *id() { return "ZmPQueue"; }
  };
  struct Base { };
  enum { Bits = 4, Levels = 4 };
  template <typename Item> struct ZmPQueueFnT {
    typedef ZmPQueueDefaultFn<Item> T;
  };
};

// ZmPQueueNodeIsItem - derive ZmPQueue::Node from Item instead of containing it
template <bool NodeIsItem_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueNodeIsItem : public NTP {
  enum { NodeIsItem = NodeIsItem_ };
};

// ZmPQueueLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueLock : public NTP {
  typedef Lock_ Lock;
};

// ZmPQueueObject - the reference-counted object type used
template <class Object_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueObject : public NTP {
  typedef Object_ Object;
};

// ZmPQueueHeapID - the heap ID
template <class HeapID_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueHeapID : public NTP {
  typedef HeapID_ HeapID;
};

// ZmPQueueBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueBase : public NTP {
  typedef Base_ Base;
};

// ZmPQueueBits - change skip list factor (power of 2)
template <int Bits_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueBits : public NTP {
  enum { Bits = Bits_ };
};

// ZmPQueueLevels - change skip list #levels
template <int Levels_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueLevels : public NTP {
  enum { Levels = Levels_ };
};

// ZmPQueueFn - override queuing functions for the type
template <typename Fn_, class NTP = ZmPQueue_Defaults>
struct ZmPQueueFn : public NTP {
  template <typename> struct ZmPQueueFnT { typedef Fn_ T; };
};

namespace ZmPQueue_ {
  template <int, int, typename = void> struct First;
  template <int Levels, typename T_>
  struct First<0, Levels, T_> { typedef T_ T; };
  template <int, int, typename T_ = void> struct Next { typedef T_ T; };
  template <int Levels, typename T_> struct Next<0, Levels, T_> { };
  template <int Levels, typename T_> struct Next<Levels, Levels, T_> { };
  template <int, int, typename T_ = void> struct NotLast { typedef T_ T; };
  template <int Levels, typename T_> struct NotLast<Levels, Levels, T_> { };
  template <int, int, typename = void> struct Last;
  template <int Levels, typename T_>
  struct Last<Levels, Levels, T_> { typedef void T; };
};

ZuTupleFields(ZmPQueue_Gap, 1, key, 2, length);

template <typename Item_, class NTP = ZmPQueue_Defaults>
class ZmPQueue : public ZuPrintable, public NTP::Base {
  ZmPQueue(const ZmPQueue &);
  ZmPQueue &operator =(const ZmPQueue &);	// prevent mis-use

public:
  typedef Item_ Item;
  typedef typename NTP::template ZmPQueueFnT<Item>::T Fn;
  typedef typename Fn::Key Key;
  enum { NodeIsItem = NTP::NodeIsItem };
  typedef typename NTP::Lock Lock;
  typedef typename NTP::Object Object;
  typedef typename NTP::HeapID HeapID;
  enum { Bits = NTP::Bits };
  enum { Levels = NTP::Levels };

  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  typedef ZmPQueue_Gap<ZuBox0(Key), ZuBox0(unsigned)> Gap;

  struct NullObject { }; // deconflict with ZuNull
  template <typename Node, typename Heap, bool NodeIsItem> class NodeFn_ :
      public ZuIf<NullObject, Object,
	ZuConversion<Object, ZuNull>::Is ||
	(NodeIsItem && ZuConversion<Object, Item>::Is)>::T,
      public Heap {
    NodeFn_(const NodeFn_ &);
    NodeFn_ &operator =(const NodeFn_ &); // prevent mis-use

  friend class ZmPQueue<Item, NTP>;

  protected:
    inline NodeFn_() {
      memset(m_next, 0, sizeof(Node *) * Levels);
      memset(m_prev, 0, sizeof(Node *) * Levels);
    }

  private:
    // access to Node instances is always guarded, so no need to protect
    // the returned object against concurrent deletion
    inline Node *next(unsigned i) { return m_next[i]; }
    inline Node *prev(unsigned i) { return m_prev[i]; }

    inline void next(unsigned i, Node *n) { m_next[i] = n; }
    inline void prev(unsigned i, Node *n) { m_prev[i] = n; }

    Node	*m_next[Levels];
    Node	*m_prev[Levels];
  };

  template <typename Heap>
  using Node_ = ZmINode<Heap, NodeIsItem, NodeFn_, Item>;
  struct NullHeap { }; // deconflict with ZuNull
  typedef ZmHeap<HeapID, sizeof(Node_<NullHeap>)> NodeHeap;
  typedef Node_<NodeHeap> Node;
  typedef typename Node::Fn NodeFn;

  typedef typename ZuIf<ZmRef<Node>, Node *, ZuIsObject_<Object>::OK>::T
    NodeRef;

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

public:
  template <typename ...Args>
  ZmPQueue(Key head, Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      m_headKey(head), m_tailKey(head) {
    memset(m_head, 0, sizeof(Node *) * Levels);
    memset(m_tail, 0, sizeof(Node *) * Levels);
  }
  ~ZmPQueue() { clean_(); }

private:
  // add at head
  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T addHead_(
      Node *node, unsigned addSeqNo) {
    Node *next;
    node->NodeFn::prev(0, 0);
    node->NodeFn::next(0, next = m_head[0]);
    m_head[0] = node;
    if (!next)
      m_tail[0] = node;
    else
      next->prev(0, node);
    addHead_<1>(node, addSeqNo);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T addHead_(
      Node *node, unsigned addSeqNo) {
    node->NodeFn::prev(Level, 0);
    if (ZuUnlikely(!(addSeqNo & ((1<<(Bits * Level)) - 1)))) {
      Node *next;
      node->NodeFn::next(Level, next = m_head[Level]);
      m_head[Level] = node;
      if (!next)
	m_tail[Level] = node;
      else
	next->prev(Level, node);
      addHead_<Level + 1>(node, addSeqNo);
      return;
    }
    node->NodeFn::next(Level, 0);
    addHeadEnd_<Level + 1>(node, addSeqNo);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T addHeadEnd_(
      Node *node, unsigned addSeqNo) {
    node->NodeFn::prev(Level, 0);
    node->NodeFn::next(Level, 0);
    addHeadEnd_<Level + 1>(node, addSeqNo);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T addHead_(
      Node *, unsigned) { }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T addHeadEnd_(
      Node *, unsigned) { }

  // insert before result from find
  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T add_(
      Node *node, Node **next_, unsigned addSeqNo) {
    Node *next = next_[0];
    Node *prev = next ? next->prev(0) : m_tail[0];
    node->NodeFn::next(0, next);
    if (ZuUnlikely(!next))
      m_tail[0] = node;
    else
      next->prev(0, node);
    node->NodeFn::prev(0, prev);
    if (ZuUnlikely(!prev))
      m_head[0] = node;
    else
      prev->next(0, node);
    add_<1>(node, next_, addSeqNo);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T add_(
      Node *node, Node **next_, unsigned addSeqNo) {
    if (ZuUnlikely(!(addSeqNo & ((1<<(Bits * Level)) - 1)))) {
      Node *next = next_[Level];
      Node *prev = next ? next->prev(Level) : m_tail[Level];
      node->NodeFn::next(Level, next);
      if (ZuUnlikely(!next))
	m_tail[Level] = node;
      else
	next->prev(Level, node);
      node->NodeFn::prev(Level, prev);
      if (ZuUnlikely(!prev))
	m_head[Level] = node;
      else
	prev->next(Level, node);
      add_<Level + 1>(node, next_, addSeqNo);
    } else {
      node->NodeFn::prev(Level, 0);
      node->NodeFn::next(Level, 0);
      addEnd_<Level + 1>(node, next_, addSeqNo);
    }
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T addEnd_(
      Node *node, Node **next_, unsigned addSeqNo) {
    node->NodeFn::prev(Level, 0);
    node->NodeFn::next(Level, 0);
    addEnd_<Level + 1>(node, next_, addSeqNo);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T add_(
      Node *, Node **, unsigned) { }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T addEnd_(
      Node *, Node **, unsigned) { }

  // delete head
  template <int Level>
  ZuInline void delHead__() {
    Node *next(m_head[Level]->next(Level));
    if (!(m_head[Level] = next))
      m_tail[Level] = 0;
    else
      next->prev(Level, 0);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T delHead_() {
    delHead_<1>();
    delHead__<0>();
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T delHead_() {
    if (m_head[Level] != m_head[0]) return;
    delHead_<Level + 1>();
    delHead__<Level>();
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T delHead_() { }

  // delete result from find
  template <int Level>
  ZuInline Node *del__(Node *node) {
    Node *next(node->NodeFn::next(Level));
    Node *prev(node->NodeFn::prev(Level));
    if (ZuUnlikely(!prev))
      m_head[Level] = next;
    else
      prev->next(Level, next);
    if (ZuUnlikely(!next))
      m_tail[Level] = prev;
    else
      next->prev(Level, prev);
    return next;
  }
  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T del_(Node **next) {
    del_<1>(next);
    next[0] = del__<Level>(next[0]);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T del_(Node **next) {
    if (next[Level] != next[0]) return;
    del_<Level + 1>(next);
    next[Level] = del__<Level>(next[Level]);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T del_(Node **) { }

  // find
  ZuInline bool findDir_(Key key) const {
    if (key < m_headKey) return true;
    if (key >= m_tailKey) return false;
    return key - m_headKey <= m_tailKey - key;
  }
  ZuInline bool findDir_(Key key, Node *prev, Node *next) const {
    if (!prev) return true;
    if (!next) return false;
    return
      key - Fn(prev->Node::item()).key() <=
      Fn(next->Node::item()).key() - key;
  }
  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T findFwd_(
      Key key, Node **next) const {
    next[Levels - 1] = m_head[Levels - 1];
    findFwd__<Level>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T findFwd_(
      Key key, Node **next) const {
    Node *node;
    if (!(node = next[Levels - Level]) ||
	!(node = node->NodeFn::prev(Levels - Level)))
      next[Levels - Level - 1] = m_head[Levels - Level - 1];
    else
      next[Levels - Level - 1] = node;
    findFwd__<Level>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T findFwd_(
      Key , Node **) const { }

  template <int Level>
  ZuInline typename ZmPQueue_::First<Level, Levels>::T findRev_(
      Key key, Node **next) const {
    next[Levels - 1] = m_tail[Levels - 1];
    findRev__<Level>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Next<Level, Levels>::T findRev_(
      Key key, Node **next) const {
    Node *node;
    if (!(node = next[Levels - Level]))
      next[Levels - Level - 1] = m_tail[Levels - Level - 1];
    else
      next[Levels - Level - 1] = node;
    findRev__<Level>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T findRev_(
      Key , Node **) const { }

  template <int Level>
  ZuInline typename ZmPQueue_::NotLast<Level, Levels>::T findFwd__(
      Key key, Node **next) const {
    Node *node = next[Levels - Level - 1];
    if (node) {
      do {
	Fn item(node->Node::item());
	if (item.key() == key) goto found;
	if (item.key() > key) goto passed;
      } while (node = node->NodeFn::next(Levels - Level - 1));
      next[Levels - Level - 1] = node;
    }
    findRev_<Level + 1>(key, next);
    return;
  found:
    found_<Level>(node, next);
    return;
  passed:
    Node *prev = node->NodeFn::prev(Levels - Level - 1);
    next[Levels - Level - 1] = node;
    if (findDir_(key, prev, node))
      findFwd_<Level + 1>(key, next);
    else
      findRev_<Level + 1>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T findFwd__(
      Key key, Node **next) const {
    Node *node = next[0];
    if (node) {
      do {
	Fn item(node->Node::item());
	if (item.key() >= key) break;
      } while (node = node->NodeFn::next(0));
      next[0] = node;
    }
  }
  template <int Level>
  ZuInline typename ZmPQueue_::NotLast<Level, Levels>::T findRev__(
      Key key, Node **next) const {
    Node *node = next[Levels - Level - 1];
    if (node) {
      do {
	Fn item(node->Node::item());
	if (item.key() == key) goto found;
	if (item.key() < key) goto passed;
      } while (node = node->NodeFn::prev(Levels - Level - 1));
    }
    next[Levels - Level - 1] = m_head[Levels - Level - 1];
    findFwd_<Level + 1>(key, next);
    return;
  found:
    found_<Level>(node, next);
    return;
  passed:
    Node *prev = node;
    next[Levels - Level - 1] = node = node->NodeFn::next(Levels - Level - 1);
    if (findDir_(key, prev, node))
      findFwd_<Level + 1>(key, next);
    else
      findRev_<Level + 1>(key, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T findRev__(
      Key key, Node **next) const {
    Node *node = next[0];
    if (node) {
      do {
	Fn item(node->Node::item());
	if (item.key() == key) { next[0] = node; return; }
	if (item.key() < key) { next[0] = node->NodeFn::next(0); return; }
      } while (node = node->NodeFn::prev(0));
    }
    next[0] = m_head[0];
  }

  template <int Level>
  ZuInline typename ZmPQueue_::NotLast<Level, Levels>::T found_(
      Node *node, Node **next) const {
    next[Levels - Level - 1] = node;
    found_<Level + 1>(node, next);
  }
  template <int Level>
  ZuInline typename ZmPQueue_::Last<Level, Levels>::T found_(
      Node *, Node **) const { }

  void find_(Key key, Node **next) const {
    if (findDir_(key))
      findFwd_<0>(key, next);
    else
      findRev_<0>(key, next);
  }

public:
  inline unsigned count() const { return m_count; }
  inline unsigned length() const { return m_length; }

  inline bool empty() const { return (!m_count); }

  inline void stats(
      uint64_t &inCount, uint64_t &inBytes, 
      uint64_t &outCount, uint64_t &outBytes) {
    inCount = m_inCount;
    inBytes = m_inBytes;
    outCount = m_outCount;
    outBytes = m_outBytes;
  }

  inline void reset(Key head) {
    Guard guard(m_lock);
    m_headKey = m_tailKey = head;
    clean_();
  }

  inline void skip() {
    Guard guard(m_lock);
    m_headKey = m_tailKey;
    clean_();
  }

  inline Key head() const {
    ReadGuard guard(m_lock);
    return m_headKey;
  }

  inline Key tail() const {
    ReadGuard guard(m_lock);
    return m_tailKey;
  }

  // returns the first gap that needs to be filled, or { 0, 0 } if none
  inline Gap gap() const {
    ReadGuard guard(m_lock);
    Node *node = m_head[0];
    Key tail = m_headKey;
    while (node) {
      Fn item(node->Node::item());
      Key key = item.key();
      if (key > tail) return Gap(tail, key - tail);
      Key end = key + item.length();
      if (end > tail) tail = end;
      node = node->NodeFn::next(0);
    }
    if (m_tailKey > tail) return Gap(tail, m_tailKey - tail);
    return Gap();
  };

private:
  inline void clipHead_(Key key) {
    while (Node *node = m_head[0]) {
      Fn item(node->Node::item());
      Key key_ = item.key();
      if (key_ >= key) return;
      Key end_ = key_ + item.length();
      if (end_ > key) {
	if (unsigned length = item.clipHead(key - key_)) {
	  m_length -= (end_ - key_) - length;
	  return;
	}
      }
      delHead_<0>();
      nodeDeref(node);
      nodeDelete(node);
      m_length -= end_ - key_;
      --m_count;
    }
  }

public:
  // override head key (sequence number); used to manually
  // advance past an unrecoverable gap or to rewind in order to
  // force re-processing of earlier items
  inline void head(Key key) {
    Guard guard(m_lock);
    if (key == m_headKey) return;
    if (key < m_headKey) { // a rewind implies a reset
      clean_();
      m_headKey = m_tailKey = key;
    } else {
      clipHead_(key);
      m_headKey = key;
      if (key > m_tailKey) m_tailKey = key;
    }
  }

  // bypass queue, update stats
  void bypass(unsigned bytes) {
    Guard guard(m_lock);
    ++m_inCount;
    m_inBytes += bytes;
    ++m_outCount;
    m_outBytes += bytes;
  }

  // immediately returns node if key == head (head is incremented)
  // returns 0 if key < head or key is already present in queue
  // returns 0 and enqueues node if key > head
  inline NodeRef rotate(NodeRef node) { return enqueue_<true>(ZuMv(node)); }

  // enqueues node
  inline void enqueue(NodeRef node) { enqueue_<false>(ZuMv(node)); }

  // unshift node onto head
  inline void unshift(NodeRef node) {
    Guard guard(m_lock);

    Fn item(node->Node::item());
    Key key = item.key();
    unsigned length = item.length();
    Key end = key + length;

    if (ZuUnlikely(key >= m_headKey)) return;

    if (ZuUnlikely(end > m_headKey)) { // clip tail
      length = item.clipTail(end - m_headKey);
      end = key + length;
    }

    if (ZuUnlikely(!length)) return;

    unsigned addSeqNo = m_addSeqNo++;

    Node *next[Levels];

    findFwd_<0>(key, next);

    Node *ptr;
    new (&ptr) NodeRef(ZuMv(node));
    add_<0>(ptr, next, addSeqNo);
    m_headKey = key;
    m_length += end - key;
    ++m_count;
  }

private:
  template <bool Dequeue>
  inline NodeRef enqueue_(NodeRef node) {
    Guard guard(m_lock);

    Fn item(node->Node::item());
    Key key = item.key();
    unsigned length = item.length();
    Key end = key + length;

    if (ZuUnlikely(end <= m_headKey)) return nullptr; // already processed

    if (ZuUnlikely(key < m_headKey)) { // clip head
      length = item.clipHead(m_headKey - key);
      key = end - length;
    }

    if (ZuUnlikely(!length)) { // zero-length heartbeats etc.
      if (end > m_tailKey) m_tailKey = end;
      return nullptr;
    }

    unsigned addSeqNo = m_addSeqNo++;

    unsigned bytes = item.bytes();

    if (ZuLikely(key == m_headKey)) { // usual case - in-order
      clipHead_(end); // remove overlapping data from queue

      return enqueue__<Dequeue>(ZuMv(node), end, length, bytes, addSeqNo);
    }

    ++m_inCount;
    m_inBytes += bytes;

    // find the item immediately following the key

    Node *next[Levels];

    find_(key, next);

    {
      Node *node_ = next[0];

      // process any item immediately following the key

      if (node_) {
	Fn item_(node_->Node::item());
	Key key_ = item_.key();
	Key end_ = key_ + item_.length();

	ZmAssert(key_ >= key);

	// if the following item spans the new item, overwrite it and return
	if (key_ == key && end_ >= end) {
	  item_.write(item);
	  return nullptr;
	}

	if (key_ == key)
	  node_ = nullptr;	// don't process the same item twice
	else
	  node_ = node_->NodeFn::prev(0);
      } else
	node_ = m_tail[0];

      // process any item immediately preceding the key

      if (node_) {
	Fn item_(node_->Node::item());
	Key key_ = item_.key();
	Key end_ = key_ + item_.length();

	ZmAssert(key_ < key);

	// if the preceding item spans the new item, overwrite it and return
	if (end_ >= end) {
	  item_.write(item);
	  return nullptr;
	}

	// if the preceding item partially overlaps the new item, clip it

	if (end_ > key) m_length -= (end_ - key_) - item_.clipTail(end_ - key);
      }
    }

    // remove all items that are completely overlapped by the new item

    while (Node *node_ = next[0]) {
      Fn item_(node_->Node::item());
      Key key_ = item_.key();
      Key end_ = key_ + item_.length();

      ZmAssert(key_ >= key);

      // item follows new item, finish search

      if (key_ >= end) break;

      // if the following item partially overlaps new item, clip it and finish

      if (end_ > end) {
	if (unsigned length = item_.clipHead(end - key_)) {
	  m_length -= (end_ - key_) - length;
	  break;
	}
      }

      // item is completely overlapped by new item, delete it

      del_<0>(next);
      nodeDeref(node_);
      nodeDelete(node_);
      m_length -= end_ - key_;
      --m_count;
    }

    // add new item into list before following item

    Node *ptr;
    new (&ptr) NodeRef(ZuMv(node));
    add_<0>(ptr, next, addSeqNo);
    if (end > m_tailKey) m_tailKey = end;
    m_length += end - key;
    ++m_count;

    return nullptr;
  }
  template <bool Dequeue>
  inline typename ZuIfT<Dequeue, NodeRef>::T enqueue__(NodeRef node,
      Key end, unsigned, unsigned bytes, unsigned) {
    m_headKey = end;
    if (end > m_tailKey) m_tailKey = end;
    ++m_inCount;
    m_inBytes += bytes;
    ++m_outCount;
    m_outBytes += bytes;
    return node;
  }
  template <bool Dequeue>
  inline typename ZuIfT<!Dequeue, NodeRef>::T enqueue__(NodeRef node,
      Key end, unsigned length, unsigned bytes, unsigned addSeqNo) {
    Node *ptr;
    new (&ptr) NodeRef(ZuMv(node));
    addHead_<0>(ptr, addSeqNo);
    if (end > m_tailKey) m_tailKey = end;
    m_length += length;
    ++m_count;
    ++m_inCount;
    m_inBytes += bytes;
    return 0;
  }

  inline NodeRef dequeue_() {
  loop:
    NodeRef node = m_head[0];
    if (!node) return 0;
    Fn item(node->Node::item());
    Key key = item.key();
    ZmAssert(key >= m_headKey);
    if (key != m_headKey) return 0;
    unsigned length = item.length();
    delHead_<0>();
    nodeDeref(node);
    m_length -= length;
    --m_count;
    if (!length) goto loop;
    Key end = key + length;
    m_headKey = end;
    ++m_outCount;
    m_outBytes += item.bytes();
    return node;
  }
public:
  inline NodeRef dequeue() {
    Guard guard(m_lock);
    return dequeue_();
  }
  // dequeues up to, but not including, item containing key
  inline NodeRef dequeue(Key key) {
    Guard guard(m_lock);
    if (m_headKey >= key) return 0;
    return dequeue_();
  }

  // shift, unlike dequeue, ignores gaps
private:
  inline NodeRef shift_() {
  loop:
    NodeRef node = m_head[0];
    if (!node) return 0;
    Fn item(node->Node::item());
    unsigned length = item.length();
    delHead_<0>();
    nodeDeref(node);
    m_length -= length;
    --m_count;
    if (!length) goto loop;
    Key end = item.key() + length;
    m_headKey = end;
    ++m_outCount;
    m_outBytes += item.bytes();
    return node;
  }
public:
  inline NodeRef shift() {
    Guard guard(m_lock);
    return shift_();
  }
  // shifts up to but not including item containing key
  inline NodeRef shift(Key key) {
    Guard guard(m_lock);
    if (m_headKey >= key) return 0;
    return shift_();
  }

  // aborts an item (leaving a gap in the queue)
  inline NodeRef abort(Key key) {
    Guard guard(m_lock);

    Node *next[Levels];

    find_(key, next);

    NodeRef node = next[0];

    if (!node) return 0;

    Fn item(node->Node::item());

    if (item.key() != key) return 0;

    del_<0>(next);
    nodeDeref(node);
    m_length -= item.length();
    --m_count;

    return node;
  }

  // find item containing key
  inline NodeRef find(Key key) const {
    ReadGuard guard(m_lock);

    Node *next[Levels];

    find_(key, next);

    NodeRef node;

    {
      Node *node_ = next[0];

      // process any item immediately following the key

      if (node_) {
	Fn item_(node_->Node::item());
	Key key_ = item_.key();

	ZmAssert(key_ >= key);

	if (ZuLikely(key_ == key)) return node = node_;

	node_ = node_->NodeFn::prev(0);
      } else
	node_ = m_tail[0];

      // process any item immediately preceding the key

      if (node_) {
	Fn item_(node_->Node::item());
	Key key_ = item_.key();
	Key end_ = key_ + item_.length();

	ZmAssert(key_ < key);

	if (ZuLikely(end_ > key)) return node = node_;
      }
    }

    return node;
  }

  void clean_() {
    while (Node *node = m_head[0]) {
      delHead_<0>();
      nodeDeref(node);
      nodeDelete(node);
    }
    m_length = 0;
    m_count = 0;
  }

public:
  template <typename S> void print(S &s) const {
    ReadGuard guard(m_lock);
    s << "head: " << m_headKey
      << "  tail: " << m_tailKey
      << "  length: " << m_length
      << "  count: " << m_count;
  }

private:
  Lock		m_lock;
  Key		  m_headKey;
  Node		  *m_head[Levels];
  Key		  m_tailKey;
  Node		  *m_tail[Levels];
  unsigned	  m_length = 0;
  unsigned	  m_count = 0;
  unsigned	  m_addSeqNo = 0;
  uint64_t	  m_inCount = 0;
  uint64_t	  m_inBytes = 0;
  uint64_t	  m_outCount = 0;
  uint64_t	  m_outBytes = 0;
};

// template resend-requesting receiver using ZmPQueue

// CRTP - application must conform to the following interface:
#if 0
typedef ZmPQueue<...> Queue;

struct App : public ZmPQRx<App, Queue> {
  typedef typename Queue::Node Msg;
  typedef typename Queue::Gap Gap;

  // access queue
  Queue &rxQueue();
 
  // process message
  void process(Msg *msg);

  // send resend request, as protocol requires it;
  // if the protocol is TCP based and now is a subset or equal to
  // prev, then a request may not need to be (re-)sent since
  // the previous (larger) request will still be outstanding
  // and may be relied upon to be fully satisfied; UDP based protocols
  // will need to send a resend request whenever this is called
  void request(const Gap &prev, const Gap &now);

  // re-send resend request, as protocol requires it
  void reRequest(const Gap &now);

  // schedule dequeue() to be called (possibly from different thread)
  void scheduleDequeue();
  // reschedule dequeue() recursively, from within dequeue() itself
  void rescheduleDequeue();
  // dequeue is idle (nothing left in queue to process)
  void idleDequeue();

  // schedule reRequest() to be called (possibly from different thread)
  // at a configured interval later
  void scheduleReRequest();
  // reschedule reRequest() recursively, from within reRequest() itself
  void rescheduleReRequest();
  // cancel any scheduled reRequest()
  void cancelReRequest();
};
#endif

template <class App, class Queue, class Lock_ = ZmNoLock>
class ZmPQRx : public ZuPrintable {
  enum {
    Queuing	= 0x01,
    Dequeuing	= 0x02
  };

public:
  template <typename S> static void printFlags(S &s, unsigned v) {
    static const char *flagNames[] = { "Queuing", "Dequeuing" };
    bool comma = false;
    for (unsigned i = 0; i < sizeof(flagNames) / sizeof(flagNames[0]); i++)
      if (v & (1U<<i)) {
	if (comma) s << ',';
	comma = true;
	s << flagNames[i];
      }
  }
  struct PrintFlags : public ZuPrintable {
    inline PrintFlags(unsigned v_) : v(v_) { }
    PrintFlags(const PrintFlags &) = default;
    PrintFlags &operator =(const PrintFlags &) = default;
    PrintFlags(PrintFlags &&) = default;
    PrintFlags &operator =(PrintFlags &&) = default;
    template <typename S> inline void print(S &s) const { printFlags(s, v); }
    unsigned v;
  };

  typedef typename Queue::Node Msg;
  typedef typename Queue::Key Key;
  typedef typename Queue::Gap Gap;

  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  // reset sequence number
  void rxReset(Key key) {
    App *app = static_cast<App *>(this);
    Guard guard(m_lock);
    app->cancelReRequest();
    m_flags &= ~(Queuing | Dequeuing);
    m_gap = Gap();
    app->rxQueue()->reset(key);
  }

  // start queueing (during snapshot recovery)
  void startQueuing() {
    Guard guard(m_lock);
    m_flags |= Queuing;
  }

  // stop queueing and begin processing messages from key onwards
  void stopQueuing(Key key) {
    App *app = static_cast<App *>(this);
    bool scheduleDequeue;
    {
      Guard guard(m_lock);
      m_flags &= ~Queuing;
      app->rxQueue()->head(key);
      scheduleDequeue = !(m_flags & Dequeuing) && app->rxQueue()->count();
      if (scheduleDequeue) m_flags |= Dequeuing;
    }
    if (scheduleDequeue) app->scheduleDequeue();
  }

  // handle a received message (possibly out of order)
  void received(ZmRef<Msg> msg) {
    App *app = static_cast<App *>(this);
    Guard guard(m_lock);
    if (ZuUnlikely(m_flags & (Queuing | Dequeuing))) {
      app->rxQueue()->enqueue(ZuMv(msg));
      return;
    }
    msg = app->rxQueue()->rotate(ZuMv(msg));
    bool scheduleDequeue = msg && app->rxQueue()->count();
    if (scheduleDequeue) m_flags |= Dequeuing;
    guard.unlock();
    if (ZuUnlikely(!msg)) { stalled(); return; }
    app->process(msg);
    if (scheduleDequeue) app->scheduleDequeue();
  }

  // dequeue a message - called via scheduleDequeue(), may reschedule itself
  void dequeue() {
    App *app = static_cast<App *>(this);
    Guard guard(m_lock);
    ZmRef<Msg> msg = app->rxQueue()->dequeue();
    bool scheduleDequeue = msg && app->rxQueue()->count();
    if (!scheduleDequeue) m_flags &= ~Dequeuing;
    guard.unlock();
    if (ZuUnlikely(!msg)) { stalled(); return; }
    app->process(msg);
    if (scheduleDequeue)
      app->rescheduleDequeue();
    else
      app->idleDequeue();
  }

  void reRequest() {
    App *app = static_cast<App *>(this);
    Gap gap;
    {
      Guard guard(m_lock);
      gap = m_gap;
    }
    app->cancelReRequest();
    if (!gap.length()) return;
    app->reRequest(gap);
    app->rescheduleReRequest();
  }

  ZuInline unsigned flags() const {
    ReadGuard guard(m_lock);
    return m_flags;
  }

  template <typename S> inline void print(S &s) const {
    ReadGuard guard(m_lock);
    s << "gap: (" << m_gap.key() << " +" << m_gap.length()
      << ")  flags: " << PrintFlags{m_flags};
  }

private:
  // receiver stalled, may schedule resend request if due to gap
  void stalled() {
    App *app = static_cast<App *>(this);
    Gap old, gap;
    {
      Guard guard(m_lock);
      if (!app->rxQueue()->count()) return;
      gap = app->rxQueue()->gap();
      if (gap == m_gap) return;
      old = m_gap;
      m_gap = gap;
    }
    app->cancelReRequest();
    if (!gap.length()) return;
    app->request(old, gap);
    app->scheduleReRequest();
  }

  Lock		m_lock;
  Gap		  m_gap;
  uint8_t	  m_flags = 0;
};

// template resend-requesting sender using ZmPQueue

// CRTP - application must conform to the following interface:
#if 0
typedef ZmPQueue<...> Queue;

struct App : public ZmPQTx<App, Queue> {
  typedef typename Queue::Node Msg;
  typedef typename Queue::Gap Gap;

  // access queue
  Queue *txQueue();

  // Note: *send*_()
  // - more indicates if messages are queued and will be sent immediately
  //   after this one - used to support message blocking by low-level sender
  // - the message can be aborted if it should not be delayed (in
  //   which case the function should return true as if successful); if the
  //   failure was not transient the application should call stop() and
  //   optionally shift() all messages to re-process them

  // send message (low level)
  bool send_(MxQMsg *msg, bool more); // true on success
  bool resend_(MxQMsg *msg, bool more); // true on success

  // send gap (can do nothing if not required)
  bool sendGap_(const MxQueue::Gap &gap, bool more); // true on success
  bool resendGap_(const MxQueue::Gap &gap, bool more); // true on success

  // archive message (low level) (once ackd by receiver(s))
  void archive_(Msg *msg);

  // retrieve message from archive containing key (key may be within message)
  // - can optionally call unshift() for subsequent messages <head
  ZmRef<Msg> retrieve_(Key key, Key head);

  // schedule send() to be called (possibly from different thread)
  void scheduleSend();
  // reschedule send() to be called (recursively, from within send() itself)
  void rescheduleSend();
  // sender is idle (nothing left in queue to send)
  void idleSend();

  // schedule resend() to be called (possibly from different thread)
  void scheduleResend();
  // schedule resend() to be called (recursively, from within resend() itself)
  void rescheduleResend();
  // resender is idle (nothing left in queue to resend)
  void idleResend();

  // schedule archive() to be called (possibly from different thread)
  void scheduleArchive();
  // schedule archive() to be called (recursively, from within archive() itself)
  void rescheduleArchive();
  // archiver is idle (nothing left to archive)
  void idleArchive();
};
#endif

template <class App, class Queue, class Lock_ = ZmNoLock>
class ZmPQTx : public ZuPrintable {
private:
  enum {
    Running		= 0x01,
    Sending		= 0x02,
    SendFailed		= 0x04,
    Archiving		= 0x08,
    Resending		= 0x10,
    ResendFailed	= 0x20
  };

public:
  template <typename S> static void printFlags(S &s, unsigned v) {
    static const char *flagNames[] = {
      "Running", "Sending", "SendFailed", "Archiving",
      "Resending", "ResendFailed"
    };
    bool comma = false;
    for (unsigned i = 0; i < sizeof(flagNames) / sizeof(flagNames[0]); i++)
      if (v & (1U<<i)) {
	if (comma) s << ',';
	comma = true;
	s << flagNames[i];
      }
  }
  struct PrintFlags : public ZuPrintable {
    inline PrintFlags(unsigned v_) : v(v_) { }
    PrintFlags(const PrintFlags &) = default;
    PrintFlags &operator =(const PrintFlags &) = default;
    PrintFlags(PrintFlags &&) = default;
    PrintFlags &operator =(PrintFlags &&) = default;
    template <typename S> inline void print(S &s) const { printFlags(s, v); }
    unsigned v;
  };

public:
  typedef typename Queue::Node Msg;
  typedef typename Queue::Fn Fn;
  typedef typename Queue::Key Key;
  typedef typename Queue::Gap Gap;

  typedef Lock_ Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  // start concurrent sending and re-sending (datagrams)
  void start() {
    App *app = static_cast<App *>(this);
    bool scheduleSend = false;
    bool scheduleArchive = false;
    bool scheduleResend = false;
    {
      Guard guard(m_lock);
#if 0
      std::cerr << (ZuStringN<200>()
	  << "start() PRE  " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
      bool alreadyRunning = m_flags & Running;
      if (!alreadyRunning) m_flags |= Running;
      if (alreadyRunning && (m_flags & SendFailed))
	scheduleSend = true;
      else if (scheduleSend = !(m_flags & Sending) &&
	  m_sendKey < app->txQueue()->tail())
	m_flags |= Sending;
      if (scheduleArchive = !(m_flags & Archiving) &&
	  m_ackdKey > m_archiveKey)
	m_flags |= Archiving;
      if (alreadyRunning && (m_flags & ResendFailed))
	scheduleResend = true;
      else if (scheduleResend = !(m_flags & Resending) && m_gap.length())
	m_flags |= Resending;
      m_flags &= ~(SendFailed | ResendFailed);
#if 0
      std::cerr << (ZuStringN<200>()
	  << "start() POST " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
    if (scheduleSend)
      app->scheduleSend();
    else
      app->idleSend();
    if (scheduleArchive) app->scheduleArchive();
    if (scheduleResend)
      app->scheduleResend();
    else
      app->idleResend();
  }

  // start concurrent sending and re-sending, from key onwards (streams)
  void start(Key key) {
    App *app = static_cast<App *>(this);
    bool scheduleSend = false;
    bool scheduleArchive = false;
    bool scheduleResend = false;
    {
      Guard guard(m_lock);
#if 0
      std::cerr << (ZuStringN<200>()
	  << "start() PRE  " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
      bool alreadyRunning = m_flags & Running;
      if (!alreadyRunning) m_flags |= Running;
      m_sendKey = m_ackdKey = key;
      if (alreadyRunning && (m_flags & SendFailed))
	scheduleSend = true;
      else if (scheduleSend = !(m_flags & Sending) &&
	  key < app->txQueue()->tail())
	m_flags |= Sending;
      if (scheduleArchive = !(m_flags & Archiving) &&
	  key > m_archiveKey)
	m_flags |= Archiving;
      if (alreadyRunning && (m_flags & ResendFailed))
	scheduleResend = true;
      else if (scheduleResend = !(m_flags & Resending) && m_gap.length())
	m_flags |= Resending;
      m_flags &= ~(SendFailed | ResendFailed);
#if 0
      std::cerr << (ZuStringN<200>()
	  << "start() POST " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
    if (scheduleSend)
      app->scheduleSend();
    else
      app->idleSend();
    if (scheduleArchive) app->scheduleArchive();
    if (scheduleResend)
      app->scheduleResend();
    else
      app->idleResend();
  }

  // stop sending
  void stop() {
    Guard guard(m_lock);
    if (!(m_flags & Running)) return;
    m_flags &= ~(Running | Sending | Resending);
  }

  // reset sequence number
  void txReset(Key key) {
    App *app = static_cast<App *>(this);
    Guard guard(m_lock);
    m_sendKey = m_ackdKey = m_archiveKey = key;
    m_gap = Gap();
    app->txQueue()->reset(key);
#if 0
      std::cerr << (ZuStringN<200>()
	  << "txReset() " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
  }

  // send message (with key already allocated)
  void send(ZmRef<Msg> msg) {
    App *app = static_cast<App *>(this);
    bool scheduleSend = false;
    bool scheduleArchive = false;
    auto key = Fn(msg->item()).key();
    {
      Guard guard(m_lock);
      if (key <= m_archiveKey) return;
      app->txQueue()->enqueue(ZuMv(msg));
      if (scheduleSend = (m_flags & (Running | Sending)) == Running &&
	  m_sendKey <= key)
	m_flags |= Sending;
      else if (scheduleArchive = !(m_flags & Archiving) && key > m_archiveKey)
	m_flags |= Archiving;
#if 0
      std::cerr << (ZuStringN<200>()
	  << "send(Msg) " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
    if (scheduleSend) app->scheduleSend();
    if (scheduleArchive) app->scheduleArchive();
  }

  // abort message
  ZmRef<Msg> abort(Key key) {
    App *app = static_cast<App *>(this);
    ZmRef<Msg> msg;
    {
      Guard guard(m_lock);
      msg = app->txQueue()->abort(key);
    }
    return msg;
  }

  // acknowlege (archive) messages up to, but not including, key
  void ackd(Key key) {
    App *app = static_cast<App *>(this);
    bool scheduleArchive;
    {
      Guard guard(m_lock);
      if (key <= m_ackdKey) return;
      m_ackdKey = key;
      if (key > m_sendKey) m_sendKey = key;
      if (scheduleArchive = !(m_flags & Archiving) && key > m_archiveKey)
	m_flags |= Archiving;
#if 0
      std::cerr << (ZuStringN<200>()
	  << "ackd(Key) " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
    if (scheduleArchive) app->scheduleArchive();
  }

  // resend messages (in response to a resend request)
private:
  bool resend_(const Gap &gap) {
    bool scheduleResend = false;
    if (!m_gap.length()) {
      m_gap = gap;
      scheduleResend = !(m_flags & Resending);
    } else {
      if (gap.key() < m_gap.key()) {
	m_gap.length() += (m_gap.key() - gap.key());
	m_gap.key() = gap.key();
	scheduleResend = !(m_flags & Resending);
      }
      if ((gap.key() + gap.length()) > (m_gap.key() + m_gap.length())) {
	m_gap.length() = (gap.key() - m_gap.key()) + gap.length();
	if (!scheduleResend) scheduleResend = !(m_flags & Resending);
      }
    }
    if (scheduleResend) m_flags |= Resending;
    return scheduleResend;
  }
public:
  void resend(const Gap &gap) {
    if (!gap.length()) return;
    App *app = static_cast<App *>(this);
    bool scheduleResend = false;
    {
      Guard guard(m_lock);
      scheduleResend = resend_(gap);
    }
    if (scheduleResend) app->scheduleResend();
  }

  // send - called via {,re}scheduleSend(), may call rescheduleSend()
  void send() {
    App *app = static_cast<App *>(this);
    bool scheduleSend = false;
    Gap sendGap;
    Key prevKey;
    ZmRef<Msg> msg;
    {
      Guard guard(m_lock);
      auto txQueue = app->txQueue();
#if 0
      std::cerr << (ZuStringN<200>()
	  << "send() " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
      if (!(m_flags & Running)) { m_flags &= ~Sending; return; }
      prevKey = m_sendKey;
      scheduleSend = prevKey < txQueue->tail();
      while (scheduleSend) {
	unsigned length;
	if (msg = txQueue->find(m_sendKey))
	  length = Fn(msg->item()).length();
	else if (msg = app->retrieve_(m_sendKey, txQueue->head()))
	  length = Fn(msg->item()).length();
	else {
	  if (!sendGap.length()) sendGap.key() = m_sendKey;
	  sendGap.length() += (length = 1);
	}
	m_sendKey += length;
	scheduleSend = m_sendKey < txQueue->tail();
	if (msg) break;
      }
      if (!scheduleSend) m_flags &= ~Sending;
    }
    if (ZuUnlikely(sendGap.length())) {
      if (ZuUnlikely(!app->sendGap_(sendGap, scheduleSend))) goto sendFailed;
      prevKey += sendGap.length();
    }
    if (ZuLikely(msg))
      if (ZuUnlikely(!app->send_(msg, scheduleSend))) goto sendFailed;
    if (scheduleSend)
      app->rescheduleSend();
    else
      app->idleSend();
    return;
  sendFailed:
    {
      Guard guard(m_lock);
      m_flags |= Sending | SendFailed;
      m_sendKey = prevKey;
#if 0
      std::cerr << (ZuStringN<200>()
	  << "send() FAIL " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
  }

  // archive - called via scheduleArchive(), may reschedule itself
  void archive() {
    App *app = static_cast<App *>(this);
    bool scheduleArchive;
    ZmRef<Msg> msg;
    {
      Guard guard(m_lock);
      if (!(m_flags & Running)) { m_flags &= ~Archiving; return; }
      scheduleArchive = m_archiveKey < m_ackdKey;
      while (scheduleArchive) {
	msg = app->txQueue()->find(m_archiveKey);
	m_archiveKey += msg ? (unsigned)Fn(msg->item()).length() : 1U;
	scheduleArchive = m_archiveKey < m_ackdKey;
	if (msg) break;
      }
      if (!scheduleArchive) m_flags &= ~Archiving;
#if 0
      std::cerr << (ZuStringN<200>()
	  << "archive() " << *this << "\n  " << *(app->txQueue()) << '\n')
	<< std::flush;
#endif
    }
    if (msg)
      app->archive_(msg);
    if (scheduleArchive)
      app->rescheduleArchive();
    else
      app->idleArchive();
  }

  // completed archiving of messages up to, but not including, key
  void archived(Key key) {
    App *app = static_cast<App *>(this);
    ZmRef<Msg> msg;
    do {
      Guard guard(m_lock);
      msg = app->txQueue()->shift(key);
    } while (msg);
  }

  // resend - called via scheduleResend(), may reschedule itself
  void resend() {
    App *app = static_cast<App *>(this);
    bool scheduleResend = false;
    Gap sendGap, prevGap;
    ZmRef<Msg> msg;
    {
      Guard guard(m_lock);
      if (!(m_flags & Running)) { m_flags &= ~Resending; return; }
      prevGap = m_gap;
      while (m_gap.length()) {
	unsigned length;
	if (msg = app->txQueue()->find(m_gap.key())) {
	  Fn item(msg->item());
	  auto end = item.key() + item.length();
	  length = end - m_gap.key();
	  if (end <= m_archiveKey)
	    while (app->txQueue()->shift(end));
	} else if (msg = app->retrieve_(m_gap.key(), app->txQueue()->head())) {
	  Fn item(msg->item());
	  auto end = item.key() + item.length();
	  length = end - m_gap.key();
	} else {
	  if (!sendGap.length()) sendGap.key() = m_gap.key();
	  sendGap.length() += (length = 1);
	}
	if (m_gap.length() <= length) {
	  m_gap = Gap();
	  scheduleResend = false;
	} else {
	  m_gap.key() += length;
	  m_gap.length() -= length;
	  scheduleResend = true;
	}
	if (msg) break;
      }
      if (!scheduleResend) m_flags &= ~Resending;
    }
    if (ZuUnlikely(sendGap.length())) {
      if (ZuUnlikely(!app->resendGap_(sendGap, scheduleResend)))
	goto resendFailed;
      unsigned length = sendGap.length();
      if (ZuLikely(prevGap.length() > length)) {
	prevGap.key() += length;
	prevGap.length() -= length;
      } else
	prevGap = Gap();
    }
    if (ZuLikely(msg))
      if (ZuUnlikely(!app->resend_(msg, scheduleResend))) goto resendFailed;
    if (scheduleResend)
      app->rescheduleResend();
    else
      app->idleResend();
    return;
  resendFailed:
    {
      Guard guard(m_lock);
      m_flags |= Resending | ResendFailed;
      m_gap = prevGap;
    }
  }

  ZuInline unsigned flags() const {
    ReadGuard guard(m_lock);
    return m_flags;
  }

  template <typename S> inline void print(S &s) const {
    ReadGuard guard(m_lock);
    s << "gap: (" << m_gap.key() << " +" << m_gap.length()
      << ")  flags: " << PrintFlags{m_flags}
      << "  send: " << m_sendKey
      << "  ackd: " << m_ackdKey
      << "  archive: " << m_archiveKey;
  }

private:
  Lock		m_lock;
  Key		  m_sendKey = 0;
  Key		  m_ackdKey = 0;
  Key		  m_archiveKey = 0;
  Gap		  m_gap;
  uint8_t	  m_flags = 0;
};

#endif /* ZmPQueue_HPP */
