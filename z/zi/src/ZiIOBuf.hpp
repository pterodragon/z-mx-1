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

// IO buffer (packet buffer)

#ifndef ZiIOBuf_HPP
#define ZiIOBuf_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <zlib/ZiLib.hpp>
#endif

#include <zlib/ZuPrint.hpp>

#include <zlib/ZmPolymorph.hpp>
#include <zlib/ZmHeap.hpp>

#pragma pack(push, 2)
struct ZiAnyIOBuf : public ZmPolymorph {
  ZuInline ZiAnyIOBuf(uint32_t size_, uint32_t length_ = 0) :
      size(size_), length(length_) { }

  uint32_t	size;
  uint32_t	length;
};
template <unsigned Size_, typename Heap>
class ZiIOBuf_ : public Heap, public ZiAnyIOBuf {
public:
  enum { Size = Size_ };

  ZuInline ZiIOBuf_() : ZiAnyIOBuf{Size} { }
  ZuInline ZiIOBuf_(void *owner_) : ZiAnyIOBuf{Size}, owner(owner_) { }
  ZuInline ZiIOBuf_(void *owner_, unsigned length_) :
      ZiAnyIOBuf{Size, length_}, owner(owner_) { alloc(length_); }
  ZuInline ~ZiIOBuf_() { if (ZuUnlikely(jumbo)) ::free(jumbo); }

  uint8_t *alloc(unsigned size_) {
    if (ZuLikely(size_ <= Size)) return data_;
    size = size_;
    return jumbo = (uint8_t *)::malloc(size_);
  }

  void free(uint8_t *ptr) {
    if (ZuUnlikely(ptr != data_)) {
      if (ZuUnlikely(jumbo == ptr)) {
	jumbo = nullptr;
	size = Size;
	length = 0;
      }
      ::free(ptr);
    }
  }

  ZuInline const uint8_t *data() const {
    const uint8_t *ptr = ZuUnlikely(jumbo) ? jumbo : data_;
    return ptr + skip;
  }
  ZuInline uint8_t *data() {
    uint8_t *ptr = ZuUnlikely(jumbo) ? jumbo : data_;
    return ptr + skip;
  }

  // reallocate (while building buffer), preserving head and tail bytes
  uint8_t *realloc(
      unsigned oldLen, unsigned newLen,
      unsigned head, unsigned tail) {
    if (ZuLikely(newLen <= Size)) {
      if (tail) memmove(data_ + newLen - tail, data_ + oldLen - tail, tail);
      return data_;
    }
    if (ZuUnlikely(newLen <= size)) {
      if (tail) memmove(jumbo + newLen - tail, jumbo + oldLen - tail, tail);
      return jumbo;
    }
    uint8_t *old = jumbo;
    jumbo = (uint8_t *)::malloc(size = newLen);
    if (ZuLikely(!old)) old = data_;
    if (head) memcpy(jumbo, old, head);
    if (tail) memcpy(jumbo + newLen - tail, old + oldLen - tail, tail);
    if (ZuUnlikely(old != data_)) ::free(old);
    return jumbo;
  }

