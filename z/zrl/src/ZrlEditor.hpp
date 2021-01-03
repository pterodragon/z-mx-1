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

// command line interface - line editor

#ifndef ZrlEditor_HPP
#define ZrlEditor_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZrlLib_HPP
#include <zlib/ZrlLib.hpp>
#endif

#include <zlib/ZtArray.hpp>

#include <zlib/ZrlLine.hpp>
#include <zlib/ZrlTerminal.hpp>
#include <zlib/ZrlApp.hpp>

namespace Zrl {

namespace Op { // line editor operation codes
  enum {
    Nop = 0,		// sentinel

    Mode,		// switch mode
    Push,		// push mode (and switch)
    Pop,		// pop mode

    // terminal driver events and control keys (from termios)
    Error,		// I/O error - causes stop
    EndOfFile,		// ^D EOF - causes stop

    SigInt,		// ^C
    SigQuit,		// quit (ctrl-backslash)
    SigSusp,		// ^Z (SIGTSTP)

    Enter,		// line entered

    // Erase,		// ^H backspace		// Left|Del
    // WErase,		// ^W word erase	// RevWord|Del|Unix
    // Kill,		// ^U			// Home|Del

    // LNext,		// ^V		// Glyph(0, '^'),Left|Mv,Push(0)

    // single glyph/row motions
    Up,
    Down,
    Left,
    Right,
    Home,
    End,

    // word motions - Unix flag implies "Unix" white-space delimited word
    FwdWord,
    RevWord,
    FwdWordEnd,
    RevWordEnd,

    MvMark,		// move to glyph mark

    Insert,		// insert/overwrite toggle
    // Delete,		// delete forward	// Right|Del

    Clear,		// clear screen and redraw line
    Redraw,		// redraw line

    Paste,		// pastes register 0 (i.e. most recent cut/copy)
    Yank,		// Emacs "yanks" the top of kill ring (per yank offset)
    Rotate,		// rotates kill ring (increments yank offset modulo 10)

    Glyph,		// insert/overwrite glyph (depending on toggle)
    InsGlyph,		// insert glyph
    OverGlyph,		// overwrite glyph

    TransGlyph,		// transpose glyphs
    TransWord,		// transpose words
    TransUnixWord,	// transpose white-space delimited words

    LowerWord,		// lower-case word
    UpperWord,		// upper-case word
    CapWord,		// capitalize word (rotates through ucfirst, uc, lc)

    SetMark,		// set glyph mark
    XchMark,		// swap cursor with glyph mark

    DigitArg,		// add digit to accumulating argument (needs KeepArg)

    Register,		// specify register (0-9 a-z + *) for next cmd

    // glyph search
    FwdGlyphSrch,	// fwd glyph search
    RevGlyphSrch,	// rev ''

    // auto-completion
    Complete,		// attempt completion
    ListComplete,	// list possible completions

    // history
    Next,		// also triggered by Down from bottom row
    Prev,		// also triggered by Up from top row

    // immediate/incremental search
    ClrIncSrch,		// clear incremental search
    FwdIncSrch,		// append vkey to search term, fwd search
    RevIncSrch,		// '', rev search

    // prompted search
    PromptSrch,		// prompt for non-incremental search
    			//   (stash line, prompt is vkey)
    EnterSrchFwd,	// enter non-incremental search term, fwd search
    EnterSrchRev,	// '', rev search
    AbortSrch,		// abort search prompt, restore line

    // repeat search
    FwdSearch,		// fwd search
    RevSearch,		// rev ''

    N
  };

  // modifiers
  enum {
    Mask	= 0x00ff;

    // Left/Right/Home/End/{Fwd,Rev}*{Word,WordEnd,GlyphSrch}/MvMark
    Mv	 	= 0x0100,	// move cursor
    Del	 	= 0x0200,	// delete span (implies move)
    Copy	= 0x0400,	// copy span (cut is Del + Copy)

