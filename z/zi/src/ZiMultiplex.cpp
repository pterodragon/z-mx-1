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

// socket I/O multiplexing

#include <ZiMultiplex.hpp>

#include <ZmAssert.hpp>

#include <ZeLog.hpp>

#ifndef _WIN32
#include <alloca.h>

#define ZiEINVAL EINVAL
#define ZiENOTCONN ENOTCONN
#define ZiECONNRESET ECONNRESET
#define ZiEALREADY EALREADY
#define ZiEINPROGRESS EINPROGRESS
#define ZiENOBUFS ENOBUFS
#else
#define ZiEINVAL WSAEINVAL
#define ZiENOTCONN WSAEDISCON
#define ZiECONNRESET WSAECONNRESET
#define ZiEALREADY WSAELREADY
#ifdef ZiMultiplex_IOCP
#define ZiEINPROGRESS WSA_IO_PENDING
#else
#define ZiEINPROGRESS WSAEINPROGRESS
#endif
#define ZiENOBUFS WSAENOBUFS
#endif

#ifdef ZiMultiplex_IOCP

#include <ZmSingleton.hpp>

extern "C" {
  typedef BOOL (PASCAL *PConnectEx)(
      SOCKET s, const struct sockaddr *sa, int salen, void *ptr, DWORD len,
      DWORD *count, OVERLAPPED *overlapped);
  typedef BOOL (PASCAL *PAcceptEx)(
      SOCKET l, SOCKET s, void *ptr, DWORD len, int localsalen, int remotesalen,
      DWORD *count, OVERLAPPED *overlapped);
  typedef void (PASCAL *PGetAcceptExSockaddrs)(
      void *buf, DWORD len, DWORD localaddrlen, DWORD remoteaddrlen,
      struct sockaddr **localsa, int *localsalen,
      struct sockaddr **remotesa, int *remotesalen);
  typedef BOOL (PASCAL *PDisconnectEx)(
      SOCKET s, OVERLAPPED *overlapped, DWORD flags, DWORD reserved);
}

#ifndef WSAID_CONNECTEX
#define WSAID_CONNECTEX \
  {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#endif
#ifndef WSAID_ACCEPTEX
#define WSAID_ACCEPTEX \
  {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#endif
#ifndef WSAID_GETACCEPTEXSOCKADDRS
#define WSAID_GETACCEPTEXSOCKADDRS \
  {0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#endif
#ifndef WSAID_DISCONNECTEX
#define WSAID_DISCONNECTEX \
  {0x7fda2e11,0x8630,0x436f,{0xa0,0x31,0xf5,0x36,0xa6,0xee,0xc1,0x57}}
#endif

class ZiMultiplex_WSExt;

template <> struct ZmCleanup<ZiMultiplex_WSExt> {
  enum { Level = ZmCleanupLevel::Platform };
};

class ZiMultiplex_WSExt {
friend class ZmSingleton<ZiMultiplex_WSExt, 1>;

  ZiMultiplex_WSExt();

public:
  ~ZiMultiplex_WSExt();

  inline BOOL connectEx(
      SOCKET s, const struct sockaddr *sa, int salen, void *ptr, DWORD len,
      DWORD *count, OVERLAPPED *overlapped) {
    if (!m_connectEx) { WSASetLastError(WSASYSNOTREADY); return FALSE; }
    return (*m_connectEx)(s, sa, salen, ptr, len, count, overlapped);
  }
  inline BOOL acceptEx(
      SOCKET l, SOCKET s, void *ptr, DWORD len, int localsalen, int remotesalen,
      DWORD *count, OVERLAPPED *overlapped) {
    if (!m_acceptEx) { WSASetLastError(WSASYSNOTREADY); return FALSE; }
    return (*m_acceptEx)(l, s, ptr, len, localsalen, remotesalen,
			 count, overlapped);
  }
  inline void getAcceptExSockaddrs(
      void *buf, DWORD len, DWORD localaddrlen, DWORD remoteaddrlen,
      struct sockaddr **localsa, int *localsalen,
      struct sockaddr **remotesa, int *remotesalen) {
    if (!m_getAcceptExSockaddrs) { WSASetLastError(WSASYSNOTREADY); return; }
    (*m_getAcceptExSockaddrs)(buf, len, localaddrlen, remoteaddrlen,
			      localsa, localsalen, remotesa, remotesalen);
  }
  inline BOOL disconnectEx(
      SOCKET s, OVERLAPPED *overlapped, DWORD flags, DWORD reserved) {
    if (!m_disconnectEx) { WSASetLastError(WSASYSNOTREADY); return FALSE; }
    return (*m_disconnectEx)(s, overlapped, flags, reserved);
  }

  static ZiMultiplex_WSExt *instance();

private:
  PConnectEx		m_connectEx;
  PAcceptEx		m_acceptEx;
  PGetAcceptExSockaddrs	m_getAcceptExSockaddrs;
  PDisconnectEx		m_disconnectEx;
};

ZiMultiplex_WSExt *ZiMultiplex_WSExt::instance()
{
  return ZmSingleton<ZiMultiplex_WSExt>::instance();
}

ZiMultiplex_WSExt::ZiMultiplex_WSExt()
{
  typedef ZiPlatform::Socket Socket;
  Socket s = (Socket)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ZiPlatform::nullSocket(s)) goto error;
  {
    static GUID connectExGUID = WSAID_CONNECTEX;
    static GUID acceptExGUID = WSAID_ACCEPTEX;
    static GUID getAcceptExSockaddrsGUID = WSAID_GETACCEPTEXSOCKADDRS;
    static GUID disconnectExGUID = WSAID_DISCONNECTEX;

    DWORD n;
    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &connectExGUID, sizeof(GUID),
		 &m_connectEx, sizeof(PConnectEx), &n, 0, 0) ||
	n != sizeof(PConnectEx))
      goto error;

    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &acceptExGUID, sizeof(GUID),
		 &m_acceptEx, sizeof(PAcceptEx), &n, 0, 0) ||
	n != sizeof(PAcceptEx))
      goto error;

    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &getAcceptExSockaddrsGUID, sizeof(GUID),
		 &m_getAcceptExSockaddrs,
		 sizeof(PGetAcceptExSockaddrs), &n, 0, 0) ||
	n != sizeof(PGetAcceptExSockaddrs))
      goto error;

    if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		 &disconnectExGUID, sizeof(GUID),
		 &m_disconnectEx,
		 sizeof(PDisconnectEx), &n, 0, 0) ||
	n != sizeof(PDisconnectEx))
      goto error;
  }
  ::closesocket(s);
  return;

error:
  if (!ZiPlatform::nullSocket(s)) ::closesocket(s);
  m_connectEx = 0;
  m_acceptEx = 0;
  m_getAcceptExSockaddrs = 0;
  m_disconnectEx = 0;
}

ZiMultiplex_WSExt::~ZiMultiplex_WSExt()
{
}

#define ConnectEx(s, sa, salen, ptr, len, count, overlapped) \
  (ZiMultiplex_WSExt::instance()->connectEx(s, sa, salen, ptr, len, \
					    count, overlapped))
#define AcceptEx(l, s, ptr, len, localsalen, remotesalen, count, overlapped) \
  (ZiMultiplex_WSExt::instance()->acceptEx(l, s, ptr, len, localsalen, \
					   remotesalen, count, overlapped))
#define GetAcceptExSockaddrs(buf, len, localaddrlen, remoteaddrlen, \
			     localsa, localsalen, remotesa, remotesalen) \
  (ZiMultiplex_WSExt::instance()->getAcceptExSockaddrs(buf, len, \
						       localaddrlen, \
						       remoteaddrlen, \
						       localsa, localsalen, \
						       remotesa, remotesalen))
#define DisconnectEx(s, overlapped, flags, reserved) \
  (ZiMultiplex_WSExt::instance()->disconnectEx(s, overlapped, flags, reserved))

#endif /* ZiMultiplex_IOCP */

#if !defined(_WIN32) && defined(ZiMultiplex_Netlink)
#include <ZiNetlink.hpp>  // support netlink for Unix only
#endif

#ifdef ZiMultiplex_EPoll

#include <sys/resource.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0
#endif

#if 0
#ifndef __NR_eventfd
#if defined(__x86_64__)
#define __NR_eventfd 284
#elif defined(__i386__)
#define __NR_eventfd 323
#else
#error Unsupported architecture!
#endif
#endif /* !__NR_eventfd */
#endif

#endif /* ZiMultiplex_EPoll */

typedef ZuStringN<120> ErrorStr;

#define Log(severity, op, result, error) \
  ZeLOG(severity, \
      ErrorStr() << op << ' ' << Zi::resultName(result) << ' ' << error)
