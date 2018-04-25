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

// FIFO ring buffer with fan-in (MPSC)

#ifndef ZmRing_HPP
#define ZmRing_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuMixin.hpp>

#include <ZmPlatform.hpp>
#include <ZmAssert.hpp>
#include <ZmBitmap.hpp>
#include <ZmTopology.hpp>
#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmAtomic.hpp>
#include <ZmTime.hpp>
#include <ZmBackTrace.hpp>

// ring buffer parameters
class ZmRingParams {
public:
  inline ZmRingParams(unsigned size) :
    m_size(size), m_ll(false), m_spin(1000), m_timeout(1) { }

  inline ZmRingParams(const ZmRingParams &p) :
    m_size(p.m_size), m_ll(p.m_ll), m_spin(p.m_spin), m_timeout(p.m_timeout) { }
  inline ZmRingParams &operator =(const ZmRingParams &p) {
    if (this != &p) {
      m_size = p.m_size;
      m_ll = p.m_ll;
      m_spin = p.m_spin;
      m_timeout = p.m_timeout;
      m_cpuset = p.m_cpuset;
    }
    return *this;
  }

  inline ZmRingParams &size(unsigned n) { m_size = n; return *this; }
  inline ZmRingParams &ll(bool b) { m_ll = b; return *this; }
  inline ZmRingParams &spin(unsigned n) { m_spin = n; return *this; }
  inline ZmRingParams &timeout(unsigned n) { m_timeout = n; return *this; }
  inline ZmRingParams &cpuset(const ZmBitmap &b) { m_cpuset = b; return *this; }

  inline unsigned size() const { return m_size; }
  inline bool ll() const { return m_ll; }
  inline unsigned spin() const { return m_spin; }
  inline unsigned timeout() const { return m_timeout; }
  inline const ZmBitmap &cpuset() const { return m_cpuset; }

  inline bool operator !() const { return !m_size; }

private:
  unsigned	m_size;
  bool		m_ll;
  unsigned	m_spin;
  unsigned	m_timeout;
  ZmBitmap	m_cpuset;
};

class ZmAPI ZmRing_ {
public:
  enum { OK = 0, EndOfFile = -1, Error = -2, NotReady = -3 };

protected:
  enum { // flags
    Ready	= 0x10000000,		// header
    EndOfFile_	= 0x20000000,		// header, head
    Waiting	= 0x40000000,		// header, tail
    Wrapped	= 0x80000000,		// head, tail
    Mask	= Waiting | EndOfFile_ // does NOT include Wrapped or Ready
  };

  enum { Head = 0, Tail };

  inline ZmRing_(const ZmRingParams &params) : m_config(params)
#ifdef _WIN32
    , m_sem{0, 0}
#endif
      { }

  int open();
  int close();

#ifdef linux
#define ZmRing_wait(index, addr, val) wait(addr, val)
#define ZmRing_wake(index, addr, n) wake(addr, n)
  // block until woken or timeout while addr == val
  int wait(ZmAtomic<uint32_t> &addr, uint32_t val);
  // wake up waiters on addr (up to n waiters are woken)
  int wake(ZmAtomic<uint32_t> &addr, unsigned n);
#endif

#ifdef _WIN32
#define ZmRing_wait(index, addr, val) wait(index, addr, val)
#define ZmRing_wake(index, addr, n) wake(index, n)
  // block until woken or timeout while addr == val
  int wait(unsigned index, ZmAtomic<uint32_t> &addr, uint32_t val);
  // wake up waiters on addr (up to n waiters are woken)
  int wake(unsigned index, unsigned n);
#endif

public:
  inline void init(const ZmRingParams &params) { m_config = params; }

  ZuInline const ZmRingParams &config() const { return m_config; }
  ZuInline ZmRingParams &config() { return m_config; }

private:
  ZmRingParams		m_config;
#ifdef _WIN32
  HANDLE		m_sem[2];
#endif
};

typedef ZuBox<int> ZmRingError_Data;
template <typename T> struct ZmRingError_Fn : public T {
  template <typename S> inline void print(S &s) const {
    switch (static_cast<int>(*this)) {
      case ZmRing_::OK:		s << "OK"; break;
      case ZmRing_::EndOfFile:	s << "EndOfFile"; break;
      case ZmRing_::Error:	s << "Error"; break;
      case ZmRing_::NotReady:	s << "NotReady"; break;
      default:			s << "Unknown"; break;
    }
  }
};
typedef ZuMixin<ZmRingError_Data, ZmRingError_Fn> ZmRingError;
template <> struct ZuPrint<ZmRingError> : public ZuPrintFn { };