    Unix	= 0x0800,	// a "Unix" word is white-space delimited
    Arg		= 0x1000,	// variable argument
    Reg		= 0x2000,	// variable register

    KeepArg	= 0x8000	// preserve argument
  };
};

// line editor command - combination of op code, argument and virtual key
class Cmd {
public:
  Cmd() = default;
  Cmd(const Cmd &) = default;
  Cmd &operator =(const Cmd &) = default;
  Cmd(Cmd &&) = default;
  Cmd &operator =(Cmd &&) = default;
  ~Cmd() = default;

  Cmd(uint32_t op) : m_value{op} { }
  Cmd(uint64_t op, uint64_t arg) : m_value{op | (arg<<16U)} { }
  Cmd(uint64_t op, uint64_t arg, uint64_t vkey) :
      m_value{op | (arg<<16U) | (vkey<<32U)} { }

  auto op() const { return m_value & 0xffffU; }
  auto arg() const { return m_value>>16U; }
  int32_t vkey() const { return m_value>>32U; }

  bool operator !() const { return !m_value; }
  ZuOpBool

private:
  uint64_t	m_value = 0;
};

using CmdSeq = ZtArray<Cmd>;

struct CmdMapping_ { // maps a mode + virtual key to a sequence of commands
  unsigned	mode;
  int		vkey;
  CmdSeq	cmds;

  using Key = ZuPair<unsigned, int>;
  struct KeyAccessor : public ZuAccessor<CmdMapping *, Key> {
    ZuInline static Key value(const CmdMapping *m) {
      return {mode, vkey};
    }
  };
};

using CmdMap =
  ZmHash<CmdMapping_,
    ZmHashObject<ZuNull,
      ZmHashNodeIsKey<true,
	ZmHashIndex<CmdMapping_::KeyAccessor,
	  ZmHashLock<ZmNoLock>>>>>;

using CmdMapping = CmdMap::Node;

// line editor mode
struct Mode {
  CmdSeq	cmds;		// default command sequence
  bool		edit = false;	// edit mode flag
};

using RegData = ZtArray<uint8_t>;
using Register = ZuPtr<RegData>;

class Registers { // maintains a unified Vi/Emacs register store
public:
  static int index(char c) {
    if (ZuLikely(c >= '0' && c <= '9')) return c - '0';	
    if (ZuLikely(c >= 'a' && c <= 'z')) return c - 'a' + 10;
    if (ZuLikely(c >= 'A' && c <= 'Z')) return c - 'A' + 10;
    if (ZuLikely(c == '/')) return 36;		// search string
    if (ZuLikely(c == '+')) return 37;		// clipboard
    if (ZuLikely(c == '*')) return 38;		// alt. clipboard
    return -1;
  }

  const RegData &get(unsigned i) {
    static const RegData null;
    if (RegData *ptr = array[i]) return *ptr;
    return null;
  }
  RegData &set(unsigned i) {
    new (&array[i]) Register{new RegData{}};
    return *(array[i]);
  }

  // Vi yank / put
  RegData &vi_yank() {
    offset = 0;
    if (count < 10) ++count;
    array[9].~Register();
    memmove(&array[1], &array[0], 9 * sizeof(Register));
    return set(0);
  }
  const RegData &vi_put() { return get(0); }

  // Emacs yank / yank-pop
  const RegData &emacs_yank() { return get(offset); }
  void emacs_rotate() {
    if (ZuUnlikely(++offset >= count)) offset = 0;
  }

private:
  ZuArrayN<Register, 39>	m_array;
  unsigned			m_offset = 0;	// mod10 offset
  unsigned			m_count = 0;
};

// command execution context
struct CmdContext {
  // cursor position and line UTF data are maintained by ZrlTerminal

  ZtArray<uint8_t>	prompt;		// current prompt
  unsigned		startPos = 0;	// start position (following prompt)
  unsigned		mode = 0;	// current mode
  Cmd			prevCmd;	// previous command
  unsigned		horizPos = 0;	// vertical motion position
  int			markPos = -1;	// glyph mark position

