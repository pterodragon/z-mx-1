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

#ifndef ZvCmdNet_HPP
#define ZvCmdNet_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZuInt.hpp>
#include <zlib/ZuByteSwap.hpp>
#include <zlib/ZuBox.hpp>

#include <zlib/ZmAssert.hpp>

// get hdr from buffer
uint32_t ZvCmd_getHdr(const uint8_t *data) {
  return *reinterpret_cast<const ZuLittleEndian<uint32_t> *>(data);
}
// return hdr length
constexpr unsigned ZvCmd_hdrLen() { return sizeof(uint32_t); }
// construct hdr from body size, type
uint32_t ZvCmd_mkHdr(uint32_t size, uint32_t type) {
  ZmAssert(size < (1U<<28));
  ZmAssert(type < (1U<<3));
  return size | (type<<28);
}
// return body length
uint32_t ZvCmd_bodyLen(uint32_t hdr) {
  return hdr &~ (((uint32_t)0xf)<<28);
}
// return body type
uint32_t ZvCmd_bodyType(uint32_t hdr) {
  return hdr>>28;
}

#endif /* ZvCmdNet_HPP */