// uses NTP (named template parameters):
//
// ZmRing<ZmFn<>			// ring of functions
//   ZmRingBase<ZmObject> >		// reference-counted

// NTP defaults
struct ZmRing_Defaults {
  struct Base { };
};

// ZmRingBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmRing_Defaults>
struct ZmRingBase : public NTP {
  typedef Base_ Base;
};

#define ZmRingAlign(x) (((x) + 8 + 15) & ~15)

template <typename T_, class NTP = ZmRing_Defaults>
class ZmRing : public NTP::Base, public ZmRing_ {
  ZmRing &operator =(const ZmRing &);	// prevent mis-use

public:
  enum { CacheLineSize = ZmPlatform::CacheLineSize };

  enum { // open() flags
    Read	= 0x00000001,
    Write	= 0x00000002,
    Shadow	= 0x00000004
  };

  typedef T_ T;

  enum { Size = ZmRingAlign(sizeof(T)) };

  template <typename ...Args>
  inline ZmRing(ZmRingParams params = ZmRingParams(0), Args &&... args) :
      NTP::Base(ZuFwd<Args>(args)...),
      ZmRing_(params),
      m_flags(0), m_ctrl(0), m_data(0), m_full(0) { }

  inline ZmRing(const ZmRing &ring) :
      ZmRing_(ring.m_params), m_flags(Shadow),
      m_ctrl(ring.m_ctrl), m_data(ring.m_data), m_full(0) { }

  ~ZmRing() { close(); }

private:
  struct Ctrl {
    ZmAtomic<uint32_t>		head;
    char			pad_1[CacheLineSize - 4];

    ZmAtomic<uint32_t>		tail;
    char			pad_2[CacheLineSize - 4];
  };

  ZuInline const Ctrl *ctrl() const { return (const Ctrl *)m_ctrl; }
  ZuInline Ctrl *ctrl() { return (Ctrl *)m_ctrl; }

  ZuInline const ZmAtomic<uint32_t> &head() const { return ctrl()->head; }
  ZuInline ZmAtomic<uint32_t> &head() { return ctrl()->head; }

  ZuInline const ZmAtomic<uint32_t> &tail() const { return ctrl()->tail; }
  ZuInline ZmAtomic<uint32_t> &tail() { return ctrl()->tail; }

public:
  ZuInline void *data() { return m_data; }

  ZuInline unsigned full() const { return m_full; }

  int open(unsigned flags) {
    if (m_ctrl) return OK;
    if (!config().size()) return Error;
    flags &= (Read | Write);
    m_size = ((config().size() + Size - 1) / Size) * Size;
    if (m_flags & Shadow) {
      m_flags |= flags;
      return OK;
    }
    m_flags = flags;
    if (!config().ll() && ZmRing_::open() != OK) return Error;
    if (!config().cpuset())
      m_ctrl = hwloc_alloc(ZmTopology::hwloc(), sizeof(Ctrl));
    else
      m_ctrl = hwloc_alloc_membind(
	  ZmTopology::hwloc(), sizeof(Ctrl),
	  config().cpuset(), HWLOC_MEMBIND_BIND, 0);
    if (!m_ctrl) { if (!config().ll()) ZmRing_::close(); return Error; }
    memset(m_ctrl, 0, sizeof(Ctrl));
    if (!config().cpuset())
      m_data = hwloc_alloc(ZmTopology::hwloc(), size());
    else
      m_data = hwloc_alloc_membind(
	  ZmTopology::hwloc(), size(),
	  config().cpuset(), HWLOC_MEMBIND_BIND, 0);
    if (!m_data) {
      hwloc_free(ZmTopology::hwloc(), m_ctrl, sizeof(Ctrl));
      m_ctrl = 0;
      if (!config().ll()) ZmRing_::close();
      return Error;
    }
    return OK;
  }

  void close() {
    if (!m_ctrl) return;
    if (m_flags & Shadow) return;
    hwloc_free(ZmTopology::hwloc(), m_ctrl, sizeof(Ctrl));
    hwloc_free(ZmTopology::hwloc(), m_data, size());
    m_ctrl = m_data = 0;
    if (!config().ll()) ZmRing_::close();
  }

  int reset() {
    if (!m_ctrl) return Error;
    memset(m_ctrl, 0, sizeof(Ctrl));
    memset(m_data, 0, size());
    m_full = 0;
    return OK;
  }

  inline unsigned ctrlSize() const { return sizeof(Ctrl); }
  inline unsigned size() const { return m_size; }

  unsigned length() {
    uint32_t head = this->head().load_() & ~Mask;
    uint32_t tail = this->tail().load_() & ~Mask;
    if (head == tail) return 0;
    if ((head ^ tail) == Wrapped) return size();
    head &= ~Wrapped;
    tail &= ~Wrapped;
    if (head > tail) return head - tail;
    return size() - (tail - head);
  }

