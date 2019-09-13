//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ZiIP::(at your option) any later version.
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

// IO buffer (packet buffer)

#ifndef ZiIOBuf_HPP
#define ZiIOBuf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <ZiLib.hpp>
#endif

#include <ZmPolymorph.hpp>
#include <ZmHeap.hpp>

#pragma pack(push, 2)
template <typename Heap> class ZiIOBuf_ : public Heap, public ZmPolymorph {
public:
  // TCP over Ethernet maximum payload is 1460 (without Jumbo frames)
  enum { Size = 1460 };

  inline ZiIOBuf_() : owner(nullptr), length(0), skip(0) { }
  inline ZiIOBuf_(void *owner_, unsigned length_) :
      owner(owner_), length(length_), skip(0) { }
  ~ZiIOBuf_() { }

  void		*owner;
  uint16_t	length;
  uint16_t	skip;
  char		data[Size];
};
#pragma pack(pop)
struct ZiIOBuf_HeapID {
  inline static const char *id() { return "ZiIOBuf"; }
};
typedef ZmHeap<ZiIOBuf_HeapID, sizeof(ZiIOBuf_<ZuNull>)> ZiIOBuf_Heap;
typedef ZiIOBuf_<ZiIOBuf_Heap> ZiIOBuf;

#endif /* ZiIOBuf_HPP */
