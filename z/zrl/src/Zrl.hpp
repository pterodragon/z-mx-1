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
//
// readline compatible blocking interface to Zrl::CLI
//
// synopsis:
//
// using namespace Zrl;
// char *readline(const char *prompt);

#ifndef Zrl_HPP
#define Zrl_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZrlCLI.hpp>

extern "C" {
  ZrlExtern char *Zrl_readline(const char *prompt);

  // optional
  ZrlExtern void Zrl_start(const char *prompt);
  ZrlExtern void Zrl_stop();
  ZrlExtern bool Zrl_running();
}

namespace Zrl {
  inline char *readline(const char *prompt) { return Zrl_readline(prompt); }

  // optional
  inline void start(const char *prompt) { return Zrl_start(prompt); }
  inline void stop() { return Zrl_stop(); }
  inline bool running() { return Zrl_running(); }
}

#endif /* Zrl_HPP */
