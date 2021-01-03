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

#include <zlib/ZrlEditor.hpp>

namespace Zrl {

Editor::Editor()
{
  // manual initialization of opcode-indexed jump table
 
  m_cmdFn[Op::Nop] = cmdNop;
  m_cmdFn[Op::Mode] = cmdMode;
  m_cmdFn[Op::Push] = cmdPush;
  m_cmdFn[Op::Pop] = cmdPop;

  m_cmdFn[Op::Error] = cmdError;
  m_cmdFn[Op::EndOfFile] = cmdEndOfFile;

  m_cmdFn[Op::SigInt] = cmdSigInt;
  m_cmdFn[Op::SigQuit] = cmdSigQuit;
  m_cmdFn[Op::SigSusp] = cmdSigSusp;

  m_cmdFn[Op::Enter] = cmdEnter;

  m_cmdFn[Op::Up] = cmdUp;
  m_cmdFn[Op::Down] = cmdDown;
  m_cmdFn[Op::Left] = cmdLeft;
  m_cmdFn[Op::Right] = cmdRight;
  m_cmdFn[Op::Home] = cmdHome;
  m_cmdFn[Op::End] = cmdEnd;

  m_cmdFn[Op::FwdWord] = cmdFwdWord;
  m_cmdFn[Op::RevWord] = cmdRevWord;
  m_cmdFn[Op::FwdWordEnd] = cmdFwdWordEnd;
  m_cmdFn[Op::RevWordEnd] = cmdRevWordEnd;

  m_cmdFn[Op::MvMark] = cmdMvMark;

  m_cmdFn[Op::Insert] = cmdInsert;

  m_cmdFn[Op::Clear] = cmdClear;
  m_cmdFn[Op::Redraw] = cmdRedraw;

  m_cmdFn[Op::Paste] = cmdPaste;
  m_cmdFn[Op::Yank] = cmdYank;
  m_cmdFn[Op::Rotate] = cmdRotate;

  m_cmdFn[Op::Glyph] = cmdGlyph;
  m_cmdFn[Op::InsGlyph] = cmdInsGlyph;
  m_cmdFn[Op::OverGlyph] = cmdOverGlyph;

  m_cmdFn[Op::TransGlyph] = cmdTransGlyph;
  m_cmdFn[Op::TransWord] = cmdTransWord;
  m_cmdFn[Op::TransUnixWord] = cmdTransUnixWord;

  m_cmdFn[Op::LowerWord] = cmdLowerWord;
  m_cmdFn[Op::UpperWord] = cmdUpperWord;
  m_cmdFn[Op::CapWord] = cmdCapWord;

  m_cmdFn[Op::SetMark] = cmdSetMark;
  m_cmdFn[Op::XchMark] = cmdXchMark;

  m_cmdFn[Op::DigitArg] = cmdDigitArg;

  m_cmdFn[Op::Register] = cmdRegister;

  m_cmdFn[Op::FwdGlyphSrch] = cmdFwdGlyphSrch;
  m_cmdFn[Op::RevGlyphSrch] = cmdRevGlyphSrch;

  m_cmdFn[Op::Complete] = cmdComplete;
  m_cmdFn[Op::ListComplete] = cmdListComplete;

  m_cmdFn[Op::Next] = cmdNext;
  m_cmdFn[Op::Prev] = cmdPrev;

  m_cmdFn[Op::ClrIncSrch] = cmdClrIncSrch;
  m_cmdFn[Op::FwdIncSrch] = cmdFwdIncSrch;
  m_cmdFn[Op::RevIncSrch] = cmdRevIncSrch;

  m_cmdFn[Op::PromptSrch] = cmdPromptSrch;
  m_cmdFn[Op::EnterSrchFwd] = cmdEnterSrchFwd;
  m_cmdFn[Op::EnterSrchRev] = cmdEnterSrchRev;
  m_cmdFn[Op::AbortSrch] = cmdAbortSrch;

  m_cmdFn[Op::FwdSearch] = cmdFwdSearch;
  m_cmdFn[Op::RevSearch] = cmdRevSearch;
}

void Editor::init(Config config, App app)
{
  m_config = config;
  m_app = app;
}
void Editor::final()
{
  m_app = {};
}

void Editor::open(ZmScheduler *sched, unsigned thread)
{
  m_tty.open(sched, thread);
}
void Editor::close()
{
  m_tty.close();
}
void Editor::isOpen() const
{
  return m_tty.isOpen();
}

// start/stop
void Editor::start(ZtString prompt)
{
  m_tty.start(
      [this, prompt = ZuMv(prompt), app = ZuMv(app)]() mutable {
	m_context = {
	  .prompt = ZuMv(prompt),
	  .histSaveOff = m_config.histOffset
	};
	m_tty.ins(m_context.prompt);
	m_context.startPos = m_tty.pos();
      },
      Terminal::KeyFn{this, [](Editor *this_, int32_t vkey) {
	return this_->process(vkey);
      }});
}
void Editor::stop()
{
  m_tty.stop();
}
bool Editor::running() const
{
  return m_tty.running();
}

void Editor::prompt(ZtString prompt)
{
  m_context.prompt = ZuMv(prompt);
}
bool Editor::addMapping(unsigned mode, int vkey, CmdSeq cmds)
{
  if (ZuUnlikely(m_modes.length() <= mode)) m_modes.grow(mode + 1);
  m_cmds.add(new CmdMapping{mode, vkey, ZuMv(cmds)});
}
bool Editor::addMode(unsigned mode, CmdSeq cmds, bool edit)
{
  if (ZuUnlikely(m_modes.length() <= mode)) m_modes.grow(mode + 1);
  m_modes[mode] = Mode{ZuMv(cmds), edit};
}

void Editor::reset()
{
  m_cmds.clean();
  m_modes.clean();
}

bool Editor::process(int32_t vkey)
{
  if (vkey == VKey::Wake) return true;
  if (CmdMapping *mapping =
      m_cmds.find(CmdMapping::Key{m_context.mode, vkey}))
    return process_(mapping->cmds, vkey);
  return process_(m_modes[m_context.mode].cmds, vkey);
}

bool Editor::process_(const CmdSeq &cmds, int32_t vkey)
{
  for (auto cmd : cmds) {
    if (cmd.op() & Op::Arg)
      if (auto arg = m_context.evalArg())
	cmd = Cmd{cmd.op(), arg};
    if (!(cmd.op() & Op::KeepArg)) m_context.clrArg();
    int32_t vkey_ = cmd.vkey();
    if (!vkey_) vkey_ = vkey;
    if (auto fn = m_cmdFns[cmd.op & Op::Mask]) {
      bool stop = (this->*fn)(cmd, vkey_)
      m_context.prevCmd = cmd;
      if (stop) return true;
    }
  }
  ZeError e;
  if (m_tty.write(&e) != Zi::OK) {
    m_app.error(e);
    return true;
  }
  return false;
}

bool Editor::cmdNop(Cmd, int32_t) { return false; } // unused

bool Editor::cmdMode(Cmd, int32_t) { m_context.mode = cmd.arg(); return false; }

bool Editor::cmdError(Cmd, int32_t) { m_app.end(); return true; }
bool Editor::cmdEndOfFile(Cmd, int32_t) { m_app.end(); return true; }

bool Editor::cmdSigInt(Cmd, int32_t) { return m_app.sig(SIGINT); }
bool Editor::cmdSigQuit(Cmd, int32_t) { return m_app.sig(SIGQUIT); }
bool Editor::cmdSigSusp(Cmd, int32_t) { return m_app.sig(SIGTSTP); }

bool Editor::cmdEnter(Cmd, int32_t)
{
  m_tty.crnl_();
  m_tty.write();
  m_context.markPos = -1;
  const auto &data = m_tty.line().data();
  ZuArray<const uint8_t> s{data.data(), data.length()};
  s.offset(m_context.startPos);
  m_app.histSave(m_context.histSaveOff++, s);
  bool stop = m_app.enter(s);
  m_tty.clear();
  if (stop) return true;
  m_tty.ins(m_context.prompt);
  m_context.startPos = m_tty.pos();
  return true;
}

ZuArray<const uint8_t> Editor::substr(unsigned begin, unsigned end)
{
  return {line.data().data() + begin, end - begin};
}

// align cursor within line if not in an edit mode - returns true if moved
bool Editor::align(unsigned pos)
{
  const auto &line = m_tty.line();
  if (ZuUnlikely(!m_modes[m_context.mode].edit &&
      pos > m_context.startPos &&
      pos >= line.width())) {
    m_tty.mv(line.align(pos - 1));
    return true;
  }
  return false;
}

// splice data in line - clears histLoadOff since line is being modified
void Editor::splice(
    unsigned off, ZuUTFSpan span,
    ZuArray<const uint8_t> replace, ZuUTFSpan rspan)
{
  m_context.histLoadOff = -1;
  m_tty.splice(off, span, replace, rspan);
}

// perform copy/del/move in conjunction with a cursor motion
void Editor::motion(unsigned op, unsigned pos, unsigned begin, unsigned end)
{
  ZuArray<const uint8_t> s;
  if (op & (Op::Copy | Op::Del)) s = substr(begin, end);
  if (op & Op::Copy) {
    if (m_context.register_ >= 0)
      m_context.registers.set(m_context.register_) = s;
    else
      m_context.registers.push() = s;
  }
  if (op & Op::Del) {
    auto span = ZuUTF<uint32_t, uint8_t>::span(s);
    if (pos < m_tty.pos()) m_tty.mv(pos);
    m_context.histLoadOff = -1;
    splice(begin, span, ZuArray<const uint8_t>{}, ZuUTFSpan{});
    align(pos);
    return;
  }
  if (op & Op::Mv) if (!align(pos)) m_tty.mv(pos);
}

// maintains consistent horizontal position during vertical movement
unsigned Editor::horizPos()
{
  unsigned prevOp = m_context.prevCmd.op();
  if (prevOp != Op::Up && prevOp != Op::Down)
    m_context.horizPos = m_tty.pos();
  return m_context.horizPos;
}

bool Editor::cmdUp(Cmd cmd, int32_t vkey)
{
  int arg = cmd.arg();
  unsigned pos = horizPos();
  unsigned width = m_tty.width();
  if (arg <= 1 && (cmd.op() & (Op::Mv | Op::Del | Op::Copy)) == Op::Mv &&
      pos < m_context.startPos + width)
    return this->cmd<Op::Prev>(cmd, vkey);
  if (!arg) arg = 1;
  const auto &line = m_tty.line();
  auto posIndex = line.position(pos);
  unsigned end = posIndex.mapping();
  if (pos < m_context.startPos + width * arg) {
    pos = (pos % width);
    if (pos < m_context.startPos) pos += width;
  } else
    pos -= width * arg;
  m_context.horizPos = pos;
  pos = line.align(pos);
  unsigned begin = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end);
  return false;
}

