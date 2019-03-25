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

// doubly linked list for types with a distinguished null value

#ifndef ZmList_HPP
#define ZmList_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuIf.hpp>
#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuConversion.hpp>

#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmHeap.hpp>
#include <ZmINode.hpp>

// NTP (named template parameters) convention:
//
// ZmList<ZtString,				// list of ZtStrings
//   ZmListLock<ZmRWLock,			// lock with R/W lock
//     ZmListCmp<ZtICmp> > >			// case-insensitive comparison

struct ZmList_Defaults {
  template <typename T> struct CmpT { typedef ZuCmp<T> Cmp; };
  template <typename T> struct ICmpT { typedef ZuCmp<T> ICmp; };
  template <typename T> struct IndexT { typedef T Index; };
  enum { NodeIsItem = 0 };
  typedef ZmLock Lock;
  typedef ZmObject Object;
  struct HeapID { inline static const char *id() { return "ZmList"; } };
  struct Base { };
};

// ZmListCmp - the item comparator
template <class Cmp_, class NTP = ZmList_Defaults>
struct ZmListCmp : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
  template <typename> struct ICmpT { typedef Cmp_ ICmp; };
};

// ZmListCmp_ - directly override the comparator
// (used by other templates to forward NTP parameters to ZmList)
template <class Cmp_, class NTP = ZmList_Defaults>
struct ZmListCmp_ : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
};

// ZmListICmp - the index comparator
template <class ICmp_, class NTP = ZmList_Defaults>
struct ZmListICmp : public NTP {
  template <typename> struct ICmpT { typedef ICmp_ ICmp; };
};

// ZmListIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator and index type
template <class Accessor, class NTP = ZmList_Defaults>
struct ZmListIndex : public NTP {
  template <typename T> struct CmpT {
    typedef typename ZuIndex<Accessor>::template CmpT<T> Cmp;
  };
  template <typename> struct ICmpT {
    typedef typename ZuIndex<Accessor>::ICmp ICmp;
  };
  template <typename> struct IndexT {
    typedef typename ZuIndex<Accessor>::I Index;
  };
};

// ZmListIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmList)
template <class Index_, class NTP = ZmList_Defaults>
struct ZmListIndex_ : public NTP {
  template <typename> struct IndexT { typedef Index_ Index; };
};

// ZmListNodeIsItem - derive ZmList::Node from Item instead of containing it
template <bool NodeIsItem_, class NTP = ZmList_Defaults>
struct ZmListNodeIsItem : public NTP {
  enum { NodeIsItem = NodeIsItem_ };
};

// ZmListLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmList_Defaults>
struct ZmListLock : public NTP {
  typedef Lock_ Lock;
};

// ZmListObject - the reference-counted object type used
template <class Object_, class NTP = ZmList_Defaults>
struct ZmListObject : public NTP {
  typedef Object_ Object;
};

// ZmListHeapID - the heap ID
template <class HeapID_, class NTP = ZmList_Defaults>
struct ZmListHeapID : public NTP {
  typedef HeapID_ HeapID;
};

// ZmListBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmList_Defaults>
struct ZmListBase : public NTP {
  typedef Base_ Base;
};

template <typename Item, class NTP = ZmList_Defaults>
class ZmList : public NTP::Base {
  ZmList(const ZmList &);
  ZmList &operator =(const ZmList &);	// prevent mis-use

public:
  typedef typename NTP::template CmpT<Item>::Cmp Cmp;
  typedef typename NTP::template ICmpT<Item>::ICmp ICmp;
  typedef typename NTP::template IndexT<Item>::Index Index;
  enum { NodeIsItem = NTP::NodeIsItem };
  typedef typename NTP::Lock Lock;
  typedef typename NTP::Object Object;
  typedef typename NTP::HeapID HeapID;

  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

protected:
  class Iterator_;
friend class Iterator_;

public:
  // node in a list

