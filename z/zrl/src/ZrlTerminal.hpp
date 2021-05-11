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

#ifndef _WIN32
#include <fcntl.h>
#include <termios.h>

#include <sys/ioctl.h>
#endif

#include <zlib/ZuString.hpp>
#include <zlib/ZuUTF.hpp>
#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuPrint.hpp>
#include <zlib/ZuPolymorph.hpp>

#include <zlib/ZtString.hpp>

#include <zlib/ZmScheduler.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZrlTerminfo.hpp>
#include <zlib/ZrlLine.hpp>

namespace Zrl {

// a virtual key is UTF32 if positive, otherwise -ve the VKey enum value
namespace VKey {
  ZtEnumValues("Zrl.VKey",
    // terminal driver events and control keys (from termios)
    _,			// unused - overlaps with ^@ when negated

    Null,		// sentinel
    Unset,		// set to Null - for suppressing keys

    // wildcards for key bindings
    Any,		// any key other than motion/Fn and system events
    AnyFn,		// any motion/Fn key
    AnySys,		// any system event

    // system event keys
    EndOfFile,		// ^D EOF - causes stop

    SigInt,		// ^C interrupt
    SigQuit,		// ^\ quit (ctrl-backslash)
    SigSusp,		// ^Z suspend (SIGTSTP)

    // input control keys
    Erase,		// ^H backspace
    WErase,		// ^W word erase
    Kill,		// ^U

    LNext,		// ^V

    Redraw,		// ^R

    // motion/Fn keys
    Enter,		// line entered

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

  inline int32_t wildcard(int32_t vkey) {
    if (vkey >= 0) return -Any;
    int i = (-vkey) & Mask;
    if (i >= Enter) return -AnyFn;
    if (i >= EndOfFile && i <= SigSusp) return -AnySys;
    return Any;
  }

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

class ZrlAPI VKeyMatch : public ZuPolymorph {
public:
  struct ZrlAPI Action {
    ZuRef<ZuPolymorph>	next;			// next VKeyMatch
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
  using ErrorFn = ZmFn<ZuString>;

  using OpenFn = ZmFn<bool>;	// (ok)
  using CloseFn = ZmFn<>;

  using StartFn = ZmFn<>;
  using KeyFn = ZmFn<int32_t>;	// return true to stop reading 

  void init(unsigned vkeyInterval) {
    m_vkeyInterval = vkeyInterval; // milliseconds
  }
  void final() {
    m_errorFn = ErrorFn{};
  }

  void open(ZmScheduler *sched, unsigned thread, OpenFn, ErrorFn);
  void close(CloseFn);

  bool isOpen() const; // blocking
  bool isOpen_() const; // terminal thread

  void start(StartFn startFn, KeyFn keyFn); // start editor
  void stop();

  bool running() const; // blocking

  template <typename ...Args>
  ZuInline void invoke(Args &&... args) {
    m_sched->invoke(m_thread, ZuFwd<Args>(args)...);
  }

  const Line &line() const { return m_line; } // terminal I/O thread

  // can be called before start(), or from within terminal thread once running
  ZtString getpass(ZuString prompt, unsigned passLen); // prompt for passwd

  // output routines - terminal I/O thread

  unsigned width() const { return m_width; } 
  unsigned height() const { return m_height; } 
  unsigned pos() const { return m_pos; }

  void mv(unsigned pos);	// move to position

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
  bool open_();
  void close_();
  void close_fds();

  void start_();// idempotent
  void stop_();	// ''

#ifndef _WIN32
  bool start__();// set tty fd to non-blocking, add it to epoll multiplexer
  void stop__(); // reverse start__()
#endif

  void wake();
  void wake_();

#ifndef _WIN32
  void sigwinch();
#endif
  void resized();

#ifndef _WIN32
  bool utf8_in() const { return (m_otermios.c_iflag & IUTF8); }
  bool utf8_out() const { return (m_otermios.c_cflag & CSIZE) == CS8; }
#else
  static constexpr bool utf8_in() { return true; }
  static constexpr bool utf8_out() { return true; }
#endif

  // low-level input

  void read();

  void addCtrlKey(char c, int32_t vkey);
#ifndef _WIN32
  void addVKey(const char *cap, const char *deflt, int vkey);
#endif

public:
  // output routines
  int write(ZeError *e = nullptr);	// write any pending output

  // all updates to line data go through splice()
  void splice(
      unsigned off, ZuUTFSpan span,
      ZuArray<const uint8_t> replace, ZuUTFSpan rspan,
      bool append);

  void clear();		// clear line data/state
  void cls();		// clear screen, redraw line
  void redraw();	// CR, redraw line
  void redraw(unsigned endPos, bool high);	// redraw cursor -> endPos
private:
  void redraw_(unsigned endPos); // redraw line

public:
  // low-level output routines that do not read/update line state
  // - these must be followed by redraw() or cls()
  void crnl_();
  // clear n positions by overwriting spaces
  void clrOver_(unsigned n);
  // clear remaining n positions on row, wrapping around to next row
  void clrWrap_(unsigned n);
  // clear remaining n positions on row, leaving cursor at start of next row
  void clrBreak_(unsigned n);
  // low-level direct output to display
  void out_(ZuString data);

  // turn on output post-processing (i.e. normal CR/NL)
  void opost_on();
  // turn off output post-processing (i.e. raw CR/NL)
  void opost_off();

  // turn on cursor
  void cursor_on();
  // turn off cursor (e.g. while highlighting)
  void cursor_off();

  unsigned bol(unsigned pos) { return pos - (pos % m_width); }
  unsigned eol(unsigned pos) { return m_line.align(pos + m_width - 1); }

private:
  void cr();
  void nl();
  void crnl();

  // out row from cursor, breaking cursor to start of next row
  void outBreak(unsigned endPos);
  // out row from cursor, wrapping cursor to start of next row
  void outWrap(unsigned endPos);
  // clear row from cursor, wrapping to start of next row
  void clrWrap(unsigned endPos);
  // clear row from cursor, breaking to start of next row
  void clrBreak(unsigned endPos);
  // out row from cursor, leaving cursor on same row at pos
  void outNoWrap(unsigned endPos, unsigned pos);
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

  bool ins_(unsigned n, bool mir);
  void del_(unsigned n);

#ifndef _WIN32
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
#endif

private:
  using Lock = ZmPLock;
  using Guard = ZmGuard<Lock>;

  // configuration (multi-byte keystroke interval timeout)

  unsigned		m_vkeyInterval = 100;	// milliseconds

  // scheduler / thread

  ZmScheduler		*m_sched = nullptr;
  unsigned		m_thread = 0;
  Lock			m_lock;	// serializes start/stop

  // error handler

  ErrorFn		m_errorFn;

  // device state

#ifndef _WIN32
  int			m_fd = -1;
  struct termios	m_otermios;
  struct termios	m_ntermios;
  struct sigaction	m_winch;
  int			m_epollFD = -1;
  int			m_wakeFD = -1, m_wakeFD2 = -1;	// wake pipe
#else /* !_WIN32 */
  HANDLE		m_wake = INVALID_HANDLE_VALUE;
  HANDLE		m_conin = INVALID_HANDLE_VALUE; // must be after m_event
  HANDLE		m_conout = INVALID_HANDLE_VALUE;
  DWORD			m_coninMode;
  UINT			m_coninCP;
  DWORD			m_conoutMode;
  UINT			m_conoutCP;
#endif /* !_WIN32 */

  bool			m_running = false;

  // input state

  ZuRef<VKeyMatch>	m_vkeyMatch;			
  KeyFn			m_keyFn;

  // output state (cursor position is relative to beginning of current line)

  unsigned		m_width = 0, m_height = 0;// width, height of screen
  Line			m_line;			// current line
  unsigned		m_pos = 0;		// cursor position (*)
  ZtArray<uint8_t>	m_out;			// output buffer

#ifndef _WIN32
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
  const char		*m_civis = nullptr;	// invisible cursor
  const char		*m_cnorm = nullptr;	// normal cursor

  ZtArray<uint8_t>	m_underline;
#endif /* !_WIN32 */
};

} // Zrl

#endif /* ZrlTerminal_HPP */
