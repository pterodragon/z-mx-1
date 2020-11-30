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
  void close();

  bool isOpen() const; // blocking

  void start(KeyFn keyFn);
  void stop();

  template <typename ...Args>
  ZuInline void invoke(Args &&... args) {
    m_sched->invoke(m_rxThread, ZuFwd<Args>(args)...);
  }

  // output routines

  void mv(unsigned pos);// move to position
  void out(ZuString s);	// overwrite string
  void ins(ZuString s);	// insert string
  void del(unsigned n); // delete n characters

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

  void addVKey(const char *cap, const char *deflt, int vkey);

  // low-level output

  int write();

  void clr();		// clear to end of line

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
  int			m_vpos = 0;		// vertical motion position
  Zrl::Line		m_line;			// current line
  ZtArray<char>		m_out;			// output buffer

  // output capabilities

  bool			m_am = false;
  bool			m_sam = false;
  bool			m_bw = false;
  bool			m_xenl = false;
  bool			m_mir = false;

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
