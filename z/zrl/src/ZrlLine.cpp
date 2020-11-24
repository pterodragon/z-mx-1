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

#include <zlib/ZrlLine.hpp>

namespace Zrl {

void Line::init(unsigned x)
{
  m_data.clear();
  m_bytes.clear();
  m_positions.clear();
}

// offset -> position
Line::Index Line::byte(unsigned off) const
{
  unsigned n = m_bytes.length();
  if (!n--) return Index{};
  if (off > n) off = n;
  return m_bytes[off];
}

// position -> offset
Line::Index Line::position(unsigned pos) const
{
  unsigned n = m_positions.length();
  if (!n--) return Index{};
  if (pos > n) pos = n;
  return m_positions[pos];
}

// left-align position to character
unsigned Line::align(unsigned pos) const
{
  unsigned n = m_positions.length();
  if (ZuUnlikely(!pos || !n--)) return 0;
  if (ZuUnlikely(pos > n)) pos = n;
  auto index = m_positions[pos];
  unsigned n;
  if (ZuUnlikely(index.padding())) {
    n = index.off() + 1;
    if (ZuUnlikely(pos <= n)) return 0;
    pos -= n;
    index = m_positions[pos];
  }
  n = index.off();
  if (ZuUnlikely(pos <= n)) return 0;
  return pos - n;
}

// forward character
unsigned Line::fwdChar(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n--)) return 0;
  if (ZuUnlikely(off >= n)) return n;
  auto index = m_bytes[off];
  return off + index.len() - index.off();
}

// backup character
unsigned Line::revChar(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!off || !n--)) return 0;
  if (ZuUnlikely(off > n)) off = n;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  off -= m_bytes[--off].off();
  return off;
}

// forward whitespace-delimited word, distinguishing alphanumeric + '_'
unsigned Line::fwdWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n--)) return 0;
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  char c = m_data[off];
  if (isspace(c)) {
    if (!fwd([](char c) { return !isspace(c); })) return n;
    if (isalnum(m_data[off])) {
      if (!fwd([](char c) { return !isalnum(c); })) return n;
    } else {
      if (!fwd([](char c) { return isspace(c) || isalnum(c); })) return n;
    }
  } else if (isalnum(c)) {
    if (!fwd([](char c) { return !isalnum(c); })) return n;
    if (isspace(m_data[off])) {
      if (!fwd([](char c) { return !isspace(c); })) return n;
    } else {
      if (!fwd([](char c) { return isspace(c) || isalnum(c); })) return n;
    }
  } else {
    if (!fwd([](char c) { return isspace(c) || isalnum(c); })) return n;
    if (isspace(m_data[off])) {
      if (!fwd([](char c) { return !isspace(c); })) return n;
    } else {
      if (!fwd([](char c) { return !isalnum(c); })) return n;
    }
  }
  return off - m_bytes[--off].off();
}

// backup whitespace-delimited word, distinguishing alphanumeric + '_'
unsigned Line::revWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!off || !n--)) return 0;
  if (ZuUnlikely(off > n)) off = n;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  char c = m_data[off];
  if (isspace(c)) {
    if (!rev([](char c) { return !isspace(c); })) return 0;
    if (isalnum(m_data[off])) {
      if (!rev([](char c) { return !isalnum(c); })) return 0;
    } else {
      if (!rev([](char c) { return isspace(c) || isalnum(c); })) return 0;
    }
  } else if (isalnum(c)) {
    if (!rev([](char c) { return !isalnum(c); })) return 0;
    if (isspace(m_data[off])) {
      if (!rev([](char c) { return !isspace(c); })) return 0;
    } else {
      if (!rev([](char c) { return isspace(c) || isalnum(c); })) return 0;
    }
  } else {
    if (!rev([](char c) { return isspace(c) || isalnum(c); })) return 0;
    if (isspace(m_data[off])) {
      if (!rev([](char c) { return !isspace(c); })) return 0;
    } else {
      if (!rev([](char c) { return !isalnum(c); })) return 0;
    }
  }
  return off + m_bytes[off].len();
}

// forward to end of whitespace-delimited word,
// distinguishing alphanumeric + '_'
unsigned Line::fwdWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n--)) return 0;
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  char c = m_data[off];
  if (isspace(c)) {
    if (!fwd([](char c) { return !isspace(c); })) return n;
  }
  if (isalnum(c)) {
    if (!fwd([](char c) { return !isalnum(c); })) return n;
  } else {
    if (!fwd([](char c) { return isspace(c) || isalnum(c); })) return n;
  }
  return off - m_bytes[--off].off();
}

