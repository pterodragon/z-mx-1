//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <stdio.h>
#include <signal.h>

#include <ZtRegex.hpp>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>

#include <ZtArray.hpp>

#include <ZeLog.hpp>

#include <ZiMultiplex.hpp>

#include "Global.hpp"

class Mx;

const char Request[] =
  "GET / HTTP/1.0\r\n"
  "User-Agent: ZiMxClient/1.0\r\n"
  "Accept: */*\r\n"
  "Host: localhost\r\n"
  "\r\n";

class Connection : public ZiConnection {
public:
  inline Connection(ZiMultiplex *mx, const ZiCxnInfo &ci, ZmTime now) :
      ZiConnection(mx, ci), m_headerLen(0), m_connectTime(now) { }
  ~Connection() { }

  inline Mx *mx() { return (Mx *)ZiConnection::mx(); }

  void disconnected();

  void connected(ZiIOContext &io) {
    m_header.size(4096);
    io.init(
      ZiIOFn::Member<&Connection::recvHeader>::fn(this),
      m_header.data(), m_header.size(), 0);
    send(ZiIOFn::Member<&Connection::sendRequest>::fn(this));
  }

  void sendRequest(ZiIOContext &io) {
    m_sendTime = ZmTimeNow();
    Global::timeInterval(0).add(m_sendTime - m_connectTime);
    //fwrite(Request, 1, len, stdout); fflush(stdout);
    io.init(ZiIOFn::Member<&Connection::sendComplete>::fn(this),
	(void *)Request, strlen(Request), 0);
  }
  void sendComplete(ZiIOContext &io) {
    if ((io.offset += io.length) >= io.size) {
      Global::timeInterval(1).add(ZmTimeNow() - m_sendTime);
      Global::sent(io.offset);
    }
  }

#include "HttpHeader.hpp"

  void recvHeader(ZiIOContext &io) {
    m_recvTime = ZmTimeNow();
    {
      bool incomplete = !httpHeaderEnd(io.ptr, io.offset, io.length);
      io.offset += io.length;
      if (incomplete) return;
    }
    {
      ZtRegex::Captures c(3);
      int i;

      m_header.length(m_headerLen, false);
      try {
	const auto &r = ZtStaticRegexUTF8("\\bContent-Length:\\s+(\\d+)");
	i = r.m(m_header, c);
      } catch (...) { i = 0; }
      if (i < 2) {
	ZeLOG(Error, "could not parse Content-Length");
	io.disconnect();
	return;
      }
      //{ printf("Content-Length: %d\n", contentLength); fflush(stdout); }
      m_content.size(ZuBox<unsigned>(c[2]));
    }
    if (io.offset > m_headerLen) {
      memcpy(m_content.data(),
	  m_header.data() + m_headerLen, io.offset - m_headerLen);
      if ((io.offset - m_headerLen) >= m_content.size()) {
	contentRcvd(io.offset);
	io.disconnect();
	return;
      }
    }
    io.init(
      ZiIOFn::Member<&Connection::recvContent>::fn(this),
      m_content.data(), m_content.size(), io.offset - m_headerLen);
  }

  void recvContent(ZiIOContext &io) {
    if ((io.offset += io.length) >= io.size) {
      contentRcvd(io.offset);
      io.disconnect();
    }
  }

  // process HTTP content
  void contentRcvd(unsigned n) {
    // { puts("Content received"); fflush(stdout); }
#if 0
    {
      fwrite(m_content, 1, m_content.size(), stdout);
      fflush(stdout);
      putchar('\n');
    }
#endif
    m_completedTime = ZmTimeNow();
    Global::timeInterval(2).add(m_completedTime - m_recvTime);
    Global::rcvd(n);
  }

private:
  ZtArray<char>	m_header;
  unsigned	m_headerLen;
  ZtArray<char>	m_content;
  ZmTime	m_connectTime;
  ZmTime	m_sendTime;
  ZmTime	m_recvTime;
  ZmTime	m_completedTime;
};

class Mx : public ZiMultiplex {
friend class Connection;

public:
  Mx(ZiIP ip, unsigned port, const ZiCxnOptions &options,
      unsigned nConnections, unsigned nConcurrent,
      unsigned maxRecv, int reconnInterval,
      ZiMxParams params) :
    ZiMultiplex(ZuMv(params)), m_ip(ip), m_port(port), m_options(options),
    m_nConnections(nConnections), m_nConcurrent(nConcurrent),
    m_maxRecv(maxRecv), m_reconnInterval(reconnInterval),
    m_nDisconnects(0) { }
  ~Mx() { }

  ZiConnection *connected(const ZiCxnInfo &ci) {
    return new Connection(this, ci, ZmTimeNow());
  }

  void disconnected(Connection *connection) {
    unsigned n = ++m_nDisconnects;
    if (n <= m_nConnections - m_nConcurrent)
      connect();
    if (n >= m_nConnections)
      Global::post();
  }

