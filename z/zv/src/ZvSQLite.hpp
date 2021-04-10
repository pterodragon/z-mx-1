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

#ifndef ZvSQLite_HPP
#define ZvSQLite_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

// #include <sqlite3.h>
#include <sqlite3ext.h> // includes sqlite3.h

// FIXME - use WAL (write ahead logging)
//
// FIXME - update ZvField usages to new interface (update all
// macro declare definitions in ZvField.hpp first, then update ZvCSV, Zdf etc.)

// FIXME - use custom functions to print/scan enums, flags, decimals
// (convert fixed -> decimal to store)

// FIXME - use ZvField run-time metadata to create tables + indices, bind, etc.
//
// FIXME - update uses of ZvFieldArray to provide Primary, Secondary, etc.
// run-time metadata (in addition to Key, key() at compile time)

#endif /* ZvSQLite_HPP */
