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

namespace Op {
ZrlExtern void print_(uint32_t op, ZmStream &s)
{
  s << name(op & Mask) << '[';
  if (op & Mv) s << "Mv";
  if (op & Del) { if (op & Mv) s << '|'; s << "Del"; }
  if (op & Copy) { if (op & (Mv|Del)) s << '|'; s << "Copy"; }
  if (op & Unix) { if (op & (Mv|Del|Copy)) s << '|'; s << "Unix"; }
  if (op & Arg) { if (op & (Mv|Del|Copy|Unix)) s << '|'; s << "Arg"; }
  if (op & Reg) { if (op & (Mv|Del|Copy|Unix|Arg)) s << '|'; s << "Reg"; }
  if (op & KeepArg) {
    if (op & (Mv|Del|Copy|Unix|Arg|Reg)) s << '|';
    s << "KeepArg";
  }
  s << ']';
}
}

Editor::Editor()
{
  // manual initialization of opcode-indexed jump table
 
  m_cmdFn[Op::Nop] = &Editor::cmdNop;
  m_cmdFn[Op::Mode] = &Editor::cmdMode;
  m_cmdFn[Op::Push] = &Editor::cmdPush;
  m_cmdFn[Op::Pop] = &Editor::cmdPop;

  m_cmdFn[Op::Error] = &Editor::cmdError;
  m_cmdFn[Op::EndOfFile] = &Editor::cmdEndOfFile;

  m_cmdFn[Op::SigInt] = &Editor::cmdSigInt;
  m_cmdFn[Op::SigQuit] = &Editor::cmdSigQuit;
  m_cmdFn[Op::SigSusp] = &Editor::cmdSigSusp;

  m_cmdFn[Op::Enter] = &Editor::cmdEnter;

  m_cmdFn[Op::Up] = &Editor::cmdUp;
  m_cmdFn[Op::Down] = &Editor::cmdDown;
  m_cmdFn[Op::Left] = &Editor::cmdLeft;
  m_cmdFn[Op::Right] = &Editor::cmdRight;
  m_cmdFn[Op::Home] = &Editor::cmdHome;
  m_cmdFn[Op::End] = &Editor::cmdEnd;

  m_cmdFn[Op::FwdWord] = &Editor::cmdFwdWord;
  m_cmdFn[Op::RevWord] = &Editor::cmdRevWord;
  m_cmdFn[Op::FwdWordEnd] = &Editor::cmdFwdWordEnd;
  m_cmdFn[Op::RevWordEnd] = &Editor::cmdRevWordEnd;

  m_cmdFn[Op::MvMark] = &Editor::cmdMvMark;

  m_cmdFn[Op::Insert] = &Editor::cmdInsert;

  m_cmdFn[Op::Clear] = &Editor::cmdClear;
  m_cmdFn[Op::Redraw] = &Editor::cmdRedraw;

  m_cmdFn[Op::Paste] = &Editor::cmdPaste;
  m_cmdFn[Op::Yank] = &Editor::cmdYank;
  m_cmdFn[Op::Rotate] = &Editor::cmdRotate;

  m_cmdFn[Op::Glyph] = &Editor::cmdGlyph;
  m_cmdFn[Op::InsGlyph] = &Editor::cmdInsGlyph;
  m_cmdFn[Op::OverGlyph] = &Editor::cmdOverGlyph;

  m_cmdFn[Op::TransGlyph] = &Editor::cmdTransGlyph;
  m_cmdFn[Op::TransWord] = &Editor::cmdTransWord;
  m_cmdFn[Op::TransUnixWord] = &Editor::cmdTransUnixWord;

  m_cmdFn[Op::LowerWord] = &Editor::cmdLowerWord;
  m_cmdFn[Op::UpperWord] = &Editor::cmdUpperWord;
  m_cmdFn[Op::CapWord] = &Editor::cmdCapWord;

  m_cmdFn[Op::SetMark] = &Editor::cmdSetMark;
  m_cmdFn[Op::XchMark] = &Editor::cmdXchMark;

  m_cmdFn[Op::DigitArg] = &Editor::cmdDigitArg;

  m_cmdFn[Op::Register] = &Editor::cmdRegister;

  m_cmdFn[Op::FwdGlyphSrch] = &Editor::cmdFwdGlyphSrch;
  m_cmdFn[Op::RevGlyphSrch] = &Editor::cmdRevGlyphSrch;

  m_cmdFn[Op::Complete] = &Editor::cmdComplete;
  m_cmdFn[Op::ListComplete] = &Editor::cmdListComplete;

  m_cmdFn[Op::Next] = &Editor::cmdNext;
  m_cmdFn[Op::Prev] = &Editor::cmdPrev;

  m_cmdFn[Op::ClrIncSrch] = &Editor::cmdClrIncSrch;
  m_cmdFn[Op::FwdIncSrch] = &Editor::cmdFwdIncSrch;
  m_cmdFn[Op::RevIncSrch] = &Editor::cmdRevIncSrch;

  m_cmdFn[Op::PromptSrch] = &Editor::cmdPromptSrch;
  m_cmdFn[Op::EnterSrchFwd] = &Editor::cmdEnterSrchFwd;
  m_cmdFn[Op::EnterSrchRev] = &Editor::cmdEnterSrchRev;
  m_cmdFn[Op::AbortSrch] = &Editor::cmdAbortSrch;

  m_cmdFn[Op::FwdSearch] = &Editor::cmdFwdSearch;
  m_cmdFn[Op::RevSearch] = &Editor::cmdRevSearch;

  // default key bindings

  m_defltMap = new Map{ "default" };

  m_defltMap->addMode(0, true);
  m_defltMap->addMode(1, true);

  m_defltMap->bind(0, -VKey::Default, { { Op::Glyph } });

  m_defltMap->bind(0, -VKey::EndOfFile, { { Op::EndOfFile} });

  m_defltMap->bind(0, -VKey::SigInt, { { Op::SigInt} });
  m_defltMap->bind(0, -VKey::SigQuit, { { Op::SigQuit} });
  m_defltMap->bind(0, -VKey::SigSusp, { { Op::SigSusp} });

  m_defltMap->bind(0, -VKey::Enter, { { Op::Enter} });

  m_defltMap->bind(0, -VKey::Erase, { { Op::Left | Op::Del} });
  m_defltMap->bind(0, -VKey::WErase,
      { { Op::RevWord | Op::Del | Op::Unix } });
  m_defltMap->bind(0, -VKey::Kill, { { Op::Home | Op::Del } });

  m_defltMap->bind(0, -VKey::LNext, {
    { Op::Glyph, 0, '^' },
    { Op::Left | Op::Mv },
    { Op::Push, 1 }
  });
  m_defltMap->bind(1, -VKey::Default, { { Op::OverGlyph }, { Op::Pop } });

  m_defltMap->bind(0, -VKey::Up, { { Op::Up | Op::Mv } });
  m_defltMap->bind(0, -VKey::Down, { { Op::Down | Op::Mv } });
  m_defltMap->bind(0, -VKey::Left, { { Op::Left | Op::Mv } });
  m_defltMap->bind(0, -VKey::Right, { { Op::Right | Op::Mv } });

  m_defltMap->bind(0, -VKey::Home, { { Op::Home | Op::Mv } });
  m_defltMap->bind(0, -VKey::End, { { Op::End | Op::Mv } });

  m_defltMap->bind(0, -VKey::Insert, { { Op::Insert } });
  m_defltMap->bind(0, -VKey::Delete, { { Op::Right | Op::Del } });
}