  // numerical argument context
  unsigned		arg = 0;	// extant arg (survives mode changes)
  unsigned		accumArg = 0;	// accumulating argument

  // glyph search context (search within line)
  uint32_t		search = 0;	// glyph to search for

  // register context
  int			register_ = -1;	// register index
  Registers		registers;	// registers

  // completion context
  unsigned		compPrefixOff = 0;	// completion prefix offset
  unsigned		compPrefixEnd = 0;	// completion prefix end
  unsigned		compEnd = 0;		// current completion end
  ZuUTFSpan		compSpan;		// current completion span
  unsigned		compWidth = 0;		// completion list width

  // history context
  int			histLoadOff = -1;	// history load offset
  unsigned		histSaveOff = 0;	// history save offset

  // history search context
  ZtArray<uint8_t>	srchTerm;	// search term
  ZuUTFSpan		srchPrmptSpan;	// search prompt span
  unsigned		srchMode = 0;	// mode prior to search prompt

  // insert/overwrite mode
  bool			overwrite = false;

  void accumDigit(unsigned i) {
    accumArg = accumArg * 10 + i;
  }

  unsigned evalArg() {
    if (accumArg) {
      if (!arg)
	arg = accumArg;
      else
	arg *= accumArg;
      accumArg = 0;
    }
    return arg;
  }

  void clrArg() { arg = accumArg = 0; }
};

// line editor configuration
struct Config {
  unsigned	maxLineLen = 32768;	// maximum line length
  unsigned	maxCompPages = 5;	// max # pages of possible completions
  unsigned	histOffset = 0;		// initial history offset

  Config(ZvCf *cf) {
    maxLineSize = cf->getInt("maxLineSize", 0, (1<<20), false, 32768);
    maxCompPages = cf->getInt("maxCompPages", 0, 100, false, 5);
    histOffset = cf->getInt("histOffset", 0, UINT_MAX, false, 0);
  }
};

// the line editor is a virtual machine that executes commands
// composed of an opcode, an argument and a virtual key (UTF32 if positive)
class ZrlAPI Editor {
  Editor(const Editor &) = delete;
  Editor &operator =(const Editor &) = delete;
  Editor(Editor &&) = delete;
  Editor &operator =(Editor &&) = delete;

  typedef (Editor::*CmdFn)(Cmd, int32_t);

public:
  Editor();
  ~Editor() = default;

  // initialization/finalization
  void init(Config config, App app);
  void final();

  // terminal open/close
  void open(ZmScheduler *sched, unsigned thread);
  void close();
  void isOpen() const;

  // start/stop
  void start(ZtString prompt);
  void stop();
  bool running() const;

  // all public functions below must be called in the terminal thread
  void prompt(ZtString prompt);

  bool addMapping(unsigned mode, int vkey, CmdSeq cmds);
  bool addMode(unsigned mode, CmdSeq cmds, bool edit);

  void reset();

private:
  bool process(int32_t vkey).
  bool process_(const CmdSeq &cmds, int32_t vkey);

  bool cmdNop(Cmd, int32_t);

  bool cmdMode(Cmd, int32_t);

  bool cmdError(Cmd, int32_t);
  bool cmdEndOfFile(Cmd, int32_t);

  bool cmdSigInt(Cmd, int32_t);
  bool cmdSigQuit(Cmd, int32_t);
  bool cmdSigSusp(Cmd, int32_t);

  bool cmdEnter(Cmd, int32_t);

  ZuArray<const uint8_t> substr(unsigned begin, unsigned end);

  // align cursor within line if not in an edit mode - returns true if moved
  bool align(unsigned pos);
  // splice data in line - clears histLoadOff since line is being modified
  void splice(
      unsigned off, ZuUTFSpan span,
      ZuArray<const uint8_t> replace, ZuUTFSpan rspan);
  // perform copy/del/move in conjunction with a cursor motion
  void motion(unsigned op, unsigned pos, unsigned begin, unsigned end);
  // maintains consistent horizontal position during vertical movement
  unsigned horizPos();

