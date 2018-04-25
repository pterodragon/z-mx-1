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

// backtrace printing

#ifndef ZmBackTrace_print_HPP
#define ZmBackTrace_print_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#ifndef ZmBackTrace__HPP
#include <ZmBackTrace_.hpp>
#endif

#include <ZuTraits.hpp>
#include <ZuPrint.hpp>
#include <ZuStringN.hpp>

#include <ZmStream.hpp>

ZmExtern void ZmBackTrace_print(ZmStream &s, const ZmBackTrace &bt);

// generic printing
template <> struct ZuPrint<ZmBackTrace> : public ZuPrintDelegate<ZmBackTrace> {
  ZuInline static void print(ZmStream &s, const ZmBackTrace &bt) {
    ZmBackTrace_print(s, bt);
  }
  template <typename S>
  ZuInline static void print(S &s_, const ZmBackTrace &bt) {
    ZmStream s(s_);
    ZmBackTrace_print(s, bt);
  }
};

#endif /* ZmBackTrace_print_HPP */