// all parse() functions return v such that
// v <= 0 - parse failure at -v
// v  > 0 - parse success ending at v

int VKey_parse(int32_t &vkey_, ZuString s, int off)
{
  int32_t vkey;
  ZtRegex::Captures c;
  if (ZtREGEX("\G\s*'([^\\])'").m(s, c, off)) { // regular
    off += c[1].length();
    vkey = c[2][0];
  } else if (ZtREGEX("\G\s*'^([@A-Z\[\\\]\^_])'").m(s, c, off)) { // ctrl
    off += c[1].length();
    vkey = c[2][0] - '@';
  } else if (ZtREGEX("\G\s*'\\([^0123x])'").m(s, c, off)) {
    off += c[1].length();
    switch (static_cast<int>(c[2][0])) {
      case 'b': vkey = '\b';   break; // BS
      case 'e': vkey = '\x1b'; break; // Esc
      case 'n': vkey = '\n';   break; // LF
      case 'r': vkey = '\r';   break; // CR
      case 't': vkey = '\t';   break; // Tab
      default: vkey = c[2][0]; break;
    }
  } else if (ZtREGEX("\G\s*'\\x([0-9a-fA-F]{2})'").m(s, c, off)) { // hex
    off += c[1].length();
    vkey = ZuBox<unsigned>{ZuFmt::Hex<>{}, c[2]};
  } else if (ZtREGEX("\G\s*'\\([0-3][0-7]{2})'").m(s, c, off)) { // octal
    off += c[1].length();
    vkey =
      (static_cast<unsigned>(c[2][0] - '0')<<6) |
      (static_cast<unsigned>(c[2][1] - '0')<<3) |
       static_cast<unsigned>(c[2][2] - '0');
  } else if (ZtREGEX("\G\s*\w+").m(s, c, off)) {
    vkey = VKey::lookup(c[1]);
    if (vkey < 0) return -off;
    off += c[1].length();
    if (ZtREGEX("\G\s*\[").m(s, c, off)) {
      off += c[1].length();
      while (ZtREGEX("\G\s*\w+").m(s, c, off)) {
	if (c[1] == "Shift") vkey |= VKey::Shift;
	else if (c[1] == "Ctrl") vkey |= VKey::Ctrl;
	else if (c[1] == "Alt") vkey |= VKey::Alt;
	else return -off;
	off += c[1].length();
	if (!ZtREGEX("\G\s*|").m(s, c, off)) break;
	off += c[1].length();
      }
      if (!ZtREGEX("\G\s*\]").m(s, c, off)) return -off;
      off += c[1].length();
    }
    vkey = -vkey;
  } else
    return -off;
  vkey_ = vkey;
  return off;
}