#define Error(op, result, error) Log(Error, op, result, error)
#define Warning(op, result, error) Log(Warning, op, result, error)

ZiConnection::ZiConnection(ZiMultiplex *mx, const ZiCxnInfo &info) :
  m_mx(mx), m_info(info), m_rxUp(1),
  m_rxRequests(0), m_rxBytes(0),
#ifdef ZiMultiplex_IOCP
  m_rxFlags(0),
#endif
  m_txUp(1), m_txRequests(0), m_txBytes(0)
{
}

ZiConnection::~ZiConnection()
{
  // precautionary code to ensure no leaking of sockets

  if (!ZiPlatform::nullSocket(m_info.socket)) {
    ZeLOG(Warning, "~ZiConnection() called with socket still open");
    ZiPlatform::closeSocket(m_info.socket);
  }
}

void ZiMultiplex::udp(ZiConnectFn fn, ZiFailFn failFn,
    ZiIP localIP, uint16_t localPort,
    ZiIP remoteIP, uint16_t remotePort,
    ZiCxnOptions options)
{
  StateGuard guard(m_stateLock);

  if (ZuUnlikely(state() != ZmScheduler::Running)) {
    guard.unlock();
    Error("udp", Zi::NotReady, ZeOK);
    failFn(false);
    return;
  }

  if (ZuUnlikely(!options.udp())) {
    guard.unlock();
    Error("udp", Zi::IOError, ZeError(ZiEINVAL));
    failFn(false);
    return;
  }

  if (ZuLikely(ZmPlatform::getTID() == m_rxTID)) {
    this->udp_(ZuMv(fn), ZuMv(failFn),
	localIP, localPort, remoteIP, remotePort, ZuMv(options));
    return;
  }
  ZmScheduler::run(rxThread(),
      [this, fn = ZuMv(fn), failFn = ZuMv(failFn),
	  localIP, localPort, remoteIP, remotePort, options = ZuMv(options)]() {
	this->udp_(fn, failFn,
	    localIP, localPort, remoteIP, remotePort, options);
	this->rx();
      });
  wake();
}

