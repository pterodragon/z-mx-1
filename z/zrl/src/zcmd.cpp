//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

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

#include <stdio.h>
#include <stdlib.h>

#include <ZuPolymorph.hpp>

#include <ZmPlatform.hpp>
#include <ZmTrap.hpp>

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>
#include <ZvCmdClient.hpp>
#include <ZvMultiplexCf.hpp>

#include <Zrl.hpp>

#ifdef _WIN32
#include <io.h>		// for _isatty
#ifndef isatty
#define isatty _isatty
#endif
#ifndef fileno
#define fileno _fileno
#endif
#else
#include <termios.h>
#include <unistd.h>	// for isatty
#include <fcntl.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

static void usage()
{
  static const char *usage =
    "usage: zcmd [HOST:]PORT [CMD [ARGS]]\n"
    "\tHOST\t- target host (default localhost)\n"
    "\tPORT\t- target port\n"
    "\tCMD\t- command to send to target\n"
    "\t\t  (reads commands from standard input if none specified)\n"
    "\tARGS\t- command arguments\n";
  std::cerr << usage << std::flush;
  ZeLog::stop();
  ZmPlatform::exit(1);
}

class ZCmd : public ZmPolymorph, public ZvCmdClient<ZCmd> {
public:
  using Base = ZvCmdClient<ZCmd>;

  struct Link : public ZvCmdCliLink<ZCmd, Link> {
  using Base = ZvCmdCliLink<ZCmd, Link>;
    Link(ZCmd *app) : Base(app) { }
    void loggedIn(const Bitmap &perms) {
      this->app()->loggedIn();
    }
    void disconnected() {
      this->app()->disconnected();
      Base::disconnected();
    }
  };

  void init(ZiMultiplex *mx, ZvCf *cf, bool interactive) {
    Base::init(mx, cf);
    m_interactive = interactive;
  }
  void final() {
    m_link = nullptr;
    Base::final();
  }

  bool interactive() const { return m_interactive; }

  void solo(ZtString s) {
    m_solo = true;
    m_soloMsg = ZuMv(s);
  }
  void target(ZuString s) {
    m_prompt.null();
    m_prompt << s << "] ";
  }

