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

// socket I/O multiplexing

#ifndef ZiMultiplex_HPP
#define ZiMultiplex_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <zlib/ZiLib.hpp>
#endif

#include <math.h>

#include <zlib/ZuCmp.hpp>
#include <zlib/ZuIndex.hpp>
#include <zlib/ZuHash.hpp>
#include <zlib/ZuLargest.hpp>
#include <zlib/ZuArrayN.hpp>

#include <zlib/ZmAtomic.hpp>
#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmScheduler.hpp>
#include <zlib/ZmHash.hpp>
#include <zlib/ZmLHash.hpp>
#include <zlib/ZmDRing.hpp>
#include <zlib/ZmFn.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmPolymorph.hpp>

#include <zlib/ZtEnum.hpp>

#include <zlib/ZePlatform.hpp>
#include <zlib/ZeLog.hpp>

#include <zlib/ZiPlatform.hpp>
#include <zlib/ZiIP.hpp>

#if defined(ZDEBUG) && !defined(ZiMultiplex_DEBUG)
#define ZiMultiplex_DEBUG	// enable testing / debugging
#include <zlib/ZmBackTracer.hpp>
#endif

#ifdef _WIN32
#define ZiMultiplex_IOCP	// Windows I/O completion ports
#endif

#ifdef linux
#define ZiMultiplex_EPoll	// Linux epoll
#endif

#ifdef NETLINK
#define ZiMultiplex_Netlink	// netlink
#endif

#ifdef ZiMultiplex_DEBUG
#define ZiDEBUG(mx, e) do { if ((mx)->debug()) ZeLOG(Debug, (e)); } while (0)
#else
#define ZiDEBUG(mx, e) ((void)0)
#endif

#ifdef ZiMultiplex_IOCP
#define ZiMultiplex__AcceptHeap 1
#define ZiMultiplex__ConnectHash 0
#endif

#ifdef ZiMultiplex_EPoll
#define ZiMultiplex__AcceptHeap 0
#define ZiMultiplex__ConnectHash 1
#endif

class ZiConnection;
class ZiMultiplex;

class ZiCxnOptions;
struct ZiCxnInfo;

// legacy compatibility
#define ZiOptions ZiCxnOptions
#define ZiConnectionInfo ZiCxnInfo

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251 4244 4800)
#endif

struct ZiIOContext {
  ZuInline ZiIOContext() : ptr(0), size(0), offset(0), length(0) { }

friend ZiConnection;
private:
  // initialize (called from within send/recv)
  template <typename Fn>
  ZuInline void init_(Fn &&fn_) {
    fn = ZuFwd<Fn>(fn_); ptr = 0; size = offset = length = 0; (*this)();
  }
public:
  // send/receive
  template <typename Fn>
  ZuInline void init(Fn &&fn_,
      void *ptr_, unsigned size_, unsigned offset_) {
    ZmAssert(size_);
    fn = ZuFwd<Fn>(fn_); ptr = ptr_; size = size_; offset = offset_; length = 0;
  }
  // UDP send
  template <typename Fn, typename Addr>
  ZuInline void init(Fn &&fn_,
      void *ptr_, unsigned size_, unsigned offset_, Addr &&addr_) {
    ZmAssert(size_);
    fn = ZuFwd<Fn>(fn_); ptr = ptr_; size = size_; offset = offset_; length = 0;
    addr = ZuFwd<Addr>(addr_);
  }
  // initially, ptr will be null and app must set it via init()
  ZuInline bool initialized() { return ptr; }

  // complete send/receive without disconnecting
  ZuInline void complete() {
    fn = ZmAnyFn();
    ptr = 0;
  }
  ZuInline bool completed() const { return !fn; }

  // complete send/receive and disconnect
  ZuInline void disconnect() {
    fn = ZmAnyFn();
    ptr = (void *)(uintptr_t)-1;
  }
  ZuInline bool disconnected() const { return ptr == (void *)(uintptr_t)-1; }

  uintptr_t operator()();

  ZmAnyFn	fn;	// callback to app - set by app (null to complete I/O)
  void		*ptr;	// pointer to buffer - set by app (0 to disconnect)
  unsigned	size;	// size of buffer - set by app
  unsigned	offset;	// offset within buffer - set by app
  unsigned	length;	// length - set by ZiMultiplex
  ZiSockAddr	addr;	// address - set by app (send) / ZiMultiplex (recv)
  ZiConnection	*cxn;	// connection - set by ZiMultiplex
};
using ZiIOFn = ZmFn<ZiIOContext &>;
ZuInline uintptr_t ZiIOContext::operator ()() { return fn.as<ZiIOFn>()(*this); }

// transient
using ZiFailFn = ZmFn<bool>;

// multicast subscription request (IGMP Report)
class ZiMReq : public ip_mreq {
public:
  ZuInline ZiMReq() {
    new (&imr_multiaddr) ZiIP();
    new (&imr_interface) ZiIP();
  }
  ZuInline ZiMReq(const ZiIP &addr, const ZiIP &mif) {
    new (&imr_multiaddr) ZiIP(addr);
    new (&imr_interface) ZiIP(mif);
  }

  ZuInline ZiMReq(const ZiMReq &m) {
    addr() = m.addr(), mif() = m.mif();
  }
  ZuInline ZiMReq &operator =(const ZiMReq &m) {
    if (this != &m) 
      addr() = m.addr(), mif() = m.mif();
    return *this;
  }

