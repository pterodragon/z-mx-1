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
#include <ZvLib.hpp>
#endif

#include <ZtRegex.hpp>

#include <ZvError.hpp>

class ZvAPI ZvRegexError : public ZtRegex::Error, public ZvError {
public:
  inline ZvRegexError(const ZtRegex::Error &e) : ZtRegex::Error(e) { }
  inline ZvRegexError(const ZvRegexError &e) : ZtRegex::Error(e) { }
  inline ZvRegexError &operator =(const ZvRegexError &e) {
    if (this != &e) *(ZtRegex::Error *)this = e;
    return *this;
  }
  virtual ~ZvRegexError() { }

  ZtZString message() const {
    return ZtSprintf("regular expression error \"%s\"", m_error);
  }
};

#endif /* ZvRegexError_HPP */
