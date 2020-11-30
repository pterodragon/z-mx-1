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

#include <zlib/ZrlTerminal.hpp>

namespace Zrl {

namespace VKey {
  enum {
    // control events
    Continue = 0,	// continue reading
    Wake,		// woken from input, mid-line
    Error,		// I/O error

    Enter,		// line entered

    EndOfFile,		// ^D EOF

    Erase,		// backspace
    WErase,		// ^W word erase
    Kill,		// ^U

    SigInt,		// ^C
    SigQuit,		// quit (ctrl-backslash)
    SigSusp,		// ^Z

    // motion / Fn keys
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    Insert,
    Delete
  };

  enum {
    Shift	= 0x0100,
    Ctrl	= 0x0200,	// implies word with left/right
    Alt		= 0x0400	// ''
  };
}

struct VKeyMatch : public ZuObject {
  struct Action {
    ZuRef<ZuObject>	next;			// next VKeyMatch
    int32_t		vkey = -1;		// virtual key
  };

  bool add(const char *s, int vkey) {
    if (!s) return false;
    return add_(this, static_cast<const uint8_t *>(s), vkey);
  }
  static bool add_(VKeyMatch *this_, const char *s, int vkey);
  bool add(uint8_t c, int vkey);

  Action *match(uint8_t c);

