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

#ifndef ZrlTerminfo_HPP
#define ZrlTerminfo_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#ifndef _WIN32

// including ncurses.h corrupts the preprocessor and compiler namespace,
// so just import what's needed (and hope that none of the below is
// a macro, which isn't the case as of 10/11/2020, and likely to never change)

extern "C" {
  struct term;
  typedef struct term TERMINAL;

  extern TERMINAL *cur_term;

  extern int setupterm(const char *, int, int *);
  extern int del_curterm(TERMINAL *);

  extern char *tigetstr(const char *);
  extern int tigetflag(const char *);
  extern int tigetnum(const char *);
  extern char *tiparm(const char *, ...);

  extern int tputs(const char *, int, int (*)(int));
};

#endif /* !_WIN32 */

#endif /* ZrlTerminfo_HPP */
