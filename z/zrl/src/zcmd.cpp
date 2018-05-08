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

#include <ZuPolymorph.hpp>

#include <ZmTrap.hpp>

#include <ZvCf.hpp>
#include <ZvCmd.hpp>

#include <Zrl.hpp>

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>		// for _isatty
#ifndef isatty
#define isatty _isatty
#endif
#ifndef fileno
#define fileno _fileno
#endif
#else
#include <unistd.h>	// for isatty
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

  fputs(usage, stderr);
  ZeLog::stop();
  exit(1);
}

static unsigned getCxnTimeout()
{
  const char *txt = getenv("ZCMD_TIMEOUT");
  return (txt && *txt) ? atoi(txt) : 10;
}

class Mx : public ZuPolymorph, public ZiMultiplex {
public:
  inline Mx() : ZiMultiplex(ZvMultiplexParams()) { }
};

class ZCmd : public ZvCmdClient {
public:
  inline ZCmd():
    ZvCmdClient(getCxnTimeout()), m_interactive(false), m_status(0) { }

  void init(ZiMultiplex *mx) {
    ZvCmdClient::init(
	mx, 0,
	ZvCmdConnFn::Member<&ZCmd::connected>::fn(this),
	ZvCmdDiscFn::Member<&ZCmd::disconnected>::fn(this),
	ZvCmdAckRcvdFn::Member<&ZCmd::ackRcvd>::fn(this));

    m_interactive = isatty(fileno(stdin));
    if (!m_interactive && m_solo.length() != 0) {
      ZtArray<char> data;
      ZuArrayN<char, BUFSIZ> buf;
      size_t length;
      while ((length = fread(buf.data(), 1, BUFSIZ, stdin)) > 0) {
	buf.length(length);
	data += buf;
      }
      stdinData(ZuMv(data));
    }
  }
  void final() {
    ZvCmdClient::final();
  }

  inline bool interactive() const { return m_interactive; }

  template <typename S> inline void solo(const S &s) { m_solo = s; }
  template <typename S> inline void target(const S &s) {
    m_prompt << s << "] ";
  }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

  inline int status() const { return m_status; }

private:
  void ackRcvd(ZvCmdLine *line, const ZvAnswer &ans) {
    m_status = 0;
    if (!!ans.message()) {
      puts(ans.message().data());
      fflush(stdout);
    }

    if (ans.flags() & (ZvCmd::Fail | ZvCmd::Error)) {
      m_status = 1;
    } else if (ans.data().length()) {
      fwrite(ans.data().data(), 1, ans.data().length(), stdout);
      fflush(stdout);
    }

    if (m_solo)
      post();
    else
      prompt();
  }

  void connected(ZvCmdLine *line) {
    if (m_solo)
      send(m_solo);
    else {
      fprintf(stderr, "For a list of valid commands: help\n"
	      "For help on a particular command: COMMAND --help\n");
      prompt();
    }
  }

  void prompt() {
    ZtString s;
    do {
      if (m_interactive) {
	try {
	  s = Zrl::readline_(m_prompt);
	} catch (const Zrl::EndOfFile &) {
	  post();
	  return;
	}
      } else {
	s.length(4096);
	if (!fgets(s, 4096, stdin)) {
	  post();
	  return;
	}
	s.calcLength();
	s.chomp();
      }
    } while (!s);
    send(ZuMv(s));
  }

  void disconnected(ZvCmdLine *line) {
    if (m_interactive) Zrl::stop();
    post();
  }

  ZmSemaphore	m_done;
  bool		m_interactive;
  ZtString	m_solo;
  ZtString	m_prompt;
  int		m_status;
};

int main(int argc, char **argv)
{
  ZmRef<Mx> mx = new Mx();
  ZmRef<ZCmd> client = new ZCmd();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  try {
    if (argc < 2) usage();

    ZiIP ip("127.0.0.1");
    ZuBox<int> port;

    {
      ZtRegex::Captures c;
      static ZtRegex r("^([^:]+):(\\d+)$", PCRE_UTF8);
      if (r.m(argv[1], c) >= 2) {
	ip = c[2];
	if (!port.scan(c[3]) || !*port || !port) usage();
      } else {
	if (!port.scan(argv[1]) || !*port || !port) usage();
      }
    }

    if (mx->start() != Zi::OK) throw ZtString("multiplexer start failed");

    if (argc > 2) {
      ZtString solo;
      for(int i = 2; i < argc; i++) {
	solo += argv[i];
	if (i < argc - 1) solo += " ";
      }

      client->solo(solo);
    } else
      client->target(argv[1]);

    client->init(mx);

    ZmTrap::sigintFn(ZmFn<>::Member<&ZCmd::post>::fn(client));
    ZmTrap::trap();

    client->connect(ip, port);

  } catch (const ZtRegex::Error &) {
    usage();
  } catch (const ZeError &e) {
    fputs(e.message(), stderr);
    fputc('\n', stderr);
    ZeLog::stop();
    exit(1);
  } catch (const ZtString &s) {
    fputs(s.data(), stderr);
    fputc('\n', stderr);
    ZeLog::stop();
    exit(1);
  } catch (...) {
    fputs("Unknown Exception\n", stderr);
    ZeLog::stop();
    exit(1);
  }

  client->wait();

  if (client->interactive()) {
    Zrl::stop();
    fflush(stdout);
  }

  client->stop();
  mx->stop(true);
  ZeLog::stop();

  client->final();

  return client->status();
}
