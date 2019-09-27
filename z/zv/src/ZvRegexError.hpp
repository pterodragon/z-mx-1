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

// generic Zv regex error exception

#ifndef ZvRegexError_HPP
#define ZvRegexError_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZtRegex.hpp>

#include <zlib/ZvError.hpp>

class ZvAPI ZvRegexError : public ZvError {
public:
  inline ZvRegexError(const ZtRegex::Error &e) : m_error(e) { }
#if 0
  inline ZvRegexError(const ZvRegexError &e) : m_error(e.m_error) { }
  inline ZvRegexError &operator =(const ZvRegexError &e) {
    if (this != &e) m_error = e.m_error;
    return *this;
  }
#endif

  void print_(ZmStream &s) const { s << m_error; }

private:
  ZtRegex::Error	m_error;
};

#endif /* ZvRegexError_HPP */