  ZuInline explicit ZiMReq(const struct ip_mreq &m) {
    addr() = m.imr_multiaddr, mif() = m.imr_interface;
  }
  ZuInline ZiMReq &operator =(const struct ip_mreq &m) {
    if ((const struct ip_mreq *)this != &m) 
      addr() = m.imr_multiaddr, mif() = m.imr_interface;
    return *this;
  }

  ZuInline int cmp(const ZiMReq &m) const {
    int r;
    if (r = addr().cmp(m.addr())) return r;
    return mif().cmp(m.mif());
  }
  ZuInline bool less(const ZiMReq &m) const {
    return addr() <= m.addr() && mif() < m.mif();
  }
  ZuInline bool equals(const ZiMReq &m) const {
    return addr() == m.addr() && mif() == m.mif();
  }
  ZuInline bool operator ==(const ZiMReq &m) const { return equals(m); }
  ZuInline bool operator !=(const ZiMReq &m) const { return !equals(m); }
  ZuInline bool operator >(const ZiMReq &m) const { return m.less(*this); }
  ZuInline bool operator >=(const ZiMReq &m) const { return !less(m); }
  ZuInline bool operator <(const ZiMReq &m) const { return less(m); }
  ZuInline bool operator <=(const ZiMReq &m) const { return !m.less(*this); }

  ZuInline bool operator !() const { return !addr() && !mif(); }
  ZuOpBool

  ZuInline uint32_t hash() const {
    return addr().hash() ^ mif().hash();
  }

  template <typename S> void print(S &s) const {
    s << addr() << "->" << mif();
  }

  ZuInline const ZiIP &addr() const { return *(const ZiIP *)&imr_multiaddr; }
  ZuInline ZiIP &addr() { return *(ZiIP *)&imr_multiaddr; }
  ZuInline const ZiIP &mif() const { return *(const ZiIP *)&imr_interface; }
  ZuInline ZiIP &mif() { return *(ZiIP *)&imr_interface; }
};
template <> struct ZuPrint<ZiMReq> : public ZuPrintFn { };
template <> struct ZuTraits<ZiMReq> : public ZuGenericTraits<ZiMReq> {
  enum { IsPOD = 1, IsComparable = 1, IsHashable = 1 };
};

#ifndef ZiCxnOptions_NMReq
#define ZiCxnOptions_NMReq 1
#endif

// protocol/socket options
namespace ZiCxnFlags {
  ZtEnumValues(
    UDP = 0,		// U - create UDP socket (default TCP)
    Multicast,          // M - combine with U for multicast server socket
    LoopBack,		// L - combine with M and U for multicast loopback
    KeepAlive,		// K - set SO_KEEPALIVE socket option
    NetLink,		// N - NetLink socket
    Nagle		// D - enable Nagle algorithm (no TCP_NODELAY)
  );
  ZtEnumNames(
    "UDP", "Multicast", "LoopBack", "KeepAlive", "NetLink", "Nagle");
  ZtEnumFlags(Flags,
      "U", UDP, "M", Multicast, "L", LoopBack, "L", KeepAlive, "N", NetLink,
      "D", Nagle);
}
class ZiCxnOptions {
  using MReqs = ZuArrayN<ZiMReq, ZiCxnOptions_NMReq>;
#ifdef ZiMultiplex_Netlink
  using FamilyName = ZuStringN<GENL_NAMSIZ>;
#endif

public:
  ZuInline ZiCxnOptions() : m_flags(0), m_ttl(0) { } // Default is TCP

  ZuInline ZiCxnOptions(const ZiCxnOptions &o) :
      m_flags(o.m_flags),
      m_mreqs(o.m_mreqs),
      m_mif(o.m_mif),
      m_ttl(o.m_ttl)
#ifdef ZiMultiplex_Netlink
      , m_familyName(o.m_familyName)
#endif
      { }

  ZuInline ZiCxnOptions(ZiCxnOptions &&o) :
      m_flags(ZuMv(o.m_flags)),
      m_mreqs(ZuMv(o.m_mreqs)),
      m_mif(ZuMv(o.m_mif)),
      m_ttl(ZuMv(o.m_ttl))
#ifdef ZiMultiplex_Netlink
      , m_familyName(ZuMv(o.m_familyName))
#endif
      { }

  ZuInline ZiCxnOptions &operator =(const ZiCxnOptions &o) {
    if (this == &o) return *this;
    m_flags = o.m_flags;
    m_mreqs = o.m_mreqs;
    m_mif = o.m_mif;
    m_ttl = o.m_ttl;
#ifdef ZiMultiplex_Netlink
    m_familyName = o.m_familyName;
#endif
    return *this;
  }

  ZuInline ZiCxnOptions &operator =(ZiCxnOptions &&o) {
    m_flags = ZuMv(o.m_flags);
    m_mreqs = ZuMv(o.m_mreqs);
    m_mif = ZuMv(o.m_mif);
    m_ttl = ZuMv(o.m_ttl);
#ifdef ZiMultiplex_Netlink
    m_familyName = ZuMv(o.m_familyName);
#endif
    return *this;
  }

