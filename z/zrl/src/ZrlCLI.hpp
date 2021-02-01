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
// high-level wrapper for Zrl::Editor
//
// synopsis:
//
// using namespace Zrl;
// CLI cli;
// cli.init(App{
//   .enter = [](ZuString s) { std::cout << s << '\n'; }
// });
// if (cli.open()) {
//   cli.start("prompt> ");
//   cli.join();
//   cli.stop();
//   cli.close();
// }

#ifndef ZrlCLI_HPP
#define ZrlCLI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZuPtr.hpp>
#include <zlib/ZuInt.hpp>
#include <zlib/ZuString.hpp>

#include <zlib/ZmFn.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZtArray.hpp>

#include <zlib/ZrlEditor.hpp>

namespace Zrl {

class ZrlAPI CLI : public Editor {
public:
  void init(App app);		// set up callbacks
  void final();			// optional teardown

  void open();			// open terminal
  using Editor::close;		// void close() - close terminal

  void start(ZuString prompt);	// start running
  void stop();			// stop running
  void join();			// block until EOF, signal or other end event
  using Editor::running;	// bool running() - check if running

  void invoke(ZmFn<> fn);	// invoke fn in terminal thread

  // prompt() must be called within terminal thread
  void prompt(ZtArray<uint8_t> prompt);	// set prompt

private:
  ZuPtr<ZmScheduler>	m_sched; // internal thread
};

} // Zrl

#endif /* ZrlCLI_HPP */
