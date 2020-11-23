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

void start(KeyFn keyFn) // async
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

void stop() // async
{
  m_sched->wakeFn(m_thread, ZmFn<>());
  m_sched->run_(m_thread, 
      ZmFn<>{this, [](Terminal *this_) { this_->stop_(); }});
  wake_();
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

  // obtain input capabilities
  if (!(m_kent = tigetstr("kent"))) m_kent = "\r";

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

  m_kcuu1 = tigetstr("kcuu1");
  m_kUP = tigetstr("kUP");
  m_kUP3 = tigetstr("kUP3");
  m_kUP5 = tigetstr("kUP5");
  m_kUP6 = tigetstr("kUP6");

  m_kcud1 = tigetstr("kcud1");
  m_kDN = tigetstr("kDN");
  m_kDN3 = tigetstr("kDN3");
  m_kDN5 = tigetstr("kDN5");
  m_kDN6 = tigetstr("kDN6");

  m_kcub1 = tigetstr("kcub1");
  m_kLFT = tigetstr("kLFT");
  m_kLFT3 = tigetstr("kLFT3");
  m_kLFT5 = tigetstr("kLFT5");
  m_kLFT6 = tigetstr("kLFT6");

  m_kcuf1 = tigetstr("kcuf1");
  m_kRIT = tigetstr("kRIT");
  m_kRIT3 = tigetstr("kRIT3");
  m_kRIT5 = tigetstr("kRIT5");
  m_kRIT6 = tigetstr("kRIT6");

  m_khome = tigetstr("khome");
  m_kHOM = tigetstr("kHOM");
  m_kHOM3 = tigetstr("kHOM3");
  m_kHOM5 = tigetstr("kHOM5");
  m_kHOM6 = tigetstr("kHOM6");

  m_kend = tigetstr("kend");
  m_kEND = tigetstr("kEND");
  m_kEND3 = tigetstr("kEND3");
  m_kEND5 = tigetstr("kEND5");
  m_kEND6 = tigetstr("kEND6");

  m_kich1 = tigetstr("kich1");
  m_kIC = tigetstr("kIC");
  m_kIC3 = tigetstr("kIC3");
  m_kIC5 = tigetstr("kIC5");
  m_kIC6 = tigetstr("kIC6");

  m_kdch1 = tigetstr("kdch1");
  m_kDC = tigetstr("kDC");
  m_kDC3 = tigetstr("kDC3");
  m_kDC5 = tigetstr("kDC5");
  m_kDC6 = tigetstr("kDC6");

  // obtain output capabilities
  m_am = tigetflag("am");
  m_bw = tigetflag("bw");

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

  // enter key
  m_vkeyMatch->add(m_kent, VKey::Enter);

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
  m_vkeyMatch->add(m_kcuu1, VKey::Up);
  m_vkeyMatch->add(m_kUP, VKey::Up | VKey::Shift);
  m_vkeyMatch->add(m_kUP3, VKey::Up | VKey::Alt);
  m_vkeyMatch->add(m_kUP5, VKey::Up | VKey::Ctrl);
  m_vkeyMatch->add(m_kUP6, VKey::Up | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kcud1, VKey::Down);
  m_vkeyMatch->add(m_kDN, VKey::Down | VKey::Shift);
  m_vkeyMatch->add(m_kDN3, VKey::Down | VKey::Alt);
  m_vkeyMatch->add(m_kDN5, VKey::Down | VKey::Ctrl);
  m_vkeyMatch->add(m_kDN6, VKey::Down | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kcub1, VKey::Left);
  m_vkeyMatch->add(m_kLFT, VKey::Left | VKey::Shift);
  m_vkeyMatch->add(m_kLFT3, VKey::Left | VKey::Alt);
  m_vkeyMatch->add(m_kLFT5, VKey::Left | VKey::Ctrl);
  m_vkeyMatch->add(m_kLFT6, VKey::Left | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kcuf1, VKey::Right);
  m_vkeyMatch->add(m_kRIT, VKey::Right | VKey::Shift);
  m_vkeyMatch->add(m_kRIT3, VKey::Right | VKey::Alt);
  m_vkeyMatch->add(m_kRIT5, VKey::Right | VKey::Ctrl);
  m_vkeyMatch->add(m_kRIT6, VKey::Right | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_khome, VKey::Home);
  m_vkeyMatch->add(m_kHOM, VKey::Home | VKey::Shift);
  m_vkeyMatch->add(m_kHOM3, VKey::Home | VKey::Alt);
  m_vkeyMatch->add(m_kHOM5, VKey::Home | VKey::Ctrl);
  m_vkeyMatch->add(m_kHOM6, VKey::Home | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kend, VKey::End);
  m_vkeyMatch->add(m_kEND, VKey::End | VKey::Shift);
  m_vkeyMatch->add(m_kEND3, VKey::End | VKey::Alt);
  m_vkeyMatch->add(m_kEND5, VKey::End | VKey::Ctrl);
  m_vkeyMatch->add(m_kEND6, VKey::End | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kich1, VKey::Insert);
  m_vkeyMatch->add(m_kIC, VKey::Insert | VKey::Shift);
  m_vkeyMatch->add(m_kIC3, VKey::Insert | VKey::Alt);
  m_vkeyMatch->add(m_kIC5, VKey::Insert | VKey::Ctrl);
  m_vkeyMatch->add(m_kIC6, VKey::Insert | VKey::Shift | VKey::Ctrl);

  m_vkeyMatch->add(m_kdch1, VKey::Delete);
  m_vkeyMatch->add(m_kDC, VKey::Delete | VKey::Shift);
  m_vkeyMatch->add(m_kDC3, VKey::Delete | VKey::Alt);
  m_vkeyMatch->add(m_kDC5, VKey::Delete | VKey::Ctrl);
  m_vkeyMatch->add(m_kDC6, VKey::Delete | VKey::Shift | VKey::Ctrl);

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
  m_kent = nullptr;

  m_kcuu1 = nullptr;
  m_kUP = nullptr;
  m_kUP3 = nullptr;
  m_kUP5 = nullptr;
  m_kUP6 = nullptr;

  m_kcud1 = nullptr;
  m_kDN = nullptr;
  m_kDN3 = nullptr;
  m_kDN5 = nullptr;
  m_kDN6 = nullptr;

  m_kcub1 = nullptr;
  m_kLFT = nullptr;
  m_kLFT3 = nullptr;
  m_kLFT5 = nullptr;
  m_kLFT6 = nullptr;

  m_kcuf1 = nullptr;
  m_kRIT = nullptr;
  m_kRIT3 = nullptr;
  m_kRIT5 = nullptr;
  m_kRIT6 = nullptr;

  m_khome = nullptr;
  m_kHOM = nullptr;
  m_kHOM3 = nullptr;
  m_kHOM5 = nullptr;
  m_kHOM6 = nullptr;

  m_kend = nullptr;
  m_kEND = nullptr;
  m_kEND3 = nullptr;
  m_kEND5 = nullptr;
  m_kEND6 = nullptr;

  m_kich1 = nullptr;
  m_kIC = nullptr;
  m_kIC3 = nullptr;
  m_kIC5 = nullptr;
  m_kIC6 = nullptr;

  m_kdch1 = nullptr;
  m_kDC = nullptr;
  m_kDC3 = nullptr;
  m_kDC5 = nullptr;
  m_kDC6 = nullptr;

  m_am = false;
  m_bw = false;

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

void Terminal::move(unsigned pos)
{
  if (pos < m_pos) {
    if (unsigned up = m_pos / m_w - pos / m_w) {
      if (ZuLikely(m_cuu))
	tputs(tiparm(m_cuu, up));
      else if (ZuLikely(m_cuu1)) {
	do {
	  tputs(m_cuu1);
	  m_pos -= m_w;
	} while (--up);
      } else if (m_bw) {
	tputs(m_cr);
	m_pos -= (m_pos % m_w);
	do {
	  tputs(m_cub1);
	  tputs(m_cr);
	  m_pos -= m_w;
	} while (--up);
      } else { // no way to go up, just reprint destination line
	tputs(m_cr);
	m_pos = pos - (pos % m_w);
	unsigned firstByte = m_line.pos2byte(m_pos).index();
	unsigned rightPos = m_pos + m_w - 1;
	unsigned lastPos = m_line.align(rightPos);
	unsigned lastByte = m_line.pos2byte(lastPos).index();
	lastByte += m_line.byte2pos(lastByte).len();
	m_out << m_line.substr(firstByte, lastByte - firstByte);
	if (rightPos > lastPos) {
	  if (m_el) {
	    tputs(m_el);
	    m_pos = lastPos;
	  } else if (m_ech) {
	    tputs(tiparm(m_ech, rightPos - lastPos));
	    m_pos = lastPos;
	  } else {
	    for (unsigned i = 0, n = rightPos - lastPos; i < n; i++)
	      m_out << ' ';
	    m_pos = rightPos;
	  }
	}
      }
      if (ZuUnlikely(m_pos == pos)) return;
    } else
      goto left;
  } else if (pos > m_pos) {
    if (unsigned down = pos / m_w - m_pos / m_w) {
      if (ZuLikely(m_cud))
	tputs(tiparm(m_cud, down));
      else if (ZuLikely(m_cud1)) {
	do {
	  tputs(m_cud1);
	  m_pos += m_w;
	} while (--down);
      } else {
	do {
	  m_out << '\n';
	  m_pos += m_w;
	} while (--down);
	tputs(m_cr);
	m_pos -= (m_pos % m_w);
      }
      if (ZuUnlikely(m_pos == pos)) return;
    } else
      goto right;
  } else // m_pos == pos
    return;
  if (pos < m_pos) {
left:
    if (ZuLikely(m_cub)) {
      tputs(tiparm(m_cub, m_pos - pos));
    } else if (ZuLikely(m_hpa)) {
      tputs(tiparm(m_hpa, pos));
    } else {
      for (unsigned i = 0, n = m_pos - pos; i < n; i++) tputs(m_cub1);
    }
  } else {
right:
    if (ZuLikely(m_cuf)) {
      tputs(tiparm(m_cuf, pos - m_pos));
    } else if (ZuLikely(m_hpa)) {
      tputs(tiparm(m_hpa, pos));
    } else if (m_cuf1) {
      for (unsigned i = 0, n = pos - m_pos; i < n; i++) tputs(m_cuf1);
    } else {
      unsigned firstByte = m_line.pos2byte(m_pos).index();
      unsigned lastPos = m_line.align(pos - 1);
      unsigned lastByte = m_line.pos2byte(lastPos).index();
      lastByte += m_line.byte2pos(lastByte).len();
      m_out << m_line.substr(firstByte, lastByte - firstByte);
      for (unsigned i = 0, n = (pos - 1) - lastPos; i < n; i++) m_out << ' ';
    }
  }
  m_pos = pos;
}


// design terminal output interface:
//
// move to horizontal position X
// insert string (leaving cursor to the right of inserted string, or at right margin)
// delete characters (leaving cursor in place)
// clear to end of line (leaving cursor in place)
// newline (append newline, move to left position)
// move up/down (retaining horizontal position)

// need following caps
//
// os/hc - reject terminal if either set
//
// am - output wraparound at far right (not due to insert mid-line)
// bw - cub1 wraparound at far left
//
// cr - carriage return (default to "\r")
// ind - line feed (default to "\n") (scroll up from bottom) (needed if no am)
// nel - new line (includes cr) (default to cr + ind)
//
// reject if no am and no ind
//
// hpa - horizontal position #1 (use tparm) - fallback to cr + overwrite
//
// Note: cub/cuf after rightmost character is undefined - need to use hpa/cr
//
// cub1 - cursor left (default to "\b")
// cuf1 - cursor right (default to reprint character under cursor)
// cuu1 - cursor up (default to "\033[A")
// cud1 - cursor down (default to "\n")
//
// prefer cub/cuf/cuu/cud for N > 1, or repeat if unavailable
//
// el - clear to end of line, fallback to ech, then to over-printing spaces through end-of-line if unavailable
//
// smir/rmir falling back to ich then to ich1 then to over-print through EOL - insert characters
//
// smdc/rmdc with dch/dch1 - delete characters, fallback to over-print if unavailable

  // FIXME - need word left/right and home/end

  unsigned left(unsigned n) { // in characters
    // convert n in characters to n in screen positions (may require
    // traversing multiple lines)
  }
  unsigned right(unsigned n) {
    // as per left
  }
  unsigned wLeft(unsigned n) {
  }
  unsigned wRight(unsigned n) {
  }

  // FIXME - need w W e E b B ge gE ^ $
  // vi-Mode
  // Shift-Left - w
  // Ctrl-Left - W

  void up(unsigned n) {
    // process as left(n*w screen positions)
  }

  void down(unsigned n) {
    // process as right(n*w screen positions)
  }

  void overwrite(ZuString s) {
    // need to calculate widths, expand ctrl-chars, wraparound lines
    // need to re-calc overwritten lines and this may result in re-wrapping
    // need to repaint subsequent lines if re-wrapped
    m_out << s;
    if (m_nel) tputs(m_nel);
    else { tputs(m_cr); tputs(m_ind); }
    out();
  }

  void insert(ZuString s) {
    // need to calculate widths, expand ctrl-chars, wraparound lines
    // repaint subsequent lines
  }

  void erase(unsigned n) {
  }

  void clear() {
  }

  void up_(unsigned n) {
  }
  void down_(unsigned n) {
  }

} // namespace Zrl
