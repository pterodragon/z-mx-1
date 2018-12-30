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

// ZpPcap - pcap interface

#ifndef ZpPcap_HPP
#define ZpPcap_HPP

#ifndef ZpLib_HPP
#include <ZpLib.hpp>
#endif

#include <pcap.h>

#include <ZmObject.hpp>
#include <ZmHash.hpp>

#include <ZtString.hpp>

#include <ZiMultiplex.hpp>

#include <ZvCf.hpp>

#include <ZpLib.hpp>

#ifdef ZDEBUG
#define ZpPcap_DEBUG
#endif

#ifdef ZpPcap_DEBUG
#define ZpDEBUG(pc, e) do { if ((pc)->debug()) ZeLOG(Debug, (e)); } while (0)
#else
#define ZpDEBUG(pc, e) ((void)0)
#endif

class ZpHandleInfo;
class ZpHandle;
class ZpPcap;

#ifdef _MSC_VER
#pragma once
#pragma warning(push)
#pragma warning(disable:4251 4800)
#endif

typedef ZuStringN<PCAP_ERRBUF_SIZE> ZpError;
// connect completion function (status, handleInfo, error)
typedef ZmFn<int, const ZpHandleInfo &, const ZpError &> ZpConnected;
// pcap completion function provided by caller
// - completion(handle, status, len, error)
typedef ZmFn<ZpHandle *, int, int, const ZpError &> ZpCompletion;
// drain function (handle, time, data, length)
typedef ZmFn<ZpHandle *, const struct timeval *, const uint8_t *, int> ZpDrain;

class Zp_Pcap : public ZmObject {
  friend class ZpPcap;
  friend class ZpHandle;
  friend class ZpHandleInfo;

  Zp_Pcap() : m_pcap(0) { memset(&m_program, 0, sizeof(bpf_program)); }

  bool create(ZuString iface, ZpError *e);
  bool activate(ZpError *e);
  bool openFile(ZuString fname, ZpError *e);
  bool compile(ZuString filter, ZpError *e, int optimize = 1);
  bool nonblock(int b, ZpError *e);
  int nonblock(ZpError *e);
  bool bufferSize(int b, ZpError *e);
  bool snaplen(int b, ZpError *e);
  bool timeout(int b, ZpError *e);
  bool promisc(int b, ZpError *e);

  void stop() { 
    pcap_breakloop(m_pcap); 
    m_pcapSem.wait();
  }

  int dispatch(u_char *userdata, int count);

  int fileno() { return pcap_fileno(m_pcap); }

  void post() { m_pcapSem.post(); }
  void finalize() { pcap_freecode(&m_program); }
  void close() { if (m_pcap) pcap_close(m_pcap); }

  int datalink();

  ZuInline bool operator !() const { return !m_pcap; }

  void print(ZmStream &s) const;
  template <typename S> inline void print(S &s_) const {
    ZmStream s(s_);
    print(s);
  }
  struct Error;
friend struct Error;
  struct Error {
    template <typename S> inline void print(S &s) const {
      if (ZuUnlikely(!p.m_pcap)) return;
      s << pcap_geterr(p.m_pcap);
    }
    const Zp_Pcap &p;
  };
  inline Error error() const { return Error{*this}; }
  struct Stats;
friend struct Stats;
  struct Stats {
    template <typename S> void print(S &s) const;
    const Zp_Pcap &p;
  };
  inline Stats stats() const { return Stats{*this}; }

private:
  pcap_t                *m_pcap; // each instance has an associated file handle
  bpf_program           m_program;
  ZmSemaphore           m_pcapSem; // used to break pcap loop on stop
};
template <> struct ZuPrint<Zp_Pcap> : public ZuPrintFn { };
template <> struct ZuPrint<Zp_Pcap::Error> : public ZuPrintFn { };
template <> struct ZuPrint<Zp_Pcap::Stats> : public ZuPrintFn { };

inline bool Zp_Pcap::create(ZuString iface, ZpError *e)
{
  m_pcap = pcap_create(iface, e ? e->data() : (char *)0);
  if (!m_pcap) {
    if (e) e->calcLength();
    return 0;
  }
  return 1;
}

inline bool Zp_Pcap::activate(ZpError *e)
{
  if (!m_pcap) {
    if (e) *e << "invalid pcap_t for activation";
    return 0;
  }
  if (pcap_activate(m_pcap)) {
    if (e) *e << error();
    return 0;
  }
  return 1;
}

