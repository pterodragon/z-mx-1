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

// concrete generic msg ID
//
// Link ID - ZuID (union of 8-byte string with uint64)
// SeqNo - uint64

#ifndef ZvMsgID_HPP
#define ZvMsgID_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuID.hpp>
#include <zlib/ZuPrint.hpp>

#include <zlib/ZvSeqNo.hpp>

struct ZvMsgID {
  ZuID		linkID;
  ZvSeqNo	seqNo;

  void update(const ZvMsgID &u) {
    linkID.update(u.linkID);
    seqNo.update(u.seqNo);
  }
  template <typename S> void print(S &s) const {
    s << "linkID=" << linkID << " seqNo=" << seqNo;
  }
};
template <> struct ZuPrint<ZvMsgID> : public ZuPrintFn { };

#endif /* ZvMsgID_HPP */
