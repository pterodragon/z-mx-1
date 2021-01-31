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

#include <zlib/ZuString.hpp>
#include <zlib/ZuUTF.hpp>
#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuPrint.hpp>

#include <zlib/ZtString.hpp>

#include <zlib/ZmScheduler.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZrlTerminfo.hpp>
#include <zlib/ZrlLine.hpp>
#include <zlib/ZrlError.hpp>

namespace Zrl {

// a virtual key is UTF32 if positive, otherwise -ve VKey value
namespace VKey {
  ZtEnumerate(
    // terminal driver events and control keys (from termios)
    _,			// unused - overlaps with ^@ when negated
    Null,		// sentinel
    Any,		// any non-Fn key (wildcard for bindings)
    AnyFn,		// any Fn key

    EndOfFile,		// ^D EOF - causes stop

    SigInt,		// ^C interrupt
    SigQuit,		// ^\ quit (ctrl-backslash)
    SigSusp,		// ^Z suspend (SIGTSTP)

    Enter,		// line entered

    Erase,		// ^H backspace
    WErase,		// ^W word erase
    Kill,		// ^U

    LNext,		// ^V

    // motion / Fn keys
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    PgUp,
    PgDn,

    Insert,
    Delete
  );

  enum {
    Mask	= 0x00ff,

    Shift	= 0x0100,
    Ctrl	= 0x0200,	// implies word with left/right
    Alt		= 0x0400	// ''
  };

  inline bool isFn(int32_t vkey) { return vkey < 0 && ((-vkey) & Mask) >= Up; }

  ZrlExtern void print_(int32_t, ZmStream &);
  struct Print {
    int32_t key;
    template <typename S>
    void print(S &s_) const { ZmStream s{s_}; print_(key, s); }
    void print(ZmStream &s) const { print_(key, s); }
    friend ZuPrintFn ZuPrintType(Print *);
  };
  inline Print print(int32_t key) { return {key}; }
}

class ZrlAPI VKeyMatch : public ZuObject {
public:
  struct ZrlAPI Action {
    ZuRef<ZuObject>	next;			// next VKeyMatch
    int32_t		vkey = -VKey::Null;	// virtual key

    void print_(ZmStream &) const;
    template <typename S> void print(S &s_) const { ZmStream s{s_}; print_(s); }
    void print(ZmStream &s) const { print_(s); }
    friend ZuPrintFn ZuPrintType(Action *);
  };

  bool add(const char *s, int32_t vkey) {
    if (!s) return false;
    return add_(this, reinterpret_cast<const uint8_t *>(s), vkey);
  }
  static bool add_(VKeyMatch *this_, const uint8_t *s, int32_t vkey);
  bool add(uint8_t c, int32_t vkey);

  const Action *match(uint8_t c) const;

  void print_(ZmStream &) const;
  template <typename S> void print(S &s_) const { ZmStream s{s_}; print_(s); }
  void print(ZmStream &s) const { print_(s); }
  friend ZuPrintFn ZuPrintType(VKeyMatch *);

private:
  ZtArray<uint8_t>	m_bytes;
  ZtArray<Action>	m_actions;
};

class ZrlAPI Terminal {
public:
  using StartFn = ZmFn<>;
  using KeyFn = ZmFn<int32_t>; // return true to stop reading 

  void init(unsigned vkeyInterval, unsigned highColor, unsigned whiteColor) {
    m_vkeyInterval = vkeyInterval; // milliseconds
    m_highColor = highColor;
    m_whiteColor = whiteColor;
  }
  void final() { }

  void open(ZmScheduler *sched, unsigned thread);
  void close();

  bool isOpen() const; // blocking

  void start(StartFn startFn, KeyFn keyFn);
  void stop();

  bool running() const; // blocking

  template <typename ...Args>
  ZuInline void invoke(Args &&... args) {
    m_sched->invoke(m_thread, ZuFwd<Args>(args)...);
  }

  const Line &line() const { return m_line; } // terminal I/O thread

  // output routines - terminal I/O thread

  unsigned width() const { return m_width; } 
  unsigned height() const { return m_height; } 
  unsigned pos() const { return m_pos; }

  void mv(unsigned pos);	// move to position
  void over(ZuString);		// overwrite string
  void ins(ZuString);		// insert string
  void del(unsigned n);		// delete n characters

  int32_t literal(int32_t vkey) const;	// reverse lookup vkey -> character

  void dumpVKeys_(ZmStream &) const;
  struct DumpVKeys {
    const Terminal &terminal;
    template <typename S>
    void print(S &s_) const { ZmStream s{s_}; terminal.dumpVKeys_(s); }
    void print(ZmStream &s) const { terminal.dumpVKeys_(s); }
    friend ZuPrintFn ZuPrintType(DumpVKeys *);
  };
  DumpVKeys dumpVKeys() const { return {*this}; }

private:
  void open_();
  void close_();
  void close_fds();

  void start_();// idempotent
  void stop_();	// ''

  void wake();
  void wake_();

  void sigwinch();
  void resized();

  bool utf8_in() const { return (m_otermios.c_iflag & IUTF8); }
  bool utf8_out() const { return (m_otermios.c_cflag & CSIZE) == CS8; }

  // low-level input

  void read();

  void addCtrlKey(char c, int32_t vkey);
  void addVKey(const char *cap, const char *deflt, int vkey);

public:
  // output routines
  int write(ZeError *e = nullptr);	// write any pending output

