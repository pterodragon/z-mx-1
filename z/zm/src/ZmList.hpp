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

// intrusive policy-based double-linked list

#ifndef ZmList_HPP
#define ZmList_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuObject.hpp>

#include <zlib/ZmLock.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmHeap.hpp>
#include <zlib/ZmNode.hpp>

// NTP (named template parameters) convention:
//
// ZmList<ZtString,				// list of ZtStrings
//   ZmListLock<ZmRWLock,			// lock with R/W lock
//     ZmListCmp<ZtICmp> > >			// case-insensitive comparison

struct ZmList_Defaults {
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct IndexT { using Index = T; };
  enum { NodeIsItem = 0 };
  using Lock = ZmLock;
  using Object = ZmObject;
  struct HeapID { static constexpr const char *id() { return "ZmList"; } };
};

// ZmListCmp - the item comparator
template <class Cmp_, class NTP = ZmList_Defaults>
struct ZmListCmp : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
  template <typename> struct ICmpT { using ICmp = Cmp_; };
};

// ZmListCmp_ - directly override the comparator
// (used by other templates to forward NTP parameters to ZmList)
template <class Cmp_, class NTP = ZmList_Defaults>
struct ZmListCmp_ : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
};

// ZmListICmp - the index comparator
template <class ICmp_, class NTP = ZmList_Defaults>
struct ZmListICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

// ZmListIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator and index type
template <class Accessor, class NTP = ZmList_Defaults>
struct ZmListIndex : public NTP {
  template <typename T> struct CmpT {
    using Cmp = typename ZuIndex<Accessor>::template CmpT<T>;
  };
  template <typename> struct ICmpT {
    using ICmp = typename ZuIndex<Accessor>::ICmp;
  };
  template <typename> struct IndexT {
    using Index = typename ZuIndex<Accessor>::I;
  };
};

// ZmListIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmList)
template <class Index_, class NTP = ZmList_Defaults>
struct ZmListIndex_ : public NTP {
  template <typename> struct IndexT { using Index = Index_; };
};

// ZmListNodeIsItem - derive ZmList::Node from Item instead of containing it
template <bool NodeIsItem_, class NTP = ZmList_Defaults>
struct ZmListNodeIsItem : public NTP {
  enum { NodeIsItem = NodeIsItem_ };
};

// ZmListLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmList_Defaults>
struct ZmListLock : public NTP {
  using Lock = Lock_;
};

// ZmListObject - the reference-counted object type used
template <class Object_, class NTP = ZmList_Defaults>
struct ZmListObject : public NTP {
  using Object = Object_;
};

// ZmListHeapID - the heap ID
template <class HeapID_, class NTP = ZmList_Defaults>
struct ZmListHeapID : public NTP {
  using HeapID = HeapID_;
};

template <typename Item, class NTP = ZmList_Defaults>
class ZmList : public ZmNodePolicy<typename NTP::Object> {
  using NodePolicy = ZmNodePolicy<typename NTP::Object>;

public:
  using Cmp = typename NTP::template CmpT<Item>::Cmp;
  using ICmp = typename NTP::template ICmpT<Item>::ICmp;
  using Index = typename NTP::template IndexT<Item>::Index;
  enum { NodeIsItem = NTP::NodeIsItem };
  using Lock = typename NTP::Lock;
  using Object = typename NodePolicy::Object;
  using HeapID = typename NTP::HeapID;

  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

protected:
  class Iterator_;
friend Iterator_;

public:
  // node in a list

