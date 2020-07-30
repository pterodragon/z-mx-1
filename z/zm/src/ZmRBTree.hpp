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

// red/black tree

#ifndef ZmRBTree_HPP
#define ZmRBTree_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuIf.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuIndex.hpp>
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuObject.hpp>
#include <zlib/ZuShadow.hpp>

#include <zlib/ZmGuard.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmHeap.hpp>
#include <zlib/ZmKVNode.hpp>

// uses NTP (named template parameters):
//
// ZmRBTree<ZtString,				// keys are ZtStrings
//   ZmRBTreeBase<ZmObject,			// base of ZmObject
//     ZmRBTreeVal<ZtString,			// values are ZtStrings
//	 ZmRBTreeValCmp<ZtICmp> > > >		// case-insensitive comparison

struct ZmRBTree_Defaults {
  using Val = ZuNull;
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct IndexT { using Index = T; };
  template <typename T> struct ValCmpT { using ValCmp = ZuCmp<T>; };
  enum { NodeIsKey = 0 };
  enum { NodeIsVal = 0 };
  using Lock = ZmLock;
  using Object = ZmObject;
  struct HeapID { static constexpr const char *id() { return "ZmRBTree"; } };
  struct Base { };
  enum { Unique = 0 };
};

// ZmRBTreeCmp - the key comparator
template <class Cmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeCmp : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
  template <typename> struct ICmpT { using ICmp = Cmp_; };
};

// ZmRBTreeCmp_ - directly override the key comparator
// (used by other templates to forward NTP parameters to ZmRBTree)
template <class Cmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeCmp_ : public NTP {
  template <typename> struct CmpT { using Cmp = Cmp_; };
};

// ZmRBTreeICmp - the index comparator
template <class ICmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

// ZmRBTreeIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator and index type
template <class Accessor, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeIndex : public NTP {
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

// ZmRBTreeIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmRBTree)
template <class Index_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeIndex_ : public NTP {
  template <typename> struct IndexT { using Index = Index_; };
};

// ZmRBTreeVal - the value type
template <class Val_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeVal : public NTP {
  using Val = Val_;
};

// ZmRBTreeValCmp - the value comparator
template <class ValCmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeValCmp : public NTP {
  template <typename> struct ValCmpT { using ValCmp = ValCmp_; };
};

// ZmRBTreeNodeIsKey - derive ZmRBTree::Node from Key instead of containing it
template <bool NodeIsKey_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeNodeIsKey : public NTP {
  enum { NodeIsKey = NodeIsKey_ };
};

// ZmRBTreeNodeIsVal - derive ZmRBTree::Node from Val instead of containing it
template <bool NodeIsVal_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeNodeIsVal : public NTP {
  enum { NodeIsVal = NodeIsVal_ };
};

// ZmRBTreeLock - the lock type used (ZmRWLock will permit concurrent reads)
template <class Lock_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeLock : public NTP {
  using Lock = Lock_;
};

// ZmRBTreeObject - the reference-counted object type used
template <class Object_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeObject : public NTP {
  using Object = Object_;
};

// ZmRBTreeHeapID - the heap ID
template <class HeapID_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeHeapID : public NTP {
  using HeapID = HeapID_;
};

// ZmRBTreeBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeBase : public NTP {
  using Base = Base_;
};

// ZmRBTreeUnique - key is unique
template <bool Unique_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeUnique : public NTP {
  enum { Unique = Unique_ };
};

enum {
  ZmRBTreeEqual = 0,
  ZmRBTreeGreaterEqual = 1,
  ZmRBTreeLessEqual = -1,
  ZmRBTreeGreater = 2,
  ZmRBTreeLess = -2
};

