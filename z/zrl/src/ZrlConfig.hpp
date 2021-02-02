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

// command line interface - application configuration

#ifndef ZrlConfig_HPP
#define ZrlConfig_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZvCf.hpp>

namespace Zrl {

// line editor configuration
struct Config {
  unsigned	vkeyInterval = 100;	// vkey seq. interval (milliseconds)
  unsigned	maxLineLen = 32768;	// maximum line length
  unsigned	maxCompPages = 5;	// max # pages of possible completions
  unsigned	histOffset = 0;		// initial history offset
  unsigned	maxStackDepth = 10;	// maximum mode stack depth
  unsigned	maxFileSize = 1<<20;	// maximum keymap file size
  unsigned	maxVKeyRedirs = 10;	// maximum # keystroke redirects
  unsigned	maxUndo = 100;		// maximum undo/redo
};

} // Zrl

#endif /* ZrlConfig_HPP */
