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

// command line interface

#ifndef ZrlGlobber_HPP
#define ZrlGlobber_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZuString.hpp>

#include <zlib/ZiGlob.hpp>

#include <zlib/ZrlApp.hpp>

namespace Zrl {

// filename glob completion
struct Globber {
  ZiGlob globber;
  void init(ZuString prefix) { globber.init(prefix); }
  bool next(ZuString &suffix) {
    if (suffix = globber.next()) {
      suffix.offset(globber.leafName().length());
      return true;
    }
    return false;
  }
  auto initFn() { return CompInitFn::Member<&init>::fn(this); }
  auto nextFn() { return CompNextFn::Member<&next>::fn(this); }
};


} // Zrl

#endif /* ZrlGlobber_HPP */