template <typename Tree_, int Direction_>
class ZmRBTreeIterator_ { // red/black tree iterator
friend Tree_;

public:
  using Tree = Tree_;
  enum { Direction = Direction_ };
  using Key = typename Tree::Key;
  using Val = typename Tree::Val;
  using Cmp = typename Tree::Cmp;
  using ValCmp = typename Tree::ValCmp;
  using Node = typename Tree::Node;
  using NodeRef = typename Tree::NodeRef;

protected:
  ZmRBTreeIterator_(Tree &tree) : m_tree(tree) {
    tree.startIterate(*this);
  }
  template <typename Index>
  ZmRBTreeIterator_(
      Tree &tree, const Index &index,
      int compare = Tree::GreaterEqual) :
    m_tree(tree) {
    tree.startIterate(*this, index, compare);
  }

public:
  void reset() { m_tree.startIterate(*this); }
  template <typename Index>
  void reset(
      const Index &index,
      int compare = Tree::GreaterEqual) {
    m_tree.startIterate(*this, index, compare);
  }

  ZuInline Node *iterate() { return m_tree.iterate(*this); }

  const Key &iterateKey() {
    Node *node = m_tree.iterate(*this);
    if (ZuLikely(node)) return node->Node::key();
    return Cmp::null();
  }
  const Val &iterateVal() {
    Node *node = m_tree.iterate(*this);
    if (ZuLikely(node)) return node->Node::val();
    return ValCmp::null();
  }

  ZuInline unsigned count() const { return m_tree.count_(); }

protected:
  Tree	&m_tree;
  Node	*m_node;
};

template <typename Tree_, int Direction_ = ZmRBTreeGreaterEqual>
class ZmRBTreeIterator :
  public Tree_::Guard,
  public ZmRBTreeIterator_<Tree_, Direction_> {
  using Tree = Tree_;
  enum { Direction = Direction_ };
  using Guard = typename Tree::Guard;
  using Node = typename Tree::Node;
  using NodeRef = typename Tree::NodeRef;

public:
  ZmRBTreeIterator(Tree &tree) :
    Guard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(tree) { }
  template <typename Index_>
  ZmRBTreeIterator(Tree &tree, const Index_ &index,
      int compare = Direction) :
    Guard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(tree, index, compare) { }

  NodeRef del(Node *node) { return this->m_tree.delIterate(node); }
};

template <typename Tree_, int Direction_ = ZmRBTreeGreaterEqual>
class ZmRBTreeReadIterator :
  public Tree_::ReadGuard,
  public ZmRBTreeIterator_<Tree_, Direction_> {
  using Tree = Tree_;
  enum { Direction = Direction_ };
  using ReadGuard = typename Tree::ReadGuard;

public:
  ZmRBTreeReadIterator(const Tree &tree) :
    ReadGuard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(
	const_cast<Tree &>(tree)) { }
  template <typename Index_>
  ZmRBTreeReadIterator(const Tree &tree, const Index_ &index,
      int compare = Direction) :
    ReadGuard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(
	const_cast<Tree &>(tree), index, compare) { }
};

template <typename Key_, class NTP = ZmRBTree_Defaults>
class ZmRBTree : public NTP::Base {
  ZmRBTree(const ZmRBTree &);
  ZmRBTree &operator =(const ZmRBTree &);	// prevent mis-use

  template <typename, int> friend class ZmRBTreeIterator_;
  template <typename, int> friend class ZmRBTreeIterator;

public:
  using Key = Key_;
  using Val = typename NTP::Val;
  using Cmp = typename NTP::template CmpT<Key>::Cmp;
  using ICmp = typename NTP::template ICmpT<Key>::ICmp;
  using Index = typename NTP::template IndexT<Key>::Index;
  using ValCmp = typename NTP::template ValCmpT<Val>::ValCmp;
  enum { NodeIsKey = NTP::NodeIsKey };
  enum { NodeIsVal = NTP::NodeIsVal };
  using Lock = typename NTP::Lock;
  using Object = typename NTP::Object;
  using HeapID = typename NTP::HeapID;
  enum { Unique = NTP::Unique };

  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  template <int Direction = ZmRBTreeGreaterEqual>
  using Iterator = ZmRBTreeIterator<ZmRBTree, Direction>;
  template <int Direction = ZmRBTreeGreaterEqual>
  using ReadIterator = ZmRBTreeReadIterator<ZmRBTree, Direction>;

