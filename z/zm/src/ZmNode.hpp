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

// container node (used by ZmHash, ZmRBTree, ...)

// ZmNode - node containing single item (no key/value)
// ZmKVNode - node containing key with (optional) value
// ZmNodePolicy - node ownership (referencing/dereferencing) policy

#ifndef ZmNode_HPP
#define ZmNode_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuNull.hpp>
#include <zlib/ZuObject.hpp>

template <
  class Heap, bool NodeIsItem,
  template <typename, typename, bool> class Fn, typename Item> class ZmNode;
// node contains item (default)
template <
  class Heap, template <typename, typename, bool> class Fn_, typename Item>
class ZmNode<Heap, 0, Fn_, Item> :
    public Fn_<ZmNode<Heap, 0, Fn_, Item>, Heap, 0> {
  ZmNode(const ZmNode &);
  ZmNode &operator =(const ZmNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmNode<Heap, 0, Fn_, Item>, Heap, 0>;

  ZuInline ZmNode() { }
  template <typename ...Args>
  ZuInline ZmNode(Args &&... args) : m_item{ZuFwd<Args>(args)...} { }

  ZuInline const Item &item() const { return m_item; }
  ZuInline Item &item() { return m_item; }

private:
  Item			m_item;
};
// node derives from item
template <
  class Heap, template <typename, typename, bool> class Fn_, typename Item>
class ZmNode<Heap, 1, Fn_, Item> :
    public Item, public Fn_<ZmNode<Heap, 1, Fn_, Item>, Heap, 1> {
  ZmNode(const ZmNode &);
  ZmNode &operator =(const ZmNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmNode<Heap, 1, Fn_, Item>, Heap, 1>;

  ZuInline ZmNode() { }
  template <typename ...Args>
  ZuInline ZmNode(Args &&... args) : Item{ZuFwd<Args>(args)...} { }

  ZuInline const Item &item() const { return *this; }
  ZuInline Item &item() { return *this; }
};

template <class Heap, bool NodeIsKey, bool NodeIsVal,
	 template <typename, typename, bool, bool> class Fn,
	 typename Key, typename Val> class ZmKVNode;
// node contains key, value (default)
template <class Heap, template <typename, typename, bool, bool> class Fn_,
	 typename Key, typename Val>
	   class ZmKVNode<Heap, 0, 0, Fn_, Key, Val> :
    public Fn_<ZmKVNode<Heap, 0, 0, Fn_, Key, Val>, Heap, 0, 0> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 0, 0, Fn_, Key, Val>, Heap, 0, 0>;

  ZuInline ZmKVNode() { }
  template <typename Key__>
  ZuInline ZmKVNode(Key__ &&key) : m_key(ZuFwd<Key__>(key)) { }
  template <typename Key__, typename Val_>
  ZuInline ZmKVNode(Key__ &&key, Val_ &&val) :
    m_key(ZuFwd<Key__>(key)), m_val(ZuFwd<Val_>(val)) { }

  ZuInline const Key &key() const { return m_key; }
  ZuInline Key &key() { return m_key; }
  ZuInline const Val &val() const { return m_val; }
  ZuInline Val &val() { return m_val; }

private:
  Key			m_key;
  Val			m_val;
};
// node contains key, value is ZuNull (common use case)
template <class Heap,
  template <typename, typename, bool, bool> class Fn_,
  typename Key> class ZmKVNode<Heap, 0, 0, Fn_, Key, ZuNull> :
    public Fn_<ZmKVNode<Heap, 0, 0, Fn_, Key, ZuNull>, Heap, 0, 0> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 0, 0, Fn_, Key, ZuNull>, Heap, 0, 0>;

  ZuInline ZmKVNode() { }
  template <typename ...Args>
  ZuInline ZmKVNode(Args &&... args) : m_key{ZuFwd<Args>(args)...} { }

  ZuInline const Key &key() const { return m_key; }
  ZuInline Key &key() { return m_key; }
  ZuInline ZuNull &val() const { static ZuNull val; return val; }

private:
  Key			m_key;
};
// node derives from key
template <class Heap,
  template <typename, typename, bool, bool> class Fn_,
  typename Key, typename Val> class ZmKVNode<Heap, 1, 0, Fn_, Key, Val> :
    public Key,
    public Fn_<ZmKVNode<Heap, 1, 0, Fn_, Key, Val>, Heap, 1, 0> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 1, 0, Fn_, Key, Val>, Heap, 1, 0>;

  ZuInline ZmKVNode() { }
  template <typename Key__>
  ZuInline ZmKVNode(Key__ &&key) : Key(ZuFwd<Key__>(key)) { }
  template <typename Key__, typename Val_>
  ZuInline ZmKVNode(Key__ &&key, Val_ &&val) :
    Key(ZuFwd<Key__>(key)), m_val(ZuFwd<Val_>(val)) { }

  ZuInline const Key &key() const { return *this; }
  ZuInline Key &key() { return *this; }
  ZuInline const Val &val() const { return m_val; }
  ZuInline Val &val() { return m_val; }

