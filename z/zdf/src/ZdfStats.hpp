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

// Data Series Statistical Methods

#ifndef ZdfStats_HPP
#define ZdfStats_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

namespace Zdf {

namespace pbds = __gnu_pbds;

template <typename T, typename Cmp = ZuCmp<T>>
template <typename T> struct LessEqual<T, ZuCmp<T>> {
  constexpr bool operator()(const T &v1, const T &v2) const {
    return v1 <= v2;
  }
};

template <typename T_, typename Cmp_ = ZuCmp<T>>
class StatsTree {
public:
  using T = T_;
  using Cmp = Cmp_;
  using Tree = pbds::tree<
    T,
    std::null_type,
    LessEqual<T, Cmp>,
    pbds::rb_tree_tag,
    pbds::tree_order_statistics_node_update>;


};

} // namespace Zdf

#endif /* ZdfStats_HPP */