  // node in a red/black tree

private:
  struct NullObject { }; // deconflict with ZuNull
  template <typename Node>
  class NodeFn_Dup {
  public:
    ZuInline void init() { m_dup = nullptr; }
    ZuInline Node *dup() const { return m_dup; }
    ZuInline void dup(Node *n) { m_dup = n; }
  private:
    Node		*m_dup;
  };
  template <typename Node>
  class NodeFn_Unique {
  public:
    ZuInline void init() { }
    ZuInline constexpr Node * const dup() const { return nullptr; }
    ZuInline void dup(Node *) { }
  };
  template <
    typename Node, typename Heap, bool NodeIsKey, bool NodeIsVal, bool Unique>
  class NodeFn_ :
      public ZuIf<NullObject, Object,
	ZuConversion<ZuNull, Object>::Is ||
	ZuConversion<ZuShadow, Object>::Is ||
	(NodeIsKey && ZuConversion<Object, Key>::Is) ||
	(NodeIsVal && ZuConversion<Object, Val>::Is)>::T,
      public ZuIf<NodeFn_Unique<Node>, NodeFn_Dup<Node>, Unique>::T,
      public Heap {
    using Base =
      typename ZuIf<NodeFn_Unique<Node>, NodeFn_Dup<Node>, Unique>::T;

  public:
    ZuInline NodeFn_() { }

    ZuInline void init() {
      Base::init();
      m_right = m_left = nullptr;
      m_parent = 0;
    }

    ZuInline bool black() {
      return m_parent & (uintptr_t)1;
    }
    ZuInline void black(bool black) {
      black ? (m_parent |= (uintptr_t)1) : (m_parent &= ~(uintptr_t)1);
    }

    // access to these methods is always guarded, so no need to protect
    // the returned object against concurrent deletion; these are private
    ZuInline Node *right() const { return m_right; }
    ZuInline Node *left() const { return m_left; }
    ZuInline Node *parent() const { return (Node *)(m_parent & ~(uintptr_t)1); }

    ZuInline void right(Node *n) { m_right = n; }
    ZuInline void left(Node *n) { m_left = n; }
    ZuInline void parent(Node *n) {
      m_parent = (uintptr_t)n | (m_parent & (uintptr_t)1);
    }

  private:
    Node		*m_right;
    Node		*m_left;
    uintptr_t		m_parent;
  };

  template <typename Node, typename Heap, bool NodeIsKey, bool NodeIsVal>
  using NodeFn = NodeFn_<Node, Heap, NodeIsKey, NodeIsVal, Unique>;
  template <typename Heap>
  using Node_ = ZmKVNode<Heap, NodeIsKey, NodeIsVal, NodeFn, Key, Val>;
  struct NullHeap { }; // deconflict with ZuNull
  using NodeHeap = ZmHeap<HeapID, sizeof(Node_<NullHeap>)>;
public:
  using Node = Node_<NodeHeap>;
  using NodeRef =
    typename ZuIf<ZmRef<Node>, Node *, ZuIsObject_<Object>::OK>::T;
private:
  using Fn = typename Node::Fn;

  // in order to support reference-counted, owned and shadowed
  // objects, some overloading is required for ref/deref/delete
  template <typename T, typename O = Object>
  ZuInline typename ZuIsObject<O>::T nodeRef(T &&o) { ZmREF(ZuFwd<T>(o)); }
  template <typename T, typename O = Object>
  ZuInline typename ZuIsObject<O>::T nodeDeref(T &&o) { ZmDEREF(ZuFwd<T>(o)); }
  template <typename T, typename O = Object>
  ZuInline typename ZuIsObject<O>::T nodeDelete(T &&) { }

