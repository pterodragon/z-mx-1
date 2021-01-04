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

// Zrl::Line encapsulates a wrapped left-to-right UTF8 mono-spaced line
// of text displayed ona terminal, comprised of variable-width glyphs.
// Each glyph is each regular width, i.e. 1 display position, or full-width,
// i.e. 2 display positions. The line wraps around the display width and
// is re-flowed such that full-width glyphs are always kept intact on the
// same line.

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
  Line() = default;
  Line(const Line &) = default;
  Line &operator =(const Line &) = default;
  Line(Line &&) = default;
  Line &operator =(Line &&) = default;
  ~Line() = default;

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

    // mapping() maps display positions <-> byte offsets
    unsigned mapping() const { return m_value>>5U; }
    // len() returns the number of elements (bytes, positions) within the glyph
    unsigned len() const { return ((m_value>>3U) & 0x3U) + 1; }
    // off() returns the offset of this element within the glyph
    unsigned off() const { return (m_value>>1U) & 0x3U; }
    // padding is only used when indexing display positions, to indicate
    // empty display positions at the right edge due to wrapping
    // full-width glyphs around to the next row; padding is unused
    // when indexing UTF8 byte data
    bool padding() const { return m_value & 0x1U; }

  private:
    uint32_t	m_value = 4U; // sentinel null value
  };

  void clear();

  const ZtArray<uint8_t> &data() const { return m_data; }
  ZtArray<uint8_t> &data() { return m_data; }

  // length in bytes
  unsigned length() const { return m_bytes.length(); }
  // width in display positions
  unsigned width() const { return m_positions.length(); }

  // substring
  ZuString substr(unsigned off, unsigned len) const {
    return {reinterpret_cast<const char *>(&m_data[off]), len};
  }

  // byte offset -> display position
  Index byte(unsigned off) const;

  // display position -> byte offset
  Index position(unsigned pos) const;

  // align display position to left-side of glyph (if glyph is full-width)
  unsigned align(unsigned pos) const;

  // glyph-based motions - given origin byte offset, returns destination
  unsigned fwdGlyph(unsigned off) const; // forward glyph
  unsigned revGlyph(unsigned off) const; // backup glyph
  unsigned fwdWord(unsigned off) const; // forward word
  unsigned revWord(unsigned off) const; // backup word
  unsigned fwdWordEnd(unsigned off) const; // forward to end of word
  unsigned revWordEnd(unsigned off) const; // backup to end of previous word
  unsigned fwdUnixWord(unsigned off) const; // forward "Unix" word
  unsigned revUnixWord(unsigned off) const; // backup ''
  unsigned fwdUnixWordEnd(unsigned off) const; // forward to end of ''
  unsigned revUnixWordEnd(unsigned off) const; // backup to end of ''

  // glyph search within line - returns (adjusted) origin if not found
  unsigned fwdSearch(unsigned off, uint32_t glyph) const; // forward search
  unsigned revSearch(unsigned off, uint32_t glyph) const; // reverse ''

  // reflow, given offset and display width
  void reflow(unsigned off, unsigned dwidth);

  bool isword_(unsigned off) const { return isword__(m_data[off]); }
  bool isspace_(unsigned off) const { return isspace__(m_data[off]); }

private:
  static bool isword__(char c) {
    return
      (c >= '0' && c <= '9') ||
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      c == '_';
  }
  static bool isspace__(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  }
  template <typename L>
  bool fwd(unsigned &off, unsigned n, L l) const {
    while (!l(m_data[off])) {
      if ((off += m_bytes[off].len()) >= n) return false;
    }
    return true;
  }
  template <typename L>
  bool rev(unsigned &off, L l) const {
    do {
      if (ZuUnlikely(!off)) return false;
      off -= m_bytes[--off].off();
    } while (!l(m_data[off]));
    return true;
  }

private:
  ZtArray<uint8_t>	m_data;		// UTF-8 byte data
  ZtArray<Index>	m_bytes;	// index offset -> display position
  ZtArray<Index>	m_positions;	// index display position -> offset
};

} // Zrl

#endif /* ZrlLine_HPP */