void ZiMultiplex::udp_(ZiConnectFn fn, ZiFailFn failFn,
    ZiIP localIP, uint16_t localPort,
    ZiIP remoteIP, uint16_t remotePort,
    ZiCxnOptions options)
{
  Socket s;

#ifndef _WIN32

  s = (Socket)::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s < 0) {
    Error("socket", Zi::IOError, ZeLastSockError);
    failFn(false); // FD exhaustion is generally not transient
    return;
  }

  if (options.multicast()) {
    int b = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	  (const char *)&b, sizeof(int)) < 0) {
      ZeError e(errno);
      ::close(s);
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, e);
      failFn(false);
      return;
    }
  }

  if (!!localIP || localPort) {
    ZiSockAddr local(localIP, localPort);
    if (bind(s, local.sa(), local.len()) < 0) {
      ZeError e(errno);
      ::close(s);
      Warning("bind", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

  if (!!remoteIP) {
    ZiSockAddr remote(remoteIP, remotePort);
    if (::connect(s, remote.sa(), remote.len()) < 0) {
      ZeError e(errno);
      ::close(s);
      Warning("connect", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

  if (options.multicast()) {
    if (!!options.mif()) {
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
	    (const char *)&options.mif(), sizeof(struct in_addr)) < 0) {
	ZeError e(errno);
	::close(s);
	Error("setsockopt(IP_MULTICAST_IF)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    if (options.ttl() > 0) {
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
	    (const char *)&options.ttl(), sizeof(int)) < 0) {
	ZeError e(errno);
	::close(s);
	Error("setsockopt(IP_MULTICAST_TTL)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    {
      int b = options.loopBack();
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP,
	    (const char *)&b, sizeof(int)) < 0) {
	ZeError e(errno);
	::close(s);
	Error("setsockopt(IP_MULTICAST_LOOP)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    if (unsigned n = options.mreqs().length()) {
      for (unsigned i = 0; i < n; i++) {
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	      (const char *)&(options.mreqs())[i],
	      sizeof(struct ip_mreq)) < 0) {
	  ZeError e(errno);
	  ::close(s);
	  Error("setsockopt(IP_ADD_MEMBERSHIP)", Zi::IOError, e);
	  failFn(false);
	  return;
	}
      }
    }
  }

#ifdef ZiMultiplex_EPoll
  if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
    ZeError e(errno);
    ::close(s);
    Error("fcntl(F_SETFL, O_NONBLOCK)", Zi::IOError, e);
    failFn(false);
    return;
  }
#endif

#else /* !_WIN32 */

  s = (Socket)::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (ZiPlatform::nullSocket(s)) {
    Error("socket", Zi::IOError, ZeLastSockError);
    failFn(false);
    return;
  }

  if (options.multicast()) {
    BOOL b = TRUE;
    if (setsockopt(
	  s, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(BOOL)) < 0) {
      ZeError e(WSAGetLastError());
      ::closesocket(s);
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, e);
      failFn(false);
      return;
    }
  }

  {
    ZiSockAddr local(localIP, localPort);
    if (bind(s, local.sa(), local.len())) {
      ZeError e(WSAGetLastError());
      ::closesocket(s);
      Warning("bind", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

  if (!!remoteIP) {
    ZiSockAddr remote(remoteIP, remotePort);
    if (::connect(s, remote.sa(), remote.len())) {
      ZeError e(WSAGetLastError());
      ::closesocket(s);
      Warning("connect", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

  if (options.multicast()) {
    if (!!options.mif()) {
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
	    (const char *)&options.mif(), sizeof(struct in_addr))) {
	ZeError e(WSAGetLastError());
	::closesocket(s);
	Error("setsockopt(IP_MULTICAST_IF)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    if (options.ttl() > 0) {
      DWORD ttl = options.ttl();
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
	    (const char *)&ttl, sizeof(DWORD))) {
	ZeError e(WSAGetLastError());
	::closesocket(s);
	Error("setsockopt(IP_MULTICAST_TTL)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    {
      BOOL b = options.loopBack();
      if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP,
	    (const char *)&b, sizeof(BOOL))) {
	ZeError e(WSAGetLastError());
	::closesocket(s);
	Error("setsockopt(IP_MULTICAST_LOOP)", Zi::IOError, e);
	failFn(false);
	return;
      }
    }
    if (int n = options.mreqs().length()) {
      for (int i = 0; i < n; i++) {
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	      (const char *)&(options.mreqs())[i], sizeof(struct ip_mreq))) {
	  ZeError e(WSAGetLastError());
	  ::closesocket(s);
	  Error("setsockopt(IP_ADD_MEMBERSHIP)", Zi::IOError, e);
	  failFn(false);
	  return;
	}
      }
    }
  }

#ifdef ZiMultiplex_IOCP
  if (!CreateIoCompletionPort((HANDLE)s, m_completionPort, 0, 0)) {
    ZeError e(GetLastError());
    ::closesocket(s);
    Error("CreateIoCompletionPort", Zi::IOError, e);
    failFn(false);
    return;
  }
#endif

#endif /* !_WIN32 */

  ZmRef<ZiConnection> cxn;

  cxn = (ZiConnection *)fn(ZiCxnInfo {
      ZiCxnType::UDP, s, options, localIP, localPort, remoteIP, remotePort });

  if (!cxn) {
    ZiPlatform::closeSocket(s);
    return;
  }

  if (!cxnAdd(cxn, s)) return;

  cxn->connected();

  ZiDEBUG(this, ZtSprintf("FD: % 3d UDP CONNECTED to %s:%u",
	(int)s, inet_ntoa(remoteIP), (unsigned)remotePort));

#ifdef ZiMultiplex_IOCP
  cxn->recv();
#endif
}

void ZiMultiplex::connect(
    ZiConnectFn fn, ZiFailFn failFn, ZiIP localIP, uint16_t localPort,
    ZiIP remoteIP, uint16_t remotePort, ZiCxnOptions options)
{
  StateGuard guard(m_stateLock);

  if (ZuUnlikely(state() != ZmScheduler::Running)) {
    guard.unlock();
    Error("connect", Zi::NotReady, ZeOK);
    failFn(false);
    return;
  }

  if (ZuUnlikely(options.udp())) {
    guard.unlock();
    Error("connect", Zi::IOError, ZeError(ZiEINVAL));
    failFn(false);
    return;
  }

  if (ZuLikely(ZmPlatform::getTID() == m_rxTID)) {
    this->connect_(ZuMv(fn), ZuMv(failFn),
	localIP, localPort, remoteIP, remotePort, ZuMv(options));
    return;
  }
  ZmScheduler::run(rxThread(),
      [this, fn = ZuMv(fn), failFn = ZuMv(failFn),
	  localIP, localPort, remoteIP, remotePort, options = ZuMv(options)]() {
	this->connect_(fn, failFn,
	    localIP, localPort, remoteIP, remotePort, options);
	this->rx();
      });
  wake();
}

void ZiMultiplex::connect_(ZiConnectFn fn, ZiFailFn failFn,
    ZiIP localIP, uint16_t localPort,
    ZiIP remoteIP, uint16_t remotePort, ZiCxnOptions options)
{
  Socket s;

#if !defined(_WIN32) && defined(ZiMultiplex_Netlink)
  if (options.netlink())
    s = (Socket)::socket(AF_NETLINK, SOCK_DGRAM, NETLINK_GENERIC);
  else
#endif
  s = (Socket)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ZiPlatform::nullSocket(s)) {
    ZeError e = ZeLastSockError;
    ZiPlatform::closeSocket(s);
    Error("socket", Zi::IOError, e);
    failFn(false);
    return;
  }

  if (!initSocket(s, options)) {
    ZiPlatform::closeSocket(s);
    failFn(false);
    return;
  }

  {
#ifndef _WIN32
#ifdef ZiMultiplex_Netlink
    if (options.netlink()) {
      ZiNetlinkSockAddr local;
      if (bind(s, local.sa(), local.len()) < 0) {
	ZeError e(errno);
	::close(s);
	Warning("bind", Zi::IOError, e);
	failFn(true);
	return;
      }
    } else
#endif
    {
      ZiSockAddr local(localIP, localPort);
      if (bind(s, local.sa(), local.len()) < 0) {
	ZeError e(errno);
	::close(s);
	Warning("bind", Zi::IOError, e);
	failFn(true);
	return;
      }
    }
#else
    ZiSockAddr local(localIP, localPort);
    if (bind(s, local.sa(), local.len())) {
      ZeError e(WSAGetLastError());
      ::closesocket(s);
      Warning("bind", Zi::IOError, e);
      failFn(true);
      return;
    }
#endif
  }

  ZiDEBUG(this, ZtSprintf("FD: % 3d CONNECTING to %s:%u", (int)s,
	inet_ntoa(remoteIP), (unsigned)remotePort));

  ZiSockAddr remote(remoteIP, remotePort);

#ifdef ZiMultiplex_IOCP
  Connect *request = new Connect(
      this, fn, failFn,
      ZiCxnType::TCPOut, s, options, localIP, localPort, remoteIP, remotePort);
  typedef ZiOverlapped::Executed Executed;
  Zi_Overlapped &overlapped = request->overlapped();
  overlapped.init(Executed::Member<&Connect::executed>::fn(request));
  ZeError e;
  if (ZuLikely(ConnectEx(s, remote.sa(), remote.len(), 0, 0, 0,
	  (OVERLAPPED *)&overlapped) ||
	(e = WSAGetLastError()).errNo() == WSA_IO_PENDING))
    return;
  delete request;
  Error("ConnectEx", Zi::IOError, e);
  failFn(false);
  return;
#endif

#ifdef ZiMultiplex_EPoll
  ZmRef<Connect> request = new Connect(this, fn, failFn,
      ZiCxnType::TCPOut, s, options, localIP, localPort, remoteIP, remotePort);

  if (!connectAdd(request, s)) return;

  connect(request);
#endif
}

#ifdef ZiMultiplex_IOCP
void ZiMultiplex::overlappedConnect(Connect *request,
    int status, unsigned, ZeError e)
{
  ZiCxnInfo &ci = request->info();

  if (ZuUnlikely(status != Zi::OK)) {
    ::closesocket(ci.socket);
    if (!m_stopping) Error("ConnectEx", status, e);
    request->fail(true);
    return;
  }

  {
    ZiSockAddr localSockAddr;
    int localSockAddrLen = localSockAddr.len();
    if (getsockname(ci.socket, localSockAddr.sa(), &localSockAddrLen)) {
      ZeError e(WSAGetLastError());
      ::closesocket(ci.socket);
      Error("getsockname", Zi::IOError, e);
      request->fail(false);
      return;
    }
    ci.localIP = localSockAddr.ip();
    ci.localPort = localSockAddr.port();
  }

  executedConnect(request->fn(), ci);
}
#endif

#ifdef ZiMultiplex_EPoll
// attempt completion of non-blocking connect
void ZiMultiplex::connect(Connect *request)
{
  ZiCxnInfo &ci = request->info();
  Socket s = ci.socket;

  if (ZuUnlikely(ci.options.udp())) { // should never occur at this point
    connectDel(s);
    ::close(s);
    Error("connect", Zi::IOError, ZeError(ZiEINVAL));
    request->fail(false);
    return;
  }

retry:
#ifdef ZiMultiplex_Netlink
  if (ci.options.netlink()) {
    ZeError e;
    if ((e = ZiNetlink::connect(
	s, ci.options.familyName(), ci.familyID, ci.portID)) != ZeOK) {
      connectDel(s);
      ::close(s);
      Error("connect", Zi::IOError, e);
      request->fail(true);
      return;
    }
  } else
#endif
  {
    ZiSockAddr remote(ci.remoteIP, ci.remotePort);
    if (::connect(s, remote.sa(), remote.len()) < 0) {
      ZeError e(errno);
      if (e.errNo() == EAGAIN || e.errNo() == EINPROGRESS) return;
      if (e.errNo() == EINTR) goto retry;
      connectDel(s);
      ::close(s);
      Error("connect", Zi::IOError, e);
      request->fail(true);
      return;
    }
  }

#ifdef ZiMultiplex_Netlink
  if (!ci.options.netlink())
#endif
  {
    ZiSockAddr local;
    socklen_t len = local.len();
    if (getsockname(s, local.sa(), &len) < 0) {
      ZeError e(errno);
      connectDel(s);
      ::close(s);
      Error("getsockname", Zi::IOError, e);
      request->fail(false);
      return;
    }
    ci.localIP = local.ip();
    ci.localPort = local.port();
  }

  ZiConnectFn fn = ZuMv(request->fn());

  connectDel(s);

  executedConnect(ZuMv(fn), ci);
}
#endif

void ZiMultiplex::executedConnect(ZiConnectFn fn, const ZiCxnInfo &ci)
{
  ZmRef<ZiConnection> cxn = (ZiConnection *)fn(ci);

  if (!cxn) {
    ZiPlatform::closeSocket(ci.socket);
    return;
  }

  if (!cxnAdd(cxn, ci.socket)) return;

  cxn->connected();

  ZiDEBUG(this, ZtSprintf("FD: % 3d TCP CONNECTED to %s:%u",
	(int)ci.socket, inet_ntoa(ci.remoteIP), (unsigned)ci.remotePort));
}

void ZiMultiplex::listen(
    ZiListenFn listenFn, ZiFailFn failFn, ZiConnectFn acceptFn,
    ZiIP localIP, uint16_t localPort, unsigned nAccepts, ZiCxnOptions options)
{
  StateGuard guard(m_stateLock);

  if (ZuUnlikely(state() != ZmScheduler::Running)) {
    guard.unlock();
    Error("listen", Zi::NotReady, ZeOK);
    failFn(false);
    return;
  }

#ifdef ZiMultiplex_Netlink
  if (options.netlink()) {
    guard.unlock();
    Error("listen", Zi::IOError, ZeError(ZiEINVAL));
    failFn(false);
    return;
  }
#endif

  if (ZuLikely(ZmPlatform::getTID() == m_rxTID)) {
    this->listen_(ZuMv(listenFn), ZuMv(failFn), ZuMv(acceptFn),
	localIP, localPort, nAccepts, ZuMv(options));
    return;
  }
  ZmScheduler::run(rxThread(),
      [this, listenFn = ZuMv(listenFn),
	  failFn = ZuMv(failFn), acceptFn = ZuMv(acceptFn),
	  localIP, localPort, nAccepts, options = ZuMv(options)]() {
	this->listen_(listenFn, failFn, acceptFn,
	    localIP, localPort, nAccepts, options);
	this->rx();
      });
  wake();
}

void ZiMultiplex::listen_(
    ZiListenFn listenFn, ZiFailFn failFn, ZiConnectFn acceptFn,
    ZiIP localIP, uint16_t localPort, unsigned nAccepts, ZiCxnOptions options)
{
  Socket lsocket;

#ifndef _WIN32

  lsocket = (Socket)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (lsocket < 0) {
    Error("socket", Zi::IOError, ZeLastSockError);
    failFn(false);
    return;
  }

  {
    int b = 1;
    if (setsockopt(lsocket,
	  SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(int)) < 0) {
      ZeError e(errno);
      ::close(lsocket);
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, e);
      failFn(false);
      return;
    }
  }

  {
    ZiSockAddr local(localIP, localPort);
    if (bind(lsocket, local.sa(), local.len()) < 0) {
      ZeError e(errno);
      ::close(lsocket);
      Warning("bind", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

#ifdef ZiMultiplex_EPoll
  if (fcntl(lsocket, F_SETFL, O_NONBLOCK) < 0) {
    ZeError e(errno);
    ::close(lsocket);
    Error("fcntl(F_SETFL, O_NONBLOCK)", Zi::IOError, e);
    failFn(false);
    return;
  }
#endif

#else /* !_WIN32 */

  lsocket = (Socket)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ZiPlatform::nullSocket(lsocket)) {
    Error("socket", Zi::IOError, ZeLastSockError);
    failFn(false);
    return;
  }

  {
    BOOL b = TRUE;
    if (setsockopt(lsocket,
	  SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(BOOL))) {
      ZeError e(WSAGetLastError());
      ::closesocket(lsocket);
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, e);
      failFn(false);
      return;
    }
  }

  {
    ZiSockAddr local(localIP, localPort);
    if (bind(lsocket, local.sa(), local.len())) {
      ZeError e(WSAGetLastError());
      ::closesocket(lsocket);
      Warning("bind", Zi::IOError, e);
      failFn(true);
      return;
    }
  }

#endif /* !_WIN32 */

  ZmRef<Listener> listener = new Listener(
      this, acceptFn, lsocket, nAccepts, localIP, localPort, options);

  if (!listenerAdd(listener, lsocket)) {
    ZiPlatform::closeSocket(lsocket);
    failFn(false);
    return;
  }

#ifdef ZiMultiplex_IOCP
  if (!CreateIoCompletionPort((HANDLE)lsocket, m_completionPort, 0, 0)) {
    ZeError e(GetLastError());
    listenerDel(lsocket);
    ::closesocket(lsocket);
    Error("CreateIoCompletionPort", Zi::IOError, e);
    failFn(false);
    return;
  }

  if (::listen(lsocket, nAccepts)) {
    ZeError e(WSAGetLastError());
    listenerDel(lsocket);
    ::closesocket(lsocket);
    Error("listen", Zi::IOError, e);
    failFn(false);
    return;
  }
#endif

#ifdef ZiMultiplex_EPoll
  if (::listen(lsocket, nAccepts) < 0) {
    ZeError e(errno);
    listenerDel(lsocket);
    ::close(lsocket);
    Error("listen", Zi::IOError, e);
    failFn(false);
    return;
  }
#endif

  m_nAccepts += nAccepts;

  listenFn(listener->info());

#ifdef ZiMultiplex_IOCP
  for (unsigned i = 0; i < nAccepts; i++) accept(listener);
#endif
}

void ZiMultiplex::stopListening(ZiIP localIP, uint16_t localPort)
{
  StateGuard guard(m_stateLock);

  if (ZuUnlikely(state() != ZmScheduler::Running)) return;

  if (ZuLikely(ZmPlatform::getTID() == m_rxTID)) {
    this->stopListening_(localIP, localPort);
    return;
  }
  ZmScheduler::run(rxThread(),
      [this, localIP, localPort]() {
	this->stopListening_(localIP, localPort);
	this->rx();
      });
  wake();
}

void ZiMultiplex::stopListening_(ZiIP localIP, uint16_t localPort)
{
  Socket lsocket;
  unsigned nAccepts;

  {
    ListenerHash::ReadIterator i(m_listeners);
    ZmRef<Listener> listener;

    while (listener = i.iterate()) {
      if (localIP == listener->info().ip &&
	  localPort == listener->info().port) {
	lsocket = listener->info().socket;
	nAccepts = listener->info().nAccepts;
	goto found;
      }
    }
    return;
  }

found:
  listenerDel(lsocket);

  // under IOCP, this should cause all pending accepts to complete
  // with an error
  ZiPlatform::closeSocket(lsocket);

  m_nAccepts -= nAccepts;
}

void ZiMultiplex::accept(Listener *listener)
{
#ifdef ZiMultiplex_IOCP
  Socket s = (Socket)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ZiPlatform::nullSocket(s)) {
    Error("socket", Zi::IOError, ZeLastSockError);
    return;
  }

  if (!initSocket(s, listener->info().options)) {
    ::closesocket(s);
    return;
  }

  Accept *request = new Accept(listener);
  request->info().socket = s;
  Zi_Overlapped &overlapped = request->overlapped();
  ZeError e;
  if (ZuLikely(AcceptEx(listener->info().socket, s, request->buf(), 0,
	  sizeof(struct sockaddr_in) + 16,
	  sizeof(struct sockaddr_in) + 16,
	  0, (OVERLAPPED *)&overlapped) ||
	(e = WSAGetLastError()).errNo() == WSA_IO_PENDING))
    return;
  delete request;
  ::closesocket(s);
  Error("AcceptEx", Zi::IOError, e);
#endif

#ifdef ZiMultiplex_EPoll
  int s;
  ZeError e;
  ZiSockAddr remote;
  socklen_t len = remote.len();

retry:
  s = ::accept(listener->info().socket, remote.sa(), &len);
  if (s < 0) {
    e = errno;
    if (e.errNo() == EAGAIN) return;
    if (e.errNo() == EINTR) goto retry;
    Error("accept", Zi::IOError, e);
    return;
  }

  if (!initSocket(s, listener->info().options)) {
    ::close(s);
    return;
  }

  ZiDEBUG(this, ZtSprintf("FD: % 3d ACCEPTING from %s:%u", s,
	inet_ntoa(remote.ip()), (unsigned)ntohs(remote.port())));

  executedConnect(listener->acceptFn(), ZiCxnInfo {
      ZiCxnType::TCPIn, s, listener->info().options,
      listener->info().ip, listener->info().port,
      remote.ip(), remote.port() });

  goto retry;
#endif
}

#ifdef ZiMultiplex_IOCP
void ZiMultiplex::overlappedAccept(Accept *request,
    int status, unsigned, ZeError e)
{
  ZmRef<Listener> listener = request->listener();

  if (ZuUnlikely(!listener->up())) {
    ::closesocket(request->info().socket);
    return;
  }

  accept(listener); // re-invoke

  ZiCxnInfo &ci = request->info();

  if (ZuUnlikely(status != Zi::OK)) {
    ::closesocket(ci.socket);
    Error("AcceptEx", status, e);
    return;
  }

  if (setsockopt(ci.socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
	(const char *)&listener->info().socket, sizeof(SOCKET))) {
    e = WSAGetLastError();
    ::closesocket(ci.socket);
    Error("setsockopt", Zi::IOError, e);
    return;
  }

  {
    ZiSockAddr *localSockAddr, *remoteSockAddr;
    int localSockAddrLen, remoteSockAddrLen;
    GetAcceptExSockaddrs(
	request->buf(), 0, sizeof(struct sockaddr_in) + 16,
	sizeof(struct sockaddr_in) + 16, (struct sockaddr **)&localSockAddr,
	&localSockAddrLen, (struct sockaddr **)&remoteSockAddr,
	&remoteSockAddrLen);
    ci.localIP = localSockAddr->ip();
    ci.localPort = localSockAddr->port();
    ci.remoteIP = remoteSockAddr->ip();
    ci.remotePort = remoteSockAddr->port();
  }

  executedConnect(listener->acceptFn(), ci);
}
#endif

void ZiConnection::connected()
{
  connected(m_rxContext);

#ifdef ZiMultiplex_IOCP
  if (ZuLikely(!m_rxContext.completed()))
    recv();
#endif

#ifdef ZiMultiplex_EPoll
  if (ZuUnlikely(m_rxContext.completed()))
    m_mx->epollRecv(this, m_info.socket, 0);
#endif
}

void ZiConnection::recv(ZiIOFn fn)
{
  m_mx->rxInvoke([this, fn = ZuMv(fn)]() { this->recv_(fn); });
}

void ZiConnection::recv_(ZiIOFn fn)
{
  m_rxContext.init_(ZuMv(fn));

#ifdef ZiMultiplex_IOCP
  if (ZuLikely(!m_rxContext.completed())) recv();
#endif

#ifdef ZiMultiplex_EPoll
  if (ZuLikely(!m_rxContext.completed()))
    if (!m_mx->epollRecv(this, m_info.socket, EPOLLIN | EPOLLRDHUP))
      m_rxContext.complete();
#endif
}

#ifdef ZiMultiplex_EPoll
bool ZiMultiplex::epollRecv(ZiConnection *cxn, int s, uint32_t events)
{
  struct epoll_event ev;
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = events | EPOLLOUT | EPOLLET;
  ev.data.u64 = (uintptr_t)cxn;
  if (epoll_ctl(m_epollFD, EPOLL_CTL_MOD, s, &ev) < 0) {
    Error("epoll_ctl(EPOLL_CTL_MOD)", Zi::IOError, ZeLastError);
    return false;
  }
  return true;
}
#endif

#ifdef ZiMultiplex_EPoll
bool ZiConnection::recv()
#else
void ZiConnection::recv()
#endif
{
#ifdef ZiMultiplex_DEBUG
  if (m_mx->trace()) m_mx->traceCapture();
#endif

  if (ZuUnlikely(!m_rxUp)) {
    m_rxContext.complete();
#ifdef ZiMultiplex_EPoll
    return false;
#else
    return;
#endif
  }

  if (ZuUnlikely(m_rxContext.completed()))
#ifdef ZiMultiplex_EPoll
    return true;
#else
    return;
#endif

#ifdef ZiMultiplex_IOCP
  unsigned len = m_rxContext.size - m_rxContext.offset;

  WSABUF wsaBuf;
  wsaBuf.buf = (char *)m_rxContext.ptr + m_rxContext.offset;
  wsaBuf.len = len;

  typedef ZiOverlapped::Executed Executed;
  Zi_Overlapped &overlapped = m_rxOverlapped;

#ifdef ZiMultiplex_DEBUG
  if (m_mx->frag()) {
    unsigned l = ((m_rxContext.offset + 8)>>1) + 1;
    if (len > l) len = l;
  }
  {
    unsigned long n;
    DWORD o;
    if (!WSAIoctl(m_info.socket, FIONREAD, 0, 0,
		  &n, sizeof(unsigned long), &o, 0, 0)) {
      ZiDEBUG(m_mx, ZtSprintf(
	    "FD: % 3d WSARecv(%u) size: %u offset: %u buffered: %lu "
	    "overlapped: %p",
	    (int)m_info.socket, len, m_rxContext.size, m_rxContext.offset, n,
	    (void *)&overlapped));
    }
  }
#endif

  m_rxFlags = 0;
  overlapped.init(Executed::Member<&ZiConnection::overlappedRecv>::fn(this));
  ZeError e;
  DWORD n;
  if (m_info.options.udp()) {
    INT addrLen = m_rxContext.addr.len();
    if (ZuLikely(WSARecvFrom(m_info.socket, &wsaBuf, 1, &n, &m_rxFlags,
	    m_rxContext.addr.sa(), &addrLen,
	    (WSAOVERLAPPED *)&overlapped, 0) != SOCKET_ERROR ||
	  (e = WSAGetLastError()).errNo() == WSA_IO_PENDING)) {
#ifdef ZiMultiplex_DEBUG
      if (m_mx->yield()) ZmPlatform::yield();
#endif
      return;
    }
  } else {
    if (ZuLikely(WSARecv(m_info.socket, &wsaBuf, 1, &n, &m_rxFlags,
	    (WSAOVERLAPPED *)&overlapped, 0) != SOCKET_ERROR ||
	  (e = WSAGetLastError()).errNo() == WSA_IO_PENDING)) {
#ifdef ZiMultiplex_DEBUG
      if (m_mx->yield()) ZmPlatform::yield();
#endif
      return;
    }
  }
  errorRecv(Zi::IOError, e);
  m_rxContext.completed();
  return;
#endif

#ifdef ZiMultiplex_EPoll
  if (ZuUnlikely(m_rxContext.completed())) {
    m_mx->epollRecv(this, m_info.socket, 0);
    return true;
  }

  unsigned len = m_rxContext.size - m_rxContext.offset;
  void *buf = (char *)m_rxContext.ptr + m_rxContext.offset;

  ZeError e;
  int n;

#ifdef ZiMultiplex_DEBUG
  if (m_mx->frag()) {
    unsigned l = ((m_rxContext.offset + 8)>>1) + 1;
    if (len > l) len = l;
  }
  ioctl(m_info.socket, FIONREAD, &n);
  ZiDEBUG(m_mx, ZtSprintf(
	"FD: % 3d recv(%u) size: %u offset: %u buffered: %u",
	(int)m_info.socket, len, m_rxContext.size, m_rxContext.offset, n));
#endif

retry:
  if (m_info.options.udp()) {
    socklen_t addrLen = m_rxContext.addr.len();
    n = ::recvfrom(m_info.socket, (char *)buf, len, 0,
	m_rxContext.addr.sa(), &addrLen);
  } else {
    n = ::recv(m_info.socket, (char *)buf, len, 0);
  }
  if (ZuUnlikely(n < 0)) e = errno;

  ZiDEBUG(m_mx, ZtSprintf(
	"FD: % 3d recv(%d): %d errno: %d (EAGAIN=%d EINTR=%d)",
	(int)m_info.socket, len, n, (int)e.errNo(), (int)EAGAIN, (int)EINTR));

  if (ZuUnlikely(n < 0)) {
    if (e.errNo() == EAGAIN) {
#ifdef ZiMultiplex_DEBUG
      if (m_mx->yield()) ZmPlatform::yield();
#endif
      return true;
    }
    if (e.errNo() == EINTR) goto retry;
    errorRecv(Zi::IOError, e);
    m_rxContext.completed();
    return false;
  }

  ZiDEBUG(m_mx, ZtHexDump(ZtSprintf(
	  "FD: % 3d recv(%d): %d", (int)m_info.socket, len, n), buf, n));

  if (ZuUnlikely(!n && !m_info.options.udp())) {
    m_rxContext.completed();
    disconnect();
    return false;
  }

  executedRecv(n);

  if (ZuUnlikely(m_rxContext.completed())) {
    if (m_rxContext.disconnected()) {
      disconnect();
      return false;
    } else {
      m_mx->epollRecv(this, m_info.socket, 0);
      return true;
    }
  }

  if (ZuUnlikely(m_rxContext.offset >= m_rxContext.size)) {
    m_rxContext.complete();
    m_mx->epollRecv(this, m_info.socket, 0);
    return true;
  }

  if (m_info.options.udp()) return true;

  len = m_rxContext.size - m_rxContext.offset;
  buf = (char *)m_rxContext.ptr + m_rxContext.offset;
  goto retry;
#endif
}

#ifdef ZiMultiplex_IOCP
void ZiConnection::overlappedRecv(int status, unsigned n, ZeError e)
{
  if (ZuUnlikely(status != Zi::OK)) {
    if (m_rxUp) errorRecv(status, e);
    return;
  }
  if (ZuUnlikely(!n && !m_info.options.udp())) {
    if (m_rxUp) disconnect();
    return;
  }

  executedRecv(n);

  if (ZuUnlikely(m_rxContext.completed())) {
    if (m_rxContext.disconnected()) disconnect();
    return;
  }

  if (ZuUnlikely(m_rxContext.offset >= m_rxContext.size)) {
    m_rxContext.complete();
    return;
  }

  recv();
}
#endif

void ZiConnection::errorRecv(int status, ZeError e)
{
  if (m_rxUp) close();
  if (status == Zi::IOError &&
      (e.errNo() == ZiENOTCONN || e.errNo() == ZiECONNRESET)) return;
  Error(
#ifndef _WIN32
      "recv",
#else
      "WSARecv",
#endif
      Zi::IOError, e);
}

void ZiConnection::executedRecv(unsigned n)
{
  ZiDEBUG(m_mx, "executedRecv()");

  ZmAssert(!m_rxContext.completed());

#ifdef ZiMultiplex_IOCP
  ZiDEBUG(m_mx, ZtHexDump(ZtSprintf(
	  "FD: % 3d WSARecv(%d)", (int)m_info.socket, n),
	(char *)m_rxContext.ptr + m_rxContext.offset, n));
#endif

  m_rxRequests++, m_rxBytes += n;
  m_rxContext.length = n;
  m_rxContext();
}

void ZiConnection::send(ZiIOFn fn)
{
  m_mx->ZmScheduler::invoke(m_mx->txThread(),
	[this, fn = ZuMv(fn)]() { this->send_(fn); });
}

void ZiConnection::send_(ZiIOFn fn)
{
  m_txContext.init_(ZuMv(fn));
  send();
}

void ZiConnection::send()
{
#ifdef ZiMultiplex_DEBUG
  if (m_mx->trace()) m_mx->traceCapture();
#endif

  if (ZuUnlikely(!m_txUp)) { m_txContext.complete(); return; }

  if (ZuLikely(m_txContext.completed())) return;

  unsigned len = m_txContext.size - m_txContext.offset;

#ifdef ZiMultiplex_IOCP
  WSABUF wsaBuf;
  wsaBuf.buf = (char *)m_txContext.ptr + m_txContext.offset;
  wsaBuf.len = len;

  typedef ZiOverlapped::Executed Executed;
  Zi_Overlapped &overlapped = m_txOverlapped;

  ZiDEBUG(m_mx, ZtHexDump(ZtSprintf(
	  "FD: % 3d WSASend(%u) size: %u offset: %u overlapped: %p",
	(int)m_info.socket, len, m_txContext.size, m_txContext.offset,
	(void *)&overlapped), wsaBuf.buf, wsaBuf.len));

  overlapped.init(Executed::Member<&ZiConnection::overlappedSend>::fn(this));
  ZeError e;
  DWORD n;
  if (m_info.options.udp() && !!m_txContext.addr) {
    if (ZuLikely(WSASendTo(m_info.socket, &wsaBuf, 1, &n, 0,
	    m_txContext.addr.sa(), m_txContext.addr.len(),
	    (WSAOVERLAPPED *)&overlapped, 0) != SOCKET_ERROR ||
	  (e = WSAGetLastError()).errNo() == WSA_IO_PENDING))
      return;
  } else {
    if (ZuLikely(WSASend(m_info.socket, &wsaBuf, 1, &n, 0,
	    (WSAOVERLAPPED *)&overlapped, 0) != SOCKET_ERROR ||
	  (e = WSAGetLastError()).errNo() == WSA_IO_PENDING))
      return;
  }
  errorSend(Zi::IOError, e);
  return;
#endif

#ifdef ZiMultiplex_EPoll
  void *buf = (char *)m_txContext.ptr + m_txContext.offset;

  ZeError e;
  int n;

retry:
  ZiDEBUG(m_mx, ZtHexDump(ZtSprintf(
	  "FD: % 3d send(%u) size: %u offset: %u",
	(int)m_info.socket, len, m_txContext.size, m_txContext.offset),
	buf, len));

  if (m_info.options.udp())
    n = ::sendto(
	m_info.socket, (const char *)buf, len, 0,
	m_txContext.addr.sa(), m_txContext.addr.len());
#ifdef ZiMultiplex_Netlink
  else if (m_info.options.netlink())
    n = ZiNetlink::send(
	m_info.socket, m_ci.familyID, m_ci.portID,
	(const void *)buf, len);
#endif
  else
    n = ::send(m_info.socket, (const char *)buf, len, 0);
  if (ZuUnlikely(n < 0)) e = errno;

  ZiDEBUG(m_mx, ZtSprintf(
	"FD: % 3d send(%d): %d errno: %d (EAGAIN=%d EINTR=%d)",
	(int)m_info.socket, len, n, (int)e.errNo(), (int)EAGAIN, (int)EINTR));

  if (ZuUnlikely(n < 0)) {
    if (e.errNo() == EAGAIN) {
#ifdef ZiMultiplex_DEBUG
      if (m_mx->yield()) ZmPlatform::yield();
#endif
      return;
    }
    if (e.errNo() == EINTR) goto retry;
    errorSend(Zi::IOError, e);
    return;
  }

  ZiDEBUG(m_mx, ZtSprintf(
	  "FD: % 3d send(%d): %d", (int)m_info.socket, len, n));

  executedSend(n);

  if (ZuLikely(m_txContext.completed())) {
    if (m_txContext.disconnected()) disconnect();
    return;
  }

  if (ZuLikely(m_txContext.offset >= m_txContext.size)) {
    m_txContext.complete();
    return;
  }

  len = m_txContext.size - m_txContext.offset;
  buf = (char *)m_txContext.ptr + m_txContext.offset;

  goto retry;
#endif
}

#ifdef ZiMultiplex_IOCP
void ZiConnection::overlappedSend(int status, unsigned n, ZeError e)
{
  if (ZuUnlikely(status != Zi::OK)) {
    if (m_txUp) errorSend(status, e);
    return;
  }

  executedSend(n);

  if (ZuLikely(m_txContext.completed())) {
    if (m_txContext.disconnected()) disconnect();
    return;
  }

  if (ZuLikely(m_txContext.offset >= m_txContext.size)) {
    m_txContext.complete();
    return;
  }

  send();
}
#endif

void ZiConnection::errorSend(int status, ZeError e)
{
  if (m_txUp) close();
  if (status == Zi::IOError &&
      (e.errNo() == ZiENOTCONN || e.errNo() == ZiECONNRESET)) return;
  Error(
#ifndef _WIN32
      "send",
#else
      "WSASend",
#endif
      status, e);
}

void ZiConnection::executedSend(unsigned n)
{
  ZiDEBUG(m_mx, "executedSend()");

#ifdef ZiMultiplex_IOCP
  ZiDEBUG(m_mx, ZtSprintf(
	  "FD: % 3d WSASend(%d)", (int)m_info.socket, n));
#endif

  m_txRequests++, m_txBytes += n;

  m_txContext.length = n;
  m_txContext();
}

bool ZiMultiplex::initSocket(Socket s, const ZiCxnOptions &options)
{
#ifndef _WIN32
  {
    int b = 1;
    static struct linger l = { 0, 0 };
    if (m_rcvBufSize && setsockopt(s, SOL_SOCKET, SO_RCVBUF,
	  (const char *)&m_rcvBufSize, sizeof(unsigned)) < 0) {
      Error("setsockopt(SO_RCVBUF)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (m_sndBufSize && setsockopt(s, SOL_SOCKET, SO_SNDBUF,
	  (const char *)&m_sndBufSize, sizeof(unsigned)) < 0) {
      Error("setsockopt(SO_SNDBUF)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	  (const char *)&b, sizeof(int)) < 0) {
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (setsockopt(s, SOL_SOCKET, SO_LINGER,
	  (const char *)&l, sizeof(struct linger)) < 0) {
      Error("setsockopt(SO_LINGER)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (options.keepAlive() && setsockopt(s, SOL_SOCKET, SO_KEEPALIVE,
	  (const char *)&b, sizeof(int)) < 0) {
      Error("setsockopt(SO_KEEPALIVE)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (!options.udp() && !options.netlink() && !options.nagle() &&
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
	  (const char *)&b, sizeof(int)) < 0) {
      Error("setsockopt(TCP_NODELAY)", Zi::IOError, ZeLastSockError);
      return false;
    }
  }

#ifdef ZiMultiplex_EPoll
  if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
    Error("fcntl(O_NONBLOCK)", Zi::IOError, ZeLastError);
    return false;
  }
#endif

#else /* !_WIN32 */

  {
    BOOL b = TRUE;
    if (m_rcvBufSize && setsockopt(s, SOL_SOCKET, SO_RCVBUF,
	  (const char *)&m_rcvBufSize, sizeof(unsigned))) {
      Error("setsockopt(SO_RCVBUF)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (m_sndBufSize && setsockopt(s, SOL_SOCKET, SO_SNDBUF,
	  (const char *)&m_sndBufSize, sizeof(unsigned))) {
      Error("setsockopt(SO_SNDBUF)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	  (const char *)&b, sizeof(BOOL))) {
      Error("setsockopt(SO_REUSEADDR)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (setsockopt(s, SOL_SOCKET, SO_DONTLINGER,
	  (const char *)&b, sizeof(BOOL))) {
      Error("setsockopt(SO_DONTLINGER)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (options.keepAlive() && setsockopt(s, SOL_SOCKET, SO_KEEPALIVE,
	  (const char *)&b, sizeof(BOOL))) {
      Error("setsockopt(SO_KEEPALIVE)", Zi::IOError, ZeLastSockError);
      return false;
    }
    if (!options.udp() && !options.netlink() && !options.nagle() &&
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
	  (const char *)&b, sizeof(BOOL))) {
      Error("setsockopt(TCP_NODELAY)", Zi::IOError, ZeLastSockError);
      return false;
    }
  }

#ifdef ZiMultiplex_IOCP
  if (!CreateIoCompletionPort((HANDLE)s, m_completionPort, 0, 0)) {
    Error("CreateIoCompletionPort", Zi::IOError, ZeLastError);
    return false;
  }
#endif

#endif /* !_WIN32 */

  return true;
}

bool ZiMultiplex::cxnAdd(ZiConnection *cxn, Socket s)
{
  m_cxns.add(cxn);

#ifdef ZiMultiplex_EPoll
  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.u64 = (uintptr_t)cxn;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, s, &ev) < 0) {
      ZeError e(errno);
      delete m_cxns.del(s);
      ZiPlatform::closeSocket(s);
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return false;
    }
  }