void Cmd::print_(ZmStream &s) const
{
  s << Op::print(op());
  if (m_value>>16) {
    s << '(';
    if (auto v = arg()) s << ZuBoxed(v);
    s << ", ";
    if (auto v = vkey()) s << VKey::print(v);
    s << ')';
  }
}

int Cmd::parse(ZuString s, int off)
{
  ZtRegex::Captures c;
  if (!ZtREGEX("\G\s*(\w+)").m(s, c, off)) return -off;
  off += c[1].length();
  {
    auto op = Op::lookup(c[2]);
    if (op < 0) return -off;
    m_value = op;
  }
  if (ZtREGEX("\G\s*\[").m(s, c, off)) {
    off += c[1].length();
    while (ZtREGEX("\G\s*(\w+)").m(s, c, off)) {
      if (c[1] = "Mv") m_value |= Op::Mv;
      else if (c[1] == "Del") m_value |= Op::Del;
      else if (c[1] == "Copy") m_value |= Op::Copy;
      else if (c[1] == "Unix") m_value |= Op::Unix;
      else if (c[1] == "Arg") m_value |= Op::Arg;
      else if (c[1] == "Reg") m_value |= Op::Reg;
      else if (c[1] == "KeepArg") m_value |= Op::KeepArg;
      else return -off;
      off += c[1].length();
      if (!ZtREGEX("\G\s*|").m(s, c, off)) break;
      off += c[1].length();
    }
    if (!ZtREGEX("\G\s*\]").m(s, c, off)) return -off;
    off += c[1].length();
  }
  if (ZtREGEX("\G\s*\(").m(s, c, off)) {
    off += c[1].length();
    if (ZtREGEX("\G\s*\d+").m(s, c, off)) {
      off += c[1].length();
      uint64_t arg = ZuBox<unsigned>{c[2]};
      m_value |= (arg<<16);
    }
    if (ZtREGEX("\G\s*,").m(s, c, off)) {
      off += c[1].length();
      int32_t vkey;
      off = VKey_parse(vkey, s, off);
      if (off <= 0) return off;
      m_value |= static_cast<uint64_t>(vkey)<<32;
    }
    if (!ZtREGEX("\G\s*\)").m(s, c, off)) return -off;
    off += c[1].length();
  }
  return off;
}

int CmdSeq_parse(CmdSeq &cmds, ZuString s, int off)
{
  ZtRegex::Captures c;
  if (!ZtREGEX("\G\s*{").m(s, c, off)) return -off;
  off += c[1].length();
  Cmd cmd;
  while (!ZtREGEX("\G\s*}").m(s, c, off)) {
    off = cmd.parse(s, off);
    if (off <= 0) return off;
    if (!ZtREGEX("\G\s*;").m(s, c, off)) return -off;
    off += c[1].length();
    cmds.push(ZuMv(cmd));
  }
  off += c[1].length();
  return off;
}