  struct NullObject { }; // deconflict with ZuNull
  template <typename Node, typename Heap, bool NodeIsItem> class NodeFn :
      public ZuIf<NullObject, Object,
	ZuConversion<Object, ZuNull>::Is ||
	(NodeIsItem && ZuConversion<Object, Item>::Is)>::T,
      public Heap {
    NodeFn(const NodeFn &);
    NodeFn &operator =(const NodeFn &);	// prevent mis-use

  friend class ZmList<Item, NTP>;
  friend class ZmList<Item, NTP>::Iterator_;

  protected:
    inline NodeFn() : m_next(0), m_prev(0) { }

  private:
    // access to Node instances is always guarded, so no need to protect
    // the returned object against concurrent deletion
    inline Node *next() { return m_next; }
    inline Node *prev() { return m_prev; }

    inline void next(Node *n) { m_next = n; }
    inline void prev(Node *n) { m_prev = n; }

    Node	*m_next, *m_prev;
  };

  template <typename Heap>
  using Node_ = ZmINode<Heap, NodeIsItem, NodeFn, Item>;
  struct NullHeap { }; // deconflict with ZuNull
  typedef ZmHeap<HeapID, sizeof(Node_<NullHeap>)> NodeHeap;
  typedef Node_<NodeHeap> Node;
  typedef typename Node::Fn Fn;

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

protected:
  class Iterator_ {			// iterator
    typedef ZmList<Item, NTP> List;
  friend class ZmList<Item, NTP>;

  protected:
    inline Iterator_(List &list) : m_list(list) {
      list.startIterate(*this);
    }

  public:
    inline void reset() { m_list.startIterate(*this); }

    inline NodeRef iterateNode() { return m_list.iterate(*this); }

    inline const Item &iterate() {
      NodeRef node = m_list.iterate(*this);
      if (ZuLikely(node)) return node->Node::item();
      return Cmp::null();
    }

  protected:
    List			&m_list;
    typename List::NodeRef	m_node;
  };

public:
  class Iterator;
friend class Iterator;
  class Iterator : private Guard, public Iterator_ {
    Iterator(const Iterator &);
    Iterator &operator =(const Iterator &);	// prevent mis-use

    typedef ZmList<Item, NTP> List;

  public:
    inline Iterator(List &list) : Guard(list.m_lock), Iterator_(list) { }

    template <typename Item_> void push(Item_ &&item)
      { this->m_list.pushIterate(*this, ZuFwd<Item_>(item)); }
    template <typename Item_> void unshift(Item_ &&item)
      { this->m_list.unshiftIterate(*this, ZuFwd<Item_>(item)); }
    void del() { this->m_list.delIterate(*this); }
  };

  class ReadIterator;
friend class ReadIterator;
  class ReadIterator : private ReadGuard, public Iterator_ {
    ReadIterator(const ReadIterator &);
    ReadIterator &operator =(const ReadIterator &);	// prevent mis-use

    typedef ZmList<Item, NTP> List;

  public:
    inline ReadIterator(List &list) :
      ReadGuard(list.m_lock), Iterator_(list) { }
  };