  ZtArray<uint8_t>	bytes;
  ZtArray<Action>	actions;
};

static bool VKeyMatch::add_(VKeyMatch *this_, const char *s, int vkey)
{
  uint8_t c = *s;
  if (!c) return false;
loop:
  unsigned i, n = this_->bytes.length();
  for (i = 0; i < n; i++) if (this_->bytes[i] == c) goto matched;
  this_->bytes.push(c);
  new (this_->actions.push()) Action{};
matched:
  if (!(c = *++s)) {
    if (this_->actions[i].vkey >= 0) return false;
    this_->actions[i].vkey = vkey;
    return true;
  }
  Action *ptr;
  if (!(ptr = this_->actions[i].next.ptr()))
    this_->actions[i].next = ptr = new Action{};
  this_ = ptr;
  goto loop;
}

bool VKeyMatch::add(uint8_t c, int vkey)
{
  if (!c) return false;
  char s[2];
  s[0] = c;
  s[1] = 0;
  return add(s, vkey);
}

VKeyMatch::Action *VKeyMatch::match(uint8_t c)
{
  unsigned i, n = this_->bytes.length();
  for (i = 0; i < n; i++)
    if (bytes[i] == c) return &actions[i];
  return nullptr;
}

void Terminal::open(ZmScheduler *sched, unsigned thread) // async
{
  m_sched = sched;
  m_thread = thread;
  m_sched->invoke(m_thread,
      ZmFn<>{this, [](Terminal *this_) { this_->open_(); }});
}

bool Terminal::isOpen() const // synchronous
{
  bool ok = false;
  thread_local ZmSemaphore sem;
  m_sched->invoke(m_thread, [this, sem = &sem, ok = &ok]() {
    ok = m_fd >= 0;
    sem->post();
  });
  sem.wait();
  return ok;
}

void Terminal::close() // async
{
  m_sched->invoke(m_thread,
      ZmFn<>{this, [](Terminal *this_) { this_->close_(); }});
}

void Terminal::start(KeyFn keyFn) // async
{
  m_sched->wakeFn(m_thread,
      ZmFn<>{this, [](Terminal *this_) { this_->wake(); }});
  m_sched->run_(m_thread,
      [this, keyFn = ZuMv(keyFn)]() mutable {
	start_();
	m_keyFn = ZuMv(keyFn);
	read();
      });
}

void Terminal::stop() // async
{
  m_sched->wakeFn(m_thread, ZmFn<>());
  m_sched->run_(m_thread, 
      ZmFn<>{this, [](Terminal *this_) { this_->stop_(); }});
  wake_();
}

void Terminal::addVKey(const char *cap, const char *deflt, int vkey)
{
  const char *ent;
  if (!(ent = tigetstr(cap))) ent = deflt;
  m_vkeyMatch->add(ent, vkey);
}

void Terminal::open_()
{
  // open tty device
  m_fd = ::open("/dev/tty", O_RDWR, 0);
  if (m_fd < 0) {
    ZeError e{errno};
    Error("open(\"/dev/tty\")", Zi::IOError, e);
  }

  // save terminal control flags
  memset(&m_termios, 0, sizeof(termios));
  tcgetattr(m_fd, &m_termios);

  // initialize terminfo
  setupterm(nullptr, m_fd, nullptr);

  // obtain output capabilities
  m_am = tigetflag("am");
  m_sam = tigetflag("sam");
  m_bw = tigetflag("bw");
  m_xenl = tigetflag("xenl");
  m_mir = tigetflag("mir");

  // xenl can manifest two different ways. The vt100 way is that, when
  // you'd expect the cursor to wrap, it stays hung at the right margin
  // (on top of the character just emitted) and doesn't wrap until the
  // *next* graphic char is emitted. The c100 way is to ignore LF
  // received just after an am wrap.

  // This is handled by emitting CR/LF after the char and assuming the
  // wrap is done, you're on the first position of the next line, and the
  // terminal is now out of its weird state.

  if (!(m_cr = tigetstr("cr"))) m_cr = "\r";
  if (!(m_ind = tigetstr("ind"))) m_ind = "\n";
  m_nel = tigetstr("nel");

  m_hpa = tigetstr("hpa");

  m_cub = tigetstr("cub");
  if (!(m_cub1 = tigetstr("cub1"))) m_cub1 = "\b";
  m_cuf = tigetstr("cuf");
  m_cuf1 = tigetstr("cuf1");
  m_cuu = tigetstr("cuu");
  m_cuu1 = tigetstr("cuu1");
  m_cud = tigetstr("cud");
  m_cud1 = tigetstr("cud1");

  m_el = tigetstr("el");
  m_ech = tigetstr("ech");

  m_smir = tigetstr("smir");
  m_rmir = tigetstr("rmir");
  m_ich = tigetstr("ich");
  m_ich1 = tigetstr("ich1");

  m_smdc = tigetstr("smdc");
  m_rmdc = tigetstr("rmdc");
  m_dch = tigetstr("dch");
  m_dch1 = tigetstr("dch1");

  // initialize keystroke matcher
  m_vkeyMatch = new VKeyMatch{};

  //		| Normal | Shift |  Alt  | Ctrl  | Ctrl + Shift
  // -----------+--------+-------+-------+-------+-------------
  //  Up	| kcuu1  | kUP   | kUP3  | kUP5  | kUP6
  //  Down	| kcud1  | kDN   | kDN3  | kDN5  | kDN6
  //  Left	| kcub1  | kLFT  | kLFT3 | kLFT5 | kLFT6
  //  Right	| kcuf1  | kRIT  | kRIT3 | kRIT5 | kRIT6
  //  Home	| khome  | kHOM  | kHOM3 | kHOM5 | kHOM6
  //  End	| kend   | kEND  | kEND3 | kEND5 | kEND6
  //  Insert	| kich1  | kIC   | kIC3  | kIC5  | kIC6
  //  Delete	| kdch1  | KDC   | kDC3  | kDC5  | kDC6

  // enter key
  addVKey("kent", "\r", VKey::Enter);

  // EOF
  m_vkeyMatch->add(m_termios.c_cc[VEOF], VKey::EndOfFile);

  // erase keys
  m_vkeyMatch->add(m_termios.c_cc[VERASE], VKey::Erase);
  m_vkeyMatch->add(m_termios.c_cc[VWERASE], VKey::WErase);
  m_vkeyMatch->add(m_termios.c_cc[VKILL], VKey::Kill);

  // signals
  m_vkeyMatch->add(m_termios.c_cc[VINTR], VKey::SigInt);
  m_vkeyMatch->add(m_termios.c_cc[VQUIT], VKey::SigQuit);
  m_vkeyMatch->add(m_termios.c_cc[VSUSP], VKey::SigSusp);

  // motion keys
  addVKey("kcuu1", nullptr, VKey::Up);
  addVKey("kUP", nullptr, VKey::Up | VKey::Shift);
  addVKey("kUP3", nullptr, VKey::Up | VKey::Alt);
  addVKey("kUP5", nullptr, VKey::Up | VKey::Ctrl);
  addVKey("kUP6", nullptr, VKey::Up | VKey::Shift | VKey::Ctrl);

  addVKey("kcud1", nullptr, VKey::Down);
  addVKey("kDN", nullptr, VKey::Down | VKey::Shift);
  addVKey("kDN3", nullptr, VKey::Down | VKey::Alt);
  addVKey("kDN5", nullptr, VKey::Down | VKey::Ctrl);
  addVKey("kDN6", nullptr, VKey::Down | VKey::Shift | VKey::Ctrl);

  addVKey("kcub1", nullptr, VKey::Left);
  addVKey("kLFT", nullptr, VKey::Left | VKey::Shift);
  addVKey("kLFT3", nullptr, VKey::Left | VKey::Alt);
  addVKey("kLFT5", nullptr, VKey::Left | VKey::Ctrl);
  addVKey("kLFT6", nullptr, VKey::Left | VKey::Shift | VKey::Ctrl);

  addVKey("kcuf1", nullptr, VKey::Right);
  addVKey("kRIT", nullptr, VKey::Right | VKey::Shift);
  addVKey("kRIT3", nullptr, VKey::Right | VKey::Alt);
  addVKey("kRIT5", nullptr, VKey::Right | VKey::Ctrl);
  addVKey("kRIT6", nullptr, VKey::Right | VKey::Shift | VKey::Ctrl);

  addVKey("khome", nullptr, VKey::Home);
  addVKey("kHOM", nullptr, VKey::Home | VKey::Shift);
  addVKey("kHOM3", nullptr, VKey::Home | VKey::Alt);
  addVKey("kHOM5", nullptr, VKey::Home | VKey::Ctrl);
  addVKey("kHOM6", nullptr, VKey::Home | VKey::Shift | VKey::Ctrl);

  addVKey("kend", nullptr, VKey::End);
  addVKey("kEND", nullptr, VKey::End | VKey::Shift);
  addVKey("kEND3", nullptr, VKey::End | VKey::Alt);
  addVKey("kEND5", nullptr, VKey::End | VKey::Ctrl);
  addVKey("kEND6", nullptr, VKey::End | VKey::Shift | VKey::Ctrl);

  addVKey("kich1", nullptr, VKey::Insert);
  addVKey("kIC", nullptr, VKey::Insert | VKey::Shift);
  addVKey("kIC3", nullptr, VKey::Insert | VKey::Alt);
  addVKey("kIC5", nullptr, VKey::Insert | VKey::Ctrl);
  addVKey("kIC6", nullptr, VKey::Insert | VKey::Shift | VKey::Ctrl);

  addVKey("kdch1", nullptr, VKey::Delete);
  addVKey("kDC", nullptr, VKey::Delete | VKey::Shift);
  addVKey("kDC3", nullptr, VKey::Delete | VKey::Alt);
  addVKey("kDC5", nullptr, VKey::Delete | VKey::Ctrl);
  addVKey("kDC6", nullptr, VKey::Delete | VKey::Shift | VKey::Ctrl);

  // set up I/O multiplexer (epoll)
  if ((m_epollFD = epoll_create(2)) < 0) {
    Error("epoll_create", Zi::IOError, ZeLastError);
    return;
  }
  if (pipe(&m_wakeFD) < 0) {
    ZeError e{errno};
    close_fds();
    Error("pipe", Zi::IOError, e);
    return;
  }
  if (fcntl(m_wakeFD, F_SETFL, O_NONBLOCK) < 0) {
    ZeError e{errno};
    close_fds();
    Error("fcntl(F_SETFL, O_NONBLOCK)", Zi::IOError, e);
    return;
  }
  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.u64 = 3;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, m_wakeFD, &ev) < 0) {
      ZeError e{errno};
      close_fds();
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return;
    }
  }

  // handle SIGWINCH
  {
    struct sigaction nwinch;
    static Terminal *this_ = nullptr;

    this_ = this;
    memset(&nwinch, 0, sizeof(struct sigaction));
    nwinch.sa_handler = [](int) { this_->sigwinch(); };
    sigemptyset(&nwinch.sa_mask);
    sigaction(SIGWINCH, &nwinch, &m_winch);
  }

  // get terminal width/height
  resized();
}