int Map_::parseMode(ZuString s, int off)
{
  ZtRegex::Captures c;
  if (!ZtREGEX("\G\s*mode\s+(\d+)").m(s, c, off)) return -off;
  off += c[1].length();
  unsigned mode = ZuBox<unsigned>{c[2]};
  bool edit;
  if (ZtREGEX("\G\s+edit").m(s, c, off)) {
    off += c[1].length();
    edit = true;
  } else
    edit = false;
  if (!ZtREGEX("\G\s*{").m(s, c, off)) return -off;
  off += c[1].length();
  addMode(mode, edit);
  int32_t vkey;
  CmdSeq cmds;
  while (!ZtREGEX("\G\s*}").m(s, c, off)) {
    off = VKey_parse(vkey, s, off);
    if (off <= 0) return off;
    off = CmdSeq_parse(cmds, s, off);
    if (off <= 0) return off;
    if (!ZtREGEX("\G\s*;").m(s, c, off)) return -off;
    off += c[1].length();
    bind(mode, vkey, ZuMv(cmds));
    cmds.clear();
  }
  off += c[1].length();
  return off;
}

int Map_::parse(ZuString s, int off)
{
  ZtRegex::Captures c;
  if (!ZtREGEX("\G\s*map\s+(\w+)").m(s, c, off)) return -off;
  off += c[1].length();
  id = c[2];
  if (!ZtREGEX("\G\s*{").m(s, c, off)) return -off;
  off += c[1].length();
  while (!ZtREGEX("\G\s*}").m(s, c, off)) {
    off = parseMode(s, off);
    if (off <= 0) return off;
    if (!ZtREGEX("\G\s*;").m(s, c, off)) return -off;
    off += c[1].length();
  }
  off += c[1].length();
  return off;
}

void Map_printCmdSeq(const CmdSeq &cmds, ZmStream &s)
{
  if (!cmds)
    s << "{}";
  else {
    s << "{ ";
    for (const auto &cmd : cmds) s << cmd << "; ";
    s << "}";
  }
}

void Binding::print_(ZmStream &s) const
{
  s << VKey::print(vkey) << ' ';
  Map_printCmdSeq(cmds, s);
}

thread_local unsigned Map_printIndentLevel = 0;

void Map_printIndent(ZmStream &s)
{
  unsigned level = Map_printIndentLevel;
  for (unsigned i = 0; i < level; i++) s << "  ";
}

void Map_printMode(unsigned i, const Mode &mode, ZmStream &s)
{
  Map_printIndent(s); s << "mode " << ZuBoxed(i) << ' ';
  if (mode.edit) s << "edit ";
  s << " {\r\n";
  ++Map_printIndentLevel;
  if (mode.cmds) {
    Map_printIndent(s); s << "Default ";
    Map_printCmdSeq(mode.cmds, s);
    s << "\r\n";
  }
  if (mode.bindings)
    for (auto i = mode.bindings->readIterator();
	auto binding = i.iterateKey().ptr(); ) {
      Map_printIndent(s); s << *binding << "\r\n";
    }
  --Map_printIndentLevel;
  Map_printIndent(s); s << "}\r\n";
}

void Map_::print_(ZmStream &s) const
{
  Map_printIndent(s); s << "map " << id << " {\r\n";
  ++Map_printIndentLevel;
  for (unsigned i = 0, n = modes.length(); i < n; i++)
    Map_printMode(i, modes[i], s);
  --Map_printIndentLevel;
  Map_printIndent(s); s << "}\r\n";
}

void Editor::dumpMaps_(ZmStream &s) const
{
  auto i = m_maps.readIterator();
  while (auto map = i.iterate()) s << *map;
  s << *m_defltMap;
}

