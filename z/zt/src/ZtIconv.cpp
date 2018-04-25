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

// iconv wrapper

#include <ZtIconv.hpp>

#include <ZmGuard.hpp>
#include <ZmSpecific.hpp>

// Note: iconv's WCHAR_T pseudo-encoding is broken

inline const char *ZtIconv_wcharEncoding() {
  wchar_t w = (wchar_t)'1';

  return sizeof(wchar_t) == 2 ? (*(char *)&w == '1' ? "UCS-2LE" : "UCS-2BE") :
				(*(char *)&w == '1' ? "UCS-4LE" : "UCS-4BE");
}

struct ZtIconv_wchar2char_;
struct ZtIconv_char2wchar_;

template <> struct ZmCleanup<ZtIconv_wchar2char_> {
  enum { Level = ZmCleanupLevel::Platform };
};
template <> struct ZmCleanup<ZtIconv_char2wchar_> {
  enum { Level = ZmCleanupLevel::Platform };
};

struct ZtIconv_wchar2char_ : public ZmObject, public ZtIconv {
  inline ZtIconv_wchar2char_() : ZtIconv("UTF-8", ZtIconv_wcharEncoding()) { }
};
struct ZtIconv_char2wchar_ : public ZmObject, public ZtIconv {
  inline ZtIconv_char2wchar_() : ZtIconv(ZtIconv_wcharEncoding(), "UTF-8") { }
};

ZtIconv *ZtIconv_wchar2char::instance()
{
  return ZmSpecific<ZtIconv_wchar2char_>::instance();
}
ZtIconv *ZtIconv_char2wchar::instance()
{
  return ZmSpecific<ZtIconv_char2wchar_>::instance();
}