  ZuInline uint32_t flags() const { return m_flags; }
  ZuInline ZiCxnOptions &flags(uint32_t flags) {
    m_flags = flags;
    return *this;
  }
  ZuInline bool udp() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<UDP);
  }
  ZuInline ZiCxnOptions &udp(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<UDP)) : (m_flags &= ~(1<<UDP));
    return *this;
  }
  ZuInline bool multicast() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<Multicast);
  }
  ZuInline ZiCxnOptions &multicast(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<Multicast)) : (m_flags &= ~(1<<Multicast));
    return *this;
  }
  ZuInline bool loopBack() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<LoopBack);
  }
  ZuInline ZiCxnOptions &loopBack(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<LoopBack)) : (m_flags &= ~(1<<LoopBack));
    return *this;
  }
  ZuInline bool keepAlive() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<KeepAlive);
  }
  ZuInline ZiCxnOptions &keepAlive(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<KeepAlive)) : (m_flags &= ~(1<<KeepAlive));
    return *this;
  }
  ZuInline const MReqs &mreqs() const {
    return m_mreqs;
  }
  ZuInline void mreq(const ZiMReq &mreq) { m_mreqs.push(mreq); }
  ZuInline const ZiIP &mif() const { return m_mif; }
  ZuInline ZiCxnOptions &mif(ZiIP ip) {
    m_mif = ip;
    return *this;
  }
  ZuInline const unsigned &ttl() const { return m_ttl; }
  ZuInline ZiCxnOptions &ttl(unsigned i) {
    m_ttl = i;
    return *this;
  }
#ifdef ZiMultiplex_Netlink
  ZuInline bool netlink() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<NetLink);
  }
  ZuInline ZiCxnOptions &netlink(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<NetLink)) : (m_flags &= ~(1<<NetLink));
    return *this;
  }
  ZuInline const ZuStringN &familyName() const { return m_familyName; }
  ZuInline ZiCxnOptions &familyName(ZuString s) {
    m_familyName = s;
    return *this;
  }
#else
  ZuInline bool netlink() const { return false; }
  ZuInline ZiCxnOptions &netlink(bool) { return *this; }
  ZuInline ZuString familyName() const { return ZuString{}; }
  ZuInline ZiCxnOptions &familyName(ZuString) { return *this; }
#endif
  ZuInline bool nagle() const {
    using namespace ZiCxnFlags;
    return m_flags & (1<<Nagle);
  }
  ZuInline ZiCxnOptions &nagle(bool b) {
    using namespace ZiCxnFlags;
    b ? (m_flags |= (1<<Nagle)) : (m_flags &= ~(1<<Nagle));
    return *this;
  }

  int cmp(const ZiCxnOptions &o) const {
    using namespace ZiCxnFlags;
    int i;
    if (i = ZuCmp<uint32_t>::cmp(m_flags, o.m_flags)) return i;
#ifdef ZiMultiplex_Netlink
    if ((m_flags & (1<<NetLink))) return m_familyName.cmp(o.m_familyName);
#endif
    if (!(m_flags & (1<<Multicast))) return i;
    if (i = m_mreqs.cmp(o.m_mreqs)) return i;
    if (i = m_mif.cmp(o.m_mif)) return i;
    return ZuBoxed(m_ttl).cmp(o.m_ttl);
  }
  ZuInline bool less(const ZiCxnOptions &o) const {
    return cmp(o) < 0;
  }
  ZuInline bool equals(const ZiCxnOptions &o) const {
    using namespace ZiCxnFlags;
    if (m_flags != o.m_flags) return false;
#ifdef ZiMultiplex_Netlink
    if ((m_flags & (1<<NetLink))) return m_familyName == o.m_familyName;
#endif
    if (!(m_flags & (1<<Multicast))) return true;
    return m_mreqs == o.m_mreqs && m_mif == o.m_mif && m_ttl == o.m_ttl;
  }

  uint32_t hash() const {
    using namespace ZiCxnFlags;
    uint32_t code = ZuHash<uint32_t>::hash(m_flags);
#ifdef ZiMultiplex_Netlink
    if (m_flags & (1<<NetLink)) return code ^ m_familyName.hash();
#endif
    if (!(m_flags & (1<<Multicast))) return code;
    return code ^ m_mreqs.hash() ^ m_mif.hash() ^ ZuBoxed(m_ttl).hash();
  }

  ZuInline bool operator ==(const ZiCxnOptions &o) const { return equals(o); }
  ZuInline bool operator !=(const ZiCxnOptions &o) const { return !equals(o); }

  template <typename S> void print(S &s) const {
    using namespace ZiCxnFlags;
    s << "flags=" << Flags::instance()->print(s, m_flags);
    if (m_flags & (1<<Multicast)) {
      s << " mreqs={";
      for (unsigned i = 0; i < m_mreqs.length(); i++) {
	if (i) s << ',';
	s << m_mreqs[i];
      }
      s << "} mif=" << m_mif << " TTL=" << ZuBoxed(m_ttl);
    }
#ifdef ZiMultiplex_Netlink
    if (m_flags & (1<<NetLink)) s << " familyName=" << m_familyName;
#endif
  }

private:
  uint32_t		m_flags;
  MReqs			m_mreqs;
  ZiIP			m_mif;
  unsigned		m_ttl;
#ifdef ZiMultiplex_Netlink
  FamilyName		m_familyName; // Generic Netlink Family Name
#endif
};
template <> struct ZuPrint<ZiCxnOptions> : public ZuPrintFn { };

