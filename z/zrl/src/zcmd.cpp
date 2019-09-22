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
  std::cerr << usage << std::flush;
  ZeLog::stop();
  ZmPlatform::exit(1);
}

static unsigned getTimeout()
{
  const char *txt = getenv("ZCMD_TIMEOUT");
  return (txt && *txt) ? atoi(txt) : 10;
}

  //
  // FIXME - container seqNo->outputFile (ZiFile)
  //
int ZvCmdMsg::redirectIn(ZmRef<ZeEvent> *e)
{
  const auto &redirect = ZtStaticRegexUTF8("\\s*<\\s*");
  ZtRegex::Captures c;
  unsigned pos = 0, n = 0;
  ZtString cmd = ZuMv(m_cmd);
  if (n = redirect.m(cmd, c, pos)) {
    m_cmd = c[0];
    ZiFile f;
    ZeError e_;
    int r = f.open(c[2], ZiFile::ReadOnly | ZiFile::GC, 0777, &e_);
    if (r == Zi::OK) {
      m_data.length(f.size());
      if (f.read(m_data.data(), m_data.length()) != (int)m_data.length())
	m_data.null();
    } else {
      if (e) *e = ZeEVENT(Error, ([file = ZtString(c[2]), e = e_]
	  (const ZeEvent &, ZmStream &s) { s << file << ": " << e; }));
    }
    return r;
  } else {
    m_cmd = ZuMv(cmd);
    return Zi::OK;
  }
}

int ZvCmdMsg::redirectOut(ZiFile &f, ZmRef<ZeEvent> *e)
{
  const auto &redirect = ZtStaticRegexUTF8("\\s*>\\s*");
  ZtRegex::Captures c;
  unsigned pos = 0, n = 0;
  ZtString cmd = ZuMv(m_cmd);
  if (n = redirect.m(cmd, c, pos)) {
    m_cmd = c[0];
    ZeError e_;
    int r = f.open(c[2],
	ZiFile::Create | ZiFile::WriteOnly | ZiFile::GC, 0777, &e_);
    if (r != Zi::OK) {
      if (e) *e = ZeEVENT(Error, ([file = ZtString(c[2]), e = e_]
	  (const ZeEvent &, ZmStream &s) { s << file << ": " << e; }));
    }
    return r;
  } else {
    m_cmd = ZuMv(cmd);
    return Zi::OK;
  }
}

class Mx : public ZuObject, public ZiMultiplex {
public:
  inline Mx() : ZiMultiplex(ZvMxParams()) { }
};

class ZCmd : public ZmPolymorph, public ZvCmdClient {
public:
  void init(ZiMultiplex *mx) {
    ZvCmdClient::init(mx, getTimeout());

    m_interactive = isatty(fileno(stdin));
    if (!m_interactive && m_solo) {
      auto &data = m_soloMsg->data();
      ZuArrayN<char, 10240> buf;
      size_t length;
      while ((length = fread(buf.data(), 1, 10240, stdin)) > 0) {
	buf.length(length);
	data << buf;
      }
    }
  }
  void final() {
    ZvCmdClient::final();
  }

  inline bool interactive() const { return m_interactive; }

  inline void solo(ZuString s) {
    m_solo = true;
    m_soloMsg = new ZvCmdMsg();
    m_soloMsg->cmd() = s;
  }
  inline void target(ZuString s) {
    m_prompt.null();
    m_prompt << s << "] ";
  }

  void wait() { m_done.wait(); }
  void post() { m_done.post(); }

  inline int status() const { return m_status; }

  void exiting() { m_exiting = true; }

private:
  void process(ZmRef<ZvCmdMsg> ack) {
    std::cout << ack->cmd() << std::flush;
    if (m_out) {
      m_out.write(ack->data().data(), ack->data().length());
      m_out.close();
    } else {
      std::cout << ack->data() << std::flush;
    }
    m_status = ack->code();
    if (m_solo)
      post();
    else
      prompt();
  }

  void error(ZmRef<ZeEvent> e) {
    ZeLog::log(ZuMv(e));
    post();
  }

  void connected() {
    if (m_solo) {
      send(ZuMv(m_soloMsg));
    } else {
      std::cout <<
	"For a list of valid commands: help\n"
	"For help on a particular command: COMMAND --help\n" << std::flush;
      prompt();
    }
  }

  void prompt() {
    ZmRef<ZvCmdMsg> msg = new ZvCmdMsg();
    auto &cmd = msg->cmd();
next:
    if (m_interactive) {
      try {
	cmd = Zrl::readline_(m_prompt);
      } catch (const Zrl::EndOfFile &) {
	post();
	return;
      }
    } else {
      cmd.length(4096);
      if (!fgets(cmd, 4096, stdin)) {
	post();
	return;
      }
      cmd.calcLength();
      cmd.chomp();
    }
    if (!cmd) goto next;
    ZmRef<ZeEvent> e;
    if (msg->redirectIn(&e) != Zi::OK) {
      if (e) std::cerr << *e << '\n' << std::flush;
      goto next;
    }
    if (msg->redirectOut(m_out, &e) != Zi::OK) {
      if (e) std::cerr << *e << '\n' << std::flush;
      goto next;
    }
    send(ZuMv(msg));
  }

  void disconnected() {
    if (m_exiting) return;
    if (m_interactive) {
      Zrl::stop();
      std::cerr << "\nserver disconnected\n" << std::flush;
    }
    ZmPlatform::exit(1);
  }

  ZmSemaphore		m_done;
  bool			m_interactive = true;
  bool			m_solo = false;
  bool			m_exiting = false;
  ZmRef<ZvCmdMsg>	m_soloMsg;
  ZtString		m_prompt;
  int			m_status = 0;
  ZiFile		m_out;
};

int main(int argc, char **argv)
{
  ZmRef<Mx> mx = new Mx();
  ZmRef<ZCmd> client = new ZCmd();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::lambdaSink(
	[](ZeEvent *e) { std::cerr << e->message() << '\n' << std::flush; }));
  ZeLog::start();

  try {
    if (argc < 2) usage();

    ZiIP ip("127.0.0.1");
    ZuBox<int> port;

    {
      ZtRegex::Captures c;
      const auto &r = ZtStaticRegexUTF8("^([^:]+):(\\d+)$");
      if (r.m(argv[1], c) >= 2) {
	ip = c[2];
	if (!port.scan(c[3]) || !*port || !port) usage();
      } else {
	if (!port.scan(argv[1]) || !*port || !port) usage();
      }
    }

    if (mx->start() != Zi::OK) throw ZtString("multiplexer start failed");

    if (argc > 2) {
      ZtArray<char> solo;
      for(int i = 2; i < argc; i++) {
	solo << argv[i];
	if (ZuLikely(i < argc - 1)) solo << ' ';
      }
      client->solo(ZuMv(solo));
    } else
      client->target(argv[1]);

    client->init(mx);

    ZmTrap::sigintFn(ZmFn<>::Member<&ZCmd::post>::fn(client));
    ZmTrap::trap();

    client->connect(ip, port);

  } catch (const ZtRegex::Error &) {
    usage();
  } catch (const ZeError &e) {
    std::cerr << e << '\n' << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  } catch (const ZtString &s) {
    std::cerr << s << '\n' << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  } catch (...) {
    std::cerr << "unknown exception\n" << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  }

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

  return client->status();
}