  struct NullObject { }; // deconflict with ZuNull
  template <typename Node, typename Heap, bool NodeIsItem> class NodeFn :
      public ZuIf<
	ZuConversion<ZuNull, Object>::Is ||
	ZuConversion<ZuShadow, Object>::Is ||
	(NodeIsItem && ZuConversion<Object, Item>::Is),
	NullObject, Object>,
      public Heap {
    NodeFn(const NodeFn &);
    NodeFn &operator =(const NodeFn &);	// prevent mis-use

  friend ZmList<Item, NTP>;
  friend ZmList<Item, NTP>::Iterator_;

  protected:
    ZuInline NodeFn() { init(); }

  private:
    ZuInline void init() { m_next = m_prev = 0; }

    // access to Node instances is always guarded, so no need to protect
    // the returned object against concurrent deletion
    ZuInline Node *next() const { return m_next; }
    ZuInline Node *prev() const { return m_prev; }

    ZuInline void next(Node *n) { m_next = n; }
    ZuInline void prev(Node *n) { m_prev = n; }

    Node	*m_next, *m_prev;
  };

  template <typename Heap>
  using Node_ = ZmNode<Heap, NodeIsItem, NodeFn, Item>;
  struct NullHeap { }; // deconflict with ZuNull
  using NodeHeap = ZmHeap<HeapID, sizeof(Node_<NullHeap>)>;
  using Node = Node_<NodeHeap>;
  using Fn = typename Node::Fn;

  using NodeRef = typename NodePolicy::template Ref<Node>::T;

private:
  using NodePolicy::nodeRef;
  using NodePolicy::nodeDeref;
  using NodePolicy::nodeDelete;

protected:
  class Iterator_ {			// iterator
    Iterator_(const Iterator_ &) = delete;
    Iterator_ &operator =(const Iterator_ &) = delete;

    using List = ZmList<Item, NTP>;
  friend List;

  protected:
    Iterator_(Iterator_ &&) = default;
    Iterator_ &operator =(Iterator_ &&) = default;

    Iterator_(List &list) : m_list(list) {
      list.startIterate(*this);
    }

  public:
    void reset() { m_list.startIterate(*this); }

    Node *iterateNode() { return m_list.iterate(*this); }

    const Item &iterate() {
      Node *node = m_list.iterate(*this);
      if (ZuLikely(node)) return node->Node::item();
      return Cmp::null();
    }

    ZuInline unsigned count() const { return m_list.count_(); }

  protected:
    List	&m_list;
    Node	*m_node;
  };

public:
  class Iterator;
friend Iterator;
  class Iterator : private Guard, public Iterator_ {
    Iterator(const Iterator &) = delete;
    Iterator &operator =(const Iterator &) = delete;

    using List = ZmList<Item, NTP>;

  public:
    Iterator(Iterator &&) = default;
    Iterator &operator =(Iterator &&) = default;

    Iterator(List &list) : Guard(list.m_lock), Iterator_(list) { }

    template <typename Item_> void push(Item_ &&item)
      { this->m_list.pushIterate(*this, ZuFwd<Item_>(item)); }
    template <typename Item_> void unshift(Item_ &&item)
      { this->m_list.unshiftIterate(*this, ZuFwd<Item_>(item)); }
    NodeRef del() { return this->m_list.delIterate(*this); }
  };

  class ReadIterator;
friend ReadIterator;
  class ReadIterator : private ReadGuard, public Iterator_ {
    ReadIterator(const ReadIterator &) = delete;
    ReadIterator &operator =(const ReadIterator &) = delete;

    using List = ZmList<Item, NTP>;

  public:
    ReadIterator(ReadIterator &&) = default;
    ReadIterator &operator =(ReadIterator &&) = default;

    ReadIterator(const List &list) :
      ReadGuard(list.m_lock), Iterator_(const_cast<List &>(list)) { }
  };

  ZmList() = default;
  ZmList(const ZmList &) = delete;
  ZmList &operator =(const ZmList &) = delete;
  ZmList(ZmList &&) = delete;
  ZmList &operator =(ZmList &&) = delete;

  ~ZmList() { clean_(); }

  unsigned count() const { ReadGuard guard(m_lock); return m_count; }
  bool empty() const { ReadGuard guard(m_lock); return !m_count; }
  ZuInline unsigned count_() const { return m_count; }
  ZuInline bool empty_() const { return !m_count; }

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
 
