//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>
#include <signal.h>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>

#include <ZtArray.hpp>
#include <ZtRegex.hpp>

#include <ZeLog.hpp>

#include <ZiMultiplex.hpp>

#include "Global.hpp"

const char *Messages[] = {
  "Hello",
  "Nice to meet ya",
  "Time to go",
  "Good bye"
};

class Mx;

class Connection : public ZiConnection {
public:
  inline Connection(
      ZiMultiplex *mx, const ZiConnectionInfo &ci, const ZiSockAddr &dest) :
    ZiConnection(mx, ci), m_counter(0), m_dest(dest) { }
  ~Connection() { }

  inline Mx *mx() { return (Mx *)ZiConnection::mx(); }

  void disconnected() { Global::post(); }

  void connected(ZiIOContext &io) {
    recvEcho(io);
    sendEcho();
  }

  void recvEcho(ZiIOContext &io) {
    m_msg.size(strlen(Messages[index()]));
    io.init(ZiIOFn::Member<&Connection::recvComplete>::fn(this),
	m_msg.data(), m_msg.size(), 0);
  }
  void recvComplete(ZiIOContext &);

  void sendEcho() {
    send(ZiIOFn::Member<&Connection::sendEcho_>::fn(this));
  }
  void sendEcho_(ZiIOContext &io) {
    unsigned i = index();
    unsigned len = strlen(Messages[i]);
    io.init(
	ZiIOFn::Member<&Connection::sendComplete>::fn(this),
	(void *)Messages[i], len, 0, m_dest);
  }
  void sendComplete(ZiIOContext &io) { io.complete(); }

  inline unsigned index() {
    return m_counter % (sizeof(Messages) / sizeof(Messages[0]));
  }

private:
  unsigned   	m_counter;
  ZiSockAddr	m_dest;
  ZtArray<char>	m_msg;
  ZiSockAddr	m_addr;
};

class Mx : public ZiMultiplex {
public:
  Mx(ZiIP localIP, unsigned localPort, ZiIP remoteIP, unsigned remotePort,
      bool connect, const ZiCxnOptions &options, unsigned nMessages,
      const ZiMultiplexParams &params) :
    ZiMultiplex(params), m_localIP(localIP), m_localPort(localPort),
    m_remoteIP(remoteIP), m_remotePort(remotePort),
    m_connect(connect), m_options(options), m_nMessages(nMessages) { }
  ~Mx() { }

  ZiConnection *connected(const ZiConnectionInfo &ci) {
    return new Connection(this, ci, m_dest);
  }

  void udp() {
    ZiIP remoteIP = m_remoteIP;
    unsigned remotePort = m_remotePort;
    m_dest = ZiSockAddr(remoteIP, remotePort);
    if (!m_connect) remoteIP = ZiIP(), remotePort = 0;


//	m_options.print(std::cout);


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

  inline unsigned nMessages() const { return m_nMessages; }

private:
  ZiIP		m_localIP;
  unsigned	m_localPort;
  ZiIP		m_remoteIP;
  unsigned	m_remotePort;
  bool		m_connect;
  ZiCxnOptions	m_options;
  unsigned	m_nMessages;
  ZiSockAddr	m_dest;
};

void usage()
{
  std::cerr <<
    "usage: ZiMxUDPClient [OPTION]...\n\n"
    "Options:\n"
    "  -t N\t\t- use N threads (default: 3 - Rx + Tx + Worker)\n"
    "  -n N\t\t- exit after N messages (default: 1)\n"
    "  -f\t\t- fragment I/O\n"
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
  exit(1);
}

int main(int argc, const char *argv[])
{
  ZiIP localIP;
  unsigned localPort = 0;
  ZiIP remoteIP("127.0.0.1");
  unsigned remotePort = 27413;
  bool connect = false;
  ZiCxnOptions options;
  unsigned nMessages = 1;
  ZiMultiplexParams params;

  options.udp(true);

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-' || argv[i][2]) usage();
    switch (argv[i][1]) {
      case 't':
	{ int j; if ((j = atoi(argv[++i])) <= 0) usage(); params.nThreads(j); }
	break;
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
	    static ZtRegex r(":", PCRE_UTF8);
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
	    static ZtRegex r(":", PCRE_UTF8);
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
	    static ZtRegex r("/", PCRE_UTF8);
	    int n = r.split(argv[++i], c);
	    if (n < 1 && n > 2) usage();
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

  ZeLog::init("ZiMxUDPClient");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  Mx mx(localIP, localPort, remoteIP, remotePort, connect, options,
      nMessages, params);

  ZmTrap::sigintFn(ZmFn<>::Ptr<&Global::post>::fn());
  ZmTrap::trap();

  if (mx.start() != Zi::OK) exit(1);

  mx.udp();

  Global::wait();
  mx.stop(true);
  
  ZeLog::stop();
  return 0;
}

void Connection::recvComplete(ZiIOContext &io)
{
  if (io.length < io.size) {
    ZeLOG(Error, "recvEcho - short packet");
    io.disconnect();
    return;
  }

  if (io.length != strlen(Messages[index()])) {
    ZeLOG(Error, "recvEcho - bad packet size");
    io.disconnect();
    return;
  }

  std::cout << ZtHexDump(
      ZtString() << io.addr.ip() << ':' << ZuBoxed(io.addr.port()) <<
      ZuString(m_msg.data(), io.length), m_msg.data(), io.length);

  fflush(stdout);

  unsigned nMessages = mx()->nMessages();
  if (++m_counter >= nMessages) {
    io.disconnect();
    return;
  }

  recvEcho(io);
  sendEcho();
}
