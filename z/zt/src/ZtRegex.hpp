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

// Perl compatible regular expressions (pcre)

#ifndef ZtRegex_HPP
#define ZtRegex_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <pcre.h>

#include <zlib/ZuAssert.hpp>
#include <zlib/ZuTraits.hpp>
#include <zlib/ZuString.hpp>
#include <zlib/ZuPrint.hpp>

#include <zlib/ZmCleanup.hpp>

#include <zlib/ZtString.hpp>

struct ZtAPI ZtRegexError {
  const char	*message = 0;
  int		code;
  int		offset;

  static const char *strerror(int);

  template <typename S> void print(S &s) const {
    if (message) {
      s << "ZtRegex Error \"" << message << "\" (" << ZuBoxed(code) << ")"
	" at offset " << ZuBoxed(offset);
    } else {
      s << "ZtRegex pcre_exec() Error: " << strerror(code);
    }
  }
};
template <> struct ZuPrint<ZtRegexError> : public ZuPrintFn { };

// ZtRegex is used by ZeLog
class ZtRegex;
template <> struct ZmCleanup<ZtRegex> {
  enum { Level = ZmCleanupLevel::Platform };
};
class ZtAPI ZtRegex {
  ZtRegex(const ZtRegex &) = delete;
  ZtRegex &operator =(const ZtRegex &) = delete;

  ZtRegex();

public:
  using Capture = ZuString;
  using Captures = ZtArray<Capture>;

  // pcre_compile() options
  ZtRegex(const char *pattern, int options = PCRE_UTF8);

  // ZtRegex is move-only (by design)
  ZtRegex(ZtRegex &&r) noexcept :
      m_regex(r.m_regex), m_extra(r.m_extra), m_captureCount(r.m_captureCount) {
    r.m_regex = 0;
    r.m_extra = 0;
    r.m_captureCount = 0;
  }
  ZtRegex &operator =(ZtRegex &&r) noexcept {
    if (this == &r) return *this;
    m_regex = r.m_regex;
    m_extra = r.m_extra;
    m_captureCount = r.m_captureCount;
    r.m_regex = 0;
    r.m_extra = 0;
    r.m_captureCount = 0;
    return *this;
  }

  ~ZtRegex();

  void study();

  unsigned captureCount() const { return m_captureCount; }

  // options below are pcre_exec() options

  // captures[0] is $`
  // captures[1] is $&
  // captures[2] is $1
  // captures[n - 1] is $' (where n = number of captured substrings + 3)
  // n is always >= 3 for a successful match, or 0 for no match
  // return value is number of captures excluding $` and $', i.e. (n - 2)
  //   0 implies no match
  //   1 implies $`, $&, $' captured (n == 3)
  //   2 implies $`, $&, $1, $' captured (n == 4)
  // etc.
  /*
     $& returns the entire matched string.
     $` returns everything before the matched string.
     $' returns everything after the matched string.
  */

  int m(ZuString s, int offset = 0, int options = 0) const {
    ZtArray<int> ovector;
    return exec(s, offset, options, ovector);
  }
  int m(ZuString s,
      Captures &captures, int offset = 0, int options = 0) const {
    ZtArray<int> ovector;
    int i = exec(s, offset, options, ovector);
    if (i) capture(s, ovector, captures);
    return i;
  }

  template <typename S> typename ZuIfT<
    ZuConversion<ZtString, S>::Is ||
    ZuConversion<ZtArray<char>, S>::Is, int>::T s(
	S &s, ZuString r, int offset = 0, int options = 0) const {
    ZtArray<int> ovector;
    int i = exec(s, offset, options, ovector);

    if (i) s.splice(ovector[0], ovector[1] - ovector[0], r.data(), r.length());
    return i;
  }

  template <typename S> typename ZuIfT<
    ZuConversion<ZtString, S>::Is ||
    ZuConversion<ZtArray<char>, S>::Is, int>::T sg(
	S &s, ZuString r, int offset = 0, int options = 0) const {
    ZtArray<int> ovector;
    int n = 0;
    int slength = s.length(), rlength = r.length();

    while (offset < slength && exec(s, offset, options, ovector)) {
      s.splice(ovector[0], ovector[1] - ovector[0], r.data(), rlength);
      offset = ovector[0] + rlength;
      if (ovector[1] == ovector[0]) offset++;
      options |= PCRE_NO_UTF8_CHECK;
      ++n;
    }

    return n;
  }

  int split(ZuString s, Captures &a, int options = 0) const {
    int offset = 0, last = 0;
    ZtArray<int> ovector;
    int slength = s.length();

    while (offset < slength && exec(s, offset, options, ovector)) {
      if (offset || ovector[1] > ovector[0])
	new (a.push()) Capture(s.data() + last, ovector[0] - last);
      last = offset = ovector[1];
      if (ovector[1] == ovector[0]) offset++;
      options |= PCRE_NO_UTF8_CHECK;
    }
    if (last < slength)
      new (a.push()) Capture(s.data() + last, slength - last);

    return a.length();
  }

  int index(const char *name) const {
    int i = pcre_get_stringnumber(m_regex, name);
    if (i < 0) return -1;
    return i + 1;
  }

private:
  template <typename S>
  int exec(const S &s, int offset, int options, ZtArray<int> &ovector) const {
    int slength = s.length();

    if (!slength) return 0;

    ovector.length(m_captureCount * 3, false);

    int c = pcre_exec(
	m_regex, m_extra, s.data(), slength,
	offset, options, ovector.data(), ovector.length());

    if (c >= 0) return c;

    if (c == PCRE_ERROR_NOMATCH) return 0;

    throw ZtRegexError{nullptr, c, -1};
  }
  template <typename S> void capture(
      const S &s, const ZtArray<int> &ovector, Captures &captures) const {
    int offset, length;
    int slength = s.length();

    captures.length(0, false);
    captures.size(m_captureCount + 2);
    new (captures.push()) Capture(s.data(), ovector[0]); // $`
    unsigned n = m_captureCount;
    for (unsigned i = 0; i < n; i++) {
      offset = ovector[i<<1];
      if (offset < 0) {
	new (captures.push()) Capture();
      } else {
	length = ovector[(i<<1) + 1] - offset;
	new (captures.push()) Capture(s.data() + offset, length);
      }
    }
    new (captures.push())
      Capture(s.data() + ovector[1], slength - ovector[1]); // $'
  }

  pcre		*m_regex;
  pcre_extra	*m_extra;
  unsigned	m_captureCount;
};

#define ZtStaticRegex(...) ZmStatic([]() { return new ZtRegex(__VA_ARGS__); })

#endif /* ZtRegex_HPP */