// listener info (socket, accept queue size, local IP/port, options)
struct ZiListenInfo {
  template <typename S> void print(S &s) const {
    s << "socket=" << ZuBoxed(socket) <<
      " nAccepts=" << nAccepts <<
      " options={" << options <<
      "} localAddr=" << ip << ':' << port;
  }

  ZiPlatform::Socket	socket;
  ZuBox0(unsigned)	nAccepts;
  ZiIP			ip;
  ZuBox0(uint16_t)	port;
  ZiCxnOptions		options;
};
template <> struct ZuPrint<ZiListenInfo> : public ZuPrintFn { };

// cxn information (direction, socket, local & remote IP/port, options)
namespace ZiCxnType {
  ZtEnumValues(TCPIn, TCPOut, UDP);
  ZtEnumNames("TCPIn", "TCPOut", "UDP");
}

struct ZiCxnInfo { // pure aggregate, no ctor
  ZuInline bool operator !() const { return !*type; }
  ZuOpBool

  template <typename S> void print(S &s) const {
    s << "type=" << ZiCxnType::name(type) <<
      " socket=" << ZuBoxed(socket) <<
      " options={" << options << "} ";
    if (!options.netlink()) {
      s << "localAddr=" << localIP << ':' << localPort <<
	" remoteAddr=" << remoteIP << ':' << remotePort;
    } else {
      s << "familyID=" << familyID;
      if (familyID) s << " portID=" << portID;
    }
  }

  ZtEnum		type;		// ZiCxnType
  ZiPlatform::Socket	socket;
  ZiCxnOptions 		options;
  ZiIP			localIP;
  uint16_t		localPort = 0;
  ZiIP			remoteIP;
  uint16_t		remotePort = 0;
  uint32_t		familyID = 0; // non-zero for connected netlink sockets
  uint32_t		portID = 0; // only valid when familyID is valid
};
template <> struct ZuPrint<ZiCxnInfo> : public ZuPrintFn { };

// display sequence:
//   mxID, type, remoteIP, remotePort, localIP, localPort,
//   socket, flags, mreqAddr, mreqIf, mif, ttl,
//   rxBufSize, rxBufLen, txBufSize, txBufLen
struct ZiCxnTelemetry {
  ZuID		mxID;		// multiplexer ID
  uint64_t	socket = 0;	// Unix file descriptor / Winsock SOCKET
  uint32_t	rxBufSize = 0;	// graphable - getsockopt(..., SO_RCVBUF, ...)
  uint32_t	rxBufLen = 0;	// graphable (*) - ioctl(..., SIOCINQ, ...)
  uint32_t	txBufSize = 0;	// graphable - getsockopt(..., SO_SNDBUF, ...)
  uint32_t	txBufLen = 0;	// graphable (*) - ioctl(..., SIOCOUTQ, ...)
  ZiIP		mreqAddr;	// mreqs[0]
  ZiIP		mreqIf;		// mreqs[0]
  ZiIP		mif;
  uint32_t	ttl = 0;
  ZiIP		localIP;	// primary key
  ZiIP		remoteIP;	// primary key
  uint16_t	localPort = 0;	// primary key
  uint16_t	remotePort = 0;	// primary key
  uint8_t	flags = 0;	// ZiCxnFlags
  int8_t	type = -1;	// ZiCxnType
};

using ZiListenFn = ZmFn<const ZiListenInfo &>;
using ZiConnectFn = ZmFn<const ZiCxnInfo &>;

#ifdef ZiMultiplex_IOCP
// overlapped I/O structure for a single request (Windows IOCP) - internal
class Zi_Overlapped {
public:
  using Executed = ZmFn<int, unsigned, ZeError>;

  ZuInline Zi_Overlapped() { }
  ZuInline ~Zi_Overlapped() { }

  template <typename Executed>
  ZuInline void init(Executed &&executed) {
    memset(&m_wsaOverlapped, 0, sizeof(WSAOVERLAPPED));
    m_executed = ZuFwd<Executed>(executed);
  }

  void complete(int status, unsigned len, ZeError e) {
    m_executed(status, len, e); // Note: may destroy this object
  }

private:
  WSAOVERLAPPED		m_wsaOverlapped;
  Executed		m_executed;
};
#endif

// cxn class - must be derived from and instantiated by caller
// when listen() or connect() completion is called with an OK status;
// derived class must supply connected() and disconnected() functions
// (and probably a destructor)
class ZiAPI ZiConnection : public ZmPolymorph {
  ZiConnection(const ZiConnection &);
  ZiConnection &operator =(const ZiConnection &);	// prevent mis-use

friend ZiMultiplex;

public:
  using Socket = ZiPlatform::Socket;

  // index on socket
  struct SocketAccessor;
friend SocketAccessor;
  struct SocketAccessor : public ZuAccessor<ZiConnection *, Socket> {
    ZuInline static Socket value(const ZiConnection *c) {
      return c->info().socket;
    }
  };

protected:
  ZiConnection(ZiMultiplex *mx, const ZiCxnInfo &ci);

public:
  virtual ~ZiConnection();

  void recv(ZiIOFn fn);
  void recv_(ZiIOFn fn);	// direct call from within rx thread

  // send
  void send(ZiIOFn fn);
  void send_(ZiIOFn fn);	// direct call from within tx thread

