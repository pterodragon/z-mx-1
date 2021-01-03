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

// command line interface - in-memory history

#include <zlib/ZrlHistory.hpp>

namespace Zrl {

void History::save(unsigned i, ZuString s)
{
  if (i < m_offset) return;
  if (i >= m_offset + m_max) {
    unsigned newOffset = (i + 1) - m_max;
    if (newOffset >= m_offset + m_max)
      m_data = {};
    else
      while (m_offset < newOffset) {
	unsigned j = m_offset % m_max;
	if (j < m_data.length()) m_data[j] = {};
	++m_offset;
      }
  }
  unsigned j = i % m_max;
  if (j >= m_data.length()) m_data.grow(j + 1);
  m_data[j] = s;
}

bool History::load(unsigned i, ZuString &s) const
{
  if (i < m_offset || i >= m_offset + m_max) return false;
  i %= m_max;
  if (i >= m_data.length() || !m_data[i]) return false;
  s = m_data[i];
  return true;
}

} // namespace Zrl
