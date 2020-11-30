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

// command line interface

#ifndef ZrlLine_HPP
#define ZrlLine_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <ZtArray.hpp>

namespace Zrl {

class ZrlAPI Line {
public:
  // encodes a 28bit index, padding, an offset and a length into 32bits
  class Index {
  public:
    Index() = default;
    Index(const Index &) = default;
    Index &operator =(const Index &) = default;
    Index(Index &&) = default;
    Index &operator =(Index &&) = default;
    ~Index() = default;

    Index(unsigned index, unsigned len, unsigned off) :
	m_value{(index<<5U) | ((len - 1)<<3U) | (off<<1U)} { }
    Index(unsigned index, unsigned len, unsigned off, bool padding) :
	m_value{(index<<5U) | ((len - 1)<<3U) | (off<<1U) | padding} { }

    bool operator !() const { return (m_value & 0x1fU) == 4U; }
    ZuOpBool

    unsigned index() const { return m_value>>5U; }
    unsigned len() const { return ((m_value>>3U) & 0x3U) + 1; }
    unsigned off() const { return (m_value>>1U) & 0x3U; }
    bool padding() const { return m_value & 0x1U; }

  private:
    uint32_t	m_value = 4U; // sentinel null value
  };

  void init(unsigned x) {
    m_data.clear();
    m_bytes.clear();
    m_positions.clear();
  }

  const ZtArray<uint8_t> &data() const { return m_data; }
  ZtArray<uint8_t> &data() { return m_data; }

  unsigned length() const { return m_bytes.length(); }
  unsigned width() const { return m_positions.length(); }

  // substring
  ZuString substr(unsigned off, unsigned len) const {
    return ZuString{reinterpret_cast<const char *>(&m_data[off]), len};
  }

  // byte offset -> display position
  Index byte(unsigned off) const;

  // display position -> byte offset
  Index position(unsigned pos) const;

  // left-align display position of overlapping character
  unsigned align(unsigned pos) const;

  // character motions
  unsigned fwdChar(unsigned off) const; // forward character
  unsigned revChar(unsigned off) const; // backup character
  unsigned fwdWord(unsigned off) const; // forward word
  unsigned revWord(unsigned off) const; // backup word
  unsigned fwdWordEnd(unsigned off) const; // forward to end of word
  unsigned revWordEnd(unsigned off) const; // backup to end of previous word
  unsigned fwdWSWord(unsigned off) const; // forward white-space delimited word
  unsigned revWSWord(unsigned off) const; // backup ''
  unsigned fwdWSWordEnd(unsigned off) const; // forward to end of ''
  unsigned revWSWordEnd(unsigned off) const; // backup to end of ''

  // reflow, given offset and display width
  void reflow(unsigned off, unsigned dwidth);

private:
  bool isword(char c) {
    return
      (c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      c == '_';
  }
  bool isspace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  }
  template <typename L>
  bool fwd(unsigned &off, unsigned len, L l)
  {
    uint32_t v;
    do {
      if (l(m_data[off])) return true;
      off += m_bytes[off].len();
    } while (off < len);
    return false;
  }
  template <typename L>
  bool rev(unsigned &off, L l)
  {
    do {
      if (ZuUnlikely(!off)) return false;
      off -= m_bytes[--off].off();
    } while (!l(m_data[off]));
    return true;
  }

private:
  ZtArray<uint8_t>	m_data;		// UTF-8 data
  ZtArray<Index>	m_bytes;	// index offset -> display position
  ZtArray<Index>	m_positions;	// index display position -> offset
};

} // Zrl

#endif /* ZrlLine_HPP */