void Terminal::close_()
{
  stop_(); // idempotent

  // reset SIGWINCH handler
  sigaction(SIGWINCH, &m_winch, nullptr);

  // finalize keystroke matcher
  m_vkeyMatch = nullptr;

  // finalize terminfo
  m_am = false;
  m_sam = false;
  m_bw = false;
  m_xenl = false;
  m_mir = false;

  m_cr = nullptr;
  m_ind = nullptr;
  m_nel = nullptr;

  m_hpa = nullptr;

  m_cub = nullptr;
  m_cub1 = nullptr;
  m_cuf = nullptr;
  m_cuf1 = nullptr;
  m_cuu = nullptr;
  m_cuu1 = nullptr;
  m_cud = nullptr;
  m_cud1 = nullptr;

  m_el = nullptr;
  m_ech = nullptr;

  m_smir = nullptr;
  m_rmir = nullptr;
  m_ich = nullptr;
  m_ich1 = nullptr;

  m_smdc = nullptr;
  m_rmdc = nullptr;
  m_dch = nullptr;
  m_dch1 = nullptr;

  del_curterm(cur_term);

  // close file descriptors
  close_fds();
}

void Terminal::close_fds()
{
  // close I/O multiplexer
  if (m_epollFD >= 0) { ::close(m_epollFD); m_epollFD = -1; }
  if (m_wakeFD >= 0) { ::close(m_wakeFD); m_wakeFD = -1; }
  if (m_wakeFD2 >= 0) { ::close(m_wakeFD2); m_wakeFD2 = -1; }

  // close tty device
  if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }
}