#endif

  return true;
}

void ZiMultiplex::cxnDel(Socket s)
{
#ifdef ZiMultiplex_EPoll
  epoll_ctl(m_epollFD, EPOLL_CTL_DEL, s, 0);
#endif

  delete m_cxns.del(s);
}

bool ZiMultiplex::listenerAdd(Listener *listener, Socket s)
{
  m_listeners.add(listener);

#ifdef ZiMultiplex_EPoll
  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.u64 = ((uintptr_t)listener) | 1;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, s, &ev) < 0) {
      ZeError e(errno);
      m_listeners.del(s);
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return false;
    }
  }
#endif

  return true;
}

void ZiMultiplex::listenerDel(Socket s)
{
#ifdef ZiMultiplex_EPoll
  epoll_ctl(m_epollFD, EPOLL_CTL_DEL, s, 0);
#endif

  if (ZmRef<Listener> listener = m_listeners.del(s))
    listener->down();
}

#ifdef ZiMultiplex_EPoll
bool ZiMultiplex::connectAdd(Connect *request, Socket s)
{
  m_connects.add(request);

  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLOUT;
    ev.data.u64 = ((uintptr_t)request) | 2;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, s, &ev) < 0) {
      ZeError e(errno);
      m_connects.del(s);
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return false;
    }
  }

  return true;
}
#endif

