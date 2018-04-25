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

// stack backtrace

// Reference implementations:
//   Breakpad
//   - http://code.google.com/p/google-breakpad/wiki/GettingStartedWithBreakpad
//   stack-trace
//   - http://www.mr-edd.co.uk/code/stack_trace
//   Visual Leak Detector
//   - http://www.codeproject.com/KB/applications/visualleakdetector.aspx
//   Stack Walker
//   - http://www.codeproject.com/KB/threads/StackWalker.aspx
//   Backtrace MinGW
//   - http://code.google.com/p/backtrace-mingw/

#ifndef ZmBackTrace_HPP
#define ZmBackTrace_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZmBackTrace_.hpp>
#include <ZmBackTrace_print.hpp>

#endif /* ZmBackTrace_HPP */