inline bool Zp_Pcap::openFile(ZuString fname, ZpError *e)
{
  m_pcap = pcap_open_offline(fname, e ? e->data() : (char *)0);
  if (!m_pcap) {
    if (e) e->calcLength();
    return 0;
  }
  return 1;
}
inline bool Zp_Pcap::compile(ZuString filter, ZpError *e, int optimize)
{
  int rc = pcap_compile(m_pcap, &m_program, filter, optimize, 0);
  if (rc < 0 || pcap_setfilter(m_pcap, &m_program)) {
    if (e) *e << error();
    return 0;
  }
  return 1;
}
inline bool Zp_Pcap::nonblock(int b, ZpError *e)
{
  if (b < 0) return 1;
  if (pcap_setnonblock(m_pcap, b, e ? e->data() : (char *)0) < 0) {
    if (e) e->calcLength();
    return 0;
  }
  return 1;
}
int Zp_Pcap::nonblock(ZpError *e) {
  int b = pcap_getnonblock(m_pcap, e ? e->data() : (char *)0);
  if (b < 0 && e) e->calcLength();
  return b;
}
inline bool Zp_Pcap::bufferSize(int b, ZpError *e)
{
  if (b < 0) return 1;
  if (pcap_set_buffer_size(m_pcap, b)) {
    if (e) *e << "failed to set buffer size";
    return 0;
  }
  return 1;
}
inline bool Zp_Pcap::snaplen(int b, ZpError *e)
{
  if (b < 0) return 1;
  if (pcap_set_snaplen(m_pcap, b)) {
    if (e) *e << "failed to set snaplen";
    return 0;
  }
  return 1;
}
inline bool Zp_Pcap::timeout(int b, ZpError *e)
{
  if (b < 0) return 1;
  if (pcap_set_timeout(m_pcap, b)) {
    if (e) *e << "failed to set timeout";
    return 0;
  }
  return 1;
}
inline bool Zp_Pcap::promisc(int b, ZpError *e)
{
  if (b < 0) return 1;
  if (pcap_set_promisc(m_pcap, b)) {
    if (e) *e << "failed to set promisc";
    return 0;
  }
  return 1;
}

template <typename S> inline void Zp_Pcap::Stats::print(S &s) const
{
  struct pcap_stat st;
  memset(&st, 0, sizeof(struct pcap_stat));
  if (pcap_stats(p.m_pcap, &st)) { s << p.error(); return; }
  s << "[rcvdPkts = " << st.ps_recv
    << "] [dropPktsOS = " << st.ps_drop
    << "] [dropPktsEth = " << st.ps_ifdrop
    << "]";
}

class ZpAPI ZpHandleInfo {
  friend class ZpHandle;
  friend class ZpPcap;

private:
  inline void pcap(Zp_Pcap *pcap) { m_pcap = pcap; }
  inline ZmRef<Zp_Pcap> pcap() { return m_pcap; }

public:
  ZpHandleInfo(ZuString iface, ZuString filter,
      int priority, int promisc, int snaplen, int timeout,
      int bufferSize, bool nonBlocking, bool asFile) :
    m_pcap(0), m_iface(iface), m_filter(filter), m_priority(priority),
    m_promisc(promisc), m_snaplen(snaplen), m_timeout(timeout), 
    m_bufferSize(bufferSize), m_nonBlocking(nonBlocking), m_asFile(asFile) { }


  ZpHandleInfo() : m_pcap(0), m_priority(0), m_promisc(-1), 
    m_snaplen(-1), m_timeout(-1), m_bufferSize(-1), m_nonBlocking(false),
    m_asFile(false) { }

  inline ZpHandleInfo(const ZpHandleInfo &hi) :
    m_pcap(hi.m_pcap), m_iface(hi.m_iface), m_filter(hi.m_filter),
    m_priority(hi.m_priority), m_promisc(hi.m_promisc), 
    m_snaplen(hi.m_snaplen), m_timeout(hi.m_timeout), 
    m_bufferSize(hi.m_bufferSize), m_nonBlocking(hi.m_nonBlocking),
    m_asFile(hi.m_asFile) { }

  inline ZpHandleInfo &operator =(const ZpHandleInfo &hi) {
    if (this == &hi) return *this;
    m_pcap = hi.m_pcap;
    m_iface = hi.m_iface;
    m_filter = hi.m_filter;
    m_priority = hi.m_priority;
    m_promisc = hi.m_promisc;
    m_snaplen = hi.m_snaplen; 
    m_timeout = hi.m_timeout;
    m_bufferSize = hi.m_bufferSize;
    m_nonBlocking = hi.m_nonBlocking;
    m_asFile = hi.m_asFile;
    return *this;
  }

  inline bool operator !() const { return !m_pcap; }

  inline const ZtString &iface() const { return m_iface; }
  inline const ZtString &filter() const { return m_filter; }
  inline int priority() const { return m_priority; }
  inline int promisc() const { return m_promisc; }
  inline int snaplen() const { return m_snaplen; }
  inline int timeout() const { return m_timeout; }
  inline int bufferSize() const { return m_bufferSize; }
  inline bool nonBlocking() const { return m_nonBlocking; }
  inline bool asFile() const { return m_asFile; }