void Terminal::wake()
{
  m_sched->run_(m_thread,
      ZmFn<>{this, [](Terminal *this_) { this_->read(); }});
  wake_();
}

void Terminal::wake_()
{
  char c = 0;
  while (::write(m_wakeFD2, &c, 1) < 0) {
    ZeError e{errno};
    if (e.errNo() != EINTR && e.errNo() != EAGAIN) {
      Error("write", Zi::IOError, e);
      break;
    }
  }
}

void Terminal::sigwinch()
{
  m_sched->run(m_thread, 
      ZmFn<>{this, [](Terminal *this_) { this_->resized(); }});
}

void Terminal::resized()
{
  struct winsize ws;
  if (ioctl(m_fd, TIOCGWINSZ, &ws) < 0) {
    m_w = tigetnum("columns");
    m_h = tigetnum("lines");
  } else {
    m_w = ws.ws_col;
    m_h = ws.ws_row;
  }
}

void Terminal::start_()
{
  termios ntermios;
  memcpy(&ntermios, &m_termios, sizeof(termios));

  ntermios.c_iflag &= ~(INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  ntermios.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
  ntermios.c_oflag &= ~(OPOST | ONLCR | OCRNL | ONOCR | ONLRET);
  ntermios.c_cflag &= ~(CSIZE | PARENB);
  ntermios.c_cflag |= CS8;

  tcsetattr(m_fd, TCSANOW, &ntermios);

  if (fcntl(m_fd, F_SETFL, O_NONBLOCK) < 0) {
    ZeError e{errno};
    close_fds();
    Error("fcntl(F_SETFL, O_NONBLOCK)", Zi::IOError, e);
    return;
  }
  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.u64 = 0;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, m_fd, &ev) < 0) {
      ZeError e{errno};
      close_fds();
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return;
    }
  }

  tputs(m_cr);
  write();
  m_pos = 0;
}

void Terminal::stop_()
{
  m_nextInterval = -1;
  m_utf.clear();
  m_utfn = 0;
  m_nextVKMatch = m_vkeyMatch.ptr();
  m_pending.clear();

  if (m_fd < 0) return;

  write();

  epoll_ctl(m_epollFD, EPOLL_CTL_DEL, m_fd, 0);
  fcntl(m_fd, F_SETFL, 0);

  tcsetattr(m_fd, TCSANOW, &m_termios);
}

// low-level input

void Terminal::read()
{
  struct epoll_event ev;
  int32_t key;
  for (;;) {
    int r = epoll_wait(m_epollFD, &ev, 1, m_nextInterval);
    if (r < 0) {
      ZeError e{errno};
      if (e.errNo() == EINTR || e.errNo() == EAGAIN)
	continue;
      Error("epoll_wait", Zi::IOError, e);
      return VKey::Error;
    }
    if (!r) { // timeout
      for (key : m_pending) if (m_keyFn(key)) goto stop;
      if (m_utfn && m_utf) {
	uint32_t u;
	ZuUTF::in(m_utf, m_utf.length(), u);
	key = u;
	if (m_keyFn(key)) goto stop;
      }
      m_nextInterval = -1;
      m_utf.clear();
      m_utfn = 0;
      m_nextVKMatch = m_vkeyMatch.ptr();
      m_pending.clear();
      continue;
    }
    if (ZuLikely(ev.data.u64 == 3)) {
      char c;
      int r = ::read(m_wakeFD, &c, 1)
      if (r >= 1) return;
      if (r < 0) {
	ZeError e{errno};
	if (e.errNo() != EINTR && e.errNo() != EAGAIN) return;
      }
      continue;
    }
    if (!(ev.events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR)))
      continue;
    uint8_t c;
    {
      int r = ::read(fd, &c, 1)
      if (!r) return VKey::EndOfFile;
      if (r < 0) {
	if (e.errNo() == EINTR || e.errNo() == EAGAIN) continue;
	return VKey::EndOfFile;
      }
    }
    key = c;
    if (!m_utfn) {
      if (auto action = m_nextVKMatch->match(u)) {
	if (action->next) {
	  m_nextInterval = m_interval;
	  m_nextVKMatch = action->next;
	  if (action->vkey >= 0) {
	    m_pending.length(1);
	    m_pending[0] = -(action->vkey);
	  } else
	    m_pending << c;
	  continue;
	}
	key = -(action->vkey);
	m_utfn = 1;
      } else {
	if (ZuUnlikely(!(m_utfn = ZuUTF8::in(&c, 4)))) m_utfn = 1;
      }
    }
    if (--m_utfn > 0) {
      m_utf << c;
      continue;
    }
    if (m_utf) {
      m_utf << c;
      uint32_t u;
      ZuUTF::in(m_utf, m_utf.length(), u);
      m_utf.clear();
      key = u;
    }
    if (m_keyFn(key)) goto stop;
  }