void ZiMultiplex::connectDel(Socket s)
{
#ifdef ZiMultiplex_EPoll
  epoll_ctl(m_epollFD, EPOLL_CTL_DEL, s, 0);
#endif

#if ZiMultiplex__ConnectHash
  m_connects.del(s);
#endif
}

void ZiConnection::disconnect()
{
  m_mx->ZmScheduler::invoke(m_mx->txThread(),
      ZmFn<>::Member<&ZiConnection::disconnect_1>::fn(this));
}

void ZiConnection::disconnect_1()
{
  if (!m_txUp) return;
  
  m_txUp = false;

  ZiMultiplex *mx = m_mx;

  mx->rxInvoke(ZmFn<>([](ZiConnection *self) { self->disconnect_2(); }, this));
}

void ZiConnection::disconnect_2()
{
  if (!m_rxUp) return;

  m_rxUp = false;

  if (m_info.options.udp()
#ifdef ZiMultiplex_NetLink
      || m_info.options.netlink()
#endif
      ) {
    executedDisconnect();
    return;
  }

  ZiDEBUG(m_mx, ZtSprintf("FD: % 3d disconnect()", (int)m_info.socket));
  
#ifdef ZiMultiplex_IOCP
  typedef ZiOverlapped::Executed Executed;
  Zi_Overlapped &overlapped = m_disconnectOverlapped;
  ZeError e;
  overlapped.init(
      Executed::Member<&ZiConnection::overlappedDisconnect>::fn(this));
  if (ZuLikely(DisconnectEx(m_info.socket, (OVERLAPPED *)&overlapped, 0, 0) ||
	(e = WSAGetLastError()).errNo() == WSA_IO_PENDING))
    return;
  errorDisconnect(Zi::IOError, e);
#endif

#ifdef ZiMultiplex_EPoll
  ZeError e;
  int i;

retry:
  i = ::shutdown(m_info.socket, SHUT_RDWR);
  if (i < 0) {
    e = errno;
    if (e.errNo() == EAGAIN) return;
    if (e.errNo() == EINTR) goto retry;
    errorDisconnect(Zi::IOError, e);
  }
  executedDisconnect();
#endif
}

