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

// compile-time tag for intrusively reference-counted objects

#ifndef ZuObject__HPP
#define ZuObject__HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZuLib_HPP
#include <ZuLib.hpp>
#endif

#include <stddef.h>

#include <ZuConversion.hpp>

struct ZuObject_ { }; // tag

template <typename T> struct ZuIsObject_ {
  enum { OK = ZuConversion<ZuObject_, T>::Is };
};
// SFINAE
template <typename T, typename R = void, bool OK = ZuIsObject_<T>::OK>
struct ZuIsObject;
template <typename T_, typename R> struct ZuIsObject<T_, R, 1> {
  typedef R T;
};
template <typename T, typename R = void, bool OK = !ZuIsObject_<T>::OK>
struct ZuNotObject;
template <typename T_, typename R> struct ZuNotObject<T_, R, 1> {
  typedef R T;
};

#endif /* ZuObject__HPP */
