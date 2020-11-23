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

private:
  // encodes a byte length and a display width into 3 bits
  class CProps {
  public:
    CProps() = default;
    CProps(const CProps &) = default;
    CProps &operator =(const CProps &) = default;
    CProps(CProps &&) = default;
    CProps &operator =(CProps &&) = default;
    ~CProps() = default;

    CProps(unsigned len, unsigned width) :
	m_value{((len - 1)<<1U) | (width - 1)} { }

    unsigned len() const { return (m_value>>1U) + 1; }
    unsigned width() const { return (m_value & 0x1U) + 1; }

  private:
    uint8_t	m_value = 0;
  };

public:
  void init(unsigned x) {
    m_data.clear();
    m_byte2pos.clear();
    m_pos2byte.clear();
  }

  const ZtArray<uint8_t> &data() const { return m_data; }
  ZtArray<uint8_t> &data() { return m_data; }

  // substring
  ZuString substr(unsigned off, unsigned len) const {
    return ZuString{reinterpret_cast<const char *>(&m_data[off]), len};
  }

  // byte offset -> display position
  Index byte2pos(unsigned off) const;

  // display position -> byte offset
  Index pos2byte(unsigned pos) const;

  // left-align display position of overlapping character
  unsigned align(unsigned pos) const;

  // reflow, given offset and display width
  void reflow(unsigned off, unsigned dwidth);

private:
  // character properties, given offset
  CProps cprops(unsigned off);

  bool isalnum(char c) {
    return
      c >= '0' && c <= '9' ||
      c >= 'a' && c <= 'z' ||
      c >= 'A' && c <= 'Z' ||
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
      off += m_byte2pos[off].len();
    } while (off < len);
    return false;
  }
  template <typename L>
  bool rev(unsigned &off, L l)
  {
    do {
      if (ZuUnlikely(!off)) return false;
      off -= m_byte2pos[--off].off();
    } while (!l(m_data[off]));
    return true;
  }


  ZtArray<uint8_t>	m_data;		// UTF-8 data
  ZtArray<Index>	m_byte2pos;	// byte offset -> display position
  ZtArray<Index>	m_pos2byte;	// display position -> byte offset
};

} // Zrl

#endif /* ZrlLine_HPP */
