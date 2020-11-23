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

#ifndef ZrlTerminal_HPP
#define ZrlTerminal_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <string.h>
#include <locale.h>
#include <unistd.h>	// for isatty
#include <fcntl.h>
#include <termios.h>

#include <sys/ioctl.h>

#include <zlib/ZuUTF.hpp>
#include <zlib/ZuArrayN.hpp>

#include <zlib/ZtString.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZrlTerminfo.hpp>
#include <zlib/ZrlError.hpp>

namespace Zrl {

class ZrlAPI Terminal {
public:
  using KeyFn = ZmFn<int32_t>; // returns true to stop reading 

  Terminal(const ZmTime &interval) : m_interval{interval.millisecs()} { }

  void open(ZmScheduler *sched, unsigned thread);

  bool isOpen() const; // blocking

  void close();

  void start(KeyFn keyFn); // async
  void stop(); // async

  template <typename ...Args>
  ZuInline void invoke(Args &&... args) {
    m_sched->invoke(m_rxThread, ZuFwd<Args>(args)...);
  }

  // output routines

  void move(unsigned pos);

private:
  void open_();
  void close_();
  void close_fds();

  void start_();
  void stop_();

  void wake();
  void wake_();

  void sigwinch();
  void resized();

  // low-level input

  void read();

  // low-level output

  int write();

  void tputs(const char *cap);
  
  template <typename ...Args>
  static char *tiparm(const char *cap, Args &&... args) {
    return ::tiparm(cap, static_cast<int>(ZuFwd<Args>(args))...);
  }

private:
  // configuration (multi-byte keystroke interval timeout)

  int			m_interval;			// milliseconds

  // scheduler / thread

  ZmScheduler		*m_sched = nullptr;
  unsigned		m_thread = 0;

  // device state

  int			m_fd = -1;
  struct termios	m_termios;
  struct sigaction	m_winch;
  int			m_epollFD = -1;
  int			m_wakeFD = -1, m_wakeFD2 = -1;	// wake pipe

  // input state

  ZuRef<VKeyMatch>	m_vkeyMatch;			

  KeyFn			m_keyFn;

  int			m_nextInterval = -1;
  ZuArrayN<uint8_t, 4>	m_utf;
  int			m_utfn = 0;
  const VKeyMatch	*m_nextVKMatch = nullptr;
  ZtArray<int32_t>	m_pending;

  // output state

  int			m_w = 0, m_h = 0;	// width, height of screen
  int			m_pos = 0;		// cursor position
  Zrl::Line		m_line;			// current line
  ZtArray<char>		m_out;			// output buffer

  // input capabilities

  const char		*m_kent = nullptr;	// enter

  const char		*m_kcuu1 = nullptr;	// up
  const char		*m_kUP = nullptr;	// shift-up
  const char		*m_kUP3 = nullptr;	// alt-up
  const char		*m_kUP5 = nullptr;	// ctrl-up
  const char		*m_kUP6 = nullptr;	// ctrl-shift-up

  const char		*m_kcud1 = nullptr;	// down
  const char		*m_kDN = nullptr;	// shift-down
  const char		*m_kDN3 = nullptr;	// alt-down
  const char		*m_kDN5 = nullptr;	// ctrl-down
  const char		*m_kDN6 = nullptr;	// ctrl-shift-down

  const char		*m_kcub1 = nullptr;	// left
  const char		*m_kLFT = nullptr;	// shift-left
  const char		*m_kLFT3 = nullptr;	// alt-left
  const char		*m_kLFT5 = nullptr;	// ctrl-left
  const char		*m_kLFT6 = nullptr;	// ctrl-shift-left

  const char		*m_kcuf1 = nullptr;	// right
  const char		*m_kRIT = nullptr;	// shift-right
  const char		*m_kRIT3 = nullptr;	// alt-right
  const char		*m_kRIT5 = nullptr;	// ctrl-right
  const char		*m_kRIT6 = nullptr;	// ctrl-shift-right

  const char		*m_khome = nullptr;	// home
  const char		*m_kHOM = nullptr;	// shift-home
  const char		*m_kHOM3 = nullptr;	// alt-home
  const char		*m_kHOM5 = nullptr;	// ctrl-home
  const char		*m_kHOM6 = nullptr;	// ctrl-shift-home

  const char		*m_kend = nullptr;	// end
  const char		*m_kEND = nullptr;	// shift-end
  const char		*m_kEND3 = nullptr;	// alt-end
  const char		*m_kEND5 = nullptr;	// ctrl-end
  const char		*m_kEND6 = nullptr;	// ctrl-shift-end

  const char		*m_kich1 = nullptr;	// insert
  const char		*m_kIC = nullptr;	// shift-insert
  const char		*m_kIC3 = nullptr;	// alt-insert
  const char		*m_kIC5 = nullptr;	// ctrl-insert
  const char		*m_kIC6 = nullptr;	// ctrl-shift-insert

  const char		*m_kdch1 = nullptr;	// delete
  const char		*m_kDC = nullptr;	// shift-delete
  const char		*m_kDC3 = nullptr;	// alt-delete
  const char		*m_kDC5 = nullptr;	// ctrl-delete
  const char		*m_kDC6 = nullptr;	// ctrl-shift-delete

  // output capabilities

  bool			m_am = false;
  bool			m_bw = false;

  const char		*m_cr = nullptr;	// carriage return
  const char		*m_ind = nullptr;	// "index" - line feed
  const char		*m_nel = nullptr;	// newline

  const char		*m_hpa = nullptr;	// horizontal position

  const char		*m_cub = nullptr;	// left - "cursor backwards"
  const char		*m_cub1 = nullptr;	// ''
  const char		*m_cuf = nullptr;	// right - "cursor forwards"
  const char		*m_cuf1 = nullptr;	// ''
  const char		*m_cuu = nullptr;	// up - "cursor up"
  const char		*m_cuu1 = nullptr;	// ''
  const char		*m_cud = nullptr;	// down - "cursor down"
  const char		*m_cud1 = nullptr;	// ''

  const char		*m_el = nullptr;	// erase line
  const char		*m_ech = nullptr;	// erase ch (no cursor motion)

  const char		*m_smir = nullptr;	// start insert
  const char		*m_rmir = nullptr;	// end insert
  const char		*m_ich = nullptr;	// insert characters
  const char		*m_ich1 = nullptr;	// insert character

  const char		*m_smdc = nullptr;	// start delete
  const char		*m_rmdc = nullptr;	// end delete
  const char		*m_dch = nullptr;	// delete characters
  const char		*m_dch1 = nullptr;	// delete character
};

} // Zrl

#endif /* ZrlTerminal_HPP */