  // all updates to line data go through splice()
  void splice(
      unsigned off, ZuUTFSpan span,
      ZuArray<const uint8_t> replace, ZuUTFSpan rspan);

  void clear();		// clear line data/state
  void cls();		// clear screen, redraw line
  void redraw();	// CR, redraw line
  void redraw(unsigned endPos, bool high);	// redraw cursor -> endPos
private:
  void redraw_(unsigned begPos, unsigned endPos); // redraw line

public:
  // low-level output routines that do not read/update line state
  // - these must be followed by redraw() or cls()
  void crnl_();
  // clear n positions on row, leaving cursor on same row
  void clr_(unsigned n);
  // clear remaining n positions on row, leaving cursor at start of next row
  void clrScroll_(unsigned n);
  // low-level direct output to display
  void out_(ZuString data);

  // turn on output post-processing (i.e. normal CR/NL)
  void opost_on();
  // turn off output post-processing (i.e. raw CR/NL)
  void opost_off();

private:
  void cr();
  void nl();
  void crnl();

  // out row from cursor, leaving cursor at start of next row
  void outScroll(unsigned endPos);
  // clear row from cursor, leaving cursor at start of next row
  void clrScroll(unsigned endPos);
  // out row from cursor, leaving cursor on same row at pos
  void outNoScroll(unsigned endPos, unsigned pos);
  // out row from cursor, clearing any trailing space to endPos
  void outClr(unsigned endPos);
  // out row from cursor to endPos
  void out(unsigned endPos);
  // clear row from cursor to endPos, using ech if possible
  void clr(unsigned endPos);
  // clear row from cursor to endPos, using spaces
  void clrOver(unsigned endPos);

  // cursor motions within row
  void mvhoriz(unsigned pos);
  void mvleft(unsigned pos);
  void mvright(unsigned pos);

  // low-level output routines that do not read/update line state
  void clrErase_(unsigned n);
  void clrOver_(unsigned n);

  bool ins_(unsigned n, bool mir);
  void del_(unsigned n);

  void tputs(const char *cap);
  
  static char *tigetstr(const char *cap) {
    char *s = ::tigetstr(cap);
    if (s == reinterpret_cast<char *>(static_cast<uintptr_t>(-1)))
      s = nullptr;
    return s;
  }
  static bool tigetflag(const char *cap) {
    int i = ::tigetflag(cap);
    if (i < 0) i = 0;
    return i;
  }
  static unsigned tigetnum(const char *cap) {
    int i = ::tigetnum(cap);
    if (i < 0) i = 0;
    return i;
  }
  template <typename ...Args>
  static char *tiparm(const char *cap, Args &&... args) {
    return ::tiparm(cap, static_cast<int>(ZuFwd<Args>(args))...);
  }

private:
  using Lock = ZmPLock;
  using Guard = ZmGuard<Lock>;

  // configuration (multi-byte keystroke interval timeout)

  unsigned		m_vkeyInterval = 100;	// milliseconds
  unsigned		m_highColor = 14;	// ANSI color
  unsigned		m_whiteColor = 15;	// ANSI color

  // scheduler / thread

  ZmScheduler		*m_sched = nullptr;
  unsigned		m_thread = 0;
  Lock			m_lock;	// serializes start/stop

  // device state

  int			m_fd = -1;
  struct termios	m_otermios;
  struct termios	m_ntermios;
  struct sigaction	m_winch;
  int			m_epollFD = -1;
  int			m_wakeFD = -1, m_wakeFD2 = -1;	// wake pipe
  bool			m_running = false;

  // input state

  ZuRef<VKeyMatch>	m_vkeyMatch;			
  KeyFn			m_keyFn;

  // output state (cursor position is relative to beginning of current line)

  unsigned		m_width = 0, m_height = 0;// width, height of screen
  Line			m_line;			// current line
  unsigned		m_pos = 0;		// cursor position (*)
  ZtArray<uint8_t>	m_out;			// output buffer

  // input capabilities

  const char		*m_smkx = nullptr;	// enter keyboard app mode
  const char		*m_rmkx = nullptr;	// leave keyboard app mode

  // output capabilities

  bool			m_am = false;		// auto-margin (crnl at right)
  bool			m_xenl = false;		// auto-margin shenanigans
  bool			m_mir = false;		// insert mode motion ok
  bool			m_hz = false;		// cannot display tilde
  bool			m_ul = false;		// transparent underline

  const char		*m_cr = nullptr;	// carriage return
  const char		*m_ind = nullptr;	// "index" - line feed
  const char		*m_nel = nullptr;	// newline

  const char		*m_clear = nullptr;	// clear screen

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

  const char		*m_smir = nullptr;	// start insert mode
  const char		*m_rmir = nullptr;	// end insert mode
  const char		*m_ich = nullptr;	// insert characters
  const char		*m_ich1 = nullptr;	// insert character

  const char		*m_smdc = nullptr;	// start delete mode
  const char		*m_rmdc = nullptr;	// end delete mode
  const char		*m_dch = nullptr;	// delete characters
  const char		*m_dch1 = nullptr;	// delete character

  const char		*m_bold = nullptr;	// bold
  const char		*m_sgr = nullptr;	// set graphics rendition
  const char		*m_sgr0 = nullptr;	// clear ''
  const char		*m_smso = nullptr;	// end standout mode
  const char		*m_rmso = nullptr;	// end standout mode
  const char		*m_setaf = nullptr;	// set ANSI foreground color

  ZtArray<uint8_t>	m_underline;
};

} // Zrl

#endif /* ZrlTerminal_HPP */