  // graceful disconnect (socket shutdown); then socket close
  void disconnect();

  // close abruptly without socket shutdown
  void close();

  virtual void connected(ZiIOContext &rxContext) = 0;
  virtual void disconnected() = 0;

  ZuInline bool up() const {
    return m_rxUp && m_txUp; // unclean read
  }

  ZuInline ZiMultiplex *mx() const { return m_mx; }
  ZuInline const ZiCxnInfo &info() const { return m_info; }

  void telemetry(ZiCxnTelemetry &data) const;

private:
  void connected();

#ifdef ZiMultiplex_EPoll
  bool recv();
#else
  void recv();
#endif
#ifdef ZiMultiplex_IOCP
  void overlappedRecv(int status, unsigned n, ZeError e);
#endif
  void errorRecv(int status, ZeError e);
  void executedRecv(unsigned n);

  void send();
  void errorSend(int status, ZeError e);
  void executedSend(unsigned n);

  void disconnect_1();
  void disconnect_2();
  void close_1();
  void close_2();
#ifdef ZiMultiplex_IOCP
  void overlappedDisconnect(int status, unsigned n, ZeError e);
#endif
  void errorDisconnect(int status, ZeError e);
  void executedDisconnect();

  ZiMultiplex			*m_mx;
  ZiCxnInfo			m_info;

#ifdef ZiMultiplex_IOCP
  Zi_Overlapped		 	 m_discOverlapped;
#endif

  // Rx thread exclusive
  unsigned			m_rxUp;
  uint64_t			m_rxRequests;
  uint64_t			m_rxBytes;
  ZiIOContext			m_rxContext;
#ifdef ZiMultiplex_IOCP
  Zi_Overlapped			m_rxOverlapped;
  DWORD				m_rxFlags;		// flags for WSARecv()
#endif

  // Tx thread exclusive
  unsigned			m_txUp;
  uint64_t			m_txRequests;
  uint64_t			m_txBytes;
  ZiIOContext			m_txContext;
};

// named parameter list for configuring ZiMultiplex
class ZiMxParams {
  ZiMxParams(const ZiMxParams &) = delete;
  ZiMxParams &operator =(const ZiMxParams &) = delete;

public:
  enum { RxThread = 1, TxThread = 2 }; // defaults

  ZiMxParams() :
    m_scheduler(ZmSchedParams()
	.nThreads(3)
	.thread(ZiMxParams::RxThread, [](auto &t) { t.isolated(true); })
	.thread(ZiMxParams::TxThread, [](auto &t) { t.isolated(true); })) { }

  ZiMxParams(ZiMxParams &&) = default;
  ZiMxParams &operator =(ZiMxParams &&) = default;

  template <typename L>
  ZiMxParams &&scheduler(L l) { l(m_scheduler); return ZuMv(*this); }

  ZiMxParams &&rxThread(unsigned tid) {
    m_rxThread = tid;
    return ZuMv(*this);
  }
  ZiMxParams &&txThread(unsigned tid) {
    m_txThread = tid;
    return ZuMv(*this);
  }
#ifdef ZiMultiplex_EPoll
  ZiMxParams &&epollMaxFDs(unsigned n)
    { m_epollMaxFDs = n; return ZuMv(*this); }
  ZiMxParams &&epollQuantum(unsigned n)
    { m_epollQuantum = n; return ZuMv(*this); }
#endif
  ZiMxParams &&rxBufSize(unsigned v)
    { m_rxBufSize = v; return ZuMv(*this); }
  ZiMxParams &&txBufSize(unsigned v)
    { m_txBufSize = v; return ZuMv(*this); }
  ZiMxParams &&listenerHash(ZuString id)
    { m_listenerHash = id; return ZuMv(*this); }
  ZiMxParams &&requestHash(ZuString id)
    { m_requestHash = id; return ZuMv(*this); }
  ZiMxParams &&cxnHash(ZuString id)
    { m_cxnHash = id; return ZuMv(*this); }
#ifdef ZiMultiplex_DEBUG
  ZiMxParams &&trace(bool b) { m_trace = b; return ZuMv(*this); }
  ZiMxParams &&debug(bool b) { m_debug = b; return ZuMv(*this); }
  ZiMxParams &&frag(bool b) { m_frag = b; return ZuMv(*this); }
  ZiMxParams &&yield(bool b) { m_yield = b; return ZuMv(*this); }
#endif

  ZmSchedParams &scheduler() { return m_scheduler; }

  unsigned rxThread() const { return m_rxThread; }
  unsigned txThread() const { return m_txThread; }
#ifdef ZiMultiplex_EPoll
  unsigned epollMaxFDs() const { return m_epollMaxFDs; }
  unsigned epollQuantum() const { return m_epollQuantum; }
#endif
  unsigned rxBufSize() const { return m_rxBufSize; }
  unsigned txBufSize() const { return m_txBufSize; }
  ZuString listenerHash() const { return m_listenerHash; }
  ZuString requestHash() const { return m_requestHash; }
  ZuString cxnHash() const { return m_cxnHash; }
#ifdef ZiMultiplex_DEBUG
  bool trace() const { return m_trace; }
  bool debug() const { return m_debug; }
  bool frag() const { return m_frag; }
  bool yield() const { return m_yield; }
#endif

private:
  ZmSchedParams		m_scheduler;
  unsigned		m_rxThread = RxThread;
  unsigned		m_txThread = TxThread;
#ifdef ZiMultiplex_EPoll
  unsigned		m_epollMaxFDs = 256;
  unsigned		m_epollQuantum = 8;
#endif
  unsigned		m_rxBufSize = 0;
  unsigned		m_txBufSize = 0;
  const char		*m_listenerHash = "ZiMultiplex.ListenerHash";
  const char		*m_requestHash = "ZiMultiplex.RequestHash";
  const char		*m_cxnHash = "ZiMultiplex.CxnHash";
#ifdef ZiMultiplex_DEBUG
  bool			m_trace = false;
  bool			m_debug = false;
  bool			m_frag = false;
  bool			m_yield = false;
#endif
};