  void failed(bool transient) {
    if (transient && m_reconnInterval > 0) {
      std::cerr << "connect to " << m_ip << ':' << ZuBoxed(m_port) <<
	" failed, retrying...\n";
      add(ZmFn<>::Member<&Mx::connect>::fn(this),
	  ZmTimeNow(m_reconnInterval));
    } else if (++m_nDisconnects >= m_nConnections) {
      std::cerr << "connect failed\n";
      Global::post();
    }
  }

  void connect() {
    ZiMultiplex::connect(
	ZiConnectFn::Member<&Mx::connected>::fn(this),
	ZiFailFn::Member<&Mx::failed>::fn(this),
	ZiIP(), 0, m_ip, m_port, m_options);
  }

  inline unsigned maxRecv() const { return m_maxRecv; }

private:
  ZiIP			m_ip;
  unsigned		m_port;
  ZiCxnOptions		m_options;
  unsigned		m_nConnections;
  unsigned		m_nConcurrent;
  unsigned		m_maxRecv;
  int			m_reconnInterval;
  ZmAtomic<unsigned>	m_nDisconnects;
};

void Connection::disconnected()
{
  mx()->disconnected(this);
}

void dumpTimers()
{
  std::cout <<
    "connect: " << Global::timeInterval(0) << '\n' <<
    "send:    " << Global::timeInterval(1) << '\n' <<
    "recv:    " << Global::timeInterval(2) << '\n';
}

void usage()
{
  std::cerr <<
    "usage: ZiMxClient [OPTION]... IP PORT\n"
    "\nOptions:\n"
    "  -t N\t- use N threads (default: 3 - Rx + Tx + Worker)\n"
    "  -c N\t- exit after N connections (default: 1)\n"
    "  -r N\t- run N connections concurrently (default: 1)\n"
    "  -d N\t- disconnect early after receiving N bytes\n"
    "  -i N\t- reconnect with interval N secs (default: 1, <=0 disables)\n"
    "  -f\t- fragment I/O\n"
    "  -y\t- yield (context switch) on every lock acquisition\n"
    "  -v\t- enable ZiMultiplex debug\n"
    "  -m N\t- epoll - N is max number of file descriptors (default: 8)\n"
    "  -q N\t- epoll - N is epoll_wait() quantum (default: 8)\n"
    "  -R N\t- receive buffer size (default: OS setting)\n"
    "  -S N\t- send buffer size (default: OS setting)\n"
    << std::flush;
  ZmPlatform::exit(1);
}

int main(int argc, char **argv)
{
  ZiIP ip;
  int port = 0;
  ZiCxnOptions options;
  int nConnections = 1;
  int nConcurrent = 1;
  int maxRecv = 0;
  int reconnInterval = 1;
  ZiMxParams params;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (!ip) {
	try {
	  ip = argv[i];
	} catch (const ZeError &e) {
	  fprintf(stderr, "%s: IP address unresolvable (%s)\n",
	      argv[i], e.message());
	  ZmPlatform::exit(1);
	} catch (...) {
	  fprintf(stderr, "%s: IP address unresolvable\n", argv[i]);
	  ZmPlatform::exit(1);
	}
	continue;
      }
      if (!port) { port = atoi(argv[i]); continue; }
      usage();
      break;
    }
    switch (argv[i][1]) {
      case 't':
	{ int j; if ((j = atoi(argv[++i])) <= 0) usage(); params.nThreads(j); }
	break;
      case 'c':
	if ((nConnections = atoi(argv[++i])) <= 0) usage();
	break;
      case 'r':
	if ((nConcurrent = atoi(argv[++i])) <= 0) usage();
	break;
      case 'd':
	if ((maxRecv = atoi(argv[++i])) <= 0) usage();
	break;
      case 'i':
	reconnInterval = atoi(argv[++i]);
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
      case 'R':
	{
	  int j;
	  if ((j = atoi(argv[++i])) <= 0) usage();
	  params.rxBufSize(j);
	}
	break;
      case 'S':
	{
	  int j;
	  if ((j = atoi(argv[++i])) <= 0) usage();
	  params.txBufSize(j);
	}
	break;
      default:
	usage();
	break;
    }
  }
  if (!ip || !port) usage();

  ZeLog::init("ZiMxClient");
  ZeLog::level(0);
  ZeLog::add(ZeLog::debugSink());
  ZeLog::start();

  Mx mx(ip, port, options, nConnections, nConcurrent, maxRecv,
      reconnInterval, ZuMv(params));

  ZmTrap::sigintFn(ZmFn<>::Ptr<&Global::post>::fn());
  ZmTrap::trap();

  if (mx.start() != Zi::OK) ZmPlatform::exit(1);

  for (int i = 0; i < nConcurrent; i++) mx.connect();

  Global::wait();
  mx.stop(true);
  dumpTimers();
  Global::dumpStats();

  ZeLog::stop();

  return 0;
}
