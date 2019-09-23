//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ZiIP::(at your option) any later version.
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

// IO buffer (packet buffer)

#ifndef ZiIOBuf_HPP
#define ZiIOBuf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <ZiLib.hpp>
#endif

#include <ZmPolymorph.hpp>
#include <ZmHeap.hpp>

#include <ZtString.hpp>

#pragma pack(push, 2)
template <typename Heap> class ZiIOBuf_ : public Heap, public ZmPolymorph {
public:
  // TCP over Ethernet maximum payload is 1460 (without Jumbo frames)
  enum { Size = 1460 };

  ZuInline ZiIOBuf_() { }
  ZuInline ZiIOBuf_(void *owner_) : owner(owner_) { }
  ZuInline ZiIOBuf_(void *owner_, unsigned length_) :
      owner(owner_), length(length_) { alloc(length); }
  ZuInline ~ZiIOBuf_() { if (ZuUnlikely(jumbo)) ::free(jumbo); }

  ZuInline uint8_t *alloc(unsigned size_) {
    if (ZuLikely(size_ <= Size)) return data_;
    size = size_;
    return jumbo = (uint8_t *)::malloc(size_);
  }

  ZuInline void free(uint8_t *ptr) {
    if (ZuUnlikely(ptr != data_)) {
      if (ZuUnlikely(jumbo == ptr)) {
	jumbo = nullptr;
	size = Size;
	length = 0;
      }
      ::free(ptr);
    }
  }

  ZuInline uint8_t *data() {
    uint8_t *ptr = ZuUnlikely(jumbo) ? jumbo : data_;
    return ptr + skip;
  }

  ZuInline uint8_t *ensure(unsigned size_) {
    if (ZuLikely(size_ <= Size)) return data_;
    if (ZuUnlikely(size_ <= size)) return jumbo;
    uint8_t *old = jumbo;
    jumbo = (uint8_t *)::malloc(size_);
    size = size_;
    if (!length) return jumbo;
    if (ZuLikely(!old)) {
      memcpy(jumbo, data_, length);
      return jumbo;
    }
    memcpy(jumbo, old, length);
    ::free(old);
    return jumbo;
  }

  void		*owner = nullptr;
  uint8_t	*jumbo = nullptr;
  uint32_t	size = Size;
  uint32_t	length = 0;
  uint32_t	skip = 0;
  uint8_t	data_[Size];
};
#pragma pack(pop)
struct ZiIOBuf_HeapID {
  inline static const char *id() { return "ZiIOBuf"; }
};
typedef ZmHeap<ZiIOBuf_HeapID, sizeof(ZiIOBuf_<ZuNull>)> ZiIOBuf_Heap;
typedef ZiIOBuf_<ZiIOBuf_Heap> ZiIOBuf;

// generic ZiIOBuf receiver
class ZiIORx {
public:
  void connected() {
    m_buf = new ZiIOBuf(this);
  }

  void disconnected() { }

  // hdr(const uint8_t *ptr, unsigned len) should adjust ptr, len and return:
  //   +ve - length of hdr+body, or INT_MAX if insufficient data
  // body(const uint8_t *ptr, unsigned len) should return:
  //   0   - skip remaining data (used to defend against DOS)
  //   -ve - disconnect immediately
  //   +ve - length of hdr+body (can be <= that originally returned by hdr())
  template <typename Hdr, typename Body>
  int process(const uint8_t *data, unsigned rxLen, Hdr hdr, Body body) {
    std::cerr << ZtHexDump("process()", data, rxLen) << std::flush;
    unsigned oldLen = m_buf->length;
    unsigned len = oldLen + rxLen;
    auto rxData = m_buf->ensure(len);
    memcpy(rxData + oldLen, data, rxLen);
    m_buf->length = len;

    auto rxPtr = rxData;
    while (len >= 4) {
      auto frameLen = hdr(rxPtr, len);

      if (len < frameLen) break;

      frameLen = body(rxPtr, frameLen);

      if (ZuUnlikely(frameLen < 0)) return -1;
      if (!frameLen) return rxLen;

      rxPtr += frameLen;
      len -= frameLen;
    }
    if (len && rxPtr != rxData) {
      memmove(rxData, rxPtr, len);
      m_buf->length = len;
    } else
      m_buf->length = 0;
    return rxLen;
  }

private:
  ZmRef<ZiIOBuf>		m_buf;
};

#endif /* ZiIOBuf_HPP */