#ifdef ZiMultiplex_IOCP
void ZiConnection::overlappedDisconnect(int status, unsigned n, ZeError e)
{
  if (ZuUnlikely(status != Zi::OK)) {
    errorDisconnect(status, e);
    return;
  }
  executedDisconnect();
}
#endif

void ZiConnection::errorDisconnect(int status, ZeError e)
{
  ZmRef<ZiConnection> self(this);
  m_mx->disconnected(self);
  Error(
#ifndef _WIN32
      "shutdown",
#else
      "DisconnectEx",
#endif
      status, e);
  ZiPlatform::closeSocket(m_info.socket);
  m_info.socket = ZiPlatform::nullSocket();
  disconnected();
}

void ZiConnection::executedDisconnect()
{
  ZmRef<ZiConnection> self(this);
  m_mx->disconnected(self);
  ZiPlatform::closeSocket(m_info.socket);
  m_info.socket = ZiPlatform::nullSocket();
  disconnected();
}

void ZiMultiplex::disconnected(ZiConnection *cxn)
{
  Socket s = cxn->info().socket;

  ZiDEBUG(this, ZtSprintf("FD: % 3d disconnected()", (int)s));

  cxnDel(s);
  
  if (ZuUnlikely(m_stopping && !m_cxns.count())) stop_2();
}

