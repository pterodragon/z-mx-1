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

// MxMD version

#ifndef MxMDVersion_HPP
#define MxMDVersion_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDExtern
#ifdef _WIN32
#ifdef MXMD_EXPORTS
#define MxMDExtern extern __declspec(dllexport)
#else
#define MxMDExtern extern __declspec(dllimport)
#endif
#else
#define MxMDExtern extern
#endif
#endif

extern "C" {
  MxMDExtern unsigned long MxMDVULong(int major, int minor, int patch);
  MxMDExtern unsigned long MxMDVersion();
  MxMDExtern const char *MxMDVerName();
  MxMDExtern int MxMDVMajor(unsigned long n);
  MxMDExtern int MxMDVMinor(unsigned long n);
  MxMDExtern int MxMDVPatch(unsigned long n);
};

#endif /* MxMDVersion_HPP */