void Map_::addMode(unsigned id, bool edit)
{
  if (ZuUnlikely(modes.length() <= id)) modes.grow(id + 1);
  modes[id] = Mode{{}, {}, edit};
}
void Map_::bind(unsigned id, int vkey, CmdSeq cmds)
{
  if (ZuUnlikely(modes.length() <= id)) modes.grow(id + 1);
  auto &mode = modes[id];
  if (vkey == VKey::Default)
    mode.cmds = ZuMv(cmds);
  else {
    if (!mode.bindings) mode.bindings = new Bindings{};
    mode.bindings->add(new Binding{vkey, ZuMv(cmds)});
  }
}
void Map_::reset()
{
  modes.clear();
}

bool Editor::loadMap(ZuString file)
{
  ZtArray<char> s;
  {
    ZiFile f;
    ZeError e;
    int r;
    if ((r = f.open(file,
	    ZiFile::ReadOnly | ZiFile::GC, 0666, 0, &e)) != Zi::OK) {
      m_loadError.null();
      m_loadError << "open(" << file << "): " << Zi::resultName(r);
      if (r == Zi::IOError) m_loadError << ' ' << e;
      return false;
    }
    unsigned len = f.size();
    if (len >= m_config.maxFileSize) {
      m_loadError.null();
      m_loadError << "open(" << file << "): file too large";
      return false;
    }
    s.length(len);
    if ((r = f.read(s.data(), len, &e)) != Zi::OK) {
      m_loadError.null();
      m_loadError << "read(" << file << "): " << Zi::resultName(r);
      if (r == Zi::IOError) m_loadError << ' ' << e;
      return false;
    }
  }
  ZuPtr<Map> map = new Map{};
  int off = map->parse(s, 0);
  if (off <= 0) {
    off = -off;
    m_loadError.null();
    m_loadError << "read(" << file << "): parsing failed at offset " << off;
    return false;
  }
  if (!map->modes[0].cmds) {
    m_loadError.null();
    m_loadError << "read(" << file << "): mode 0 not defined";
    return false;
  }
  m_maps.add(map.mvptr());
  return true;
}