  // writer

  inline void *push() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

  retry:
    uint32_t head_ = this->head().load_();
    if (ZuUnlikely(head_ & EndOfFile_)) return 0;
    uint32_t head = head_ & ~Mask;
    uint32_t tail = this->tail() & ~Mask; // acquire
    if (ZuUnlikely((head ^ tail) == Wrapped)) {
      ++m_full;
      if (ZuUnlikely(!config().ll()))
	if (ZmRing_wait(Tail, this->tail(), tail) != OK) return 0;
      goto retry;
    }

    head += Size;
    if ((head & ~Wrapped) >= size()) head = (head ^ Wrapped) - size();

    if (ZuUnlikely(this->head().cmpXch(head, head_) != head_))
      goto retry;

    ZmAtomic<uint32_t> *ptr =
      (ZmAtomic<uint32_t> *)&((uint8_t *)data())[head_ & ~(Wrapped | Mask)];
    return (void *)&ptr[2];
  }
  inline void push2(void *ptr_) {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    ZmAtomic<uint32_t> *ptr = &((ZmAtomic<uint32_t> *)ptr_)[-2];

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(ptr->xch(Ready) & Waiting))
	this->ZmRing_wake(Head, *ptr, 1);
    } else
      *ptr = Ready; // release
  }

  void eof(bool b = true) {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    ZmAtomic<uint32_t> *ptr =
      (ZmAtomic<uint32_t> *)&((uint8_t *)data())[head & ~(Wrapped | Mask)];
    if (!b) {
      *ptr = 0; // release
      this->head() = head & ~EndOfFile_; // release
      return;
    }
    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(ptr->xch(EndOfFile_) & Waiting))
	this->ZmRing_wake(Head, *ptr, 1);
    } else
      *ptr = EndOfFile_; // release
    this->head() = head | EndOfFile_; // release
  }

  // can be called by writers after push() returns 0; returns
  // NotReady (no readers), EndOfFile,
  // or amount of space remaining in ring buffer (>= 0)
  int writeStatus() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    if (ZuUnlikely(head & EndOfFile_)) return EndOfFile;
    head &= ~Mask;
    uint32_t tail = this->tail() & ~Mask;
    if ((head ^ tail) == Wrapped) return 0;
    head &= ~Wrapped;
    tail &= ~Wrapped;
    if (head < tail) return tail - head;
    return size() - (head - tail);
  }

  // reader

  inline T *shift() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    uint32_t tail = this->tail().load_() & ~Mask;
  retry:
    ZmAtomic<uint32_t> *ptr =
      (ZmAtomic<uint32_t> *)&((uint8_t *)data())[tail & ~Wrapped];
    uint32_t header = *ptr; // acquire
    if (!(header &~ Waiting)) {
      if (ZuUnlikely(!config().ll()))
	if (ZmRing_wait(Head, *ptr, 0) != OK) return 0;
      goto retry;
    }

    if (ZuUnlikely(header & EndOfFile_)) return 0;
    ptr->store_(0);
    return (T *)&ptr[2];
  }
  inline void shift2() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    uint32_t tail = this->tail().load_() & ~Mask;
    tail += Size;
    if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->tail().xch(tail & ~Waiting) & Waiting))
	this->ZmRing_wake(Tail, this->tail(), 1);
    } else
      this->tail() = tail; // release
  }

  // can be called by a reader after shift() returns 0; returns
  // EndOfFile (< 0), or amount of data remaining in ring buffer (>= 0)
  int readStatus() const {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    uint32_t tail = this->tail().load_() & ~Mask;
    {
      ZmAtomic<uint32_t> *ptr = (ZmAtomic<uint32_t> *)
	&((uint8_t *)(const_cast<ZmRing *>(this)->data()))[tail & ~Wrapped];
      if (ZuUnlikely(ptr->load_() & EndOfFile_)) return EndOfFile;
    }
    uint32_t head = const_cast<ZmRing *>(this)->head() & ~Mask; // acquire
    if ((head ^ tail) == Wrapped) return size();
    head &= ~Wrapped;
    tail &= ~Wrapped;
    if (head >= tail) return head - tail;
    return size() - (tail - head);
  }

  inline unsigned count() const {
    int i = readStatus();
    if (i < 0) return 0;
    return i / Size;
  }

private:
  uint32_t		m_flags;
  void			*m_ctrl;
  void			*m_data;
  uint32_t		m_size;
  uint32_t		m_full;
};

#endif /* ZmRing_HPP */