stop:
  stop_();
}

// low-level output

int Terminal::write()
{
  unsigned n = m_out.length();
  while (::write(m_fd, m_out.data(), n) < n) {
    ZeError e{errno};
    if (e.errNo() != EINTR && e.errNo() != EAGAIN) {
      Error("write", Zi::IOError, e);
      return Zi::IOError;
    }
  }
  m_out.clear();
  return Zi::OK;
}

void Terminal::tputs(const char *cap)
{
  thread_local Terminal *this_;
  this_ = this;
  ::tputs(cap, 1, [](int c) -> int {
    this_->m_out << static_cast<char>(c);
    return 0;
  });
}

// output routines

// all cursor motion is in screen position units, regardless of
// half/full-width characters drawn - e.g. to back up over a
// full-width character, use 2x cub1 or cub(2)

// Note: cub/cuf after right-most character is undefined - need to use hpa/cr

void Terminal::cr()
{
  unsigned n = m_pos % m_w;
  if (n) {
    tputs(m_cr);
    m_pos -= n;
  }
}

void Terminal::nl()
{
  tputs(m_ind);
  m_pos += m_w;
}

void Terminal::crnl_()
{
  if (ZuLikely(m_nel)) {
    tputs(m_nel);
  } else {
    tputs(m_cr);
    tputs(m_ind);
  }
}

void Terminal::crnl()
{
  if (ZuLikely(m_nel)) {
    tputs(m_nel);
    m_pos -= m_pos % m_w;
    m_pos += m_w;
  } else {
    cr();
    nl();
  }
}

// Note: MS Console SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
// is am + xenl

// redraw line from cursor, leaving cursor at start of next line
void Terminal::redrawScroll(unsigned endPos)
{
  ZmAssert(!(endPos % m_w));		// endPos is EOL
  ZmAssert(m_pos >= endPos - m_w);	// cursor is >=BOL
  ZmAssert(m_pos < endPos);		// cursor is <EOL

  redraw_(endPos);
  clrScroll(endPos);
}

// clear line from cursor, leaving cursor at start of next line
void Terminal::clrScroll(unsigned endPos)
{
  if (m_pos < endPos) {
    if (ZuLikely(m_el)) {
      tputs(m_el);
      crnl();
      return;
    }
    if (ZuLikely(m_ech)) {
      tputs(tiparm(m_ech, endPos - m_pos));
      crnl();
      return;
    }
    clr_(endPos);
  }
  if (m_sam)
    tputs(m_ind);
  else if (!m_am || m_xenl)
    crnl_();
}

// redraw line from cursor, leaving cursor on same line at pos
void Terminal::redrawNoScroll(unsigned endPos, unsigned pos)
{
  ZmAssert(!(endPos % m_w));		// endPos is EOL
  ZmAssert(m_pos >= endPos - m_w);	// cursor is >=BOL
  ZmAssert(m_pos < endPos);		// cursor is <EOL
  ZmAssert(pos >= endPos - m_w);	// pos is >=BOL
  ZmAssert(pos < endPos);		// pos is <EOL

  if (ZuLikely(!m_am || m_xenl)) {
    // normal case - terminal is am + xenl, or no am, i.e. doesn't scroll
    // immediately following overwrite of right-most character, just output
    // entire line then move cursor within line as needed
    redraw_(endPos);
    if (m_pos < endPos) {
      if (ZuLikely(m_el)) {	// clear to EOL
	tputs(m_el);
	if (m_pos != pos) mvhoriz(pos);
	return;
      }
      if (ZuLikely(m_ech)) {	// clear to EOL with ech
	tputs(tiparm(m_ech, endPos - m_pos));
	if (m_pos != pos) mvhoriz(pos);
	return;
      }
      clr_(endPos);		// clear to EOL with spaces
    }
    // right-most character on line was output, MUST move cursor, NO cub
    if (m_sam) {
      m_pos = endPos - m_w;
      if (pos > m_pos) mvright(pos);
      return;
    }
    if (m_hpa) {
      tputs(tiparm(m_hpa, pos - (pos % m_w)));
      m_pos = pos;
      return;
    }
    tputs(m_cr);
    m_pos = endPos - m_w;
    if (pos > m_pos) mvright(pos);
    return;
  }

  if (m_ich || m_ich1 || (m_smir && m_rmir)) {
    // insert right-most character to leave cursor on same line
    redraw(endPos - 2);
    ++m_pos; // skip a position
    redraw(endPos);
    --m_pos;
    mvleft(endPos - 2);
    if (m_ich1) {
      tputs(m_ich1);
      redraw(endPos - 1);
    } else if (m_ich) {
      tputs(tiparm(m_ich, 1));
      redraw(endPos - 1);
    } else if (m_smir) {
      tputs(m_smir);
      redraw(endPos - 1);
      tputs(m_rmir);
    }
    if (pos < m_pos) mvleft(pos);
    return;
  }

  // dumb terminal, cannot output right-most character leaving cursor on line
  redraw(endPos - 1);
  if (pos < m_pos) mvleft(pos);
}