  template <typename T, typename O = Object>
  ZuInline typename ZuNotObject<O>::T nodeRef(T &&) { }
  template <typename T, typename O = Object>
  ZuInline typename ZuNotObject<O>::T nodeDeref(T &&) { }
  template <typename T, typename O = Object>
  ZuInline typename ZuIfT<!ZuIsObject_<O>::OK && !ZuIsShadow_<O>::OK>::T
  nodeDelete(T &&o) { delete ZuFwd<T>(o); }
  template <typename T, typename O = Object>
  ZuInline typename ZuIfT<!ZuIsObject_<O>::OK && ZuIsShadow_<O>::OK>::T
  nodeDelete(T &&) { }

public:
  template <typename ...Args>
  ZmRBTree(Args &&... args) : NTP::Base{ZuFwd<Args>(args)...} { }

  ~ZmRBTree() { clean(); }

  ZuInline Lock &lock() const { return m_lock; }

  unsigned count() const { ReadGuard guard(m_lock); return m_count; }
  ZuInline unsigned count_() const { return m_count; }

  void add(Node *newNode) {
    newNode->init();

    Guard guard(m_lock);
    Node *node;

    nodeRef(newNode);

    if (!(node = m_root)) {
      newNode->black(1);
      m_root = m_minimum = m_maximum = newNode;
      ++m_count;
      return;
    }

    bool minimum = true, maximum = true;
    const Key &key = newNode->Node::key();

    for (;;) {
      int c = Cmp::cmp(node->Node::key(), key);

      if constexpr (!Unique) {
	if (!c) {
	  Node *child;

	  newNode->Fn::dup(child = node->Fn::dup());
	  if (child) child->Fn::parent(newNode);
	  node->Fn::dup(newNode);
	  newNode->Fn::parent(node);
	  ++m_count;
	  return;
	}
      }

      if (c >= 0) {
	if (!node->Fn::left()) {
	  node->Fn::left(newNode);
	  newNode->Fn::parent(node);
	  if (minimum) m_minimum = newNode;
	  break;
	}

	node = node->Fn::left();
	maximum = false;
      } else {
	if (!node->Fn::right()) {
	  node->Fn::right(newNode);
	  newNode->Fn::parent(node);
	  if (maximum) m_maximum = newNode;
	  break;
	}

	node = node->Fn::right();
	minimum = false;
      }
    }

    rebalance(newNode);
    ++m_count;
  }
  template <typename Key__>
  typename ZuNotConvertible<
	typename ZuDeref<Key__>::T, NodeRef, NodeRef>::T
      add(Key__ &&key) {
    NodeRef node = new Node(ZuFwd<Key__>(key));
    add(node);
    return node;
  }
  template <typename Key__, typename Val_>
  NodeRef add(Key__ &&key, Val_ &&val) {
    NodeRef node = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    this->add(node);
    return node;
  }