  template <typename ...Args>
  ZmList(Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      m_count(0), m_head(0), m_tail(0) { }

  ~ZmList() { clean_(); }

  inline unsigned count() const { return m_count; }
  inline bool empty() const { return (!m_count); }

  void join(ZmList &list) { // join lists (the other is left empty)
    int count;
    Node *head, *tail;
    {
      Guard guard(list.m_lock);
      head = list.m_head, tail = list.m_tail;
      count = list.m_count;
      list.m_head = list.m_tail = 0;
      list.m_count = 0;
    }
    if (head) {
      Guard guard(m_lock);
      m_tail->Fn::next(head);
      head->Fn::prev(m_tail);
      m_tail = tail;
      m_count += count;
    }
  }
 
  template <typename Item_> inline void add(Item_ &&item) {
    push(ZuFwd<Item_>(item));
  }
  template <typename Index_> inline NodeRef find(const Index_ &index) {
    Guard guard(m_lock);
    Node *node;

    if (!m_count) return 0;

    for (node = m_head;
	 node && !ICmp::equals(node->Node::item(), index);
	 node = node->Fn::next());

    return node;
  }
  template <typename Index_>
  inline typename ZuNotConvertible<Index_, NodeRef, NodeRef>::T
      del(const Index_ &index) {
    Guard guard(m_lock);
    Node *node;

    if (!m_count) return 0;

    for (node = m_head;
	 node && !ICmp::equals(node->Node::item(), index);
	 node = node->Fn::next());

    NodeRef ret = node;

    if (node) del_(node);

    return ret;
  }
  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef_, NodeRef>::T
      del(const NodeRef_ &node_) {
    const NodeRef &node = node_;
    Guard guard(m_lock);
    if (ZuLikely(node)) {
      del_(node);
      nodeDelete(node);
    }
  }

  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef_, NodeRef>::T
      push(const NodeRef_ &node_) {
    const NodeRef &node = node_;
    Guard guard(m_lock);

    nodeRef(node);
    node->Fn::next(0);
    node->Fn::prev(m_tail);
    if (!m_tail)
      m_head = node;
    else
      m_tail->Fn::next(node);
    m_tail = node;
    ++m_count;
  }
  template <typename Item_>
  inline typename ZuNotConvertible<typename ZuDeref<Item_>::T, NodeRef>::T
      push(Item_ &&item) {
    push(NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  inline NodeRef popNode() {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_tail)) return 0;

    if (!(m_tail = node->Fn::prev()))
      m_head = 0;
    else
      m_tail->Fn::next(0);

    NodeRef ret = node;

    nodeDeref(node);
    --m_count;

    return ret;
  }
  inline Item pop() {
    NodeRef node = popNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    return ZuMv(node->Node::item());
  }
  inline NodeRef rpopNode() {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_tail)) return 0;

    if (!(m_tail = node->Fn::prev()))
      m_tail = node;
    else {
      node->Fn::next(m_head);
      m_head->Fn::prev(node);
      (m_head = node)->Fn::prev(0);
      m_tail->Fn::next(0);
    }

    return node;
  }
  Item rpop() {
    NodeRef node = rpopNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::item();
  }
  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef_, NodeRef>::T
      unshift(const NodeRef_ &node_) {
    const NodeRef &node = node_;
    Guard guard(m_lock);

    nodeRef(node);
    node->Fn::prev(0);
    node->Fn::next(m_head);
    if (!m_head)
      m_tail = node;
    else
      m_head->Fn::prev(node);
    m_head = node;
    ++m_count;
  }
  template <typename Item_>
  inline typename ZuNotConvertible<typename ZuDeref<Item_>::T, NodeRef>::T
  unshift(Item_ &&item) {
    unshift(NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  inline NodeRef shiftNode() {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_head)) return 0;

    if (!(m_head = node->Fn::next()))
      m_tail = 0;
    else
      m_head->Fn::prev(0);

    NodeRef ret = node;

    nodeDeref(node);
    --m_count;

    return ret;
  }
  inline Item shift() {
    NodeRef node = shiftNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    Item item = ZuMv(node->Node::item());
    nodeDelete(node);
    return item;
  }
  inline NodeRef rshiftNode() {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_head)) return 0;

    if (!(m_head = node->Fn::next()))
      m_head = node;
    else {
      node->Fn::prev(m_tail);
      m_tail->Fn::next(node);
      (m_tail = node)->Fn::next(0);
      m_head->Fn::prev(0);
    }

    return node;
  }
  inline Item rshift() {
    NodeRef node = rshiftNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    Item item = ZuMv(node->Node::item());
    nodeDelete(node);
    return item;
  }

  inline Item head() const {
    ReadGuard guard(m_lock);
    if (ZuUnlikely(!m_head)) return Cmp::null();
    return m_head->Node::item();
  }
  inline NodeRef headNode() const {
    ReadGuard guard(m_lock);
    return m_head;
  }
  inline Item tail() const {
    ReadGuard guard(m_lock);
    if (ZuUnlikely(!m_tail)) return Cmp::null();
    return m_tail->Node::item();
  }
  inline NodeRef tailNode() const {
    ReadGuard guard(m_lock);
    return m_tail;
  }

  void clean() {
    Guard guard(m_lock);
    clean_();
  }

  inline auto iterator() { return Iterator(*this); }
  inline auto readIterator() const { return ReadIterator(*this); }