  template <typename Item_> void add(Item_ &&item) {
    push(ZuFwd<Item_>(item));
  }
  template <typename Index_> NodeRef find(const Index_ &index) {
    Guard guard(m_lock);
    Node *node;

    if (!m_count) return 0;

    for (node = m_head;
	 node && !ICmp::equals(node->Node::item(), index);
	 node = node->Fn::next());

    return node;
  }
  template <typename Index_>
  ZuNotConvertible<Index_, Node *, NodeRef>
      del(const Index_ &index) {
    Guard guard(m_lock);
    Node *node;

    if (!m_count) return 0;

    for (node = m_head;
	 node && !ICmp::equals(node->Node::item(), index);
	 node = node->Fn::next());

    if (ZuLikely(node)) del_(node);

    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }
  NodeRef del(Node *node) {
    Guard guard(m_lock);

    if (ZuLikely(node)) del_(node);

    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }

  template <typename NodeRef_>
  ZuConvertible<NodeRef_, NodeRef>
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
  ZuNotConvertible<ZuDeref<Item_>, NodeRef>
      push(Item_ &&item) {
    push(NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  NodeRef popNode() {
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
  Item pop() {
    NodeRef node = popNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    return ZuMv(node->Node::item());
  }
  NodeRef rpopNode() {
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
  ZuConvertible<NodeRef_, NodeRef>
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
  ZuNotConvertible<ZuDeref<Item_>, NodeRef>
  unshift(Item_ &&item) {
    unshift(NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  NodeRef shiftNode() {
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
  Item shift() {
    NodeRef node = shiftNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    Item item = ZuMv(node->Node::item());
    nodeDelete(node);
    return item;
  }
  NodeRef rshiftNode() {
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
  Item rshift() {
    NodeRef node = rshiftNode();
    if (ZuUnlikely(!node)) return Cmp::null();
    Item item = ZuMv(node->Node::item());
    nodeDelete(node);
    return item;
  }

  Item head() const {
    ReadGuard guard(m_lock);
    if (ZuUnlikely(!m_head)) return Cmp::null();
    return m_head->Node::item();
  }
  NodeRef headNode() const {
    ReadGuard guard(m_lock);
    return m_head;
  }
  Item tail() const {
    ReadGuard guard(m_lock);
    if (ZuUnlikely(!m_tail)) return Cmp::null();
    return m_tail->Node::item();
  }
  NodeRef tailNode() const {
    ReadGuard guard(m_lock);
    return m_tail;
  }

  void clean() {
    Guard guard(m_lock);
    clean_();
  }

  auto iterator() { return Iterator(*this); }
  auto readIterator() const { return ReadIterator(*this); }

protected:
  void startIterate(Iterator_ &iterator) {
    iterator.m_node = nullptr;
  }
  Node *iterate(Iterator_ &iterator) {
    Node *node = iterator.m_node;

    if (!node)
      node = m_head;
    else
      node = node->Fn::next();

    if (!node) return 0;

    return iterator.m_node = node;
  }
  template <typename NodeRef_>
  ZuConvertible<NodeRef_, NodeRef>
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
  ZuNotConvertible<ZuDeref<Item_>, NodeRef>
      pushIterate(Iterator_ &iterator, Item_ &&item) {
    pushIterate(iterator, NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  template <typename NodeRef_>
  ZuConvertible<NodeRef, NodeRef_>
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
  ZuNotConvertible<ZuDeref<Item_>, NodeRef>
      unshiftIterate(Iterator_ &iterator, Item_ &&item) {
    unshiftIterate(iterator, NodeRef(new Node(ZuFwd<Item_>(item))));
  }
  NodeRef delIterate(Iterator_ &iterator) {
    if (!m_count) return nullptr;

    Node *node = iterator.m_node;

    if (ZuLikely(node)) {
      iterator.m_node = node->Fn::prev();
      del_(node);
    }

    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
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
    unsigned	  m_count = 0;
    Node	  *m_head = nullptr;
    Node	  *m_tail = nullptr;
};

#endif /* ZmList_HPP */