void Terminal::redraw(unsigned endPos)
{
  redraw_(endPos);
  if (m_pos < endPos) clr_(endPos);
}

#if defined(ZDEBUG) && !defined(Zrl_DEBUG)
#define Zrl_DEBUG
#endif

#ifdef Zrl_DEBUG
#define validatePos(pos) \
  ZmAssert(m_pos >= pos - (pos % m_w)); \
  ZmAssert(m_pos < pos - (pos % m_w) + m_w); \
  ZmAssert(m_pos < pos)
#else
#define validatePos(pos)
#endif

void Terminal::redraw_(unsigned endPos)
{
  validatePos(endPos);

  unsigned off = m_line.position(m_pos).index();
  unsigned lastPos = m_line.align(endPos - 1);
  auto position = m_line.position(lastPos);
  lastPos += position.len();
  unsigned lastOff = position.index();
  lastOff += m_line.byte(lastOff).len();
  if (lastOff > off)
    m_out << m_line.substr(off, lastOff - off);
  m_pos = lastPos;
}

// clear with ech, falling back to spaces
void Terminal::clr(unsigned endPos)
{
  if (ZuLikely(m_ech)) {
    validatePos(pos);

    tputs(tiparm(m_ech, endPos - m_pos));
  } else
    clr_(endPos);
}

// clear by overwriting spaces
void Terminal::clr_(unsigned endPos)
{
  validatePos(endPos);

  for (unsigned i = 0, n = endPos - m_pos; i < n; i++) m_out << ' ';
  m_pos = endPos;
}

// move cursor to position
void Terminal::mv(unsigned pos)
{
  if (pos < m_pos) {
    if (unsigned up = m_pos / m_w - pos / m_w) {
      if (ZuLikely(m_cuu))
	tputs(tiparm(m_cuu, up));
      else if (ZuLikely(m_cuu1)) {
	do { tputs(m_cuu1); m_pos -= m_w; } while (--up);
      } else if (m_bw) {
	cr();
	do { tputs(m_cub1); tputs(m_cr); m_pos -= m_w; } while (--up);
      } else { // no way to go up, just reprint destination line
	tputs(m_cr);
	m_pos = pos - (pos % m_w);
	redrawNoScroll(m_pos + m_w, pos);
	return;
      }
      if (ZuUnlikely(m_pos == pos)) return;
    } else
      mvleft(pos);
  } else if (pos > m_pos) {
    if (unsigned down = pos / m_w - m_pos / m_w) {
      if (ZuLikely(m_cud))
	tputs(tiparm(m_cud, down));
      else if (ZuLikely(m_cud1)) {
	do { tputs(m_cud1); m_pos += m_w; } while (--down);
      } else {
	do { nl(); } while (--down);
	cr();
      }
      if (ZuUnlikely(m_pos == pos)) return;
    } else
      mvright(pos);
  } else // m_pos == pos
    return;
  mvhoriz(pos);
}

// move cursor horizontally
void Terminal::mvhoriz(unsigned pos)
{
  if (pos < m_pos)
    mvleft(pos);
  else
    mvright(pos);
}

void Terminal::mvleft(unsigned pos)
{
  ZmAssert(m_pos >= pos - (pos % m_w));		// cursor is >=BOL
  ZmAssert(m_pos < pos - (pos % m_w) + m_w);	// cursor is <EOL
  ZmAssert(m_pos > pos);			// cursor is right of pos

  if (ZuLikely(m_cub)) {
    tputs(tiparm(m_cub, m_pos - pos));
  } else if (ZuLikely(m_hpa)) {
    tputs(tiparm(m_hpa, pos - (pos % m_w)));
  } else {
    for (unsigned i = 0, n = m_pos - pos; i < n; i++) tputs(m_cub1);
  }
  m_pos = pos;
}