// display sequence:
//   id, state, nThreads, rxThread, txThread,
//   priority, stackSize, partition, rxBufSize, txBufSize,
//   queueSize, ll, spin, timeout
struct ZiMxTelemetry { // not graphable
  ZuID		id;		// primary key
  uint32_t	stackSize = 0;
  uint32_t	queueSize = 0;
  uint32_t	spin = 0;
  uint32_t	timeout = 0;
  uint32_t	rxBufSize = 0;
  uint32_t	txBufSize = 0;
  uint16_t	rxThread = 0;
  uint16_t	txThread = 0;
  uint16_t	partition = 0;
  int8_t	state = -1; // RAG: Running - Green; Stopped - Red; * - Amber
  uint8_t	ll = 0;
  uint8_t	priority = 0;
  uint8_t	nThreads = 0;
};

class ZiAPI ZiMultiplex : public ZmScheduler {
  ZiMultiplex(const ZiMultiplex &);
  ZiMultiplex &operator =(const ZiMultiplex &);	// prevent mis-use

friend ZiConnection;

  class Listener_;
#if !ZiMultiplex__AcceptHeap
  class Accept_;
#else
  template <typename> class Accept_;
#endif
#if ZiMultiplex__ConnectHash
  class Connect_;
#else
  template <typename> class Connect_;
#endif

  class Listener_ {
  friend ZiMultiplex;
#if !ZiMultiplex__AcceptHeap
  friend Accept_;
#else
  template <typename> friend class Accept_;
#endif

    using Socket = ZiPlatform::Socket;

  public:
    struct SocketAccessor;
  friend SocketAccessor;
    struct SocketAccessor : public ZuAccessor<Listener_ *, Socket> {
      ZuInline static Socket value(const Listener_ &l) {
	return l.info().socket;
      }
    };

  protected:
    template <typename ...Args> ZuInline Listener_(ZiMultiplex *mx,
	ZiConnectFn acceptFn, Args &&... args) :
      m_mx(mx), m_acceptFn(acceptFn), m_up(1),
      m_info{ZuFwd<Args>(args)...} { }

  private:
    ZuInline const ZiConnectFn &acceptFn() const { return m_acceptFn; }
    ZuInline bool up() const { return m_up; }
    ZuInline void down() { m_up = 0; }
    ZuInline const ZiListenInfo &info() const { return m_info; }

    ZiMultiplex		*m_mx;
    ZiConnectFn		m_acceptFn;
    bool		m_up;
    ZiListenInfo	m_info;
  };
  struct Listener_HeapID : public ZmHeapSharded {
    static constexpr const char *id() { return "ZiMultiplex.Listener"; }
  };
  using ListenerHash =
    ZmHash<Listener_,
      ZmHashNodeIsKey<true,
	ZmHashIndex<Listener_::SocketAccessor,
	  ZmHashLock<ZmNoLock,
	    ZmHashObject<ZuObject,
	      ZmHashHeapID<Listener_HeapID> > > > > >;
  using Listener = ListenerHash::Node;

#if ZiMultiplex__AcceptHeap
  // heap-allocated asynchronous accept, exclusively used by IOCP
  template <typename> class Accept_;
template <typename> friend class Accept_;
  template <typename Heap> class Accept_ : public Heap {
  friend ZiMultiplex;

    Accept_(Listener *listener) : m_listener(listener), m_info{
	  ZiCxnType::TCPIn,
	  ZiPlatform::nullSocket(),
	  listener->m_info.options} {
      m_overlapped.init(
	  Zi_Overlapped::Executed::Member<&Accept_::executed>::fn(this));
    }

    ZuInline ZmRef<Listener> listener() const { return m_listener; }
    ZuInline ZiCxnInfo &info() { return m_info; }
    ZuInline Zi_Overlapped &overlapped() { return m_overlapped; }
    ZuInline void *buf() { return (void *)&m_buf[0]; }

    void executed(int status, unsigned n, ZeError e) {
      m_listener->m_mx->overlappedAccept(this, status, n, e);
      delete this;
    }

    ZmRef<Listener>	m_listener;
    ZiCxnInfo		m_info;
    Zi_Overlapped	m_overlapped;
    char		m_buf[(sizeof(struct sockaddr_in) + 16) * 2];
  };
  struct Accept_HeapID {
    static constexpr const char *id() { return "ZiMultiplex.Accept"; }
  };
  using Accept_Heap = ZmHeap<Accept_HeapID, sizeof(Accept_<ZuNull>)>;
  using Accept = Accept_<Accept_Heap>; 
#endif