  template <typename ...Args>
  void login(Args &&... args) {
    m_link = new Link(this);
    m_link->login(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void access(Args &&... args) {
    m_link = new Link(this);
    m_link->access(ZuFwd<Args>(args)...);
  }

  void disconnect() { m_link->disconnect(); }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

  int code() const { return m_code; }

  void exiting() { m_exiting = true; }

private:
  void loggedIn() {
    if (m_solo) {
      send(ZuMv(m_soloMsg));
    } else {
      std::cout <<
	"For a list of valid commands: help\n"
	"For help on a particular command: COMMAND --help\n" << std::flush;
      prompt();
    }
  }

  void disconnected() {
    if (m_exiting) return;
    if (m_interactive) {
      Zrl::stop();
      std::cerr << "\nserver disconnected\n" << std::flush;
    }
    ZmPlatform::exit(1);
  }

  void prompt() {
    ZtString cmd;
next:
    if (m_interactive) {
      try {
	cmd = Zrl::readline_(m_prompt);
      } catch (const Zrl::EndOfFile &) {
	post();
	return;
      }
    } else {
      cmd.size(4096);
      if (!fgets(cmd, cmd.size() - 1, stdin)) {
	post();
	return;
      }
      cmd[cmd.size() - 1] = 0;
      cmd.calcLength();
      cmd.chomp();
    }
    if (!cmd) goto next;
    send(cmd);
  }

  void send(ZtString cmd) {
    FILE *out = stdout;
    const auto &redirect = ZtStaticRegexUTF8("\\s*>\\s*");
    ZtRegex::Captures c;
    unsigned pos = 0, n = 0;
    ZtString cmd_ = ZuMv(cmd);
    if (n = redirect.m(cmd, c, pos)) {
      if (!(out = fopen(c[2], "w"))) {
	logError(ZtString{c[2]}, ": ", ZeLastError);
	return;
      }
      cmd = c[0];
    } else {
      cmd = ZuMv(cmd_);
    }
    auto seqNo = m_seqNo++;
    m_fbb.Clear();
    m_fbb.Finish(ZvCmd::fbs::CreateRequest(m_fbb,
	  seqNo, Zfb::Save::str(m_fbb, cmd)));
    m_link->sendCmd(m_fbb, seqNo, [this, out](const ZvCmd::fbs::ReqAck *ack) {
      m_code = ack->code();
      using namespace Zfb::Load;
      {
	auto s = str(ack->out());
	fwrite(s.data(), 1, s.length(), out);
	if (out != stdout) fclose(out);
      }
      if (m_solo)
	post();
      else
	prompt();
    });
  }

private:
  ZmSemaphore		m_done;
  ZtString		m_prompt;
  ZtString		m_soloMsg;
  ZmRef<Link>		m_link;
  ZvCmdSeqNo		m_seqNo = 0;
  Zfb::IOBuilder	m_fbb;
  int			m_code = 0;
  bool			m_interactive = true;
  bool			m_solo = false;
  bool			m_exiting = false;
};

static ZtString getpass_(const ZtString &prompt, unsigned passLen)
{
  ZtString passwd;
  passwd.size(passLen + 4); // allow for \r\n and null terminator
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  DWORD omode, nmode;
  GetConsoleMode(h, &omode);
  nmode = (omode | ENABLE_LINE_INPUT) & ~ENABLE_ECHO_INPUT;
  SetConsoleMode(h, nmode);
  DWORD n = 0;
  WriteConsole(h, prompt.data(), prompt.length(), &n, nullptr);
  n = 0;
  ReadConsole(h, passwd.data(), passwd.size() - 1, &n, nullptr);
  if (n < passwd.size()) passwd.length(n);
  WriteConsole(h, "\r\n", 2, &n, nullptr);
  mode |= ENABLE_ECHO_INPUT;
  SetConsoleMode(h, omode);
#else
  int fd = ::open("/dev/tty", O_RDWR, 0);
  if (fd < 0) return passwd;
  struct termios oflags, nflags;
  memset(&oflags, 0, sizeof(termios));
  tcgetattr(fd, &oflags);
  memcpy(&nflags, &oflags, sizeof(termios));
  nflags.c_lflag = (nflags.c_lflag & ~ECHO) | ECHONL;
  if (tcsetattr(fd, TCSANOW, &nflags)) return passwd;
  ::write(fd, prompt.data(), prompt.length());
  FILE *in = fdopen(fd, "r");
  if (!fgets(passwd, passwd.size() - 1, in)) return passwd;
  passwd[passwd.size() - 1] = 0;
  passwd.calcLength();
  tcsetattr(fd, TCSANOW, &oflags);
  fclose(in);
  ::close(fd);
#endif
  passwd.chomp();
  return passwd;
}

int main(int argc, char **argv)
{
  if (argc < 2) usage();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::lambdaSink(
	[](ZeEvent *e) { std::cerr << e->message() << '\n' << std::flush; }));
  ZeLog::start();

  bool interactive = isatty(fileno(stdin));
  auto keyID = ::getenv("ZCMD_KEY_ID");
  auto secret = ::getenv("ZCMD_KEY_SECRET");
  ZtString user, passwd, server;
  ZuBox<unsigned> totp;
  ZuBox<unsigned> port;

  try {
    ZtString user, server;
    ZuBox<unsigned> port;

    {
      ZtRegex::Captures c;
      const auto &r = ZtStaticRegexUTF8("^([^@]+)@([^:]+):(\\d+)$");
      if (r.m(argv[1], c) == 3) {
	user = c[2];
	server = c[3];
	port = c[4];
      }
    }
    if (!user) {
      ZtRegex::Captures c;
      const auto &r = ZtStaticRegexUTF8("^([^:]+):(\\d+)$");
      if (r.m(argv[1], c) == 2) {
	server = c[2];
	port = c[3];
      }
    }
    if (!server) {
      server = "localhost";
      port = argv[1];
    }
    if (!*port || !port) usage();
  } catch (const ZtRegex::Error &) {
    usage();
  }
  if (user) keyID = secret = nullptr;
  if (keyID) {
    if (!secret) {
      std::cerr << "set ZCMD_KEY_SECRET "
	"to use with ZCMD_KEY_ID\n" << std::flush;
      ::exit(1);
    }
  } else {
    if (!interactive || argc > 2) {
      std::cerr << "set ZCMD_KEY_ID and ZCMD_KEY_SECRET "
	"to use non-interactively\n" << std::flush;
      ::exit(1);
    }
    passwd = getpass_("password: ", 100);
    totp = getpass_("totp: ", 6);
  }

  ZiMultiplex *mx = new ZiMultiplex(
      ZiMxParams()
	.scheduler([](auto &s) {
	  s.nThreads(4)
	  .thread(1, [](auto &t) { t.isolated(1); })
	  .thread(2, [](auto &t) { t.isolated(1); })
	  .thread(3, [](auto &t) { t.isolated(1); }); })
	.rxThread(1).txThread(2));

  mx->start();

  ZmRef<ZCmd> client = new ZCmd();

  ZmTrap::sigintFn(ZmFn<>{client, [](ZCmd *client) { client->post(); }});
  ZmTrap::trap();

  {
    ZmRef<ZvCf> cf = new ZvCf();
    cf->set("thread", "3");
    client->init(mx, cf, interactive);
  }

  if (argc > 2) {
    ZtString solo;
    for (int i = 2; i < argc; i++) {
      solo << argv[i];
      if (ZuLikely(i < argc - 1)) solo << ' ';
    }
    client->solo(ZuMv(solo));
  } else
    client->target(argv[1]);

  if (keyID)
    client->access(server, port, keyID, secret);
  else
    client->login(server, port, user, passwd, totp);

  client->wait();

  if (client->interactive()) {
    Zrl::stop();
    std::cout << std::flush;
  }

  client->exiting();
  client->disconnect();

  mx->stop(true);

  ZeLog::stop();

  client->final();

  return client->code();
}
