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

// red/black tree

#ifndef ZmRBTree_HPP
#define ZmRBTree_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuIf.hpp>
#include <ZuNull.hpp>
#include <ZuCmp.hpp>
#include <ZuIndex.hpp>
#include <ZuConversion.hpp>

#include <ZmGuard.hpp>
#include <ZmLock.hpp>
#include <ZmObject.hpp>
#include <ZmRef.hpp>
#include <ZmHeap.hpp>
#include <ZmKVNode.hpp>

// uses NTP (named template parameters):
//
// ZmRBTree<ZtString,				// keys are ZtStrings
//   ZmRBTreeBase<ZmObject,			// base of ZmObject
//     ZmRBTreeVal<ZtString,			// values are ZtStrings
//	 ZmRBTreeValCmp<ZtICmp> > > >		// case-insensitive comparison

struct ZmRBTree_Defaults {
  typedef ZuNull Val;
  template <typename T> struct CmpT { typedef ZuCmp<T> Cmp; };
  template <typename T> struct ICmpT { typedef ZuCmp<T> ICmp; };
  template <typename T> struct IndexT { typedef T Index; };
  template <typename T> struct ValCmpT { typedef ZuCmp<T> ValCmp; };
  enum { NodeIsKey = 0 };
  enum { NodeIsVal = 0 };
  typedef ZmLock Lock;
  typedef ZmObject Object;
  struct HeapID { inline static const char *id() { return "ZmRBTree"; } };
  struct Base { };
};

// ZmRBTreeCmp - the key comparator
template <class Cmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeCmp : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
  template <typename> struct ICmpT { typedef Cmp_ ICmp; };
};

// ZmRBTreeCmp_ - directly override the key comparator
// (used by other templates to forward NTP parameters to ZmRBTree)
template <class Cmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeCmp_ : public NTP {
  template <typename> struct CmpT { typedef Cmp_ Cmp; };
};

// ZmRBTreeICmp - the index comparator
template <class ICmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeICmp : public NTP {
  template <typename> struct ICmpT { typedef ICmp_ ICmp; };
};

