//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuLib.hpp>

#include <stdio.h>
#include <signal.h>

#include <zlib/ZmTime.hpp>
#include <zlib/ZmTrap.hpp>

#include <zlib/ZtArray.hpp>
#include <zlib/ZtRegex.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiMultiplex.hpp>

#include "Global.hpp"

void error(ZiConnection *, const char *op, int result, ZeError e)
{
  ZeLOG(Error, ZtString() << op << ' ' << Zi::resultName(result) << ' ' << e);
}

class Mx;

class Connection : public ZiConnection {
public:
  Connection(
      ZiMultiplex *mx, const ZiConnectionInfo &ci,
      ZiIP remoteIP, unsigned remotePort) :
    ZiConnection(mx, ci), m_counter(0) {
    if (!!remoteIP) m_dest.init(remoteIP, remotePort);
  }
  ~Connection() { }

  Mx *mx() { return (Mx *)ZiConnection::mx(); }

  void disconnected() { Global::post(); }

  void connected(ZiIOContext &io) { recvEcho(io); }

  void recvEcho(ZiIOContext &io) {
    m_msg.size(128);
    io.init(ZiIOFn::Member<&Connection::recvComplete>::fn(this),
	m_msg.data(), m_msg.size(), 0);
  }
  void recvComplete(ZiIOContext &);

  void sendEcho(ZiIOContext &io) {
    io.init(ZiIOFn::Member<&Connection::sendComplete>::fn(this),
	m_msg.data(), m_msg.length(), 0, m_echo);
  }
  void sendComplete(ZiIOContext &io);

private:
  unsigned	m_counter;
  ZtArray<char>	m_msg;
  ZiSockAddr	m_dest;
  ZiSockAddr	m_echo;
};

class Mx : public ZiMultiplex {
friend Connection;

public:
  Mx(ZiIP localIP, unsigned localPort, ZiIP remoteIP, unsigned remotePort,
      bool connect, const ZiCxnOptions &options, unsigned nMessages,
      ZiMxParams params) :
    ZiMultiplex(ZuMv(params)),
    m_localIP(localIP), m_localPort(localPort),
    m_remoteIP(remoteIP), m_remotePort(remotePort),
    m_connect(connect), m_options(options), m_nMessages(nMessages) { }
  ~Mx() { }
		
  ZiConnection *connected(const ZiConnectionInfo &ci) {
    return new Connection(this, ci, m_remoteIP, m_remotePort);
  }

  void udp() {
    ZiIP remoteIP = m_remoteIP;
    unsigned remotePort = m_remotePort;
    if (!m_connect) remoteIP = ZiIP(), remotePort = 0;
    ZiMultiplex::udp(
	ZiConnectFn::Member<&Mx::connected>::fn(this),
	ZiFailFn::Member<&Mx::failed>::fn(this),
	m_localIP, m_localPort, remoteIP, remotePort, m_options);
  }

  void failed(bool transient) {
    if (transient)
      add(ZmFn<>::Member<&Mx::udp>::fn(this), ZmTimeNow(1));
    else
      Global::post();
  }

  unsigned nMessages() const { return m_nMessages; }

private:
  ZiIP		m_localIP;
  unsigned	m_localPort;
  ZiIP		m_remoteIP;
  unsigned	m_remotePort;
  bool		m_connect;
  ZiCxnOptions	m_options;
  unsigned	m_nMessages;
};

void Connection::recvComplete(ZiIOContext &io)
{
  m_msg.length(io.offset + io.length);

  std::cout << ZtHexDump(
      ZtString() << io.addr.ip() << ':' << ZuBoxed(io.addr.port()) << ' ' <<
      ZuString(m_msg.data(), io.length), m_msg.data(), io.length);
  fflush(stdout);

  m_echo = !m_dest ? io.addr : m_dest;
  send(ZiIOFn::Member<&Connection::sendEcho>::fn(this));
}

void Connection::sendComplete(ZiIOContext &io)
{
  int nMessages = mx()->nMessages();
  if (nMessages >= 0 && ++m_counter >= (unsigned)nMessages)
    io.disconnect();
  else
    io.complete();
}

