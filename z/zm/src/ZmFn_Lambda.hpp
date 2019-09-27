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

// extend ZmFn to work with heap-allocated lambdas

#ifndef ZmFn_Lambda_HPP
#define ZmFn_Lambda_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmHeap.hpp>

// lambda wrapper object (heap-allocated)
template <typename L, typename HeapID> struct ZmLambda {
  template <typename Heap>
  struct T_ : public Heap, public ZmPolymorph, public L {
    T_() = delete;
    T_(const T_ &) = delete;
    T_ &operator =(const T_ &) = delete;
    T_(T_ &&) = delete;
    T_ &operator =(T_ &&) = delete;
    template <typename L_> ZuInline T_(L_ &&l) : L(ZuFwd<L_>(l)) { }
  };
  typedef T_<ZmHeap<HeapID, sizeof(T_<ZuNull>)> > T;
};

#endif /* ZmFn_Lambda_HPP */
