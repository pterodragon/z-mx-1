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

// function introspection
//
// below example uses SFINAE to invoke toString only if a class defines it
//
// ZuCan(toString, CanToString); // defines CanToString template
// ...
// template <typename U> inline static typename
//   ZuIfT<CanToString<U, std::string (U::*)()>::OK, std::string>::T
//     toString(U *u) { return u->toString(); } // can
// template <typename U> inline static typename
//   ZuIfT<!CanToString<U, std::string (U::*)()>::OK, std::string>::T
//     toString(U *) { return ""; } // cannot
// ...
// T *t;
// std::string s = toString(t); // works regardless if T defines toString

#ifndef ZuCan_HPP
#define ZuCan_HPP

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244 4800)
#endif

#define ZuCan(fn, name) \
template <typename ZuCan_T, typename ZuCan_Fn> \
class name { \
  typedef char Small; \
  struct Big { char _[2]; }; \
  template <typename _, _> struct Check; \
  template <typename _> \
  /* __ parameter below is named due to VS2010 bug */ \
  static Small test(Check<ZuCan_Fn, &_::fn> *__); \
  template <typename _> \
  static Big test(...); \
public: \
  name(); /* keep gcc quiet */ \
  enum _ { \
    OK = sizeof(test<ZuCan_T>(0)) == sizeof(Small) \
  }; \
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZuCan_HPP */
