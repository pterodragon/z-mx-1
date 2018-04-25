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

// MxBase version

#ifndef MxBaseVersion_HPP
#define MxBaseVersion_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxBaseExtern
#ifdef _WIN32
#ifdef MXBASE_EXPORTS
#define MxBaseExtern extern __declspec(dllexport)
#else
#define MxBaseExtern extern __declspec(dllimport)
#endif
#else
#define MxBaseExtern extern
#endif
#endif

extern "C" {
  MxBaseExtern unsigned long MxBaseVULong(int major, int minor, int patch);
  MxBaseExtern unsigned long MxBaseVersion();
  MxBaseExtern const char *MxBaseVerName();
  MxBaseExtern int MxBaseVMajor(unsigned long n);
  MxBaseExtern int MxBaseVMinor(unsigned long n);
  MxBaseExtern int MxBaseVPatch(unsigned long n);
};

#endif /* MxBaseVersion_HPP */
