//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

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

// Perl compatible regular expressions (pcre)

#define ZtRegex_CPP

#include <ZtRegex.hpp>

ZtRegex::ZtRegex(const char *pattern, int options) : m_extra(0)
{
  Error error;

  m_regex = pcre_compile2(
    pattern, options, &error.code, &error.message, &error.offset, 0);

  if (!m_regex) throw error;

  if (!pcre_fullinfo(m_regex, 0, PCRE_INFO_CAPTURECOUNT, &m_captureCount))
    m_captureCount++;
  else
    m_captureCount = 1;
}

ZtRegex::~ZtRegex()
{
  if (m_extra) (pcre_free_study)(m_extra);
  if (m_regex) (pcre_free)(m_regex);
}

void ZtRegex::study()
{
  Error error;

  error.offset = -1;
  error.code = -1;

  m_extra = pcre_study(m_regex, PCRE_STUDY_JIT_COMPILE, &error.message);

  if (error.message) {
    if (m_extra) (pcre_free)(m_extra);
    m_extra = 0;
    throw error;
  }
}