void usage()
{
  std::cerr <<
    "usage: ZiMxUDPServer [OPTION]...\n\n"
    "Options:\n"
    "  -t N\t\t- use N threads (default: 3 - Rx + Tx + Worker)\n"
    "  -n N\t\t- exit after N messages (default: infinite)\n"
    "  -f N\t\t- fragment I/O into N fragments\n"
    "  -y\t\t- yield (context switch) on every lock acquisition\n"
    "  -v\t\t- enable ZiMultiplex debug\n"
    "  -m N\t\t- epoll - N is max number of file descriptors (default: 8)\n"
    "  -q N\t\t- epoll - N is epoll_wait() quantum (default: 8)\n"
    "  -b [HOST:]PORT- bind to HOST:PORT (HOST defaults to INADDR_ANY)\n"
    "  -d HOST:PORT\t- send to HOST:PORT\n"
    "  -c\t\t- connect() - filter packets received from other sources\n"
    "  -M\t\t- use multicast\n"
    "  -L\t\t- use multicast loopback\n"
    "  -D IP\t\t- multicast to interface IP\n"
    "  -T N\t\t- multicast with TTL N\n"
    "  -G IP[/IF]\t- multicast subscribe to group IP on interface IF\n"
    "\t\t  IF is an IP address that defaults to 0.0.0.0\n"
    "\t\t  -G can be specified multiple times\n"
    << std::flush;
  ZmPlatform::exit(1);
}

extern const char Colon[] = ":";
extern const char Slash[] = "/";

int main(int argc, const char *argv[])
{
  ZiIP localIP("127.0.0.1");
  unsigned localPort = 27413;
  ZiIP remoteIP;
  unsigned remotePort = 0;
  bool connect = false;
  ZiCxnOptions options;
  unsigned nMessages = 1;
  ZmSchedParams schedParams;
  ZiMxParams params;

  options.udp(true);

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-' || argv[i][2]) usage();
    switch (argv[i][1]) {
      case 't': {
	int j;
	if ((j = atoi(argv[++i])) <= 0) usage();
	schedParams.nThreads(j);
      } break;
      case 'n':
	if ((nMessages = atoi(argv[++i])) <= 0) usage();
	break;
#ifdef ZiMultiplex_DEBUG
      case 'f':
	params.frag(true);
	break;
      case 'y':
	params.yield(true);
	break;
      case 'v':
	params.debug(true);
	break;
#endif
      case 'm':
	{
	  int j;
	  if ((j = atoi(argv[++i])) <= 0) usage();
#ifdef ZiMultiplex_EPoll
	  params.epollMaxFDs(j);
#endif
	}
	break;
      case 'q':
	{
	  int j;
	  if ((j = atoi(argv[++i])) <= 0) usage();
#ifdef ZiMultiplex_EPoll
	  params.epollQuantum(j);
#endif
	}
	break;
      case 'b':
	{
	  ZtRegex::Captures c;
	  try {
	    const auto &r = ZtStaticRegex(":");
	    int n = r.split(argv[++i], c);
	    if (n != 2) usage();
	    localIP = c[0].length() ? ZiIP(c[0]) : ZiIP();
	    localPort = atoi(c[1]);
	  } catch (...) { usage(); }
	}
	break;
      case 'd':
	{
	  ZtRegex::Captures c;
	  try {
	    const auto &r = ZtStaticRegex(":");
	    int n = r.split(argv[++i], c);
	    if (n != 2) usage();
	    remoteIP = c[0];
	    remotePort = atoi(c[1]);
	  } catch (...) { usage(); }
	}
	break;
      case 'c':
	connect = true;
	break;
      case 'M':
	options.multicast(true);
	break;
      case 'L':
	options.loopBack(true);
	break;
      case 'D':
	{
	  ZiIP mif(argv[++i]);
	  if (!mif) usage();
	  options.mif(mif);
	}
	break;
      case 'T':
	{
	  int ttl;
	  if ((ttl = atoi(argv[++i])) < 0) usage();
	  options.ttl(ttl);
	}
	break;
      case 'G':
	{
	  ZtRegex::Captures c;
	  try {
	    const auto &r = ZtStaticRegex("/");
	    int n = r.split(argv[++i], c);
	    if (n < 1 || n > 2) usage();
	    ZiIP addr(c[0]);
	    if (!addr.multicast()) usage();
	    ZiIP mif;
	    if (n == 2) mif = c[1];
	    options.mreq(ZiMReq(addr, mif));
	  } catch (...) { usage(); }
	}
	break;
      default:
	usage();
	break;
    }
  }

  ZeLog::init("ZiMxUDPServer");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::fileSink("&2"));
  ZeLog::start();

  Mx mx(localIP, localPort, remoteIP, remotePort, connect, options,
      nMessages, ZuMv(params));

  ZmTrap::sigintFn(ZmFn<>::Ptr<&Global::post>::fn());
  ZmTrap::trap();

  if (mx.start() != Zi::OK) ZmPlatform::exit(1);

  mx.udp();

  Global::wait();
  mx.stop(true);
  
  ZeLog::stop();
  return 0;
}

