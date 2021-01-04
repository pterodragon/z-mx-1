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

void Line::clear()
{
  m_data.clear();
  m_bytes.clear();
  m_positions.clear();
}

// byte offset -> display position
Line::Index Line::byte(unsigned off) const
{
  if (off >= m_bytes.length()) return Index{m_positions.length(), 0, 0};
  return m_bytes[off];
}

// display position -> byte offset
Line::Index Line::position(unsigned pos) const
{
  if (pos >= m_positions.length()) return Index{m_bytes.length(), 0, 0};
  return m_positions[pos];
}

// left-align position to glyph
unsigned Line::align(unsigned pos) const
{
  unsigned n = m_positions.length();
  if (ZuUnlikely(pos >= n)) return n;
  auto index = m_positions[pos];
  if (ZuUnlikely(index.padding())) {
    n = index.off() + 1;
    if (ZuUnlikely(pos < n)) return 0;
    pos -= n;
    index = m_positions[pos];
  }
  n = index.off();
  if (ZuUnlikely(pos < n)) return 0;
  return pos - n;
}

// forward glyph
unsigned Line::fwdGlyph(unsigned off) const
{
  unsigned n = m_bytes.length();
  if (ZuUnlikely(off >= n)) return n;
  auto index = m_bytes[off];
  return off + index.len() - index.off();
}

// backup glyph
unsigned Line::revGlyph(unsigned off) const
{
  unsigned n = m_bytes.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off >= n)) off = n - 1;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  off -= m_bytes[--off].off();
  return off;
}

// forward whitespace-delimited word, distinguishing alphanumeric + '_'
unsigned Line::fwdWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  if (isspace__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
    if (isword__(m_data[off])) {
      if (!fwd(off, n, [](char c) { return !isword__(c); })) return n;
    } else {
      if (!fwd(off, n, [](char c) { return isspace__(c) || isword__(c); }))
	return n;
    }
  } else if (isword__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isword__(c); })) return n;
    if (isspace__(m_data[off])) {
      if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
    } else {
      if (!fwd(off, n, [](char c) { return isspace__(c) || isword__(c); }))
	return n;
    }
  } else {
    if (!fwd(off, n, [](char c) { return isspace__(c) || isword__(c); }))
      return n;
    if (isspace__(m_data[off])) {
      if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
    } else {
      if (!fwd(off, n, [](char c) { return !isword__(c); })) return n;
    }
  }
  n = m_bytes[--off].off();
  return off - n;
}

// backup whitespace-delimited word, distinguishing alphanumeric + '_'
unsigned Line::revWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off >= n)) off = n - 1;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  if (isspace__(m_data[off])) {
    if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
    if (isword__(m_data[off])) {
      if (!rev(off, [](char c) { return !isword__(c); })) return 0;
    } else {
      if (!rev(off, [](char c) { return isspace__(c) || isword__(c); }))
	return 0;
    }
  } else if (isword__(m_data[off])) {
    if (!rev(off, [](char c) { return !isword__(c); })) return 0;
    if (isspace__(m_data[off])) {
      if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
    } else {
      if (!rev(off, [](char c) { return isspace__(c) || isword__(c); }))
	return 0;
    }
  } else {
    if (!rev(off, [](char c) { return isspace__(c) || isword__(c); }))
      return 0;
    if (isspace__(m_data[off])) {
      if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
    } else {
      if (!rev(off, [](char c) { return !isword__(c); })) return 0;
    }
  }
  return off + m_bytes[off].len();
}

// forward to end of whitespace-delimited word,
// distinguishing alphanumeric + '_'
unsigned Line::fwdWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  if (isspace__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
  }
  if (isword__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isword__(c); })) return n;
  } else {
    if (!fwd(off, n, [](char c) { return isspace__(c) || isword__(c); }))
      return n;
  }
  n = m_bytes[--off].off();
  return off - n;
}

