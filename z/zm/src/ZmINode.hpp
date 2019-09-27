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

// container node (used by ZmList, ZmPQueue, ...)

// ZmINode - node containing single item (no key/value)

#ifndef ZmINode_HPP
#define ZmINode_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuNull.hpp>

template <
  class Heap, bool NodeIsItem,
  template <typename, typename, bool> class Fn, typename Item> class ZmINode;
// node contains item (default)
template <
  class Heap, template <typename, typename, bool> class Fn_, typename Item>
class ZmINode<Heap, 0, Fn_, Item> :
    public Fn_<ZmINode<Heap, 0, Fn_, Item>, Heap, 0> {
  ZmINode(const ZmINode &);
  ZmINode &operator =(const ZmINode &);	// prevent mis-use

public:
  typedef Fn_<ZmINode<Heap, 0, Fn_, Item>, Heap, 0> Fn;

  ZuInline ZmINode() { }
  template <typename ...Args>
  ZuInline ZmINode(Args &&... args) : m_item{ZuFwd<Args>(args)...} { }

  ZuInline const Item &item() const { return m_item; }
  ZuInline Item &item() { return m_item; }

private:
  Item			m_item;
};
// node derives from item
template <
  class Heap, template <typename, typename, bool> class Fn_, typename Item>
class ZmINode<Heap, 1, Fn_, Item> :
    public Item, public Fn_<ZmINode<Heap, 1, Fn_, Item>, Heap, 1> {
  ZmINode(const ZmINode &);
  ZmINode &operator =(const ZmINode &);	// prevent mis-use

public:
  typedef Fn_<ZmINode<Heap, 1, Fn_, Item>, Heap, 1> Fn;

  ZuInline ZmINode() { }
  template <typename ...Args>
  ZuInline ZmINode(Args &&... args) : Item{ZuFwd<Args>(args)...} { }

  ZuInline const Item &item() const { return *this; }
  ZuInline Item &item() { return *this; }
};

#endif /* ZmINode_HPP */
