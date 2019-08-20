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

// ZTLS library main header

#ifndef ZTLSLib_HPP
#define ZTLSLib_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuLib.hpp>

#ifdef _WIN32

#ifdef ZI_EXPORTS
#define ZTLSAPI ZuExport_API
#define ZTLSExplicit ZuExport_Explicit
#else
#define ZTLSAPI ZuImport_API
#define ZTLSExplicit ZuImport_Explicit
#endif
#define ZTLSExtern extern ZTLSAPI

#else

#define ZTLSAPI
#define ZTLSExplicit
#define ZTLSExtern extern

#endif

#endif /* ZTLSLib_HPP */