  bool cmdUp(Cmd, int32_t);
  bool cmdDown(Cmd, int32_t);
  bool cmdLeft(Cmd, int32_t);
  bool cmdRight(Cmd, int32_t);
  bool cmdHome(Cmd, int32_t);
  bool cmdEnd(Cmd, int32_t);
  bool cmdFwdWord(Cmd, int32_t);
  bool cmdRevWord(Cmd, int32_t);
  bool cmdFwdWordEnd(Cmd, int32_t);
  bool cmdRevWordEnd(Cmd, int32_t);
  bool cmdMvMark(Cmd, int32_t);

  bool cmdInsert(Cmd, int32_t);

  bool cmdClear(Cmd, int32_t);
  bool cmdRedraw(Cmd, int32_t);

  bool cmdPaste(Cmd, int32_t);

  bool cmdYank(Cmd, int32_t);
  bool cmdRotate(Cmd, int32_t);

  // insert/overwrite glyph
  bool glyph(Cmd, int32_t vkey, bool overwrite);

  bool cmdGlyph(Cmd cmd, int32_t);
  bool cmdInsGlyph(Cmd cmd, int32_t);
  bool cmdOverGlyph(Cmd cmd, int32_t);

  bool cmdTransGlyph(Cmd, int32_t);
  bool cmdTransWord(Cmd, int32_t);
  bool cmdTransUnixWord(Cmd, int32_t);

  typedef void (*TransformWordFn)(void *, ZuArray<uint8_t>);
  void transformWord(TransformWordFn fn);

  bool cmdSetMark(Cmd, int32_t);
  bool cmdXchMark(Cmd, int32_t);

  bool cmdDigitArg(Cmd, int32_t);
  bool cmdRegister(Cmd, int32_t);

  bool cmdFwdGlyphSrch(Cmd, int32_t);
  bool cmdRevGlyphSrch(Cmd, int32_t);

  // initialize possible completions context
  void initComplete();
  // (re-)start possible completions enumeration
  void startComplete();
  // apply auto-completion
  void completed(ZuArray<const uint8_t> data);
  bool cmdComplete(Cmd, int32_t);
  bool cmdListComplete(Cmd, int32_t);

  // loads data previously retrieved from app at history offset
  void histLoad(int offset, ZuArray<const uint8_t> data, bool save);

  bool cmdNext(Cmd, int32_t);
  bool cmdPrev(Cmd, int32_t);

  // adds a key to incremental search term
  bool addIncSrch(int32_t vkey);

  // simple/fast substring matcher - returns true if search term is in data
  bool match(ZuArray<uint8_t> data);
  // searches forward skipping N-1 matches - returns true if found
  bool searchFwd(int arg);
  // searches backward skipping N-1 matches - returns true if found
  bool searchRev(int arg);

  bool cmdClrIncSrch(Cmd, int32_t);
  bool cmdFwdIncSrch(Cmd, int32_t);
  bool cmdRevIncSrch(Cmd, int32_t);

  bool cmdPromptSrch(Cmd cmd, int32_t vkey);

  // perform non-incremental search operation (abort, forward or reverse)
  struct SearchOp { enum { Abort = 0, Fwd, Rev }; };
  void searchOp(int op);

  bool cmdEnterSrchFwd(Cmd, int32_t);
  bool cmdEnterSrchRev(Cmd, int32_t);
  bool cmdAbortSrch(Cmd, int32_t);

  bool cmdFwdSearch(Cmd, int32_t);
  bool cmdRevSearch(Cmd, int32_t);

private:
  CmdFn		m_cmdFns[Op::N] = { nullptr };	// opcode jump table

  Config	m_config;		// configuration

  CmdMap	m_cmds;			// vkey -> command mapping
  ZtArray<Mode>	m_modes;		// modes

  App		m_app;			// application callbacks

  Terminal	m_tty;			// terminal

  CmdContext	m_context;		// command execution context
};

} // Zrl

#endif /* ZrlLine_HPP */