private:
  Val		m_val;
};
// node derives from key, value is ZuNull (common use case)
template <class Heap,
  template <typename, typename, bool, bool> class Fn_,
  typename Key> class ZmKVNode<Heap, 1, 0, Fn_, Key, ZuNull> :
    public Key,
    public Fn_<ZmKVNode<Heap, 1, 0, Fn_, Key, ZuNull>, Heap, 1, 0> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 1, 0, Fn_, Key, ZuNull>, Heap, 1, 0>;

  ZuInline ZmKVNode() { }
  template <typename ...Args>
  ZuInline ZmKVNode(Args &&... args) : Key{ZuFwd<Args>(args)...} { }

  ZuInline const Key &key() const { return *this; }
  ZuInline Key &key() { return *this; }
  ZuInline ZuNull &val() const { static ZuNull val; return val; }
};
// node derives from value
template <class Heap,
  template <typename, typename, bool, bool> class Fn_,
  typename Key, typename Val> class ZmKVNode<Heap, 0, 1, Fn_, Key, Val> :
    public Val,
    public Fn_<ZmKVNode<Heap, 0, 1, Fn_, Key, Val>, Heap, 0, 1> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 0, 1, Fn_, Key, Val>, Heap, 0, 1>;

  ZuInline ZmKVNode() { }
  template <typename Key__>
  ZuInline ZmKVNode(Key__ &&key) : m_key(ZuFwd<Key__>(key)) { }
  template <typename Key__, typename Val_>
  ZuInline ZmKVNode(Key__ &&key, Val_ &&val) :
    m_key(ZuFwd<Key__>(key)), Val(ZuFwd<Val_>(val)) { }

  ZuInline const Key &key() const { return m_key; }
  ZuInline Key &key() { return m_key; }
  ZuInline const Val &val() const { return *this; }
  ZuInline Val &val() { return *this; }

private:
  Key		m_key;
};
// node derives from both key and value
template <class Heap,
  template <typename, typename, bool, bool> class Fn_,
  typename Key, typename Val> class ZmKVNode<Heap, 1, 1, Fn_, Key, Val> :
    public Key, public Val,
    public Fn_<ZmKVNode<Heap, 1, 1, Fn_, Key, Val>, Heap, 1, 1> {
  ZmKVNode(const ZmKVNode &);
  ZmKVNode &operator =(const ZmKVNode &);	// prevent mis-use

public:
  using Fn = Fn_<ZmKVNode<Heap, 1, 1, Fn_, Key, Val>, Heap, 1, 1>;

  ZuInline ZmKVNode() { }
  template <typename Key__>
  ZuInline ZmKVNode(Key__ &&key) : Key(ZuFwd<Key__>(key)) { }
  template <typename Key__, typename Val_>
  ZuInline ZmKVNode(Key__ &&key, Val_ &&val) :
    Key(ZuFwd<Key__>(key)), Val(ZuFwd<Val_>(val)) { }

  ZuInline const Key &key() const { return *this; }
  ZuInline Key &key() { return *this; }
  ZuInline const Val &val() const { return *this; }
  ZuInline Val &val() { return *this; }
};

template <typename, bool> struct ZmNodePolicy_;
template <typename O>
struct ZmNodePolicy : public ZmNodePolicy_<O, ZuIsObject_<O>::OK> { };
// ref-counted nodes
template <typename O> struct ZmNodePolicy_<O, true> {
  using Object = O;
  template <typename T_> struct Ref { using T = ZmRef<T_>; };
  template <typename T> ZuInline void nodeRef(T &&o) { ZmREF(ZuFwd<T>(o)); }
  template <typename T> ZuInline void nodeDeref(T &&o) { ZmDEREF(ZuFwd<T>(o)); }
  template <typename T> ZuInline void nodeDelete(T &&) { }
};
// own nodes (with caller-specified base), delete if not returned to caller
template <typename O> struct ZmNodePolicy_<O, false> {
  using Object = O;
  template <typename T_> struct Ref { using T = T_ *; };
  template <typename T> ZuInline void nodeRef(T &&) { }
  template <typename T> ZuInline void nodeDeref(T &&) { }
  template <typename T> ZuInline void nodeDelete(T &&o) { delete ZuFwd<T>(o); }
};
// own nodes, delete if not returned to caller
template <> struct ZmNodePolicy<ZuNull> {
  using Object = ZuNull;
  template <typename T_> struct Ref { using T = T_ *; };
  template <typename T> ZuInline void nodeRef(T &&) { }
  template <typename T> ZuInline void nodeDeref(T &&) { }
  template <typename T> ZuInline void nodeDelete(T &&o) { delete ZuFwd<T>(o); }
};
// shadow nodes, never delete
template <> struct ZmNodePolicy<ZuShadow> {
  using Object = ZuNull;
  template <typename T_> struct Ref { using T = T_ *; };
  template <typename T> ZuInline void nodeRef(T &&) { }
  template <typename T> ZuInline void nodeDeref(T &&) { }
  template <typename T> ZuInline void nodeDelete(T &&) { }
};

#endif /* ZmNode_HPP */