// backup to end of whitespace-delimited word,
// distinguishing alphanumeric + '_'
unsigned Line::revWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off >= n)) off = n - 1;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  if (isword__(m_data[off])) {
    if (!rev(off, [](char c) { return !isword__(c); })) return 0;
  } else {
    if (!rev(off, [](char c) { return isspace__(c) || isword__(c); }))
      return 0;
  }
  if (isspace__(m_data[off])) {
    if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
  }
  return off;
}

// forward whitespace-delimited word
unsigned Line::fwdUnixWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  if (isspace__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
    if (!fwd(off, n, [](char c) { return isspace__(c); })) return n;
  } else {
    if (!fwd(off, n, [](char c) { return isspace__(c); })) return n;
    if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
  }
  n = m_bytes[--off].off();
  return off - n;
}

// backup whitespace-delimited word
unsigned Line::revUnixWord(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off >= n)) off = n - 1;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  if (isspace__(m_data[off])) {
    if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
    if (!rev(off, [](char c) { return isspace__(c); })) return 0;
  } else {
    if (!rev(off, [](char c) { return isspace__(c); })) return 0;
    if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
  }
  return off + m_bytes[off].len();
}

// forward to end of whitespace-delimited word
unsigned Line::fwdUnixWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(off >= n)) return n;
  off -= m_bytes[off].off();
  if (isspace__(m_data[off])) {
    if (!fwd(off, n, [](char c) { return !isspace__(c); })) return n;
  }
  if (!fwd(off, n, [](char c) { return isspace__(c); })) return n;
  n = m_bytes[--off].off();
  return off - n;
}

// backup to end of whitespace-delimited word
unsigned Line::revUnixWordEnd(unsigned off) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off >= n)) off = n - 1;
  off -= m_bytes[off].off();
  if (ZuUnlikely(!off)) return 0;
  if (!rev(off, [](char c) { return isspace__(c); })) return 0;
  if (isspace__(m_data[off])) {
    if (!rev(off, [](char c) { return !isspace__(c); })) return 0;
  }
  return off;
}

// forward glyph search - returns (adjusted) origin if not found
unsigned Line::fwdSearch(unsigned off, uint32_t glyph) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(off >= n)) return n;
  unsigned orig = off;
  unsigned l;
  do {
    uint32_t u;
    l = ZuUTF8::in(&m_data[off], n - off, u);
    if (ZuUnlikely(!l)) break;
    if (u == glyph) return off;
  } while ((off += l) < n);
  return orig;
}

// reverse glyph search - returns (adjusted) origin if not found
unsigned Line::revSearch(unsigned off, uint32_t glyph) const
{
  unsigned n = m_data.length();
  if (ZuUnlikely(!n)) return 0;
  if (ZuUnlikely(off > n)) off = n - 1;
  ++off; while (--off > 0 && !ZuUTF8::initial(m_data[off]));
  unsigned orig = off;
  while (off) {
    uint32_t u;
    unsigned l = ZuUTF8::in(&m_data[off], n - off, u);
    if (ZuUnlikely(!l)) break;
    if (u == glyph) return off;
    while (--off > 0 && !ZuUTF8::initial(m_data[off]));
  }
  return orig;
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
      pos = byte.mapping();
      pwidth = position(pos).len();
      pos += pwidth;
    }
  }

  while (off < len) {
    auto span = ZuUTF<uint32_t, uint8_t>::gspan(
	ZuArray<uint8_t>{&m_data[off], len - off});
    if (ZuUnlikely(!span)) break;
    unsigned glen = span.inLen();
    unsigned gwidth = span.width();
    m_bytes.grow(off + glen);
    {
      unsigned x = pos % dwidth;
      unsigned padding = (x + gwidth > dwidth) ? (dwidth - x) : 0U;
      m_positions.grow(pos + padding + gwidth);
      for (unsigned i = 0; i < padding; i++)
	m_positions[pos++] = Index{off, padding, i, true};
    }
    for (unsigned i = 0; i < glen; i++)
      m_bytes[off + i] = Index{pos, glen, i};
    for (unsigned i = 0; i < gwidth; i++)
      m_positions[pos + i] = Index{off, gwidth, i};
    off += glen;
    pos += gwidth;
    pwidth = gwidth;
  }
}

} // namespace Zrl