void Terminal::mvright(unsigned pos)
{
  validatePos(pos);

  if (ZuLikely(m_cuf)) {
    tputs(tiparm(m_cuf, pos - m_pos));
  } else if (ZuLikely(m_hpa)) {
    tputs(tiparm(m_hpa, pos - (pos % m_w)));
#if 0
  } else if (m_cuf1) {
    for (unsigned i = 0, n = pos - m_pos; i < n; i++) tputs(m_cuf1);
#endif
  } else {
    redraw(pos);
    return;
  }
  m_pos = pos;
}

// 0___ 012_ 0123 0123 0123  I0__ I012 I012 I012  012I 012I 012I
// ____ ____ ____ 4___ 4567  ____ ____ 34__ 3456  3___ 34__ 3456
// ____ ____ ____ ____ 8___  ____ ____ ____ 78__  ____ ____ 78__

// <>__ 0<>_ 01<> 01<> 01<>  I<>_ I0<> I01_ I01_  0<>I 01I_ 01I_
// ____ ____ ____ 3___ 3<>_  ____ ____ <>__ <>3_  ____ <>3_ <>3_
// ____ ____ ____ ____ 5___  ____ ____ ____ <>5_  ____ ____ <>5_

// 0___ 012_ 0123 0123 0123  []0_ []01 []01 []01  012_ 012_ 012_
// ____ ____ ____ 4___ 4567  ____ 2___ 234_ 2345  []__ []3_ []34
// ____ ____ ____ ____ 8___  ____ ____ ____ 678_  ____ ____ 5678

// <>__ 0<>_ 01<> 01<> 01<>  []<> []0_ []01 []01  0<>_ 01[] 01[]
// ____ ____ ____ 3___ 3<>_  ____ <>__ <>__ <>3_  []__ <>__ <>3_
// ____ ____ ____ ____ 5___  ____ ____ ____ <>5_  ____ ____ <>5_

bool Terminal::ins_(unsigned n, bool mir)
{
  if (mir) return true;
  if (m_ich) {
    tputs(tiparm(m_ich, n));
    return false;
  }
  if (m_smir) {
    tputs(m_smir);
    return true;
  }
  for (unsigned i = 0; i < n; i++) tputs(m_ich1);
  return false;
}

void Terminal::del_(unsigned n)
{
  if (m_smdc) tputs(m_smdc);
  if (m_dch)
    tputs(tiparm(m_dch, n));
  else
    for (unsigned i = 0; i < n; i++) tputs(m_dch1);
  if (m_rmdc) tputs(m_rmdc);
}