  // heap-allocated non-blocking / asynchronous connect
#if ZiMultiplex__ConnectHash
  class Connect_
#else
  template <typename> class Connect_;
template <typename> friend class Connect_;
  template <typename Heap> class Connect_ : public Heap
#endif
  {
  friend ZiMultiplex;

    using Socket = ZiPlatform::Socket;

#ifdef ZiMultiplex__ConnectHash
  public:
    struct SocketAccessor;
  friend SocketAccessor;
    struct SocketAccessor : public ZuAccessor<Connect_ *, Socket> {
      ZuInline static Socket value(const Connect_ &c) {
	return c.info().socket;
      }
    };
#endif

  protected:
    template <typename ...Args> ZuInline Connect_(
	ZiMultiplex *mx, ZiConnectFn fn, ZiFailFn failFn, Args &&... args) :
      m_mx(mx), m_fn(fn), m_failFn(failFn), m_info{ZuFwd<Args>(args)...} {
#ifdef ZiMultiplex_IOCP
      m_overlapped.init(
	  Zi_Overlapped::Executed::Member<&Connect_::executed>::fn(this));
#endif
    }

  private:
    ZuInline void fail(bool transient) { m_failFn(transient); }

    ZuInline const ZiConnectFn &fn() const { return m_fn; }
    ZuInline const ZiCxnInfo &info() const { return m_info; }
    ZuInline ZiCxnInfo &info() { return m_info; }

#ifdef ZiMultiplex_IOCP
    ZuInline Zi_Overlapped &overlapped() { return m_overlapped; }

    ZuInline void executed(int status, unsigned n, ZeError e) {
      m_mx->overlappedConnect(this, status, n, e);
      delete this;
    }
#endif

    ZiMultiplex		*m_mx;
    ZiConnectFn		m_fn;
    ZiFailFn		m_failFn;
    ZiCxnInfo		m_info;
#ifdef ZiMultiplex_IOCP
    Zi_Overlapped	m_overlapped;
#endif
  };
  struct Connect_HeapID : public ZmHeapSharded {
    static constexpr const char *id() { return "ZiMultiplex.Connect"; }
  };
#if ZiMultiplex__ConnectHash
  using ConnectHash =
    ZmHash<Connect_,
      ZmHashNodeIsKey<true,
	ZmHashIndex<Connect_::SocketAccessor,
	  ZmHashLock<ZmNoLock,
	    ZmHashObject<ZuObject,
	      ZmHashHeapID<Connect_HeapID> > > > > >;
  using Connect = ConnectHash::Node;
#else
  using ConnectHeap = ZmHeap<Connect_HeapID, sizeof(Connect_<ZuNull>)>;
  using Connect = Connect_<ConnectHeap>;
#endif

  struct CxnHash_HeapID : public ZmHeapSharded {
    static constexpr const char *id() { return "ZiMultiplex.CxnHash"; }
  };
  using CxnHash =
    ZmHash<ZmRef<ZiConnection>,
      ZmHashIndex<ZiConnection::SocketAccessor,
	ZmHashLock<ZmNoLock,
	  ZmHashObject<ZuNull,
	    ZmHashHeapID<CxnHash_HeapID> > > > >;

  using StateLock = ZmLock;
  using StateGuard = ZmGuard<StateLock>;
  using ShutdownCond = ZmCondition<StateLock>;

public:
  using Socket = ZiPlatform::Socket;

  ZiMultiplex(ZiMxParams mxParams = ZiMxParams());
  ~ZiMultiplex();

  int start();
  void stop(bool drain);
private:
  void stop_1();	// Rx thread - disconnect all connections
  void stop_2();	// Rx thread - stop connecting / listening / accepting
  void stop_3();	// App thread - clean up

public:
  void allCxns(ZmFn<ZiConnection *> fn);
  void allCxns_(ZmFn<ZiConnection *> fn);			// Rx thread

  void listen(
      ZiListenFn listenFn, ZiFailFn failFn, ZiConnectFn acceptFn,
      ZiIP localIP, uint16_t localPort, unsigned nAccepts,
      ZiCxnOptions options = ZiCxnOptions());
  void listen_(							// Rx thread
      ZiListenFn listenFn, ZiFailFn failFn, ZiConnectFn acceptFn,
      ZiIP localIP, uint16_t localPort, unsigned nAccepts,
      ZiCxnOptions options = ZiCxnOptions());
  void stopListening(ZiIP localIP, uint16_t localPort);
  void stopListening_(ZiIP localIP, uint16_t localPort);	// Rx thread

  void connect(
      ZiConnectFn fn, ZiFailFn failFn,
      ZiIP localIP, uint16_t localPort,
      ZiIP remoteIP, uint16_t remotePort,
      ZiCxnOptions options = ZiCxnOptions());
  void connect_(						// Rx thread
      ZiConnectFn fn, ZiFailFn failFn,
      ZiIP localIP, uint16_t localPort,
      ZiIP remoteIP, uint16_t remotePort,
      ZiCxnOptions options = ZiCxnOptions());

  void udp(
      ZiConnectFn fn, ZiFailFn failFn,
      ZiIP localIP, uint16_t localPort,
      ZiIP remoteIP, uint16_t remotePort,
      ZiCxnOptions options = ZiCxnOptions());
  void udp_(							// Rx thread
      ZiConnectFn fn, ZiFailFn failFn,
      ZiIP localIP, uint16_t localPort,
      ZiIP remoteIP, uint16_t remotePort,
      ZiCxnOptions options = ZiCxnOptions());