protected:
  inline void startIterate(Iterator_ &iterator) {
    iterator.m_node = 0;
  }
  inline NodeRef iterate(Iterator_ &iterator) {
    Node *node = iterator.m_node;

    if (!node)
      node = m_head;
    else
      node = node->Fn::next();

    if (!node) return 0;

    return iterator.m_node = node;
  }
  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef_, NodeRef>::T
      pushIterate(Iterator_ &iterator, const NodeRef_ &node_) {
    const NodeRef &node = node_;
    Node *prevNode = iterator.m_node;

    if (!prevNode) { push(node); return; }

    nodeRef(node);
    if (Node *nextNode = prevNode->Fn::next()) {
      node->Fn::next(nextNode);
      nextNode->Fn::prev(node);
    } else {
      m_tail = node;
      node->Fn::next(0);
    }
    node->Fn::prev(prevNode);
    prevNode->Fn::next(node);
    ++m_count;
  }
  template <typename Item_>
  inline typename ZuNotConvertible<typename ZuDeref<Item_>::T, NodeRef>::T
      pushIterate(Iterator_ &iterator, Item_ &&item) {
    pushIterate(iterator, NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  template <typename NodeRef_>
  inline typename ZuConvertible<NodeRef, NodeRef_>::T
      unshiftIterate(Iterator_ &iterator, const NodeRef_ &node_) {
    const NodeRef &node = node_;
    Node *nextNode = iterator.m_node;

    if (!nextNode) { unshift(node); return; }

    nodeRef(node);
    if (Node *prevNode = nextNode->Fn::prev()) {
      node->Fn::prev(prevNode);
      prevNode->Fn::next(node);
    } else {
      m_head = node;
      node->Fn::prev(0);
    }
    node->Fn::next(nextNode);
    nextNode->Fn::prev(node);
    ++m_count;
  }
  template <typename Item_>
  inline typename ZuNotConvertible<typename ZuDeref<Item_>::T, NodeRef>::T
      unshiftIterate(Iterator_ &iterator, Item_ &&item) {
    unshiftIterate(iterator, NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  void delIterate(Iterator_ &iterator) {
    if (!m_count) return;

    NodeRef node = iterator.m_node;

    if (ZuLikely(node)) {
      del_(node);
      nodeDelete(node);
    }
  }

  void del_(Node *node) {
    Node *prevNode(node->Fn::prev());
    Node *nextNode(node->Fn::next());

    ZmAssert(prevNode || nextNode || (m_head == node && m_tail == node));

    if (!prevNode)
      m_head = nextNode;
    else
      prevNode->Fn::next(nextNode);

    if (!nextNode)
      m_tail = prevNode;
    else
      nextNode->Fn::prev(prevNode);

    nodeDeref(node);
    --m_count;
  }

  void clean_() {
    if (!m_count) return;

    Node *node = m_head, *prevNode;

    while (prevNode = node) {
      node = prevNode->Fn::next();
      nodeDeref(prevNode);
      nodeDelete(prevNode);
    }

    m_head = m_tail = 0;
    m_count = 0;
  }

  Lock		m_lock;
  unsigned	  m_count;
  Node		  *m_head, *m_tail;
};

#endif /* ZmList_HPP */
