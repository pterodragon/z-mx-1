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

#ifndef ZuSeries_HPP
#define ZuSeries_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <ZuInt.hpp>

namespace ZuSeries {
  class Reader_ {
  public:
    Reader_(const uint8_t *start, const uint8_t *end) :
	m_start(start), m_end(end) { }

    ZuInline const uint8_t *start() const { return m_start; }
    ZuInline const uint8_t *end() const { return m_end; }

  protected:
    template <bool RLE = true>
    bool readValue(int64_t &value_) {
      if (m_rle) {
	--m_rle;
	value_ = m_prev;
	return true;
      }
      if (ZuUnlikely(m_start >= m_end)) return false;
      unsigned byte = *m_start;
      if (byte & 0x80) {
	if constexpr (!RLE) return false;
	++m_start;
	m_rle = byte & 0x7f;
	value_ = m_prev;
	return true;
      }
      int64_t value;
      if (!(byte & 0x20)) { // 5 bits
	++m_start;
	value = byte & 0x1f;
      } else if ((byte & 0x30) == 0x20) { // 12 bits
	if (m_start + 1 >= m_end) return false;
	++m_start;
	value = byte & 0xf;
	value |= static_cast<int64_t>(*m_start++)<<4;
      } else if ((byte & 0x38) == 0x30) { // 19 bits
	if (m_start + 2 >= m_end) return false;
	++m_start;
	value = byte & 0x7;
	value |= static_cast<int64_t>(*m_start++)<<3;
	value |= static_cast<int64_t>(*m_start++)<<11;
      } else if ((byte & 0x3c) == 0x38) { // 26 bits
	if (m_start + 3 >= m_end) return false;
	++m_start;
	value = byte & 0x3;
	value |= static_cast<int64_t>(*m_start++)<<2;
	value |= static_cast<int64_t>(*m_start++)<<10;
	value |= static_cast<int64_t>(*m_start++)<<18;
      } else if ((byte & 0x3e) == 0x3c) { // 33 bits
	if (m_start + 4 >= m_end) return false;
	++m_start;
	value = byte & 0x1;
	value |= static_cast<int64_t>(*m_start++)<<1;
	value |= static_cast<int64_t>(*m_start++)<<9;
	value |= static_cast<int64_t>(*m_start++)<<17;
	value |= static_cast<int64_t>(*m_start++)<<25;
      } else if ((byte & 0x3f) == 0x3e) { // 40 bits
	if (m_start + 5 >= m_end) return false;
	++m_start;
	value = *m_start++;
	value |= static_cast<int64_t>(*m_start++)<<8;
	value |= static_cast<int64_t>(*m_start++)<<16;
	value |= static_cast<int64_t>(*m_start++)<<24;
	value |= static_cast<int64_t>(*m_start++)<<32;
      } else { // 64 bits
	if (m_start + 8 >= m_end) return false;
	++m_start;
	value = *reinterpret_cast<const uint64_t *>(m_start); // misaligned
	m_start += 8;
      }
      if (byte & 0x40) value = ~value;
      value_ = m_prev = value;
      return true;
    }

  private:
    const uint8_t	*m_start, *m_end;
    int64_t		m_prev = 0;
    unsigned		m_rle = 0;
  };

  class Reader : public Reader_ {
    typedef bool (Reader::*ReadFn)(int64_t &);

  public:
    Reader(const uint8_t *start, const uint8_t *end) : Reader_(start, end) { }

  private:
    ReadFn		m_readFn = &Reader::readFirst;

  public:
    ZuInline bool read(int64_t &value) {
      bool ok = (this->*m_readFn)(value);
      return ok;
    }

  private:
    bool readFirst(int64_t &value) {
      if (ZuUnlikely(!readValue<false>(value))) return false;
      m_readFn = &Reader::readNext;
      return true;
    }
    ZuInline bool readNext(int64_t &value) {
      return readValue(value);
    }
  };

  class DeltaReader : public Reader_ {
    typedef bool (DeltaReader::*ReadFn)(int64_t &);

  public:
    DeltaReader(const uint8_t *start, const uint8_t *end) :
	Reader_(start, end) { }

  private:
    ReadFn		m_readFn = &DeltaReader::readFirst;
    int64_t		m_base = 0;

  public:
    ZuInline bool read(int64_t &value) {
      return (this->*m_readFn)(value);
    }

  private:
    bool readFirst(int64_t &value_) {
      int64_t value;
      if (ZuUnlikely(!readValue<false>(value))) return false;
      value_ = m_base = value;
      m_readFn = &DeltaReader::readNext;
      return true;
    }
    bool readNext(int64_t &value_) {
      int64_t value;
      if (ZuUnlikely(!readValue(value))) return false;
      m_base += value;
      value_ = m_base;
      return true;
    }
  };

  class Delta2Reader : public Reader_ {
    typedef bool (Delta2Reader::*ReadFn)(int64_t &);

  public:
    Delta2Reader(const uint8_t *start, const uint8_t *end) :
	Reader_(start, end) { }

  private:
    ReadFn		m_readFn = &Delta2Reader::readFirst;
    int64_t		m_base = 0;
    int64_t		m_delta = 0;

  public:
    ZuInline bool read(int64_t &value) {
      return (this->*m_readFn)(value);
    }

