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

// MxMD

#include <MxMDVersion.hpp>

#include "../version.h"

unsigned long MxMDVULong(int major, int minor, int patch)
{
  return MXMD_VULONG(major, minor, patch);
}

unsigned long MxMDVersion() { return MXMD_VERSION; }
const char *MxMDVerName() { return MXMD_VERNAME; }

int MxMDVMajor(unsigned long n)
{
  return MXMD_VMAJOR(n);
}
int MxMDVMinor(unsigned long n)
{
  return MXMD_VMINOR(n);
}
int MxMDVPatch(unsigned long n)
{
  return MXMD_VPATCH(n);
}