void ZiConnection::close()
{
  m_mx->ZmScheduler::invoke(m_mx->txThread(),
      ZmFn<>::Member<&ZiConnection::close_1>::fn(this));
}

void ZiConnection::close_1()
{
  if (!m_txUp) return;
  
  m_txUp = false;

  ZiMultiplex *mx = m_mx;

  mx->rxInvoke(ZmFn<>([](ZiConnection *self) { self->close_2(); }, this));
}

void ZiConnection::close_2()
{
  if (!m_rxUp) return;
  
  m_rxUp = false;

  ZiDEBUG(m_mx, ZtSprintf("FD: % 3d close()", (int)m_info.socket));

  executedDisconnect();
}

ZiMultiplex::ZiMultiplex(ZiMultiplexParams params) :
  ZmScheduler(params.scheduler()),
  m_stopping(0), m_drain(false),
  m_rxThread(params.rxThread()),
  m_listeners(ZmHashParams().bits(4).loadFactor(1).cBits(4).init(
	params.listenerHash())),
  m_nAccepts(0),
#if ZiMultiplex__ConnectHash
  m_connects(ZmHashParams().bits(5).loadFactor(1).cBits(4).init(
	params.requestHash())),
#endif
  m_cxns(ZmHashParams().bits(8).loadFactor(1).cBits(4).init(params.cxnHash())),
  m_txThread(params.txThread()),
  m_rcvBufSize(params.rcvBufSize()),
  m_sndBufSize(params.sndBufSize())
#ifdef ZiMultiplex_EPoll
  , m_epollFD(-1),
  m_epollMaxFDs(params.epollMaxFDs()),
  m_epollQuantum(params.epollQuantum()),
  m_wakeFD(-1), m_wakeFD2(-1)
#endif
#ifdef ZiMultiplex_DEBUG
  , m_trace(params.trace()),
  m_debug(params.debug()),
  m_frag(params.frag()),
  m_yield(params.yield())
#endif
{
}

ZiMultiplex::~ZiMultiplex()
{
}

int ZiMultiplex::start()
{
  StateGuard guard(m_stateLock);

  // deal with multiple, potentially overlapping invocations

  switch (state()) {
    case ZmScheduler::Stopping:
      ZmScheduler::stop();
    case ZmScheduler::Stopped:
      break;
    case ZmScheduler::Running:
      return Zi::OK;
    default:
      ZmScheduler::start();
      return Zi::OK;
  }

#ifdef ZiMultiplex_IOCP
  {
    WSADATA wd;
    DWORD errNo = WSAStartup(MAKEWORD(2, 2), &wd);
    if (errNo) {
      Error("WSAStartup", Zi::IOError, ZeError(errNo));
      return Zi::IOError;
    }
  }

  m_completionPort =
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, nThreads());
  if (!m_completionPort) {
    ZeError e(GetLastError());
    WSACleanup();
    Error("CreateIoCompletionPort", Zi::IOError, e);
    return Zi::IOError;
  }
#endif

#ifdef ZiMultiplex_EPoll
  {
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGPIPE);
    sigaddset(&s, SIGURG);
    pthread_sigmask(SIG_BLOCK, &s, 0);
  }
  if ((m_epollFD = epoll_create(m_epollMaxFDs)) < 0) {
    Error("epoll_create", Zi::IOError, ZeLastError);
    return Zi::IOError;
  }
  if (pipe(&m_wakeFD) < 0) {
    ZeError e(errno);
    ::close(m_epollFD); m_epollFD = -1;
    Error("pipe", Zi::IOError, e);
    return Zi::IOError;
  }
  if (fcntl(m_wakeFD, F_SETFL, O_NONBLOCK) < 0) {
    ZeError e(errno);
    ::close(m_epollFD); m_epollFD = -1;
    ::close(m_wakeFD); m_wakeFD = -1;
    ::close(m_wakeFD2); m_wakeFD2 = -1;
    Error("fcntl(F_SETFL, O_NONBLOCK)", Zi::IOError, e);
    return Zi::IOError;
  }
  {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.u64 = 3;
    if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, m_wakeFD, &ev) < 0) {
      ZeError e(errno);
      ::close(m_epollFD); m_epollFD = -1;
      ::close(m_wakeFD); m_wakeFD = -1;
      ::close(m_wakeFD2); m_wakeFD2 = -1;
      Error("epoll_ctl(EPOLL_CTL_ADD)", Zi::IOError, e);
      return Zi::IOError;
    }
  }
#endif

  ZmScheduler::run(rxThread(),
      ZmFn<>::Member<&ZiMultiplex::rxStart>::fn(this));
  ZmScheduler::start();
  return Zi::OK;
}