  // ensure at least size_ bytes in buffer, preserving any existing data
  uint8_t *ensure(unsigned size_) {
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

private:
  void append(const uint8_t *data, unsigned length_) {
    unsigned total = length + length_;
    memcpy(ensure(total) + length, data, length_);
    length = total;
  }

  template <typename U, typename R = void,
    bool B = ZuPrint<U>::Delegate> struct MatchPDelegate;
  template <typename U, typename R>
  struct MatchPDelegate<U, R, true> { using T = R; };
  template <typename U, typename R = void,
    bool B = ZuPrint<U>::Buffer> struct MatchPBuffer;
  template <typename U, typename R>
  struct MatchPBuffer<U, R, true> { using T = R; };

  template <typename P>
  typename MatchPDelegate<P>::T append(const P &p) {
    ZuPrint<P>::print(*this, p);
  }
  template <typename P>
  typename MatchPBuffer<P>::T append(const P &p) {
    unsigned length_ = ZuPrint<P>::length(p);
    length += ZuPrint<P>::print(
	reinterpret_cast<char *>(ensure(length + length_) + length),
	length_, p);
  }

  template <typename U, typename R = void,
    bool B = ZuConversion<U, char>::Same> struct MatchChar;
  template <typename U, typename R>
  struct MatchChar<U, R, true> { using T = R; };

  template <typename U, typename R = void,
    bool S = ZuTraits<U>::IsString &&
	     !ZuTraits<U>::IsWString &&
	     !ZuTraits<U>::IsPrimitive
    > struct MatchString;
  template <typename U, typename R>
  struct MatchString<U, R, true> { using T = R; };

  template <typename U, typename R = void,
    bool B = ZuTraits<U>::IsPrimitive &&
	     ZuTraits<U>::IsReal &&
	     !ZuConversion<U, char>::Same
    > struct MatchReal;
  template <typename U, typename R>
  struct MatchReal<U, R, true> { using T = R; };

  template <typename U, typename R = void,
    bool B = ZuPrint<U>::OK && !ZuPrint<U>::String> struct MatchPrint;
  template <typename U, typename R>
  struct MatchPrint<U, R, true> { using T = R; };

public:
  template <typename C>
  ZuInline typename MatchChar<C, ZiIOBuf_ &>::T operator <<(C c) {
    this->append(&c, 1);
    return *this;
  }
  template <typename S>
  ZuInline typename MatchString<S, ZiIOBuf_ &>::T operator <<(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    append(s.data(), s.length());
    return *this;
  }
  ZuInline ZiIOBuf_ &operator <<(const char *s_) {
    ZuString s(s_);
    append(s.data(), s.length());
    return *this;
  }
  template <typename R>
  ZuInline typename MatchReal<R, ZiIOBuf_ &>::T operator <<(const R &r) {
    append(ZuBoxed(r));
    return *this;
  }
  template <typename P>
  ZuInline typename MatchPrint<P, ZiIOBuf_ &>::T operator <<(const P &p) {
    append(p);
    return *this;
  }

  struct Traits : public ZuBaseTraits<ZiIOBuf_> {
    using Elem = char;
    enum {
      IsCString = 0, IsString = 1, IsWString = 0, IsHashable = 1
    };
    static const char *data(const ZiIOBuf_ &buf) {
      return reinterpret_cast<const char *>(buf.data());
    }
    static unsigned length(const ZiIOBuf_ &buf) {
      return buf.length;
    }
  };
  friend Traits ZuTraitsType(ZiIOBuf_ *);

  void		*owner = nullptr;
  uint8_t	*jumbo = nullptr;
  uint32_t	skip = 0;
  uint8_t	data_[Size];
};
#pragma pack(pop)

struct ZiIOBuf_HeapID {
  static constexpr const char *id() { return "ZiIOBuf"; }
};
template <unsigned Size>
using ZiIOBuf_Heap = ZmHeap<ZiIOBuf_HeapID, sizeof(ZiIOBuf_<Size, ZuNull>)>;
 
// TCP over Ethernet maximum payload is 1460 (without Jumbo frames)
template <unsigned Size = 1460>
using ZiIOBuf = ZiIOBuf_<Size, ZiIOBuf_Heap<Size>>;

// generic ZiIOBuf receiver
template <typename Buf_> class ZiIORx {
public:
  using Buf = Buf_;

  void connected() { m_buf = new Buf(this); }
  void disconnected() { }

  // hdr(const uint8_t *ptr, unsigned len) should adjust ptr, len and return:
  //   +ve - length of hdr+body, or INT_MAX if insufficient data
  // body(const uint8_t *ptr, unsigned len) should return:
  //   0   - skip remaining data (used to defend against DOS)
  //   -ve - disconnect immediately
  //   +ve - length of hdr+body (can be <= that originally returned by hdr())
  template <typename Hdr, typename Body>
  int process(const uint8_t *data, unsigned rxLen, Hdr hdr, Body body) {
    unsigned oldLen = m_buf->length;
    unsigned len = oldLen + rxLen;
    auto rxData = m_buf->ensure(len);
    memcpy(rxData + oldLen, data, rxLen);
    m_buf->length = len;

    auto rxPtr = rxData;
    while (len >= 4) {
      int frameLen = hdr(rxPtr, len);

      if (frameLen < 0 || len < (unsigned)frameLen) break;

      frameLen = body(rxPtr, frameLen);

      if (ZuUnlikely(frameLen < 0)) return -1; // error
      if (!frameLen) return rxLen; // EOF - discard remainder

      rxPtr += frameLen;
      len -= frameLen;
    }
    if (len && rxPtr != rxData) memmove(rxData, rxPtr, len);
    m_buf->length = len;
    return rxLen;
  }

private:
  ZmRef<Buf>		m_buf;
};

#endif /* ZiIOBuf_HPP */
