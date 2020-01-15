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

#ifndef ZvTelClient_HPP
#define ZvTelClient_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZvTelemetry.hpp>

#include <zlib/telreq_fbs.h>

namespace ZvTelemetry {

using CliFn = ZmFn<const fbs::Telemetry *>;
struct CliWatch_ {
  WatchKey		key;
  void			*link;	// ZvCmdCliLink<App> *
  CliFn			fn;
};
struct CliWatchKeyAccessor : public ZuAccessor<CliWatch_, WatchKey> {
  static const WatchKey &value(const CliWatch_ &watch) {
    return watch.key;
  }
};
using CliWatchTree =
  ZmRBTree<CliWatch_,
    ZmRBTreeIndex<CliWatchKeyAccessor,
      ZmRBTreeNodeIsKey<true,
	ZmRBTreeLock<ZmRWLock> > > >;
using CliWatch = CliWatchTree::Node;

} // ZvTelemetry

#endif /* ZvTelClient_HPP */