// ZmRBTreeIndex - use a different type as the index, rather than the key as is
// common use case - the key is a struct and the index is a data member
// uncommon use case - the index is calculated from the key in some way
// pass an Accessor to override the comparator and index type
template <class Accessor, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeIndex : public NTP {
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

// ZmRBTreeIndex_ - directly override the index type
// (used by other templates to forward NTP parameters to ZmRBTree)
template <class Index_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeIndex_ : public NTP {
  template <typename> struct IndexT { typedef Index_ Index; };
};

// ZmRBTreeVal - the value type
template <class Val_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeVal : public NTP {
  typedef Val_ Val;
};

// ZmRBTreeValCmp - the value comparator
template <class ValCmp_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeValCmp : public NTP {
  template <typename> struct ValCmpT { typedef ValCmp_ ValCmp; };
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
  typedef Lock_ Lock;
};

// ZmRBTreeObject - the reference-counted object type used
template <class Object_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeObject : public NTP {
  typedef Object_ Object;
};

// ZmRBTreeHeapID - the heap ID
template <class HeapID_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeHeapID : public NTP {
  typedef HeapID_ HeapID;
};

// ZmRBTreeBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmRBTree_Defaults>
struct ZmRBTreeBase : public NTP {
  typedef Base_ Base;
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
  typedef Tree_ Tree;
  enum { Direction = Direction_ };
  typedef typename Tree::Key Key;
  typedef typename Tree::Val Val;
  typedef typename Tree::Cmp Cmp;
  typedef typename Tree::ValCmp ValCmp;
  typedef typename Tree::Node Node;
  typedef typename Tree::NodeRef NodeRef;

protected:
  inline ZmRBTreeIterator_(Tree &tree) : m_tree(tree) {
    tree.startIterate(*this);
  }
  template <typename Index>
  inline ZmRBTreeIterator_(
      Tree &tree, const Index &index,
      int compare = Tree::GreaterEqual) :
    m_tree(tree) {
    tree.startIterate(*this, index, compare);
  }

public:
  inline void reset() {
    m_tree.startIterate(*this);
  }
  template <typename Index>
  inline void reset(
      const Index &index,
      int compare = Tree::GreaterEqual) {
    m_tree.startIterate(*this, index, compare);
  }

  inline Node *iterate() { return m_tree.iterate(*this); }

  inline const Key &iterateKey() {
    Node *node = m_tree.iterate(*this);
    if (ZuLikely(node)) return node->Node::key();
    return Cmp::null();
  }
  inline const Val &iterateVal() {
    Node *node = m_tree.iterate(*this);
    if (ZuLikely(node)) return node->Node::val();
    return ValCmp::null();
  }

  ZuInline unsigned count() const { return m_tree.count_(); }

protected:
  Tree	&m_tree;
  Node	*m_node;
  Node	*m_last;
};

template <typename Tree_, int Direction_ = ZmRBTreeGreaterEqual>
class ZmRBTreeIterator :
  public Tree_::Guard,
  public ZmRBTreeIterator_<Tree_, Direction_> {
  typedef Tree_ Tree;
  enum { Direction = Direction_ };
  typedef typename Tree::Guard Guard;

public:
  inline ZmRBTreeIterator(Tree &tree) :
    Guard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(tree) { }
  template <typename Index_>
  inline ZmRBTreeIterator(Tree &tree, const Index_ &index,
      int compare = Direction) :
    Guard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(tree, index, compare) { }

  void del() { this->m_tree.delIterate(*this); }
};

template <typename Tree_, int Direction_ = ZmRBTreeGreaterEqual>
class ZmRBTreeReadIterator :
  public Tree_::ReadGuard,
  public ZmRBTreeIterator_<Tree_, Direction_> {
  typedef Tree_ Tree;
  enum { Direction = Direction_ };
  typedef typename Tree::ReadGuard ReadGuard;

public:
  inline ZmRBTreeReadIterator(const Tree &tree) :
    ReadGuard(tree.lock()),
    ZmRBTreeIterator_<Tree, Direction>(
	const_cast<Tree &>(tree)) { }
  template <typename Index_>
  inline ZmRBTreeReadIterator(const Tree &tree, const Index_ &index,
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
  typedef Key_ Key;
  typedef typename NTP::Val Val;
  typedef typename NTP::template CmpT<Key>::Cmp Cmp;
  typedef typename NTP::template ICmpT<Key>::ICmp ICmp;
  typedef typename NTP::template IndexT<Key>::Index Index;
  typedef typename NTP::template ValCmpT<Val>::ValCmp ValCmp;
  enum { NodeIsKey = NTP::NodeIsKey };
  enum { NodeIsVal = NTP::NodeIsVal };
  typedef typename NTP::Lock Lock;
  typedef typename NTP::Object Object;
  typedef typename NTP::HeapID HeapID;

  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  template <int Direction = ZmRBTreeGreaterEqual>
  using Iterator = ZmRBTreeIterator<ZmRBTree, Direction>;
  template <int Direction = ZmRBTreeGreaterEqual>
  using ReadIterator = ZmRBTreeReadIterator<ZmRBTree, Direction>;

  // node in a red/black tree

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

  template <typename, typename> friend class ZmRBTree;

  protected:
    ZuInline NodeFn() { }

  private:
    ZuInline void init() { m_right = m_left = m_dup = 0; m_parent = 0; }

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
    ZuInline Node *dup() const { return m_dup; }
    ZuInline Node *parent() const { return (Node *)(m_parent & ~(uintptr_t)1); }

    ZuInline void right(Node *n) { m_right = n; }
    ZuInline void left(Node *n) { m_left = n; }
    ZuInline void dup(Node *n) { m_dup = n; }
    ZuInline void parent(Node *n) {
      m_parent = (uintptr_t)n | (m_parent & (uintptr_t)1);
    }

    Node		*m_right;
    Node		*m_left;
    Node		*m_dup;
    uintptr_t		m_parent;
  };

  template <typename Heap>
  using Node_ = ZmKVNode<Heap, NodeIsKey, NodeIsVal, NodeFn, Key, Val>;
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
public:
  template <typename ...Args>
  ZmRBTree(Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      m_root(0), m_minimum(0), m_maximum(0), m_count(0) { };

  ~ZmRBTree() { clean_(); }

  inline Lock &lock() const { return m_lock; }

  inline unsigned count() const { ReadGuard guard(m_lock); return m_count; }
  inline unsigned count_() const { return m_count; }

  inline void add(Node *newNode) {
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

      if (!c) {
	Node *child;

	newNode->Fn::dup(child = node->Fn::dup());
	if (child) child->Fn::parent(newNode);
	node->Fn::dup(newNode);
	newNode->Fn::parent(node);
	++m_count;
	return;
      }

      if (c > 0) {
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
  inline typename ZuNotConvertible<
	typename ZuDeref<Key__>::T, NodeRef, NodeRef>::T
      add(Key__ &&key) {
    NodeRef node = new Node(ZuFwd<Key__>(key));
    add(node);
    return node;
  }
  template <typename Key__, typename Val_>
  inline NodeRef add(Key__ &&key, Val_ &&val) {
    NodeRef node = new Node(ZuFwd<Key__>(key), ZuFwd<Val_>(val));
    this->add(node);
    return node;
  }

  template <typename Index_>
  inline NodeRef find(const Index_ &index) const {
    ReadGuard guard(m_lock);
    return find_(index);
  }
  template <typename Index_>
  inline Node *findPtr(const Index_ &index) const {
    ReadGuard guard(m_lock);
    return find_(index);
  }
  template <typename Index_>
  inline const Key &findKey(const Index_ &index) const {
    ReadGuard guard(m_lock);
    Node *node = find_(index);
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  template <typename Index_>
  inline const Val &findVal(const Index_ &index) const {
    ReadGuard guard(m_lock);
    Node *node = find_(index);
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  inline NodeRef minimum() const {
    ReadGuard guard(m_lock);
    return m_minimum;
  }
  inline Node *minimumPtr() const {
    ReadGuard guard(m_lock);
    return m_minimum;
  }
  inline const Key &minimumKey() const {
    NodeRef node = minimum();
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  inline const Val &minimumVal() const {
    NodeRef node = minimum();
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  inline NodeRef maximum() const {
    ReadGuard guard(m_lock);
    return m_maximum;
  }
  inline Node *maximumPtr() const {
    ReadGuard guard(m_lock);
    return m_maximum;
  }
  inline const Key &maximumKey() const {
    NodeRef node = maximum();
    if (ZuUnlikely(!node)) return Cmp::null();
    return node->Node::key();
  }
  inline const Val &maximumVal() const {
    NodeRef node = maximum();
    if (ZuUnlikely(!node)) return ValCmp::null();
    return node->Node::val();
  }

  template <typename Index_>
  inline typename ZuIfT<
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
    NodeRef *ZuMayAlias(ptr) = (NodeRef *)&node;
    return ZuMv(*ptr);
  }
  inline NodeRef del(Node *node) {
    if (ZuUnlikely(!node)) return nullptr;
    Guard guard(m_lock);
    delNode_(node);
    NodeRef *ZuMayAlias(ptr) = (NodeRef *)&node;
    return ZuMv(*ptr);
  }
  inline void delNode_(Node *node) {
    Node *parent = node->Fn::parent();
    Node *dup = node->Fn::dup();

    if (parent && parent->Fn::dup() == node) {
      parent->Fn::dup(dup);
      if (dup) dup->Fn::parent(parent);
    } else if (dup) {
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
    } else
      delRebalance(node);

    --m_count;
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

  template <typename Index_, typename Val_>
  inline typename ZuIfT<
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
	NodeRef *ZuMayAlias(ptr) = (NodeRef *)&node;
	return ZuMv(*ptr);
      }
    } while (node = node->Fn::dup());
    return nullptr;
  }
  template <typename Index_, typename Key__>
  inline typename ZuIfT<
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
	NodeRef *ZuMayAlias(ptr) = (NodeRef *)&node;
	return ZuMv(*ptr);
      }
    } while (node = node->Fn::dup());
    return nullptr;
  }

  template <int Direction = ZmRBTreeGreaterEqual>
  inline auto iterator() {
    return Iterator<Direction>(*this);
  }
  template <int Direction, typename Index_>
  inline auto iterator(Index_ &&index) {
    return Iterator<Direction>(*this, ZuFwd<Index_>(index));
  }
  template <int Direction = ZmRBTreeGreaterEqual>
  inline auto readIterator() const {
    return ReadIterator<Direction>(*this);
  }
  template <int Direction, typename Index_>
  inline auto readIterator(Index_ &&index) const {
    return ReadIterator<Direction>(*this, ZuFwd<Index_>(index));
  }

  void clean() {
    Guard guard(m_lock);

    clean_();
  }

protected:
  template <typename Index_>
  inline Node *find_(const Index_ &index) const {
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
  inline Node *find_(const Index_ &index, int compare) const {
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

    if (next = node->Fn::dup()) return next;

    if (next = node->Fn::parent())
      while (node == next->Fn::dup()) {
	node = next;
	if (!(next = node->Fn::parent())) break;
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

    if (prev = node->Fn::dup()) return prev;

    if (prev = node->Fn::parent())
      while (node == prev->Fn::dup()) {
	node = prev;
	if (!(prev = node->Fn::parent())) break;
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
  inline typename ZuIfT<(Direction >= 0)>::T startIterate(
      Iterator_<Direction> &iterator) {
    iterator.m_node = m_minimum;
    iterator.m_last = 0;
  }
  template <int Direction>
  inline typename ZuIfT<(Direction < 0)>::T startIterate(
      Iterator_<Direction> &iterator) {
    iterator.m_node = m_maximum;
    iterator.m_last = 0;
  }
  template <int Direction, typename Index_>
  inline void startIterate(
      Iterator_<Direction> &iterator, const Index_ &index, int compare) {
    iterator.m_node = find_(index, compare);
    iterator.m_last = 0;
  }

  template <int Direction>
  inline typename ZuIfT<(Direction > 0), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = next(node);
    return iterator.m_last = node;
  }
  template <int Direction>
  inline typename ZuIfT<(!Direction), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = node->Fn::dup();
    return iterator.m_last = node;
  }
  template <int Direction>
  inline typename ZuIfT<(Direction < 0), Node *>::T iterate(
      Iterator_<Direction> &iterator) {
    Node *node = iterator.m_node;
    if (!node) return nullptr;
    iterator.m_node = prev(node);
    return iterator.m_last = node;
  }

  template <int Direction>
  void delIterate(Iterator_<Direction> &iterator) {
    if (!m_count) return;

    Node *node = iterator.m_last;

    if (!node) return;

    {
      Node *parent = node->Fn::parent();
      Node *dup = node->Fn::dup();

      if (parent && parent->Fn::dup() == node) {
	parent->Fn::dup(dup);
	if (dup) dup->Fn::parent(parent);
	nodeDeref(node);
	nodeDelete(node);
	--m_count;
	return;
      } else if (dup) {
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
	nodeDeref(node);
	nodeDelete(node);
	--m_count;
	return;
      }
    }

    delRebalance(node);

    nodeDeref(node);
    nodeDelete(node);
    --m_count;
  }

// clean tree

  void clean_() {
    {
      auto i = iterator();
      while (i.iterate()) i.del();
    }

    m_minimum = m_maximum = m_root = nullptr;
    m_count = 0;
  }

  mutable Lock	m_lock;
    Node	  *m_root;
    Node	  *m_minimum;
    Node	  *m_maximum;
    unsigned	  m_count;
};

#endif /* ZmRBTree_HPP */