  ZuInline unsigned rxThread() const { return m_rxThread; }
  ZuInline unsigned txThread() const { return m_txThread; }

  template <typename ...Args> ZuInline void rxRun(Args &&... args) {
    run(m_rxThread, ZuFwd<Args>(args)...);
  }
  template <typename ...Args> ZuInline void rxInvoke(Args &&... args) {
    invoke(m_rxThread, ZuFwd<Args>(args)...);
  }
  template <typename ...Args> ZuInline void txRun(Args &&... args) {
    run(m_txThread, ZuFwd<Args>(args)...);
  }
  template <typename ...Args> ZuInline void txInvoke(Args &&... args) {
    invoke(m_txThread, ZuFwd<Args>(args)...);
  }

#ifdef ZiMultiplex_DEBUG
  ZuInline bool trace() const { return m_trace; }
  ZuInline void trace(bool b) { m_trace = b; }
  ZuInline bool debug() const { return m_debug; }
  ZuInline void debug(bool b) { m_debug = b; }
  ZuInline bool frag() const { return m_frag; }
  ZuInline void frag(bool b) { m_frag = b; }
  ZuInline bool yield() const { return m_yield; }
  ZuInline void yield(bool b) { m_yield = b; }
#else
  ZuInline void debugWarning(const char *fn) {
    ZeLOG(Warning,
	ZtSprintf("ZiMultiplex::%s(true) called while "
	  "ZiMultiplex_DEBUG undefined", fn));
  }
  ZuInline bool trace() const { return false; }
  ZuInline void trace(bool b) { if (b) debugWarning("debug"); }
  ZuInline bool debug() const { return false; }
  ZuInline void debug(bool b) { if (b) debugWarning("debug"); }
  ZuInline bool frag() const { return false; }
  ZuInline void frag(bool b) { if (b) debugWarning("frag"); }
  ZuInline bool yield() const { return false; }
  ZuInline void yield(bool b) { if (b) debugWarning("yield"); }
#endif

#ifdef ZiMultiplex_EPoll
  ZuInline unsigned epollMaxFDs() const { return m_epollMaxFDs; }
  ZuInline unsigned epollQuantum() const { return m_epollQuantum; }
#endif
  ZuInline unsigned rxBufSize() const { return m_rxBufSize; }
  ZuInline unsigned txBufSize() const { return m_txBufSize; }

  void telemetry(ZiMxTelemetry &data) const;

private:
  void drain();			// prevents mis-use of ZmScheduler::drain

  ZuInline void busy() { ZmScheduler::busy(); }
  ZuInline void idle() { ZmScheduler::idle(); }

  void rx();			// handle I/O completions (IOCP) or
  				// readiness notifications (epoll, ports, etc.)
  void wake();			// wake up rx(), cause it to return

#ifdef ZiMultiplex_EPoll
  void connect(Connect *);
#endif
#ifdef ZiMultiplex_IOCP
  void overlappedConnect(Connect *, int status, unsigned, ZeError e);
#endif
  void executedConnect(ZiConnectFn, const ZiCxnInfo &);

  void accept(Listener *);
#ifdef ZiMultiplex_IOCP
  void overlappedAccept(Accept *, int status, unsigned n, ZeError e);
#endif

  void disconnected(ZiConnection *cxn);

#ifdef ZiMultiplex_EPoll
  bool epollRecv(ZiConnection *, int s, uint32_t events);
#endif

  bool initSocket(Socket, const ZiCxnOptions &);

  bool cxnAdd(ZiConnection *, Socket);
  void cxnDel(Socket);

  bool listenerAdd(Listener *, Socket);
  void listenerDel(Socket);

  bool connectAdd(Connect *, Socket);
  void connectDel(Socket);

#ifdef ZiMultiplex_EPoll
  bool readWake();
  void writeWake();
#endif

  StateLock		m_stateLock;	// guards ZmScheduler state
    ZmSemaphore		  *m_stopping;
    bool		  m_drain;

  unsigned		m_rxThread;
    // Rx exclusive
    ZmRef<ListenerHash>	  m_listeners;
    unsigned		  m_nAccepts;	// total #accepts for all listeners
#if ZiMultiplex__ConnectHash
    ZmRef<ConnectHash>	  m_connects;
#endif
    ZmRef<CxnHash>	  m_cxns;		// connections

  unsigned		m_txThread;

  unsigned		m_rxBufSize;	// setsockopt SO_RCVBUF option
  unsigned		m_txBufSize;	// setsockopt SO_SNDBUF option

#ifdef ZiMultiplex_IOCP
  HANDLE		m_completionPort;
#endif

#ifdef ZiMultiplex_EPoll
  int			m_epollFD;
  unsigned		m_epollMaxFDs;
  unsigned		m_epollQuantum;
  int			m_wakeFD, m_wakeFD2;	// wake pipe
#endif

#ifdef ZiMultiplex_DEBUG
  bool			m_trace;
  bool			m_debug;
  bool			m_frag;
  bool			m_yield;

  void traceCapture() { m_tracer.capture(1); }
public:
  template <typename S> void traceDump(S &s) { m_tracer.dump(s); }
private:
  ZmBackTracer<64>	m_tracer;
#endif
};

#endif /* ZiMultiplex_HPP */