bool Editor::cmdDown(Cmd cmd, int32_t vkey)
{
  int arg = cmd.arg();
  unsigned pos = horizPos();
  unsigned width = m_tty.width();
  const auto &line = m_tty.line();
  unsigned finPos = line.width();
  if (arg <= 1 && (cmd.op() & (Op::Mv | Op::Del | Op::Copy)) == Op::Mv &&
      pos + width > finPos)
    return this->cmd<Op::Next>(cmd, vkey);
  if (!arg) arg = 1;
  auto posIndex = line.position(pos);
  unsigned begin = posIndex.mapping();
  if (pos + width * arg > finPos) {
    pos = (finPos - (finPos % width)) + (pos % width);
    if (pos > finPos) pos -= width;
  } else
    pos += width * arg;
  m_context.horizPos = pos;
  pos = line.align(pos);
  unsigned end = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end);
  return false;
}

bool Editor::cmdLeft(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      begin = line.revGlyph(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdRight(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      end = line.fwdGlyph(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdHome(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  unsigned end = line.position(pos).mapping();
  pos = m_context.startPos;
  unsigned begin = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end);
  return false;
}

bool Editor::cmdEnd(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  unsigned begin = line.position(pos).mapping();
  pos = line.width();
  unsigned end = line.length();
  if (begin < end) motion(cmd.op(), pos, begin, end);
  return false;
}

bool Editor::cmdFwdWord(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      if (cmd.op & Op::Unix)
	end = line.fwdUnixWord(end);
      else
	end = line.fwdWord(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdRevWord(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      if (cmd.op & Op::Unix)
	begin = line.revUnixWord(begin);
      else
	begin = line.revWord(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdFwdWordEnd(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      if (cmd.op & Op::Unix)
	end = line.fwdUnixWordEnd(end);
      else
	end = line.fwdWordEnd(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdRevWordEnd(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      if (cmd.op & Op::Unix)
	begin = line.revUnixWordEnd(begin);
      else
	begin = line.revWordEnd(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

bool Editor::cmdMvMark(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  auto posIndex = line.position(pos);
  if (pos == m_context.markPos) return;
  unsigned begin, end;
  if (pos > m_context.markPos) {
    end = posIndex.mapping();
    pos = m_context.markPos;
    posIndex = line.position(pos);
    begin = posIndex.mapping();
  } else {
    begin = posIndex.mapping();
    pos = m_context.markPos;
    posIndex = line.position(pos);
    pos -= posIndex.off();
    end = posIndex.mapping();
  }
  if (begin < end) motion(cmd.op(), pos, begin, end);
  return false;
}

bool Editor::cmdInsert(Cmd, int32_t)
{
  m_context.overwrite = !m_context.overwrite;
  return false;
}

bool Editor::cmdClear(Cmd, int32_t) { m_tty.cls(); return false; }
bool Editor::cmdRedraw(Cmd, int32_t) { m_tty.redraw(); return false; }

bool Editor::cmdPaste(Cmd, int32_t)
{
  int index = m_context.register_;
  if (index < 0) index = 0;
  if (const auto &data = m_context.registers.get(index)) {
    if (ZuUnlikely(m_context.overwrite))
      m_tty.over(data);
    else {
      const auto &line = m_tty.line();
      if (line.length() + data.length() < m_config.maxLineLen)
	m_tty.ins(data);
    }
  }
  return false;
}

bool Editor::cmdYank(Cmd cmd, int32_t)
{
  int arg = cmd.arg();
  // if (arg < 1) arg = 1; // redundant
  for (int i = 1; i < arg; i++) m_context.registers.rotate(); // Emacs
  if (const auto &data = m_context.registers.yank()) {
    if (ZuUnlikely(m_context.overwrite))
      m_tty.over(data);
    else {
      const auto &line = m_tty.line();
      if (line.length() + data.length() < m_config.maxLineLen)
	m_tty.ins(data);
    }
    align(m_tty.pos());
  }
  return false;
}
bool Editor::cmdRotate(Cmd cmd, int32_t)
{
  if (m_context.prevCmd.op() != Op::Yank) return false;
  const auto &data = m_context.registers.yank();
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  for (int i = 0; i < arg; i++) m_context.registers.rotate(); // Emacs
  const auto &replace = m_context.registers.yank();
  if (&data == &replace) return false;
  auto span = ZuUTF<uint32_t, uint8_t>::span(data);
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  auto rspan = ZuUTF<uint32_t, uint8_t>::span(replace);
  if (rspan.outLen() > span.outLen() &&
      line.length() + (rspan.outLen() - span.outLen()) >= m_config.maxLineLen)
    return false;
  unsigned width = span.width();
  if (pos < m_context.startPos + width)
    pos = m_context.startPos;
  else
    pos -= width;
  m_tty.mv(pos);
  splice(line.position(pos).mapping(), span, replace, rspan);
  align(m_tty.pos());
  return false;
}

bool Editor::glyph(Cmd cmd, int32_t vkey, bool overwrite)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  ZuArrayN<uint8_t, 4> data;
  data.length(ZuUTF8::out(data.data(), 4, vkey));
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  const auto &line = m_tty.line();
  for (int i = 0; i < arg; i++) {
    if (overwrite)
      m_tty.over(data);
    else {
      if (line.length() + data.length() >= m_config.maxLineLen) break;
      m_tty.ins(data);
    }
  }
  align(m_tty.pos());
  return false;
}
bool Editor::cmdGlyph(Cmd cmd, int32_t vkey)
{
  return glyph(cmd, vkey, m_context.overwrite);
}
bool Editor::cmdInsGlyph(Cmd cmd, int32_t vkey)
{
  return glyph(cmd, vkey, false);
}
bool Editor::cmdOverGlyph(Cmd cmd, int32_t vkey)
{
  return glyph(cmd, vkey, true);
}

bool Editor::cmdTransGlyph(Cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
  const auto &line = m_tty.line();
  unsigned rbegin = line.position(pos).mapping();
  unsigned rend = rbegin + line.byte(rbegin).len();
  unsigned lbegin = line.revGlyph(rbegin);
  const auto &data = line.data();
  ZtArray<uint8_t> replace;
  replace.length(rend - lbegin);
  memcpy(&replace[0], &data[rbegin], rend - rbegin);
  memcpy(&replace[rend - rbegin], &data[lbegin], rbegin - lbegin);
  auto span = ZuUTF<uint32_t, uint8_t>::span(this->substr(lbegin, rend));
  splice(lbegin, span, replace, span);
}
bool Editor::cmdTransWord(Cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
  const auto &line = m_tty.line();
  unsigned start = line.position(m_context.startPos).mapping();
  unsigned rend = line.position(pos).mapping();
  rend = line.fwdWordEnd(rend);
  unsigned rbegin = line.revWord(rend);
  unsigned mbegin = rbegin;
  do {
    if (mbegin <= start) return false;
    mbegin = line.revWordEnd(mbegin);
  } while (!line.isword_(mbegin));
  unsigned lbegin = line.revWord(mbegin);
  if (lbegin <= start) return false;
  mbegin = line.fwdGlyph(mbegin);
  rend = line.fwdGlyph(rend);
  ZtArray<uint8_t> replace;
  replace.length(rend - lbegin);
  const auto &data = line.data();
  memcpy(&replace[0], &data[rbegin], rend - rbegin);
  memcpy(&replace[rend - rbegin], &data[mbegin], rbegin - mbegin);
  memcpy(&replace[rend - mbegin], &data[lbegin], mbegin - lbegin);
  auto span = ZuUTF<uint32_t, uint8_t>::span(this->substr(lbegin, rend));
  splice(lbegin, span, replace, span);
}
bool Editor::cmdTransUnixWord(Cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
  const auto &line = m_tty.line();
  unsigned start = line.position(m_context.startPos).mapping();
  unsigned rend = line.position(pos).mapping();
  rend = line.fwdUnixWordEnd(rend);
  unsigned rbegin = line.revUnixWord(rend);
  if (rbegin <= start) return false;
  unsigned mbegin = line.revUnixWordEnd(rbegin);
  if (mbegin <= start) return false;
  unsigned lbegin = line.revUnixWord(mbegin);
  if (lbegin <= start) return false;
  mbegin = line.fwdGlyph(mbegin);
  rend = line.fwdGlyph(rend);
  ZtArray<uint8_t> replace;
  replace.length(rend - lbegin);
  const auto &data = line.data();
  memcpy(&replace[0], &data[rbegin], rend - rbegin);
  memcpy(&replace[rend - rbegin], &data[mbegin], rbegin - mbegin);
  memcpy(&replace[rend - mbegin], &data[lbegin], mbegin - lbegin);
  auto span = ZuUTF<uint32_t, uint8_t>::span(this->substr(lbegin, rend));
  splice(lbegin, span, replace, span);
}

void Editor::transformWord(TransformWordFn fn, void *fnContext)
{
  unsigned pos = m_tty.pos();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return;
  const auto &line = m_tty.line();
  unsigned start = line.position(m_context.startPos).mapping();
  unsigned end = line.position(pos).mapping();
  if (!line.isword_(end)) return;
  end = line.fwdWordEnd(end);
  unsigned begin = line.revWord(end);
  if (begin <= start) return;
  end = line.fwdGlyph(end);
  ZtArray<uint8_t> replace;
  replace.length(end - begin);
  const auto &data = line.data();
  memcpy(&replace[0], &data[begin], end - begin);
  fn(fnContext, replace);
  auto span = ZuUTF<uint32_t, uint8_t>::span(substr(begin, end));
  splice(lbegin, span, replace, span);
}

static bool isupper__(char c) { return c >= 'A' && c <= 'Z'; }
static char toupper__(char c)
{
  return c + (static_cast<int>('A') - static_cast<int>('a'));
}
static bool islower__(char c) { return c >= 'a' && c <= 'z'; }
static char tolower__(char c)
{
  return c + (static_cast<int>('a') - static_cast<int>('A'));
}

typedef void (*TransformCharFn)(uint8_t, uint8_t &);

static TransformWordFn transformChar(TransformCharFn fn)
{
  return [](ZuArray<uint8_t> replace, void *fn_) {
    auto fn = reinterpret_cast<TransformCharFn>(fn_);
    for (unsigned i = 0, n = replace.length(); i < n; ) {
      uint8_t c = replace[i];
      unsigned j = ZuUTF8::in(c);
      if (!j) return;
      fn(c, replace[i]);
      i += j;
    }
  };
}

bool Editor::cmdLowerWord(Cmd, int32_t)
{
  TransformCharFn fn = [](uint8_t c, uint8_t &replace) {
    if (isupper__(c)) replace = tolower__(c);
  };
  transformWord(transformChar(fn), fn);
  return false;
}
bool Editor::cmdUpperWord(Cmd, int32_t)
{
  TransformCharFn fn = [](uint8_t c, uint8_t &replace) {
    if (islower__(c)) replace = toupper__(c);
  }
  transformWord(transformChar(fn), fn);
  return false;
}
bool Editor::cmdCapWord(Cmd, int32_t)
{
  transformWord([](ZuArray<uint8_t> replace, void *) {
    if (unsigned n = replace.length()) {
      uint8_t c = replace[0];
      if (isupper__(c)) {
	if (n < 2 || isupper__(replace[1])) { // lc
	  replace[0] = tolower__(c);
	  ++i;
	  while (i < n) {
	    uint8_t c = replace[i];
	    unsigned j = ZuUTF8::in(c);
	    if (!j) return;
	    if (isupper__(c)) replace[i] = tolower__(c);
	    i += j;
	  }
	} else { // uc
	  ++i;
	  while (i < n) {
	    uint8_t c = replace[i];
	    unsigned j = ZuUTF8::in(c);
	    if (!j) return;
	    if (islower__(c)) replace[i] = toupper__(c);
	    i += j;
	  }
	}
      } else if (islower__(c)) { // ucfirst
	replace[0] = toupper__(c);
	++i;
	while (i < n) {
	  uint8_t c = replace[i];
	  unsigned j = ZuUTF8::in(c);
	  if (!j) return;
	  if (isupper__(c)) replace[i] = tolower__(c);
	  i += j;
	}
      }
    }
  }, nullptr);
  return false;
}

bool Editor::cmdSetMark(Cmd, int32_t)
{
  m_context.markPos = m_tty.pos();
  return false;
}
bool Editor::cmdXchMark(Cmd, int32_t)
{
  int pos = m_context.markPos;
  if (pos >= static_cast<int>(m_context.startPos)) {
    int finPos = m_tty.line().width();
    if (pos > finPos) pos = finPos;
    m_context.markPos = m_tty.pos();
    if (!align(pos)) m_tty.mv(pos);
  }
  return false;
}

static bool isdigit__(char c) { return c >= '0' && c <= '9'; }
static unsigned digit__(char c) { return c - '0'; }

bool Editor::cmdDigitArg(Cmd, int32_t vkey)
{
  if (vkey >= static_cast<int>('0') && vkey <= static_cast<int>('9'))
    m_context.accumDigit(vkey - '0');
  return false;
}

bool Editor::cmdRegister(Cmd, int32_t vkey)
{
  if (vkey > 0 && vkey <= static_cast<int>(CHAR_MAX))
    m_context.register_ = m_context.registers.index(vkey);
  else
    m_context.register_ = -1;
  return false;
}

bool Editor::cmdFwdGlyphSrch(Cmd, int32_t vkey)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      end = line.fwdSearch(end, vkey);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}
bool Editor::cmdRevGlyphSrch(Cmd, int32_t vkey)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      begin = line.revSearch(begin, vkey);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end);
  }
  return false;
}

void Editor::initComplete()
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  m_context.compPrefixEnd = line.position(pos).mapping();
  if (pos == line.width()) pos = line.align(pos - 1);
  unsigned start = line.position(m_context.startPos).mapping();
  unsigned end = line.position(pos).mapping();
  end = line.fwdUnixWordEnd(end);
  unsigned begin = line.revUnixWord(end);
  if (begin < start) begin = start;
  end = line.fwdGlyph(end);
  if (end < begin) end = begin;
  m_context.compPrefixOff = begin;
  m_context.compPrefixEnd = m_context.compEnd = end;
  m_context.compSpan = {};
}
void Editor::startComplete()
{
  m_app.compInit(substr(m_context.compPrefixOff, m_context.compPrefixEnd));
}
void Editor::completed(ZuArray<const uint8_t> data)
{
  auto span = m_context.compSpan;
  auto rspan = ZuUTF<uint32_t, uint8_t>::span(data);
  const auto &line = m_tty.line();
  m_tty.mv(line.position(m_context.compPrefixEnd));
  if (rspan.outLen() > span.outLen() &&
      line.length() + (rspan.outLen() - span.outLen()) >= m_config.maxLineLen)
    return;
  splice(m_context.compPrefixEnd, span, data, rspan);
  m_context.compEnd = m_context.compPrefixEnd + data.length();
  m_context.compSpan = rspan;
}
bool Editor::cmdComplete(Cmd, int32_t)
{
  unsigned prevOp = m_context.prevCmd.op();
  if (prevOp != Op::Complete) {
    if (prevOp != Op::ListComplete) initComplete();
    startComplete();
  }
  ZuString suffix;
  if (m_app.compNext(&suffix) || m_app.compNext(&suffix))
    completed(suffix);
  return false;
}
bool Editor::cmdListComplete(Cmd, int32_t)
{
  unsigned prevOp = m_context.prevCmd.op();
  if (prevOp != Op::ListComplete) {
    if (prevOp != Op::Complete) initComplete();
    startComplete();
  }
  m_context.compWidth = 0;
  unsigned ttyWidth = m_tty.width();
  unsigned maxHeight = m_config.maxCompPages * m_tty.height();
  unsigned colWidth, colHeight, nCols;
  auto prefix = substr(m_context.compPrefixOff, m_context.compPrefixEnd);
  auto prefixSpan = ZuUTF<uint32_t, uint8_t>::span(prefix);
  ZuString suffix;
  ZtArray<ZtArray<char>> matches;
  while (m_app.compNext(suffix)) {
    matches.push(ZtString{suffix});
    auto suffixSpan = ZuUTF<uint32_t, uint8_t>::span(suffix);
    auto width = (prefixSpan + suffixSpan).width() + 1;
    if (width > colWidth) colWidth = width;
    // abort if more than maxCompPages of output
    nCols = colWidth > (ttyWidth>>1) ? 1 : ttyWidth / colWidth;
    colHeight = (matches.length() + nCols - 1) / nCols;
    if (colHeight > maxHeight) break;
  }
  if (!matches) return false;
  m_context.compWidth = colWidth = ttyWidth / nCols;
  if (m_tty.write() != Zi::OK) return false;
  m_tty.crnl_();
  for (row = 0; row < colHeight; row++) {
    unsigned i = row;
    for (col = 0; col < nCols; col++) {
      ZuString suffix = matches[i];
      m_tty.out_(prefix);
      m_tty.out_(suffix);
      auto suffixSpan = ZuUTF<uint32_t, uint8_t>::span(suffix);
      auto width = (prefixSpan + suffixSpan).width();
      if (width < m_context.compWidth)
	m_tty.clr_(m_context.compWidth - width);
      if ((i += colHeight) >= matches.length()) break;
    }
    auto width = col * colWidth;
    if (width < ttyWidth)
      m_tty.clrScroll_(ttyWidth - width);
    if (m_tty.write() != Zi::OK) return false;
  }
  m_tty.redraw();
}

void Editor::histLoad(int offset, ZuArray<const uint8_t> data, bool save)
{
  m_context.histLoadOff = offset;
  unsigned pos = m_context.startPos;
  m_tty.mv(pos);
  const auto &line = m_tty.line();
  unsigned begin = line.position(pos).mapping();
  unsigned end = line.length();
  auto orig = substr(begin, end);
  if (save) m_app.histSave(m_context.histSaveOff, orig);
  m_tty.splice(
      begin, ZuUTF<uint32_t, uint8_t>::span(orig),
      data, ZuUTF<uint32_t, uint8_t>::span(data));
}
bool Editor::cmdNext(Cmd cmd, int32_t)
{
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  if (m_context.histLoadOff >= 0 &&
      m_context.histLoadOff <= m_context.histSaveOff - arg) {
    int offset = m_context.histLoadOff + arg;
    ZuString data;
    if (m_app.histLoad(offset, data)) histLoad(offset, data, false);
  }
  return false;
}
bool Editor::cmdPrev(Cmd cmd, int32_t)
{
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  if (m_context.histLoadOff < 0)
    m_context.histLoadOff = m_context.histSaveOff;
  if (m_context.histLoadOff >= arg) {
    bool save = m_context.histLoadOff == m_context.histSaveOff;
    int offset = (m_context.histLoadOff -= arg);
    ZuString data;
    if (m_app.histLoad(offset, data)) histLoad(offset, data, save);
  }
  return false;
}

bool Editor::cmdClrIncSrch(Cmd, int32_t)
{
  m_context.srchTerm = {};
  if (m_context.histLoadOff >= 0 &&
      m_context.histLoadOff < m_context.histSaveOff) {
    m_context.histLoadOff = -1;
    ZuString data;
    int offset = m_context.histSaveOff;
    if (!m_app.histLoad(offset, data)) {
      data = ZuString{};
      offset = -1;
    }
    histLoad(offset, data, false);
  }
  return false;
}
// adds a key to incremental search term
bool Editor::addIncSrch(int32_t vkey)
{
  if (vkey <= 0) return false;
  ZuStringN<4> utf;
  utf.length(ZuUTF8::out(utf, 4, vkey));
  if (m_context.srchTerm.length + utf.length() >= m_config.maxLineLen)
    return false;
  m_context.srchTerm << utf;
  return true;
}

// simple/fast raw substring matcher
bool Editor::match(ZuArray<uint8_t> data)
{
  const auto &term = m_context.srchTerm;
  if (ZuUnlikely(data.length() < term.length())) return false;
  for (unsigned i = 0, n = data.length() - term.length(); i <= n; i++) {
    unsigned j = 0, m = term.length();
    for (; j < m; j++) if (data[i + j] != term[j]) goto cnt;
    return true;
    cnt:static_cast<void>(0); // bah
  }
  return false;
}
// searches forward skipping N-1 matches - returns true if found
bool Editor::searchFwd(int arg)
{
  if (ZuUnlikely(arg <= 0)) return false;
  int offset = m_context.histLoadOff;
  if (ZuUnlikely(offset < 0)) return false;
  ZuString data;
  while (offset < m_context.histSaveOff) {
    if (!m_app.histLoad(++offset, data)) break;
    if (match(data) && !--arg) {
      histLoad(offset, data, false);
      return true;
    }
  }
  return false;
}
// searches backward skipping N-1 matches - returns true if found
bool Editor::searchRev(int arg)
{
  if (ZuUnlikely(arg <= 0)) return false;
  int offset = m_context.histLoadOff;
  if (ZuUnlikely(offset < 0)) offset = m_context.histSaveOff;
  if (ZuUnlikely(!offset)) return false;
  bool save = offset == m_context.histSaveOff;
  ZuString data;
  while (offset > 0) {
    if (!m_app.histLoad(--offset, data)) break;
    if (match(data) && !--arg) {
      histLoad(offset, data, save);
      return true;
    }
  }
  return false;
}

bool Editor::cmdFwdIncSrch(Cmd, int32_t vkey)
{
  if (addIncSrch(vkey)) {
    int arg = cmd.arg();
    if (arg < 1) arg = 1;
    searchFwd(arg);
  }
  return false;
}
bool Editor::cmdRevIncSrch(Cmd, int32_t vkey)
{
  if (addIncSrch(vkey)) {
    int arg = cmd.arg();
    if (arg < 1) arg = 1;
    searchRev(arg);
  }
  return false;
}

bool Editor::cmdPromptSrch(Cmd cmd, int32_t vkey)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  m_context.srchMode = mode;
  m_context.mode = cmd.arg();
  unsigned pos = m_context.startPos;
  m_tty.mv(pos);
  const auto &line = m_tty.line();
  unsigned begin = line.position(pos).mapping();
  unsigned end = line.length();
  auto orig = substr(begin, end);
  if (m_context.histLoadOff < 0)
    m_context.histLoadOff = m_context.histSaveOff;
  if (m_context.histLoadOff == m_context.histSaveOff)
    m_app.histSave(m_context.histSaveOff, orig);
  ZuStringN<4> prompt;
  prompt.length(ZuUTF8::out(prompt, 4, vkey));
  m_context.srchPrmptSpan = ZuUTF<uint32_t, uint8_t>::span(prompt);
  m_context.startPos += m_context.srchPrmptSpan.width();
  m_tty.splice(
      begin, ZuUTF<uint32_t, uint8_t>::span(orig),
      prompt, m_context.srchPrmptSpan);
}

// perform non-incremental search operation (abort, forward or reverse)
struct SearchOp { enum { Abort = 0, Fwd, Rev }; };
void Editor::searchOp(constexpr int op)
{
  unsigned pos = m_context.startPos;
  const auto &line = m_tty.line();
  unsigned begin = line.position(pos).mapping();
  unsigned end = line.length();
  if (op != SearchOp::Abort && end > begin)
    m_context.srchTerm = substr(begin, end);
  m_context.startPos = (pos -= m_context.srchPrmptSpan.width());
  m_tty.mv(pos);
  begin = line.position(pos).mapping();
  auto orig = substr(begin, end);
  ZuString data;
  if (m_app.histLoad(m_context.histSaveOff, data))
    m_context.histLoadOff = m_context.histSaveOff;
  else {
    data = {};
    m_context.histLoadOff = -1;
  }
  bool found = false;
  if (op == SearchOp::Fwd)
    found = searchFwd(1);
  else if (op == SearchOp::Rev)
    found = searchRev(1);
  if (!found)
    m_tty.splice(
	begin, ZuUTF<uint32_t, uint8_t>::span(orig),
	data, ZuUTF<uint32_t, uint8_t>::span(data));
  m_context.mode = m_context.srchMode;
  m_context.srchPrmptSpan = {};
  m_context.srchMode = 0;
}
bool Editor::cmdEnterSrchFwd(Cmd, int32_t)
{
  searchOp(SearchOp::Fwd);
}
bool Editor::cmdEnterSrchRev(Cmd, int32_t)
{
  searchOp(SearchOp::Rev);
}
bool Editor::cmdAbortSrch(Cmd, int32_t)
{
  searchOp(SearchOp::Abort);
}

bool Editor::cmdFwdSearch(Cmd cmd, int32_t)
{
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  searchFwd(arg);
  return false;
}
bool Editor::cmdRevSearch(Cmd cmd, int32_t)
{
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  searchRev(arg);
  return false;
}

} // namespace Zrl