  void print(ZmStream &s) const;
  template <typename S> inline void print(S &s_) const {
    ZmStream s(s_);
    print(s);
  }

private:
  void stats_(ZmStream &s) const;
public:
  struct Stats;
friend struct Stats;
  struct Stats {
    inline void print(ZmStream &s) const { p.stats_(s); }
    template <typename S> inline void print(S &s_) const {
      ZmStream s(s_);
      p.stats_(s);
    }
    const ZpHandleInfo &p;
  };
  inline Stats stats() const { return Stats{*this}; }

private:
  ZmRef<Zp_Pcap>        m_pcap;
  ZtString              m_iface;
  ZtString              m_filter;
  int			m_priority;
  int                   m_promisc;
  int                   m_snaplen;
  int                   m_timeout;
  int			m_bufferSize;
  bool			m_nonBlocking;
  bool			m_asFile;
};
template <> struct ZuPrint<ZpHandleInfo> : public ZuPrintFn { };
template <> struct ZuPrint<ZpHandleInfo::Stats> : public ZuPrintFn { };

class ZpAPI ZpHandle : public ZmPolymorph {
  ZpHandle(const ZpHandle &);
  ZpHandle &operator =(const ZpHandle &);

  friend class ZpPcap;

  struct FilterAccessor;
  friend struct FilterAccessor;
  struct FilterAccessor : public ZuAccessor<ZpHandle *, const ZtString &> {
    inline static const ZtString &value(ZpHandle *h) {
      return h->m_info.filter();
    }
  };

public:
  struct DrainBounce {
    DrainBounce() { }

    inline void bounce(
	u_char *userarg, const pcap_pkthdr *pkthdr, const u_char *packet) {
#ifdef ZpPcap_DEBUG
      if (m_handle->m_debugDrain)
	m_handle->m_debugDrain.operator()(
	    m_handle, &pkthdr->ts, packet, pkthdr->caplen);
#else
      if (m_handle->m_drain)
	m_handle->m_drain.operator()(
	    m_handle, &pkthdr->ts, packet, pkthdr->caplen);
#endif
    }
    
    inline void handle(ZpHandle *handle) { m_handle = handle; }

  private:
    ZpHandle *m_handle;
  };

protected:
  ZpHandle(ZpPcap *pcap, const ZpHandleInfo &info);

public:
  void drain(ZpDrain d);
  // (re)start the dispatcher (if it exists)
  void start();

  // stop dispatching events
  void shutdown();
  void disconnect();

  // calls disconnect then closes pcap handle
  void close();
  // returns offset of next layer from start of captured data based on
  // data link type (eg 14 for ethernet)
  int dataLinkOffset();

  virtual void connected() = 0;
  virtual void disconnected() = 0;

  const ZpHandleInfo &info() const { return m_info; }

private:
  void stats_(ZmStream &s) const;
public:
  struct Stats;
friend struct Stats;
  struct Stats {
    inline void print(ZmStream &s) const { h.stats_(s); }
    template <typename S> inline void print(S &s_) const {
      ZmStream s(s_);
      h.stats_(s);
    }
    const ZpHandle &h;
  };
  inline Stats stats() const { return Stats{*this}; }

private:
  void dispatch();
#ifdef ZpPcap_DEBUG
  void debugDrain(ZpHandle *handle,
      const struct timeval *ts, const uint8_t *data, int len);
#endif

private:
  ZpPcap                *m_pcap;
  ZpHandleInfo          m_info;
  DrainBounce		m_bounce;
  ZpDrain               m_drain;
  ZmAtomic<unsigned>	m_started;
#ifdef ZpPcap_DEBUG
  ZpDrain               m_debugDrain;
#endif
};
template <> struct ZuPrint<ZpHandle::Stats> : public ZuPrintFn { };

class ZpAPI ZpPcap : public ZmObject {
  ZpPcap(const ZpPcap&);
  const ZpPcap &operator =(const ZpPcap&);

  friend class ZpHandle;

  typedef ZmHash<ZmRef<ZpHandle>,
	    ZmHashObject<ZuNull,
	      ZmHashIndex<ZpHandle::FilterAccessor> > > HandleHash;

  inline void del(const ZtString &c) { delete m_handles->del(c); }
  
 public:
  ZpPcap(ZiMultiplex *mx, ZvCf *cf);

  void connect(ZpConnected connected, const ZpHandleInfo &info);

  inline ZiMultiplex *mx() { return m_mx; }
  inline int maxCapture() const { return m_maxCapture; }

private:
  void stats_(ZmStream &s) const;
public:
  struct Stats;
friend struct Stats;
  struct Stats {
    template <typename S> inline void print(S &s_) const {
      ZmStream s(s_);
      p.stats_(s);
    }
    const ZpPcap &p;
  };
  inline Stats stats() const { return Stats{*this}; }

#ifdef ZpPcap_DEBUG
  inline bool debug() { return m_debug; }
  inline void debug(bool b) { m_debug = b; }
#endif

private:
  ZiMultiplex		*m_mx;
  ZmRef<HandleHash>	m_handles;
  int			m_maxCapture; // max packets captured in dispatch
#ifdef ZpPcap_DEBUG
  bool			m_debug;
#endif
};
template <> struct ZuPrint<ZpPcap::Stats> : public ZuPrintFn { };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZpPcap_HPP */