void ZiMultiplex::stop(bool drain)
{
  StateGuard guard(m_stateLock);

  // deal with multiple, potentially overlapping invocations
  switch (state()) {
    case ZmScheduler::Draining:
    case ZmScheduler::Drained:
    case ZmScheduler::Stopping:
      wake();
      ZmScheduler::stop();
    case ZmScheduler::Stopped:
      return;
    default:
      break;
  }

  if (m_stopping) return;

  thread_local ZmSemaphore stopping;

  m_stopping = &stopping;
  m_drain = drain;

  rxInvoke(ZmFn<>([](ZiMultiplex *self) { self->stop_1(); }, this));

  stopping.wait();
  wake();

  stop_3();

  m_stopping = 0;
}

void ZiMultiplex::stop_1()
{
  if (!m_cxns.count()) { stop_2(); return; }

  CxnHash::ReadIterator i(m_cxns);
  while (ZmRef<ZiConnection> cxn = i.iterateKey())
    cxn->disconnect();
}

void ZiMultiplex::stop_2()
{
#ifdef ZiMultiplex_EPoll
  {
    ConnectHash::Iterator i(m_connects);
    while (ZmRef<Connect> connect = i.iterate()) {
      i.del();
      ::close(connect->info().socket);
    }
  }
#endif

  {
    ListenerHash::Iterator i(m_listeners);
    while (ZmRef<Listener> listener = i.iterate()) {
      i.del();
      listener->down();
      ZiPlatform::closeSocket(listener->info().socket);
    }
  }

  // drain any I/O completions
#ifdef ZiMultiplex_IOCP
  {
    DWORD len;
    ULONG_PTR key;
    Zi_Overlapped *overlapped;
    while (GetQueuedCompletionStatus(
	  m_completionPort, &len, &key, (OVERLAPPED **)&overlapped, 0)) {
      if (overlapped) overlapped->complete(Zi::OK, len, ZeOK);
    }
  }
#endif

  if (m_stopping) // paranoia
    m_stopping->post();
}

void ZiMultiplex::stop_3()
{
  // stop scheduler
  if (m_drain) ZmScheduler::drain();
  ZmScheduler::stop();

  m_stopping = 0;
  m_drain = false;

  // close down underlying I/O platform

#ifdef ZiMultiplex_IOCP
  CloseHandle(m_completionPort);
  m_completionPort = ZiPlatform::nullHandle();

  WSACleanup();	// this is reference counted
#endif

#ifdef ZiMultiplex_EPoll
  if (m_wakeFD >= 0) { ::close(m_wakeFD); m_wakeFD = -1; }
  if (m_wakeFD2 >= 0) { ::close(m_wakeFD2); m_wakeFD2 = -1; }
  if (m_epollFD >= 0) { ::close(m_epollFD); m_epollFD = -1; }
#endif
}

void ZiMultiplex::threadInit(unsigned tid) { }

void ZiMultiplex::threadFinal(unsigned tid) { }

void ZiMultiplex::rxStart()
{
  m_rxTID = ZmPlatform::getTID();
  rx();
}

void ZiMultiplex::rx()
{
#ifdef ZiMultiplex_IOCP
  DWORD len;
  ULONG_PTR key;
  Zi_Overlapped *overlapped;
  ZeError e;

  for (;;) {
    ZiDEBUG(this, ZtSprintf(
	  "wait() nThreads: % 2d nConnections: % 4d nListeners: % 3d",
	  nThreads(), m_cxns.count(), m_listeners.count()));
    if (!GetQueuedCompletionStatus(
	  m_completionPort, &len, &key, (OVERLAPPED **)&overlapped, INFINITE)) {
      e = GetLastError();
      ZiDEBUG(this, ZtSprintf("wait() overlapped: %p errNo: %d",
	    (void *)overlapped, (int)e.errNo()));
      if (!overlapped) {
	Error("GetQueuedCompletionStatus", Zi::IOError, e);
	return;
      }
      overlapped->complete(Zi::IOError, 0, e);
    } else {
      ZiDEBUG(this, ZtSprintf("wait() overlapped: %p", (void *)overlapped));
      if (!overlapped) { // PostQueuedCompletionStatus() called
	ZiDEBUG(this, "wait() woken by PostQueuedCompletionStatus()");
	return;
      }
      overlapped->complete(Zi::OK, len, ZeOK);
    }
  }
#endif

#ifdef ZiMultiplex_EPoll
  bool wake = false;

  int r;
  ZeError e;
  struct epoll_event *ev =
    (struct epoll_event *)alloca(m_epollQuantum * sizeof(struct epoll_event));

  for (;;) {
    ZiDEBUG(this, ZtSprintf(
	  "wait() nThreads: % 2d nConnections: % 4d epollFD: % 3d "
	  "wakeFD: % 3d wakeFD2: % 3d nListeners: % 3d",
	  nThreads(), m_cxns.count(), m_epollFD, m_wakeFD, m_wakeFD2,
	  m_listeners.count()));

#if 0
#ifdef ZiMultiplex_DEBUG
    ZmTime now(ZmTime::Now);
#endif
#endif

    r = epoll_wait(m_epollFD, ev, m_epollQuantum, -1);

#if 0
#ifdef ZiMultiplex_DEBUG
    now = ZmTimeNow() - now;
    if (now.dtime() > .001) {
      ZeLOG(Info, ZtSprintf("slow epoll_wait(): %.6f", now.dtime()));
    }
#endif
#endif

    if (r < 0) {
      e = errno;
      if (e.errNo() == EINTR || e.errNo() == EAGAIN) continue;
      Error("epoll_wait", Zi::IOError, e);
      return;
    }

    if (ZuLikely(r)) {
      for (unsigned i = 0; i < (unsigned)r; i++) {
	uint32_t events = ev[i].events;
	uintptr_t v = ev[i].data.u64;

	if (ZuLikely(!(v & 3))) {
	  ZiConnection *cxn = (ZiConnection *)v;
	  if (events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR))
	    if (ZuUnlikely(!cxn->recv())) continue;
	  if (events & EPOLLOUT) this->run(m_txThread,
	      ZmFn<>::Member<static_cast<void (ZiConnection::*)()>(
		&ZiConnection::send)>::fn(cxn));
	  continue;
	}

	if (ZuLikely(v == 3)) { wake = readWake(); continue; }

	if (ZuLikely((v & 3) == 1)) {
	  Listener *listener = (Listener *)(v & ~(uintptr_t)3);
	  if (ZuLikely(!(events & EPOLLERR))) {
	    accept(listener);
	    continue;
	  }
	  int n;
	  int s = listener->info().socket;
	  if (ioctl(s, FIONREAD, &n) < 0) {
	    e = errno;
	    listenerDel(s);
	    ::close(s);
	    Error("listen", Zi::IOError, e);
	  }
	  continue;
	}
	  
	if (ZuLikely((v & 3) == 2)) {
	  ZmRef<Connect> request = (Connect *)(v & ~(uintptr_t)3);
	  if (ZuLikely(!(events & EPOLLERR))) {
	    connect(request);
	    continue;
	  }
	  int n;
	  int s = request->info().socket;
	  if (ioctl(s, FIONREAD, &n) < 0)
	    e = errno;
	  else {
	    int errNo = EIO;
	    socklen_t l = sizeof(int);
	    if (getsockopt(s, SOL_SOCKET, SO_ERROR, &errNo, &l) < 0)
	      errNo = EIO;
	    e = errNo;
	  }
	  connectDel(s);
	  ::close(s);
	  Error("connect", Zi::IOError, e);
	  request->fail(true);
	  continue;
	}
      }
    }
    
    if (wake) return;
  }
#endif
}

void ZiMultiplex::wake()
{
  ZiDEBUG(this, "wake");

#ifdef ZiMultiplex_IOCP
  PostQueuedCompletionStatus(m_completionPort, 0, 0, 0);
#endif

#ifdef ZiMultiplex_EPoll
  writeWake();
#endif
}

#ifdef ZiMultiplex_EPoll
bool ZiMultiplex::readWake()
{
  ZiDEBUG(this, ZtSprintf("FD: % 3d readWake", m_wakeFD));

  char c;
  return ::read(m_wakeFD, &c, 1) >= 1;
}

void ZiMultiplex::writeWake()
{
  ZiDEBUG(this, ZtSprintf("FD: % 3d writeWake", m_wakeFD2));

  char c = 0;
  while (::write(m_wakeFD2, &c, 1) < 0) {
    ZeError e(errno);
    if (e.errNo() != EINTR && e.errNo() != EAGAIN) {
      Error("write", Zi::IOError, e);
      break;
    }
  }
}
#endif
