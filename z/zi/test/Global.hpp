//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZmAtomic.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmTimeInterval.hpp>

class Global {
  Global(const Global &);
  Global &operator =(const Global &);	// prevent mis-use

friend struct ZmSingletonCtor<Global>;

  Global() :
    m_recvRequests(0), m_recvBytes(0),
    m_sendRequests(0), m_sendBytes(0) { }

public:
  using TimeInterval = ZmTimeInterval<ZmPLock>;

  ZuInline static void wait() { instance()->m_sem.wait(); }
  ZuInline static void post() { instance()->m_sem.post(); }

  ZuInline static TimeInterval &timeInterval(int i) {
    return instance()->m_timeIntervals[i];
  }

  ZuInline static void sent(unsigned n) { instance()->sent_(n); }
  ZuInline static void rcvd(unsigned n) { instance()->rcvd_(n); }

  ZuInline static void dumpStats() { instance()->dumpStats_(); }

private:
  ZuInline void sent_(unsigned n) { ++m_sendRequests; m_sendBytes += n; }
  ZuInline void rcvd_(unsigned n) { ++m_recvRequests; m_recvBytes += n; }

  void dumpStats_() {
    std::cout <<
      "Recv " << ZuBoxed(m_recvRequests) << ':' << ZuBoxed(m_recvBytes) <<
      " Send " << ZuBoxed(m_sendRequests) << ':' << ZuBoxed(m_sendBytes) <<
      '\n';
  }

  static Global *instance();

  ZmSemaphore		m_sem;
  TimeInterval		m_timeIntervals[8];
  uint64_t		m_recvRequests;
  uint64_t		m_recvBytes;
  uint64_t		m_sendRequests;
  uint64_t		m_sendBytes;
};

Global *Global::instance() { return ZmSingleton<Global>::instance(); }
