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

// command line interface - application callbacks

#ifndef ZrlApp_HPP
#define ZrlApp_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZuString.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZrlApp.hpp>

namespace Zrl {

using EnterFn = ZmFn<ZuString>;
using EndFn = ZmFn<>;
using SigFn = ZmFn<int>;
using ErrorFn = ZmFn<ZeError>;

using CompInitFn = ZmFn<ZuString>;	// (prefix)
using CompNextFn = ZmFn<ZuString &>;	// (suffix)

using HistSaveFn = ZmFn<unsigned, ZuString>;
using HistLoadFn = ZmFn<unsigned, ZuString &>;

struct App {
  EnterFn	enter;		// line entered
  EndFn		end;		// end of input (EOF)
  SigFn		sig;		// signal (^C ^\ ^Z)
  ErrorFn	error;		// I/O error

  CompInitFn	compInit;	// initialize possible completions of prefix
  CompNextFn	compNext;	// enumerate next completion in sequence

  HistSaveFn	histSave;	// save line in history with index
  HistLoadFn	histLoad;	// load line from history given index
};

} // Zrl

#endif /* ZrlApp_HPP */