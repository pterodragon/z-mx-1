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

// Mx CSV common definitions

#ifndef MxCSV_HPP
#define MxCSV_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseLib_HPP
#include <MxBaseLib.hpp>
#endif

#include <ZiIP.hpp>

#include <ZvCSV.hpp>

#include <MxBase.hpp>

typedef ZvCSVColumn<ZvCSVColType::Bool, MxBool> MxBoolCol;
typedef ZvCSVColumn<ZvCSVColType::Int, MxInt> MxIntCol;
typedef ZvCSVColumn<ZvCSVColType::Int, MxUInt> MxUIntCol;
typedef ZvCSVColumn<ZvCSVColType::Float, MxFloat> MxFloatCol;
typedef ZvCSVColumn<ZvCSVColType::Time, MxDateTime> TimeCol;
typedef ZvCSVColumn<ZvCSVColType::String, MxSymString> MxSymStringCol;
typedef ZvCSVColumn<ZvCSVColType::String, MxIDString> MxIDStringCol;
template <typename Map> using MxEnumCol = ZvCSVEnumColumn<MxEnum, Map>;
template <typename Map> using MxFlagsCol = ZvCSVFlagsColumn<MxFlags, Map>;
template <typename Map> using MxFlags64Col = ZvCSVFlagsColumn<MxFlags64, Map>;

class MxIPCol : public ZvCSVColumn<ZvCSVColType::Func, ZiIP> {
  typedef ZvCSVColumn<ZvCSVColType::Func, ZiIP> Base;
  typedef Base::ParseFn ParseFn;
  typedef Base::PlaceFn PlaceFn;
public:
  template <typename ID> inline MxIPCol(ID &&id, int offset) noexcept :
    Base(ZuFwd<ID>(id), offset,
	ParseFn::Ptr<&MxIPCol::parse>::fn(),
	PlaceFn::Ptr<&MxIPCol::place>::fn()) { }
  static void parse(ZiIP *i, ZuString b) { *i = b; }
  static void place(ZtArray<char> &b, const ZiIP *i) { b = i->string(); }
};

#endif /* MxCSV_HPP */