// backup to end of whitespace-delimited word,
// distinguishing alphanumeric + '_'
unsigned Line::revWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!off || !n--)) return 0;
  if (ZuUnlikely(off > n)) off = n;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  char c = m_data[off];
  if (isalnum(c)) {
    if (!rev([](char c) { return !isalnum(c); })) return 0;
  } else {
    if (!rev([](char c) { return isspace(c) || isalnum(c); })) return 0;
  }
  if (isspace(c)) {
    if (!rev([](char c) { return !isspace(c); })) return 0;
  }
  return off;
}

// forward whitespace-delimited word
unsigned Line::fwdWSWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n--)) return 0;
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  char c = m_data[off];
  if (isspace(c)) {
    if (!fwd([](char c) { return !isspace(c); })) return n;
    if (!fwd([](char c) { return isspace(c); })) return n;
  } else {
    if (!fwd([](char c) { return isspace(c); })) return n;
    if (!fwd([](char c) { return !isspace(c); })) return n;
  }
  return off - m_bytes[--off].off();
}

// backup whitespace-delimited word
unsigned Line::revWSWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!off || !n--)) return 0;
  if (ZuUnlikely(off > n)) off = n;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  char c = m_data[off];
  if (isspace(c)) {
    if (!rev([](char c) { return !isspace(c); })) return 0;
    if (!rev([](char c) { return isspace(c); })) return 0;
  } else {
    if (!rev([](char c) { return isspace(c); })) return 0;
    if (!rev([](char c) { return !isspace(c); })) return 0;
  }
  return off + m_bytes[off].len();
}

// forward to end of whitespace-delimited word
unsigned Line::fwdWSWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n--)) return 0;
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  char c = m_data[off];
  if (isspace(c)) {
    if (!fwd([](char c) { return !isspace(c); })) return n;
  }
  if (!fwd([](char c) { return isspace(c); })) return n;
  return off - m_bytes[--off].off();
}

// backup to end of whitespace-delimited word
unsigned Line::revWSWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!off || !n--)) return 0;
  if (ZuUnlikely(off > n)) off = n;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  char c = m_data[off];
  if (!rev([](char c) { return isspace(c); })) return 0;
  if (isspace(c)) {
    if (!rev([](char c) { return !isspace(c); })) return 0;
  }
  return off;
}

// reflow, given offset and display width
void Line::reflow(unsigned off, unsigned dwidth)
{
  m_bytes.grow(m_data.length());
  m_positions.grow(m_data.length());

  unsigned len = m_data.length();
  unsigned pwidth, pos;

  if (!off) {
    pwidth = 0;
    pos = 0;
  } else {
    if (auto byte = this->byte(off - 1)) {
      pos = byte.index();
      pwidth = position(pos).len();
      pos += pwidth;
    }
  }

  while (off < len) {
    auto cprops = this->cprops(off);
    unsigned clen = cprops.len();
    unsigned cwidth = cprops.width();
    m_bytes.grow(off + clen);
    {
      unsigned x = pos % dwidth;
      unsigned padding = (x + width > dwidth) ? (dwidth - x) : 0U;
      m_positions.grow(pos + padding + cwidth);
      for (unsigned i = 0; i < padding; i++)
	m_positions[pos++] = Index{off, padding, i, true};
    }
    for (unsigned i = 0; i < clen; i++)
      m_bytes[off + i] = Index{pos, clen, i};
    for (unsigned i = 0; i < cwidth; i++)
      m_positions[pos + i] = Index{off, cwidth, i};
    off += clen;
    pos += cwidth;
    pwidth = cwidth;
  }
}

// character properties, given offset
Line::CProps Line::cprops(unsigned off)
{
  uint8_t c = m_data[off];
  uint32_t len, width;
  if (c < 0x20 || c == 0x7f) {
    len = 1;
    width = 2;
  } else {
    uint32_t u;
    len = ZuUTF::in(&m_data[off], m_data.length() - off, u);
    width = ZuUTF::width(u);
  }
  return CProps{len, width};
}

} // Zrl

#endif /* ZrlLine_HPP */
