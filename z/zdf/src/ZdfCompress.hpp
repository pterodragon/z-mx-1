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

// 64bit data series compression
// - byte-aligned
// - signed data
// - Huffman-coded length prefix
// - single-byte RLE
// - absolute, delta (first derivative), and delta-of-delta (second derivative)

#ifndef ZdfRW_HPP
#define ZdfRW_HPP

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuInt.hpp>
#include <ZuByteSwap.hpp>

namespace ZdfCompress {

class Decoder {
public:
  Decoder() : m_pos(nullptr), m_end(nullptr) { }
  Decoder(const Decoder &r) :
      m_pos(r.m_pos), m_end(r.m_end),
      m_prev(r.m_prev), m_rle(r.m_rle), m_count(r.m_count) { }
  Decoder &operator =(const Decoder &r) {
    this->~Decoder(); // nop
    new (this) Decoder{r};
    return *this;
  }
  Decoder(Decoder &&r) noexcept :
      m_pos(r.m_pos), m_end(r.m_end),
      m_prev(r.m_prev), m_rle(r.m_rle), m_count(r.m_count) { }
  Decoder &operator =(Decoder &&r) noexcept {
    this->~Decoder(); // nop
    new (this) Decoder{ZuMv(r)};
    return *this;
  }
  ~Decoder() = default;

  Decoder(const uint8_t *start, const uint8_t *end) :
      m_pos(start), m_end(end) { }

  bool operator !() const { return !m_pos; }
  ZuOpBool

  ZuInline const uint8_t *pos() const { return m_pos; }
  ZuInline const uint8_t *end() const { return m_end; }
  ZuInline unsigned count() const { return m_count; }

  bool seek(unsigned count) {
    while (count) {
      if (m_rle) {
	if (m_rle >= count) {
	  m_rle -= count;
	  m_count += count;
	  return true;
	}
	m_count += m_rle;
	count -= m_rle;
	m_rle = 0;
      } else {
	if (!read_(nullptr)) return false;
	--count;
      }
    }
    return true;
  }

  template <typename L>
  bool search(L l) {
    int64_t value;
    const uint8_t *origPos;
    do {
      if (m_rle) {
	if (l(m_prev)) return true;
	m_count += m_rle;
	m_rle = 0;
      }
      origPos = m_pos;
      if (!read_(&value)) return false;
    } while (!l(value));
    m_pos = origPos;	// back-up by one value
    m_rle = 0;		// just in case we began RLE
    --m_count;
    return true;
  }

  bool read(int64_t &value) {
    if (m_rle) {
      --m_rle;
      value = m_prev;
      ++m_count;
      return true;
    }
    return read_(&value);
  }

private:
  bool read_(int64_t *value_) {
    if (ZuUnlikely(m_pos >= m_end)) return false;
    unsigned byte = *m_pos;
    if (byte & 0x80) {
      ++m_pos;
      m_rle = byte & 0x7f;
      if (value_) *value_ = m_prev;
      ++m_count;
      return true;
    }
    int64_t value;
    if (!(byte & 0x20)) {				// 5 bits
      ++m_pos;
      value = byte & 0x1f;
    } else if ((byte & 0x30) == 0x20) {			// 12 bits
      if (m_pos + 2 > m_end) return false;
      ++m_pos;
      value = byte & 0xf;
      value |= static_cast<int64_t>(*m_pos++)<<4;
    } else if ((byte & 0x38) == 0x30) {			// 19 bits
      if (m_pos + 3 > m_end) return false;
      ++m_pos;
      value = byte & 0x7;
      value |= static_cast<int64_t>(*m_pos++)<<3;
      value |= static_cast<int64_t>(*m_pos++)<<11;
    } else if ((byte & 0x3c) == 0x38) {			// 26 bits
      if (m_pos + 4 > m_end) return false;
      ++m_pos;
      value = byte & 0x3;
      value |= static_cast<int64_t>(*m_pos++)<<2;
      value |= static_cast<int64_t>(*m_pos++)<<10;
      value |= static_cast<int64_t>(*m_pos++)<<18;
    } else if ((byte & 0x3e) == 0x3c) {			// 33 bits
      if (m_pos + 5 > m_end) return false;
      ++m_pos;
      value = byte & 0x1;
      value |= static_cast<int64_t>(*m_pos++)<<1;
      value |= static_cast<int64_t>(*m_pos++)<<9;
      value |= static_cast<int64_t>(*m_pos++)<<17;
      value |= static_cast<int64_t>(*m_pos++)<<25;
    } else if ((byte & 0x3f) == 0x3e) {			// 40 bits
      if (m_pos + 6 > m_end) return false;
      ++m_pos;
      value = *m_pos++;
      value |= static_cast<int64_t>(*m_pos++)<<8;
      value |= static_cast<int64_t>(*m_pos++)<<16;
      value |= static_cast<int64_t>(*m_pos++)<<24;
      value |= static_cast<int64_t>(*m_pos++)<<32;
    } else {						// 64 bits
      if (m_pos + 9 > m_end) return false;
      ++m_pos;
      // potentially misaligned
      value = *reinterpret_cast<const ZuLittleEndian<uint64_t> *>(m_pos);
      m_pos += 8;
    }
    if (byte & 0x40) value = ~value;
    if (value_) *value_ = m_prev = value;
    ++m_count;
    return true;
  }

private:
  const uint8_t	*m_pos, *m_end;
  int64_t	m_prev = 0;
  unsigned	m_rle = 0;
  unsigned	m_count = 0;
};

template <typename Base = Decoder>
class DeltaDecoder : public Base {
public:
  DeltaDecoder() : Base() { }
  DeltaDecoder(const DeltaDecoder &r) : Base(r), m_base(r.m_base) { }
  DeltaDecoder &operator =(const DeltaDecoder &r) {
    this->~DeltaDecoder(); // nop
    new (this) DeltaDecoder{r};
    return *this;
  }
  DeltaDecoder(DeltaDecoder &&r) :
      Base(static_cast<Base &&>(r)), m_base(r.m_base) { }
  DeltaDecoder &operator =(DeltaDecoder &&r) {
    this->~DeltaDecoder(); // nop
    new (this) DeltaDecoder{ZuMv(r)};
    return *this;
  }

