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

// Z lz4 wrapper library

#ifndef Zlz4Lib_HPP
#define Zlz4Lib_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuLib.hpp>

#ifdef _WIN32

#ifdef ZLZ4_EXPORTS
#define Zlz4API ZuExport_API
#define Zlz4Explicit ZuExport_Explicit
#else
#define Zlz4API ZuImport_API
#define Zlz4Explicit ZuImport_Explicit
#endif
#define Zlz4Extern extern Zlz4API

#else

#define Zlz4API
#define Zlz4Explicit
#define Zlz4Extern extern

#endif

#endif /* Zlz4Lib_HPP */