void Terminal::splice(
    unsigned off, ZuUTFSpan span, ZuUTFSpan rspan, ZuString replace)
{
  unsigned endPos = m_pos + span.width();
  unsigned endBOLPos = endPos - (endPos % m_w);
  bool shiftLeft = false, shiftRight = false;
  ZtArray<ZuUTFSpan> shiftLines;

  if (off + span.inLen() < m_line.length()) {
    unsigned trailWidth = m_line.width() - endPos; // width of trailing data
    unsigned trailLines = (trailWidth + m_w - 1) / m_w; // #lines of ''

    ZmAssert(trailWidth);

    // it's worth optimizing the common case where a long line of input
    // is being interactively edited at the beginning; if shifting the
    // trailing data, and the shift is less than half the width of the
    // display, and the width of the trailing data is greater than the
    // shift, it's worth leaving the displayed data in place and using
    // insertions/deletions on each trailing line
    if (rspan.width() < span.width()) {
      unsigned shiftWidth = span.width() - rspan.width();
      if (shiftWidth < (m_w>>1) && trailWidth > shiftWidth &&
	  (m_dch || m_dch1 || (m_smdc && m_rmdc))) {
	// shift left using deletions on each line
	shiftLeft = true;
	// a display left shift can result in both +ve and -ve byte offsets
	int shiftOff =
	  static_cast<int>(rspan.inLen()) - static_cast<int>(span.inLen());
	shiftLines.size(trailLines);
	unsigned bolPos = endBOLPos;
	while (bolPos < m_line.width()) {
	  unsigned eolPos = m_line.align(bolPos + m_w - 1);
	  shiftLines.push(ZuUTFSpan{
	    m_line.position(eolPos).index() + shiftOff, 0, eolPos});
	  bolPos += m_w;
	}
      }
    } else if (rspan.width() > span.width()) {
      unsigned shiftWidth = rspan.width() - span.width();
      if (shiftWidth < (m_w>>1) && trailWidth > shiftWidth &&
	  (m_ich || m_ich1 || (m_smir && m_rmir))) {
	// shift right using insertions on each line
	shiftRight = true;
	// a right shift can result in both +ve and -ve byte offsets
	int shiftOff =
	  static_cast<int>(rspan.inLen()) - static_cast<int>(span.inLen());
	shiftLines.size(trailLines);
	unsigned bolPos = endBOLPos;
	while (bolPos < m_line.width()) {
	  shiftLines.push(ZuUTFSpan{
	    m_line.position(bolPos).index() + shiftOff, 0, bolPos});
	  bolPos += m_w;
	}
      }
    }
  }

  // splice in the new data
  m_line.data().splice(off, span.inLen(), replace);

  // reflow the line, from offset onwards
  m_line.reflow(off);

  // recalculate endPos
  endPos = m_pos + rspan.width();
  endBOLPos = endPos - (endPos % m_w);

  // redraw/scroll all but last line of replacement data
  {
    unsigned bolPos = m_pos - (m_pos % m_w);
    while (bolPos < endBOLPos) redrawScroll(bolPos += m_w);
  }

  // redraw/scroll all but last line of trailing data
  unsigned lastBOLPos = m_line.width();
  lastBOLPos -= (lastBOLPos % m_w);

  unsigned bolPos = endBOLPos;
  if (shiftLeft) {
    unsigned line = 0;
    while (bolPos < lastBOLPos) {
      auto shiftLine = shiftLines[line++];
      unsigned rightPos = m_line.byte(shiftLine.inLen()).index();
      del_(shiftLine.width() - (rightPos - m_pos));
      rightPos += m_line.position(rightPos).len();
      mvright(rightPos);
      redrawScroll(bolPos += m_w);
    }
    if (bolPos < m_line.width()) {
      auto shiftLine = shiftLines[line];
      unsigned rightPos = m_line.byte(shiftLine.inLen()).index();
      del_(shiftLine.width() - (rightPos - m_pos));
      rightPos += m_line.position(rightPos).len();
      mvright(rightPos);
      redraw(m_line.width());
    }
  } else if (shiftRight) {
    unsigned line = 0;
    bool smir = false;
    while (bolPos < lastBOLPos) {
      auto shiftLine = shiftLines[line++];
      unsigned leftPos = m_line.byte(shiftLine.inLen()).index();
      smir = ins_(leftPos - m_pos, smir);
      redraw(leftPos);
      if (smir && !m_mir) { tputs(m_rmir); smir = false; }
      crnl();
      bolPos += m_w;
    }
    if (bolPos < m_line.width()) {
      auto shiftLine = shiftLines[line];
      unsigned leftPos = m_line.byte(shiftLine.inLen()).index();
      smir = ins_(leftPos - m_pos, smir);
      redraw(leftPos);
      if (smir) tputs(m_rmir);
    }
  } else {
    while (bolPos < lastBOLPos) redrawScroll(bolPos += m_w);
    if (bolPos < m_line.width()) redraw(m_line.width());
  }

  // if the size of the line was reduced, clear to the end of the previous data
  if (rspan.width() < span.width()) {
    unsigned clrPos = m_line.width() + (span.width() - rspan.width());
    unsigned clrBOLPos = clrPos - (clrPos % m_w);
    while (bolPos < clrBOLPos) clrScroll(bolPos += m_w);
    if (bolPos < clrPos) clr(clrPos);
  }

  // move cursor to final position
  mv(endPos);
}

void Terminal::out(ZuString s)
{
  unsigned off = position(m_pos).index();
  auto rspan = ZuUTF<uint32_t, uint8_t>::span(s);
  ZuArray<uint8_t> removed{m_line};
  removed.offset(off);
  auto span = ZuUTF<uint32_t, uint8_t>::nspan(removed, rspan.outLen());
  splice(off, span, rspan, s);
}

void Terminal::ins(ZuString s)
{
  unsigned off = position(m_pos).index();
  auto rspan = ZuUTF<uint32_t, uint8_t>::span(s);
  splice(off, ZuUTFSpan{}, rspan, s);
}

void Terminal::del(unsigned n)
{
  unsigned off = position(m_pos).index();
  ZuArray<uint8_t> removed{m_line};
  removed.offset(off);
  auto span = ZuUTF<uint32_t, uint8_t>::nspan(removed, n);
  splice(off, span, ZuUTFSpan{}, s);
}

} // namespace Zrl