  template <typename Index_>
  NodeRef find(const Index_ &index) const {
    ReadGuard guard(m_lock);
    return find_(index);
  }
  template <typename Index_>
  Node *findPtr(const Index_ &index) const {
    ReadGuard guard(m_lock);
    return find_(index);
  }
  template <typename Index_>
  const Key &findKey(const Index_ &index) const {
    ReadGuard guard(m_lock);
    Node *node = find_(index);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_>
  const Val &findVal(const Index_ &index) const {
    ReadGuard guard(m_lock);
    Node *node = find_(index);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  NodeRef minimum() const {
    ReadGuard guard(m_lock);
    return m_minimum;
  }
  Node *minimumPtr() const {
    ReadGuard guard(m_lock);
    return m_minimum;
  }
  const Key &minimumKey() const {
    NodeRef node = minimum();
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  const Val &minimumVal() const {
    NodeRef node = minimum();
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  NodeRef maximum() const {
    ReadGuard guard(m_lock);
    return m_maximum;
  }
  Node *maximumPtr() const {
    ReadGuard guard(m_lock);
    return m_maximum;
  }
  const Key &maximumKey() const {
    NodeRef node = maximum();
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  const Val &maximumVal() const {
    NodeRef node = maximum();
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  template <typename Index_>
  typename ZuIfT<
    !ZuConversion<Index_, Node *>::Exists, NodeRef>::T
      del(const Index_ &index) {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_root)) return nullptr;

    int c;

    for (;;) {
      if (!node) return nullptr;

      if (!(c = ICmp::cmp(node->Node::key(), index))) break;

      if (c > 0)
	node = node->Fn::left();
      else
	node = node->Fn::right();
    }

    delNode_(node);
    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }
  NodeRef del(Node *node) {
    if (ZuUnlikely(!node)) return nullptr;
    Guard guard(m_lock);
    delNode_(node);
    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }
  void delNode_(Node *node) {
    if constexpr (!Unique) {
      Node *parent = node->Fn::parent();
      Node *dup = node->Fn::dup();

      if (parent && parent->Fn::dup() == node) {
	parent->Fn::dup(dup);
	if (dup) dup->Fn::parent(parent);
	--m_count;
	return;
      }
      if (dup) {
	{
	  Node *child;

	  dup->Fn::left(child = node->Fn::left());
	  if (child) child->Fn::parent(dup);
	  dup->Fn::right(child = node->Fn::right());
	  if (child) child->Fn::parent(dup);
	}
	if (!parent) {
	  m_root = dup;
	  dup->Fn::parent(0);
	} else if (node == parent->Fn::right()) {
	  parent->Fn::right(dup);
	  dup->Fn::parent(parent);
	} else {
	  parent->Fn::left(dup);
	  dup->Fn::parent(parent);
	}
	dup->black(node->black());
	if (node == m_minimum) m_minimum = dup;
	if (node == m_maximum) m_maximum = dup;
	--m_count;
	return;
      }
    }
    delRebalance(node);
    --m_count;
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

  template <typename Index_, typename Val_>
  typename ZuIfT<
    !ZuConversion<Index_, Node *>::Exists &&
    ZuConversion<Val_, Val>::Exists, NodeRef>::T
      del(const Index_ &index, const Val_ &val) {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_root)) return nullptr;

    int c;

    for (;;) {
      if (!node) return nullptr;

      if (!(c = ICmp::cmp(node->Node::key(), index))) break;

      if (c > 0)
	node = node->Fn::left();
      else
	node = node->Fn::right();
    }

    do {
      if (ValCmp::equals(node->Node::val(), val)) {
	delNode_(node);
	NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
	return ZuMv(*ptr);
      }
    } while (node = node->Fn::dup());
    return nullptr;
  }
  template <typename Index_, typename Key__>
  typename ZuIfT<
    !ZuConversion<Index_, Node *>::Exists &&
    ZuConversion<Key__, Key>::Exists &&
    !ZuConversion<Key__, Val>::Exists, NodeRef>::T
      del(const Index_ &index, const Key__ &key) {
    Guard guard(m_lock);
    Node *node;

    if (!(node = m_root)) return nullptr;

    int c;

    for (;;) {
      if (!node) return nullptr;

      if (!(c = ICmp::cmp(node->Node::key(), index))) break;

      if (c > 0)
	node = node->Fn::left();
      else
	node = node->Fn::right();
    }

    do {
      if (node->Node::key() == key) { // not Cmp::equals()
	delNode_(node);
	NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
	return ZuMv(*ptr);
      }
    } while (node = node->Fn::dup());
    return nullptr;
  }

  template <int Direction = ZmRBTreeGreaterEqual>
  auto iterator() {
    return Iterator<Direction>(*this);
  }
  template <int Direction, typename Index_>
  auto iterator(Index_ &&index) {
    return Iterator<Direction>(*this, ZuFwd<Index_>(index));
  }
  template <int Direction = ZmRBTreeGreaterEqual>
  auto readIterator() const {
    return ReadIterator<Direction>(*this);
  }
  template <int Direction, typename Index_>
  auto readIterator(Index_ &&index) const {
    return ReadIterator<Direction>(*this, ZuFwd<Index_>(index));
  }

// clean tree

  void clean() {
    auto i = iterator();
    while (auto node = i.iterate()) i.del(node);
    m_minimum = m_maximum = m_root = nullptr;
    m_count = 0;
  }

protected:
  template <typename Index_>
  Node *find_(const Index_ &index) const {
    Node *node;

    if (!(node = m_root)) return nullptr;

    int c;

    for (;;) {
      if (!node) return nullptr;

      if (!(c = ICmp::cmp(node->Node::key(), index))) return node;

      if (c > 0)
	node = node->Fn::left();
      else
	node = node->Fn::right();
    }
  }
  template <typename Index_>
  Node *find_(const Index_ &index, int compare) const {
    if (compare == ZmRBTreeEqual) {
      Node *node;

      if (!(node = m_root)) return nullptr;

      int c;

      for (;;) {
	if (!node) return nullptr;

	if (!(c = ICmp::cmp(node->Node::key(), index))) return node;

	if (c > 0)
	  node = node->Fn::left();
	else
	  node = node->Fn::right();
      }
    } else {
      Node *node, *foundNode = 0;
      bool equal =
	(compare == ZmRBTreeGreaterEqual || compare == ZmRBTreeLessEqual);
      bool greater =
	(compare == ZmRBTreeGreaterEqual || compare == ZmRBTreeGreater);
      bool less =
	(compare == ZmRBTreeLessEqual || compare == ZmRBTreeLess);

      if (!(node = m_root)) return nullptr;

      int c;

      for (;;) {
	if (!node) return foundNode;

	c = ICmp::cmp(node->Node::key(), index);

	if (!c) {
	  if (equal) return node;
	  if (greater)
	    node = node->Fn::right();
	  else
	    node = node->Fn::left();
	} else if (c > 0) {
	  if (greater) foundNode = node;
	  node = node->Fn::left();
	} else {
	  if (less) foundNode = node;
	  node = node->Fn::right();
	}
      }
    }
  }

  void rotateRight(Node *node, Node *parent) {
    Node *left = node->Fn::left();
    Node *mid = left->Fn::right();

  // move left to the right, under node's parent
  // (make left the root if node is the root)

    if (parent) {
      if (parent->Fn::left() == node)
	parent->Fn::left(left);
      else
	parent->Fn::right(left);
    } else
      m_root = left;
    left->Fn::parent(parent);

  // node descends to left's right

    left->Fn::right(node), node->Fn::parent(left);

  // mid switches from left's right to node's left

    node->Fn::left(mid); if (mid) mid->Fn::parent(node);
  }

  void rotateLeft(Node *node, Node *parent) {
    Node *right = node->Fn::right();
    Node *mid = right->Fn::left();

  // move right to the left, under node's parent
  // (make right the root if node is the root)

    if (parent) {
      if (parent->Fn::right() == node)
	parent->Fn::right(right);
      else
	parent->Fn::left(right);
    } else
      m_root = right;
    right->Fn::parent(parent);

  // node descends to right's left

    right->Fn::left(node), node->Fn::parent(right);

  // mid switches from right's left to node's right

    node->Fn::right(mid); if (mid) mid->Fn::parent(node);
  }

  void rebalance(Node *node) {
  // rebalance until we hit a black node (the root is always black)

    for (;;) {
      Node *parent = node->Fn::parent();

      if (!parent) { node->black(1); return; }	// force root to black

      if (parent->black()) return;

      Node *gParent = parent->Fn::parent();

      if (parent == gParent->Fn::left()) {
	Node *uncle = gParent->Fn::right();

	if (uncle && !uncle->black()) {
	  parent->black(1);
	  uncle->black(1);
	  (node = gParent)->black(0);
	} else {
	  if (node == parent->Fn::right()) {
	    rotateLeft(node = parent, gParent);
	    gParent = (parent = node->Fn::parent())->Fn::parent();
	  }
	  parent->black(1);
	  gParent->black(0);
	  rotateRight(gParent, gParent->Fn::parent());
	  m_root->black(1);				// force root to black
	  return;
	}
      } else {
	Node *uncle = gParent->Fn::left();

	if (uncle && !uncle->black()) {
	  parent->black(1);
	  uncle->black(1);
	  (node = gParent)->black(0);
	} else {
	  if (node == parent->Fn::left()) {
	    rotateRight(node = parent, gParent);
	    gParent = (parent = node->Fn::parent())->Fn::parent();
	  }
	  parent->black(1);
	  gParent->black(0);
	  rotateLeft(gParent, gParent->Fn::parent());
	  m_root->black(1);				// force root to black
	  return;
	}
      }
    }
  }

  void delRebalance(Node *node) {
    Node *successor = node;
    Node *child, *parent;

    if (!successor->Fn::left())
      child = successor->Fn::right();
    else if (!successor->Fn::right())
      child = successor->Fn::left();
    else {
      successor = successor->Fn::right();
      while (successor->Fn::left()) successor = successor->Fn::left();
      child = successor->Fn::right();
    }

    if (successor != node) {
      node->Fn::left()->Fn::parent(successor);
      successor->Fn::left(node->Fn::left());
      if (successor != node->Fn::right()) {
	parent = successor->Fn::parent();
	if (child) child->Fn::parent(parent);
	successor->Fn::parent()->Fn::left(child);
	successor->Fn::right(node->Fn::right());
	node->Fn::right()->Fn::parent(successor);
      } else
	parent = successor;

      Node *childParent = parent;

      parent = node->Fn::parent();

      if (!parent)
	m_root = successor;
      else if (node == parent->Fn::left())
	parent->Fn::left(successor);
      else
	parent->Fn::right(successor);
      successor->Fn::parent(parent);

      bool black = node->black();

      node->black(successor->black());
      successor->black(black);

      successor = node;

      parent = childParent;
    } else {
      parent = node->Fn::parent();

      if (child) child->Fn::parent(parent);

      if (!parent)
	m_root = child;
      else if (node == parent->Fn::left())
	parent->Fn::left(child);
      else
	parent->Fn::right(child);

      if (node == m_minimum) {
	if (!node->Fn::right())
	  m_minimum = parent;
	else {
	  Node *minimum = child;

	  do {
	    m_minimum = minimum;
	  } while (minimum = minimum->Fn::left());
	}
      }

      if (node == m_maximum) {
	if (!node->Fn::left())
	  m_maximum = parent;
	else {
	  Node *maximum = child;

	  do {
	    m_maximum = maximum;
	  } while (maximum = maximum->Fn::right());
	}
      }
    }

    if (successor->black()) {
      Node *sibling;

      while (parent && (!child || child->black()))
	if (child == parent->Fn::left()) {
	  sibling = parent->Fn::right();
	  if (!sibling->black()) {
	    sibling->black(1);
	    parent->black(0);
	    rotateLeft(parent, parent->Fn::parent());
	    sibling = parent->Fn::right();
	  }
	  if ((!sibling->Fn::left() || sibling->Fn::left()->black()) &&
	      (!sibling->Fn::right() || sibling->Fn::right()->black())) {
	    sibling->black(0);
	    child = parent;
	    parent = child->Fn::parent();
	  } else {
	    if (!sibling->Fn::right() || sibling->Fn::right()->black()) {
	      if (sibling->Fn::left()) sibling->Fn::left()->black(1);
	      sibling->black(0);
	      rotateRight(sibling, parent);
	      sibling = parent->Fn::right();
	    }
	    sibling->black(parent->black());
	    parent->black(1);
	    if (sibling->Fn::right()) sibling->Fn::right()->black(1);
	    rotateLeft(parent, parent->Fn::parent());
	    break;
	  }
	} else {
	  sibling = parent->Fn::left();
	  if (!sibling->black()) {
	    sibling->black(1);
	    parent->black(0);
	    rotateRight(parent, parent->Fn::parent());
	    sibling = parent->Fn::left();
	  }
	  if ((!sibling->Fn::right() || sibling->Fn::right()->black()) &&
	      (!sibling->Fn::left() || sibling->Fn::left()->black())) {
	    sibling->black(0);
	    child = parent;
	    parent = child->Fn::parent();
	  } else {
	    if (!sibling->Fn::left() || sibling->Fn::left()->black()) {
	      if (sibling->Fn::right()) sibling->Fn::right()->black(1);
	      sibling->black(0);
	      rotateLeft(sibling, parent);
	      sibling = parent->Fn::left();
	    }
	    sibling->black(parent->black());
	    parent->black(1);
	    if (sibling->Fn::left()) sibling->Fn::left()->black(1);
	    rotateRight(parent, parent->Fn::parent());
	    break;
	  }
	}
      if (child) child->black(1);
    }
  }

  Node *next(Node *node) {
    Node *next;

    if constexpr (!Unique) {
      if (next = node->Fn::dup()) return next;

      if (next = node->Fn::parent())
	while (node == next->Fn::dup()) {
	  node = next;
	  if (!(next = node->Fn::parent())) break;
	}
    }

    if (next = node->Fn::right()) {
      node = next;
      while (node = node->Fn::left()) next = node;
      return next;
    }

    if (!(next = node->Fn::parent())) return nullptr;

    while (node == next->Fn::right()) {
      node = next;
      if (!(next = node->Fn::parent())) return nullptr;
    }

    return next;
  }

  Node *prev(Node *node) {
    Node *prev;

    if constexpr (!Unique) {
      if (prev = node->Fn::dup()) return prev;

      if (prev = node->Fn::parent())
	while (node == prev->Fn::dup()) {
	  node = prev;
	  if (!(prev = node->Fn::parent())) break;
	}
    }

    if (prev = node->Fn::left()) {
      node = prev;
      while (node = node->Fn::right()) prev = node;
      return prev;
    }

    if (!(prev = node->Fn::parent())) return nullptr;

    while (node == prev->Fn::left()) {
      node = prev;
      if (!(prev = node->Fn::parent())) return nullptr;
    }

    return prev;
  }

// iterator functions

  template <int Direction>
  using Iterator_ = ZmRBTreeIterator_<ZmRBTree, Direction>;

  template <int Direction>
  typename ZuIfT<(Direction >= 0)>::T startIterate(
      Iterator_<Direction> &iterator) {
    iterator.m_node = m_minimum;
  }
  template <int Direction>
  typename ZuIfT<(Direction < 0)>::T startIterate(
      Iterator_<Direction> &iterator) {
    iterator.m_node = m_maximum;
  }
  template <int Direction, typename Index_>
  void startIterate(
      Iterator_<Direction> &iterator, const Index_ &index, int compare) {
    iterator.m_node = find_(index, compare);
  }

  template <int Direction>
  typename ZuIfT<(Direction > 0), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = next(node);
    return node;
  }
  template <int Direction>
  typename ZuIfT<(!Direction), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = node->Fn::dup();
    return node;
  }
  template <int Direction>
  typename ZuIfT<(Direction < 0), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = prev(node);
    return node;
  }

  NodeRef delIterate(Node *node) {
    if (ZuUnlikely(!node)) return nullptr;
    delNode_(node);
    NodeRef *ZuMayAlias(ptr) = reinterpret_cast<NodeRef *>(&node);
    return ZuMv(*ptr);
  }

  mutable Lock	m_lock;
    Node	  *m_root = nullptr;
    Node	  *m_minimum = nullptr;
    Node	  *m_maximum = nullptr;
    unsigned	  m_count = 0;
};

#endif /* ZmRBTree_HPP */
