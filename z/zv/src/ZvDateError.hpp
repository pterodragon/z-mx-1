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

// generic Zv date parsing exception

#ifndef ZvDateError_HPP
#define ZvDateError_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

#include <zlib/ZtArray.hpp>
#include <zlib/ZtDate.hpp>

#include <zlib/ZvError.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4275)
#endif

class ZvAPI ZvDateError : public ZvError {
public:
  template <typename S> inline ZvDateError(S &&s) : m_value(ZuFwd<S>(s)) { }

#if 0
  inline ZvDateError(const ZvDateError &e) : m_value(e.m_value) { }
  inline ZvDateError &operator =(const ZvDateError &e) {
    if (this != &e) m_value = e.m_value;
    return *this;
  }

  inline ZvDateError(ZvDateError &&e) noexcept : m_value(ZuMv(e.m_value)) { }
  inline ZvDateError &operator =(ZvDateError &&e) noexcept {
    m_value = ZuMv(e.m_value);
    return *this;
  }
#endif

  void print_(ZmStream &s) const {
    s << "invalid CSV date/time \"" << m_value << '"';
  }

private:
  ZtArray<char>		m_value;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZvDateError_HPP */