void Editor::init(Config config, App app)
{
  m_config = config;
  m_map = m_defltMap;
  m_app = app;
  m_tty.init(m_config.vkeyInterval);
}
void Editor::final()
{
  m_tty.final();
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
bool Editor::isOpen() const
{
  return m_tty.isOpen();
}

// start/stop
void Editor::start(StartFn fn)
{
  m_tty.start(
      [this, fn = ZuMv(fn)]() mutable {
	m_context = { .histSaveOff = m_config.histOffset };
	m_tty.opost_on();
	fn(*this);
	m_tty.opost_off();
	m_tty.ins(m_context.prompt);
	m_tty.write();
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

bool Editor::map(ZuString id)
{
  if (auto map = m_maps.find(id)) {
    m_map = map;
    return true;
  }
  return false;
}

void Editor::prompt(ZtArray<uint8_t> prompt)
{
  m_context.prompt = ZuMv(prompt);
}

bool Editor::process(int32_t vkey)
{
  if (!m_map) return false;
  const auto &mode = m_map->modes[m_context.mode];
  if (mode.bindings)
    if (auto kv = mode.bindings->find(vkey))
      return process_(kv->key()->cmds, vkey);
  return process_(mode.cmds, m_tty.literal(vkey));
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
    if (auto fn = m_cmdFn[cmd.op() & Op::Mask]) {
      bool stop = (this->*fn)(cmd, vkey_);
      m_context.prevCmd = cmd;
      if (stop) return true;
    }
  }
  if (m_tty.write() != Zi::OK) {
    m_app.end();
    return true;
  }
  return false;
}

bool Editor::cmdNop(Cmd, int32_t) { return false; } // unused

bool Editor::cmdMode(Cmd cmd, int32_t)
{
  m_context.mode = cmd.arg();
  return false;
}
bool Editor::cmdPush(Cmd cmd, int32_t)
{
  if (ZuLikely(m_context.stack.length() < m_config.maxStackDepth)) {
    m_context.stack.push(m_context.mode);
    m_context.mode = cmd.arg();
  }
  return false;
}
bool Editor::cmdPop(Cmd cmd, int32_t)
{
  m_context.mode = m_context.stack.pop();
  return false;
}

bool Editor::cmdError(Cmd, int32_t) {
  m_tty.opost_on();
  m_app.end();
  m_tty.opost_off();
  return true;
}
bool Editor::cmdEndOfFile(Cmd, int32_t) {
  m_tty.opost_on();
  m_app.end();
  m_tty.opost_off();
  return true;
}

bool Editor::cmdSigInt(Cmd, int32_t) {
  m_tty.opost_on();
  bool stop = m_app.sig(SIGINT);
  m_tty.opost_off();
  return stop;
}
bool Editor::cmdSigQuit(Cmd, int32_t) {
  m_tty.opost_on();
  bool stop = m_app.sig(SIGQUIT);
  m_tty.opost_off();
  return stop;
}
bool Editor::cmdSigSusp(Cmd, int32_t) {
  m_tty.opost_on();
  bool stop = m_app.sig(SIGTSTP);
  m_tty.opost_off();
  return stop;
}

bool Editor::cmdEnter(Cmd, int32_t)
{
  m_tty.crnl_();
  m_tty.write();
  m_context.markPos = -1;
  const auto &data = m_tty.line().data();
  ZuArray<const uint8_t> s{data.data(), data.length()};
  s.offset(m_context.startPos);
  m_context.histLoadOff = -1;
  m_app.histSave(m_context.histSaveOff++, s);
  m_tty.opost_on();
  bool stop = m_app.enter(s);
  m_tty.opost_off();
  m_tty.clear();
  if (stop) return true;
  m_tty.ins(m_context.prompt);
  m_context.startPos = m_tty.pos();
  return false;
}

ZuArray<const uint8_t> Editor::substr(unsigned begin, unsigned end)
{
  const auto &data = m_tty.line().data();
  return {&data[begin], end - begin};
}

// align cursor within line if not in an edit mode - returns true if moved
bool Editor::align(unsigned pos)
{
  const auto &line = m_tty.line();
  if (ZuUnlikely(!m_map->modes[m_context.mode].edit &&
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
void Editor::motion(
    unsigned op, unsigned pos,
    unsigned begin, unsigned end,
    unsigned begPos, unsigned endPos)
{
  ZuArray<const uint8_t> s;
  if (op & (Op::Copy | Op::Del)) s = substr(begin, end);
  if (op & Op::Copy) {
    int index = m_context.register_;
    if (index >= 0)
      m_context.registers.set(index) = s;
    else
      m_context.registers.vi_yank() = s;
  }
  if (op & Op::Del) {
    auto span = ZuUTF<uint32_t, uint8_t>::span(s);
    m_context.histLoadOff = -1;
    if (pos > begPos) {
      if (pos > endPos)
	pos -= (endPos - begPos);
      else
	pos = begPos;
    }
    m_tty.mv(begPos);
    splice(begin, span, ZuArray<const uint8_t>{}, ZuUTFSpan{});
  }
  if (op & (Op::Del | Op::Mv)) if (!align(pos)) m_tty.mv(pos);
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
    return cmdPrev(cmd, vkey);
  if (!arg) arg = 1;
  const auto &line = m_tty.line();
  unsigned end = line.position(pos).mapping();
  unsigned endPos = pos;
  if (pos < m_context.startPos + width * arg) {
    pos = (m_context.startPos - (m_context.startPos % width)) + (pos % width);
    if (pos < m_context.startPos) pos += width;
  } else
    pos -= width * arg;
  m_context.horizPos = pos;
  pos = line.align(pos);
  unsigned begin = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
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
    return cmdNext(cmd, vkey);
  if (!arg) arg = 1;
  unsigned begin = line.position(pos).mapping();
  unsigned begPos = pos;
  if (pos + width * arg > finPos) {
    pos = (finPos - (finPos % width)) + (pos % width);
    if (pos > finPos) {
      if (pos > width)
	pos -= width;
      else
	pos = finPos;
    }
  } else
    pos += width * arg;
  m_context.horizPos = pos;
  pos = line.align(pos);
  unsigned end = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  return false;
}

bool Editor::cmdLeft(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned endPos = pos;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      begin = line.revGlyph(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
  }
  return false;
}

bool Editor::cmdRight(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned begPos = pos;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      end = line.fwdGlyph(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  }
  return false;
}

bool Editor::cmdHome(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  unsigned end = line.position(pos).mapping();
  unsigned endPos = pos;
  pos = m_context.startPos;
  unsigned begin = line.position(pos).mapping();
  if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
  return false;
}

bool Editor::cmdEnd(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  unsigned begin = line.position(pos).mapping();
  unsigned begPos = pos;
  pos = line.width();
  unsigned end = line.length();
  if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  return false;
}

bool Editor::cmdFwdWord(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned begPos = pos;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      if (cmd.op() & Op::Unix)
	end = line.fwdUnixWord(end);
      else
	end = line.fwdWord(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  }
  return false;
}

bool Editor::cmdRevWord(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned endPos = pos;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      if (cmd.op() & Op::Unix)
	begin = line.revUnixWord(begin);
      else
	begin = line.revWord(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
  }
  return false;
}

bool Editor::cmdFwdWordEnd(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned begPos = pos;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      if (cmd.op() & Op::Unix)
	end = line.fwdUnixWordEnd(end);
      else
	end = line.fwdWordEnd(end);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  }
  return false;
}

bool Editor::cmdRevWordEnd(Cmd cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned endPos = pos;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      if (cmd.op() & Op::Unix)
	begin = line.revUnixWordEnd(begin);
      else
	begin = line.revWordEnd(begin);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
  }
  return false;
}

bool Editor::cmdMvMark(Cmd cmd, int32_t vkey)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  auto posIndex = line.position(pos);
  if (ZuLikely(pos != m_context.markPos)) {
    unsigned begin, end, begPos, endPos;
    if (pos > m_context.markPos) {
      end = posIndex.mapping();
      endPos = pos;
      pos = m_context.markPos;
      begPos = pos;
      posIndex = line.position(pos);
      begin = posIndex.mapping();
    } else {
      begin = posIndex.mapping();
      begPos = pos;
      pos = m_context.markPos;
      endPos = pos;
      posIndex = line.position(pos);
      pos -= posIndex.off();
      end = posIndex.mapping();
    }
    if (begin < end) motion(cmd.op(), pos, begin, end, begPos, endPos);
  }
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
  for (int i = 1; i < arg; i++) m_context.registers.emacs_rotate();
  if (const auto &data = m_context.registers.emacs_yank()) {
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
  const auto &data = m_context.registers.emacs_yank();
  int arg = cmd.arg();
  if (arg < 1) arg = 1;
  for (int i = 0; i < arg; i++) m_context.registers.emacs_rotate();
  const auto &replace = m_context.registers.emacs_yank();
  if (&data == &replace) return false;
  auto span = ZuUTF<uint32_t, uint8_t>::span(data);
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  auto rspan = ZuUTF<uint32_t, uint8_t>::span(replace);
  if (rspan.inLen() > span.inLen() &&
      line.length() + (rspan.inLen() - span.inLen()) >= m_config.maxLineLen)
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
  const auto &line = m_tty.line();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
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
  return false;
}
bool Editor::cmdTransWord(Cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
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
  return false;
}
bool Editor::cmdTransUnixWord(Cmd, int32_t)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return false;
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
  return false;
}

void Editor::transformWord(TransformWordFn fn, void *fnContext)
{
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos == line.width()) pos = line.align(pos - 1);
  if (pos <= m_context.startPos) return;
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
  splice(begin, span, replace, span);
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

static Editor::TransformWordFn transformChar(TransformCharFn fn)
{
  return [](void *fn_, ZuArray<uint8_t> replace) {
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
  transformWord(transformChar(fn), reinterpret_cast<void *>(fn));
  return false;
}
bool Editor::cmdUpperWord(Cmd, int32_t)
{
  TransformCharFn fn = [](uint8_t c, uint8_t &replace) {
    if (islower__(c)) replace = toupper__(c);
  };
  transformWord(transformChar(fn), reinterpret_cast<void *>(fn));
  return false;
}
bool Editor::cmdCapWord(Cmd, int32_t)
{
  transformWord([](void *, ZuArray<uint8_t> replace) {
    if (unsigned n = replace.length()) {
      unsigned i = 0;
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

bool Editor::cmdFwdGlyphSrch(Cmd cmd, int32_t vkey)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  unsigned pos = m_tty.pos();
  const auto &line = m_tty.line();
  if (pos < line.width()) {
    unsigned begin = line.position(pos).mapping(), end = begin;
    unsigned begPos = pos;
    unsigned final_ = line.length();
    int arg = cmd.arg();
    do {
      end = line.fwdSearch(end, vkey);
      if (end > final_) end = final_;
    } while (end < final_ && --arg > 0);
    pos = line.byte(end).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, begPos, pos);
  }
  return false;
}
bool Editor::cmdRevGlyphSrch(Cmd cmd, int32_t vkey)
{
  if (ZuUnlikely(vkey <= 0)) return false;
  unsigned pos = m_tty.pos();
  if (pos > m_context.startPos) {
    const auto &line = m_tty.line();
    unsigned end = line.position(pos).mapping(), begin = end;
    unsigned endPos = pos;
    unsigned start = line.position(m_context.startPos).mapping();
    int arg = cmd.arg();
    do {
      begin = line.revSearch(begin, vkey);
      if (begin < start) begin = start;
    } while (begin > start && --arg > 0);
    pos = line.byte(begin).mapping();
    if (begin < end) motion(cmd.op(), pos, begin, end, pos, endPos);
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
  m_tty.mv(line.byte(m_context.compPrefixEnd).mapping());
  if (rspan.inLen() > span.inLen() &&
      line.length() + (rspan.inLen() - span.inLen()) >= m_config.maxLineLen)
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
  if (m_app.compNext(suffix) || m_app.compNext(suffix))
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
  ZtArray<ZtArray<uint8_t>> matches;
  {
    ZuString suffix;
    while (m_app.compNext(suffix)) {
      matches.push(ZuArray<const uint8_t>{suffix});
      auto suffixSpan = ZuUTF<uint32_t, uint8_t>::span(suffix);
      auto width = (prefixSpan + suffixSpan).width() + 1;
      if (width > colWidth) colWidth = width;
      // abort if more than maxCompPages of output
      nCols = colWidth > (ttyWidth>>1) ? 1 : ttyWidth / colWidth;
      colHeight = (matches.length() + nCols - 1) / nCols;
      if (colHeight > maxHeight) break;
    }
  }
  if (!matches) return false;
  m_context.compWidth = colWidth = ttyWidth / nCols;
  if (m_tty.write() != Zi::OK) return false;
  m_tty.crnl_();
  for (unsigned row = 0; row < colHeight; row++) {
    unsigned i = row;
    unsigned col;
    for (col = 0; col < nCols; col++) {
      ZuArray<uint8_t> suffix = matches[i];
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
  return false;
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
      data = ZuArray<const uint8_t>{};
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
  ZuArrayN<uint8_t, 4> utf;
  utf.length(ZuUTF8::out(utf.data(), 4, vkey));
  if (m_context.srchTerm.length() + utf.length() >= m_config.maxLineLen)
    return false;
  m_context.srchTerm << utf;
  return true;
}

// simple/fast raw substring matcher
bool Editor::match(ZuArray<const uint8_t> data)
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

bool Editor::cmdFwdIncSrch(Cmd cmd, int32_t vkey)
{
  if (addIncSrch(vkey)) {
    int arg = cmd.arg();
    if (arg < 1) arg = 1;
    searchFwd(arg);
  }
  return false;
}
bool Editor::cmdRevIncSrch(Cmd cmd, int32_t vkey)
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
  m_context.stack.push(m_context.mode);
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
  ZuArrayN<uint8_t, 4> prompt;
  prompt.length(ZuUTF8::out(prompt.data(), 4, vkey));
  m_context.srchPrmptSpan = ZuUTF<uint32_t, uint8_t>::span(prompt);
  m_context.startPos += m_context.srchPrmptSpan.width();
  m_tty.splice(
      begin, ZuUTF<uint32_t, uint8_t>::span(orig),
      prompt, m_context.srchPrmptSpan);
  return false;
}

// perform non-incremental search operation (abort, forward or reverse)
struct SearchOp { enum { Abort = 0, Fwd, Rev }; };
void Editor::srchEndPrompt(int op)
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
  m_context.mode = m_context.stack.pop();
  m_context.srchPrmptSpan = {};
}
bool Editor::cmdEnterSrchFwd(Cmd, int32_t)
{
  srchEndPrompt(SearchOp::Fwd);
  return false;
}
bool Editor::cmdEnterSrchRev(Cmd, int32_t)
{
  srchEndPrompt(SearchOp::Rev);
  return false;
}
bool Editor::cmdAbortSrch(Cmd, int32_t)
{
  srchEndPrompt(SearchOp::Abort);
  return false;
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
