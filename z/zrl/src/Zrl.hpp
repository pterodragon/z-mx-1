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

// command line interface using a readline-compatible library

#ifndef Zrl_HPP
#define Zrl_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <ZrlLib.hpp>
#endif

#include <ZvCf.hpp>

extern "C" {
  ZrlExtern char *Zrl_complete(char *prefix, int state);
  ZrlExtern char **Zrl_completions(char *text, int start, int end);
};

struct ZrlAPI Zrl {
  struct EndOfFile { };	// thrown by readline and readline_ on EOF

  static void init(const char *program);
  static void syntax(ZvCf *cf);
  static void stop();

  static ZmRef<ZvCf> readline(const char *prompt);
  static ZtString readline_(const char *prompt);
};

#endif /* Zrl_HPP */