  private:
    bool readFirst(int64_t &value_) {
      int64_t value;
      if (ZuUnlikely(!readValue<false>(value))) return false;
      value_ = m_base = value;
      m_readFn = &Delta2Reader::readSecond;
      return true;
    }
    bool readSecond(int64_t &value_) {
      int64_t value;
      if (ZuUnlikely(!readValue<false>(value))) return false;
      m_base += m_delta = value;
      value_ = m_base;
      m_readFn = &Delta2Reader::readNext;
      return true;
    }
    bool readNext(int64_t &value_) {
      int64_t value;
      if (ZuUnlikely(!readValue(value))) return false;
      m_delta += value;
      m_base += m_delta;
      value_ = m_base;
      return true;
    }
  };

  class Writer_ {
  public:
    Writer_(uint8_t *start, uint8_t *end) : m_start(start), m_end(end) { }

    ZuInline uint8_t *start() const { return m_start; }
    ZuInline uint8_t *end() const { return m_end; }

  private:
    uint8_t	*m_start, *m_end;
    uint8_t	*m_rle = nullptr;
    int64_t	m_prev = 0;

  protected:
    template <bool RLE = true>
    bool writeValue(int64_t value_) {
      if constexpr (RLE) {
	if (ZuLikely(value_ == m_prev)) {
	  if (m_rle) {
	    if (++*m_rle == 0xff) m_rle = nullptr;
	    return true;
	  }
	  if (m_start >= m_end) return false;
	  *(m_rle = m_start++) = 0x80;
	  return true;
	} else
	  m_rle = nullptr;
      }
      unsigned negative = value_ < 0;
      int64_t value = value_;
      if (negative) value = ~value;
      unsigned n = !value ? 0 : 64U - __builtin_clzll(value);
      n = (n + 1) / 7;
      if (n >= 6) n = 8;
      if (m_start + n >= m_end) return false;
      if (n == 8) n = 6;
      negative <<= 6;
      switch (n) {
	case 0: // 5 bits
	  *m_start++ = negative | value;
	  break;
	case 1: // 12 bits
	  *m_start++ = negative | 0x20 | (value & 0xf);
	  value >>= 4; *m_start++ = value;
	  break;
	case 2: // 19 bits
	  *m_start++ = negative | 0x30 | (value & 0x7);
	  value >>= 3; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value;
	  break;
	case 3: // 26 bits
	  *m_start++ = negative | 0x38 | (value & 0x3);
	  value >>= 2; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value;
	  break;
	case 4: // 33 bits
	  *m_start++ = negative | 0x3c | (value & 0x1);
	  value >>= 1; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value;
	  break;
	case 5: // 40 bits
	  *m_start++ = negative | 0x3e;
	  *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value & 0xff;
	  value >>= 8; *m_start++ = value;
	case 6: // 64 bits
	  *m_start++ = negative | 0x3f;
	  *reinterpret_cast<uint64_t *>(m_start) = value; // misaligned
	  m_start += 8;
	  break;
      }
      m_prev = value_;
      return true;
    }
  };

  class Writer : public Writer_ {
    typedef bool (Writer::*WriteFn)(int64_t);

  public:
    Writer(uint8_t *start, uint8_t *end) : Writer_(start, end) { }

  private:
    WriteFn	m_writeFn = &Writer::writeFirst;

  public:
    ZuInline bool write(int64_t value) {
      return (this->*m_writeFn)(value);
    }

  private:
    bool writeFirst(int64_t value) {
      if (ZuUnlikely(!writeValue<false>(value))) return false;
      m_writeFn = &Writer::writeNext;
      return true;
    }
    bool writeNext(int64_t value) {
      return writeValue(value);
    }
  };

  class DeltaWriter : public Writer_ {
    typedef bool (DeltaWriter::*WriteFn)(int64_t);

  public:
    DeltaWriter(uint8_t *start, uint8_t *end) : Writer_(start, end) { }

  private:
    WriteFn	m_writeFn = &DeltaWriter::writeFirst;
    int64_t	m_base = 0;

  public:
    ZuInline bool write(int64_t value) {
      return (this->*m_writeFn)(value);
    }

  private:
    bool writeFirst(int64_t value) {
      if (ZuUnlikely(!writeValue<false>(value))) return false;
      m_base = value;
      m_writeFn = &DeltaWriter::writeNext;
      return true;
    }
    bool writeNext(int64_t value) {
      int64_t delta = value - m_base;
      if (ZuUnlikely(!writeValue(delta))) return false;
      m_base = value;
      return true;
    }
  };

  class Delta2Writer : public Writer_ {
    typedef bool (Delta2Writer::*WriteFn)(int64_t);

  public:
    Delta2Writer(uint8_t *start, uint8_t *end) : Writer_(start, end) { }

  private:
    WriteFn	m_writeFn = &Delta2Writer::writeFirst;
    int64_t	m_base = 0;
    int64_t	m_delta = 0;

  public:
    ZuInline bool write(int64_t value) {
      return (this->*m_writeFn)(value);
    }

  private:
    bool writeFirst(int64_t value) {
      if (ZuUnlikely(!writeValue<false>(value))) return false;
      m_base = value;
      m_writeFn = &Delta2Writer::writeSecond;
      return true;
    }
    bool writeSecond(int64_t value) {
      int64_t delta = value - m_base;
      if (ZuUnlikely(!writeValue<false>(delta))) return false;
      m_delta = delta;
      m_writeFn = &Delta2Writer::writeNext;
      return true;
    }
    bool writeNext(int64_t value) {
      int64_t delta = value - m_base;
      int64_t delta2 = delta - m_delta;
      if (ZuUnlikely(!writeValue(delta2))) return false;
      m_base = value;
      m_delta = delta;
      return true;
    }
  };
}

#endif /* ZuSeries_HPP */
