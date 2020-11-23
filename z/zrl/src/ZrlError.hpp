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

#ifndef ZrlError_HPP
#define ZrlError_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZuStringN.hpp>

#include <zlib/ZePlatform.hpp>

namespace Zrl {

using ErrorStr = ZuStringN<120>;

}

#define ZrlLog(severity, op, result, error) \
  ZeLOG(severity, \
      Zrl::ErrorStr() << op << ' ' << Zi::resultName(result) << ' ' << error)
#define ZrlError(op, result, error) ZrlLog(Error, op, result, error)
#define ZrlWarning(op, result, error) ZrlLog(Warning, op, result, error)

#endif /* ZrlError_HPP */