  DeltaDecoder(const uint8_t *start, const uint8_t *end) :
      Base(start, end) { }

  template <typename L>
  bool search(L l) {
    return Base::search([this, l = ZuMv(l)](int64_t skip) {
      int64_t value = m_base + skip;
      if (l(value)) return true;
      m_base = value;
      return false;
    });
  }

  bool read(int64_t &value_) {
    int64_t value;
    if (ZuUnlikely(!Base::read(value))) return false;
    value_ = (m_base += value);
    return true;
  }

private:
  int64_t		m_base = 0;
};

class Encoder {
  Encoder(const Encoder &) = delete;
  Encoder &operator =(const Encoder &) = delete;

public:
  Encoder(uint8_t *start, uint8_t *end) : m_pos(start), m_end(end) { }

  Encoder() : m_pos(nullptr), m_end(nullptr) { }
  Encoder(Encoder &&w) noexcept :
      m_pos(w.m_pos), m_end(w.m_end),
      m_rle(w.m_rle), m_prev(w.m_prev), m_count(w.m_count) {
    w.m_pos = nullptr;
    w.m_end = nullptr;
    w.m_rle = nullptr;
    w.m_prev = 0;
    w.m_count = 0;
  }
  Encoder &operator =(Encoder &&w) noexcept {
    this->~Encoder(); // nop
    new (this) Encoder{ZuMv(w)};
    return *this;
  }
  ~Encoder() = default;

  ZuInline uint8_t *pos() const { return m_pos; }
  ZuInline uint8_t *end() const { return m_end; }
  ZuInline unsigned count() const { return m_count; }

  bool operator !() const { return !m_pos; }
  ZuOpBool

  bool write(int64_t value_) {
    if (ZuLikely(value_ == m_prev)) {
      if (m_rle) {
	if (++*m_rle == 0xff) m_rle = nullptr;
	++m_count;
	return true;
      }
      if (m_pos >= m_end) return false;
      *(m_rle = m_pos++) = 0x80;
      ++m_count;
      return true;
    } else
      m_rle = nullptr;
    unsigned negative = value_ < 0;
    int64_t value = value_;
    if (negative) value = ~value;
    unsigned n = !value ? 0 : 64U - __builtin_clzll(value);
    n = (n + 1) / 7;
    if (n >= 6) n = 8;
    if (m_pos + n >= m_end) return false;
    if (n == 8) n = 6;
    negative <<= 6;
    switch (n) {
      case 0:							// 5 bits
	*m_pos++ = negative | value;
	break;
      case 1:							// 12 bits
	*m_pos++ = negative | 0x20 | (value & 0xf);
	value >>= 4; *m_pos++ = value;
	break;
      case 2:							// 19 bits
	*m_pos++ = negative | 0x30 | (value & 0x7);
	value >>= 3; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value;
	break;
      case 3:							// 26 bits
	*m_pos++ = negative | 0x38 | (value & 0x3);
	value >>= 2; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value;
	break;
      case 4:							// 33 bits
	*m_pos++ = negative | 0x3c | (value & 0x1);
	value >>= 1; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value;
	break;
      case 5:							// 40 bits
	*m_pos++ = negative | 0x3e;
	*m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value & 0xff;
	value >>= 8; *m_pos++ = value;
	break;
      case 6:							// 64 bits
	*m_pos++ = negative | 0x3f;
	// potentially misaligned
	*reinterpret_cast<ZuLittleEndian<uint64_t> *>(m_pos) = value;
	m_pos += 8;
	break;
    }
    m_prev = value_;
    ++m_count;
    return true;
  }

  ZuInline int64_t last() const { return m_prev; }

private:
  uint8_t	*m_pos, *m_end;
  uint8_t	*m_rle = nullptr;
  int64_t	m_prev = 0;
  unsigned	m_count = 0;
};

template <typename Base = Encoder>
class DeltaEncoder : public Base {
  DeltaEncoder(const DeltaEncoder &) = delete;
  DeltaEncoder &operator =(const DeltaEncoder &) = delete;

public:
  DeltaEncoder(uint8_t *start, uint8_t *end) : Base(start, end) { }

  DeltaEncoder() { }
  DeltaEncoder(DeltaEncoder &&w) noexcept :
      Base{static_cast<Base &&>(w)}, m_base{w.m_base} {
    w.m_base = 0;
  }
  DeltaEncoder &operator =(DeltaEncoder &&w) noexcept {
    this->~DeltaEncoder(); // nop
    new (this) DeltaEncoder{ZuMv(w)};
    return *this;
  }
  ~DeltaEncoder() = default;

  bool write(int64_t value) {
    int64_t delta = value - m_base;
    if (ZuUnlikely(!Base::write(delta))) return false;
    m_base = value;
    return true;
  }

  ZuInline int64_t last() const { return m_base + Base::last(); }

private:
  int64_t	m_base = 0;
};

} // namespace ZdfCompress

#endif /* ZdfRW_HPP */
