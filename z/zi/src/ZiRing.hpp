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

// shared memory ring buffer IPC with broadcast fan-out to multiple readers
// (up to 64)

// Note: this is not conventional SPMC since every consumer processes
// every message, i.e. every message is broadcast to all consumers;
// SPMC conventionally means that each message is only processed once by a
// single consumer, selected non-deterministically according to availability

// supports fixed- and variable length messages
// messages are C/C++ POD types

// two modes are supported: normal and low-latency
// normal uses futexes (Linux) or semaphores (Windows) to wait/wake-up
// low-latency uses 100% CPU - readers spin, no yielding or system calls

// performance varies with message size; for typical sizes (100-10000 bytes):
// normal	- expect .1 -.5  mics latency,	1- 5M msgs/sec
// low-latency	- expect .01-.05 mics latency, 10-50M msgs/sec

// ring buffer size heuristics
// normal	- use  100x average message size
// low-latency	- use 1000x average message size

#ifndef ZiRing_HPP
#define ZiRing_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <ZiLib.hpp>
#endif

#include <ZmPlatform.hpp>
#include <ZmBitmap.hpp>
#include <ZmTopology.hpp>
#include <ZmLock.hpp>
#include <ZmGuard.hpp>
#include <ZmAtomic.hpp>

#include <ZePlatform.hpp>

#include <ZiPlatform.hpp>
#include <ZiFile.hpp>

#ifdef ZiRing_FUNCTEST
#define ZiRing_bp(x) (bp_##x.reached(#x))
#else
#ifdef ZiRing_STRESSTEST
#define ZiRing_bp(x) ZmPlatform::yield()
#else
#define ZiRing_bp(x) (void)0
#endif
#endif

// default traits for a (fixed-length) item in a ring buffer
template <typename T> struct ZiRingTraits {
  inline static unsigned size(const T &t) { return sizeof(T); }
};

// ZiRingMsg - variable length message (discriminated union)
class ZiRingMsg {
public:
  inline ZiRingMsg() : m_type(0), m_length(0) { }
  inline ZiRingMsg(unsigned type, unsigned length) :
    m_type(type), m_length(length) { }

  inline unsigned type() const { return m_type; }
  inline unsigned length() const { return m_length; }
  inline void *ptr() { return (void *)&this[1]; }
  inline const void *ptr() const { return (const void *)&this[1]; }

private:
  uint32_t	m_type;
  uint32_t	m_length;
};

// traits for ZiRingMsg
template <> struct ZiRingTraits<ZiRingMsg> {
  inline static unsigned size(const ZiRingMsg &msg) {
    return sizeof(ZiRingMsg) + msg.length();
  }
};

// ring buffer parameters
class ZiAPI ZiRingParams {
public:
  typedef ZiPlatform::Path Path;

  inline ZiRingParams() :
    m_size(0),
    m_ll(false), m_spin(1000),
    m_timeout(1), m_killWait(1), m_coredump(false) { }

  template <typename Name>
  inline ZiRingParams(const Name &name) :
    m_name(name), m_size(0),
    m_ll(false), m_spin(1000),
    m_timeout(1), m_killWait(1), m_coredump(false) { }
  template <typename Name>
  inline ZiRingParams(const Name &name, const ZiRingParams &p) :
    m_name(name), m_size(p.m_size),
    m_ll(p.m_ll), m_spin(p.m_spin),
    m_timeout(p.m_timeout),
    m_cpuset(p.m_cpuset),
    m_killWait(p.m_killWait),
    m_coredump(p.m_coredump) { }

  inline ZiRingParams(const ZiRingParams &p) :
    m_name(p.m_name), m_size(p.m_size),
    m_ll(p.m_ll), m_spin(p.m_spin),
    m_timeout(p.m_timeout),
    m_cpuset(p.m_cpuset),
    m_killWait(p.m_killWait),
    m_coredump(p.m_coredump) { }
  inline ZiRingParams &operator =(const ZiRingParams &p) {
    if (this != &p) {
      m_name = p.m_name;
      m_size = p.m_size;
      m_ll = p.m_ll;
      m_spin = p.m_spin;
      m_timeout = p.m_timeout;
      m_cpuset = p.m_cpuset;
      m_killWait = p.m_killWait;
      m_coredump = p.m_coredump;
    }
    return *this;
  }

  template <typename Name>
  inline ZiRingParams &name(const Name &s) { m_name = s; return *this; }
  inline ZiRingParams &size(unsigned n) { m_size = n; return *this; }
  inline ZiRingParams &ll(bool b) { m_ll = b; return *this; }
  inline ZiRingParams &spin(unsigned n) { m_spin = n; return *this; }
  inline ZiRingParams &timeout(unsigned n) { m_timeout = n; return *this; }
  inline ZiRingParams &cpuset(const ZmBitmap &b) { m_cpuset = b; return *this; }
  inline ZiRingParams &killWait(unsigned n) { m_killWait = n; return *this; }
  inline ZiRingParams &coredump(bool b) { m_coredump = b; return *this; }

  ZuInline const ZtString &name() const { return m_name; }
  ZuInline unsigned size() const { return m_size; }
  ZuInline bool ll() const { return m_ll; }
  ZuInline unsigned spin() const { return m_spin; }
  ZuInline unsigned timeout() const { return m_timeout; }
  ZuInline const ZmBitmap &cpuset() const { return m_cpuset; }
  ZuInline unsigned killWait() const { return m_killWait; }
  ZuInline bool coredump() const { return m_coredump; }

private:
  ZtString	m_name;
  unsigned	m_size;
  bool		m_ll;
  unsigned	m_spin;
  unsigned	m_timeout;
  ZmBitmap	m_cpuset;
  unsigned	m_killWait;
  bool		m_coredump;
};

class ZiAPI ZiRing_ {
protected:
  enum { // head+tail flags
    EndOfFile	= 0x20000000,
    Waiting	= 0x40000000,
    Wrapped	= 0x80000000,
    Mask	= EndOfFile | Waiting // does NOT include Wrapped
  };

  enum { Head = 0, Tail };

  ZiRing_(const ZiRingParams &params) : m_config(params)
#ifdef _WIN32
    , m_sem{0, 0}
#endif
      { }

  int open(ZeError *e = 0);
  int close(ZeError *e = 0);

#ifdef linux
#define ZiRing_wait(index, addr, val) wait(addr, val)
#define ZiRing_wake(index, addr, n) wake(addr, n)
  // block until woken or timeout while addr == val
  int wait(ZmAtomic<uint32_t> &addr, uint32_t val);
  // wake up waiters on addr (up to n waiters are woken)
  int wake(ZmAtomic<uint32_t> &addr, unsigned n);
#endif

#ifdef _WIN32
#define ZiRing_wait(index, addr, val) wait(index, addr, val)
#define ZiRing_wake(index, addr, n) wake(index, n)
  // block until woken or timeout while addr == val
  int wait(unsigned index, ZmAtomic<uint32_t> &addr, uint32_t val);
  // wake up waiters on addr (up to n waiters are woken)
  int wake(unsigned index, unsigned n);
#endif

  static void getpinfo(uint32_t &pid, ZmTime &start);
  static bool alive(uint32_t pid, ZmTime start);
  static bool kill(uint32_t pid, bool coredump);

public:
  ZuInline const ZiRingParams &config() const { return m_config; }
  ZuInline ZiRingParams &config() { return m_config; }

private:
  ZiRingParams		m_config;
#ifdef _WIN32
  HANDLE		m_sem[2];
#endif
};

// NTP (named template parameters):
//
// ZiRing<ZiRingMsg,			// ring of ZiRingMsg
//   ZiRingBase<ZmObject> >		// base of ZmObject

// NTP defaults
struct ZiRing_Defaults {
  struct Base { };
};

// ZiRingBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZiRing_Defaults>
struct ZiRingBase : public NTP {
  typedef Base_ Base;
};

// ring buffer
template <typename T_, class NTP = ZiRing_Defaults>
class ZiRing : public NTP::Base, public ZiRing_ {
  ZiRing(const ZiRing &);
  ZiRing &operator =(const ZiRing &);	// prevent mis-use

public:
  enum { CacheLineSize = ZmPlatform::CacheLineSize };

  enum { // open() flags
    Create	= 0x00000001,
    Read	= 0x00000002,
    Write	= 0x00000004
  };

  typedef T_ T;
  typedef ZiRingTraits<T> Traits;

  template <typename ...Args>
  ZiRing(const ZiRingParams &params, Args &&... args) :
      NTP::Base{ZuFwd<Args>(args)...},
      ZiRing_(params), m_flags(0), m_id(-1), m_tail(0) { }

  ~ZiRing() { close(); }

#ifndef ZiRing_FUNCTEST
private:
#endif
  struct Ctrl {
    ZmAtomic<uint32_t>		head;
    char			pad_1[CacheLineSize - 4];

    ZmAtomic<uint32_t>		tail;
    char			pad_2[CacheLineSize - 4];

    ZmAtomic<uint32_t>		openSize; // opened size && latency
    ZmAtomic<uint32_t>		rdrCount; // reader count
    ZmAtomic<uint64_t>		rdrMask;  // active readers
    ZmAtomic<uint64_t>		attMask;  // readers pending attach
    ZmAtomic<uint64_t>		attSeqNo; // attach/detach seqNo

    uint32_t			rdrPID[64];
    ZmTime			rdrTime[64];
  };

  ZuInline const Ctrl *ctrl() const { return (const Ctrl *)m_ctrl.addr(); }
  ZuInline Ctrl *ctrl() { return (Ctrl *)m_ctrl.addr(); }

  ZuInline const ZmAtomic<uint32_t> &head() const { return ctrl()->head; }
  ZuInline ZmAtomic<uint32_t> &head() { return ctrl()->head; }

  ZuInline const ZmAtomic<uint32_t> &tail() const { return ctrl()->tail; }
  ZuInline ZmAtomic<uint32_t> &tail() { return ctrl()->tail; }

  ZuInline ZmAtomic<uint32_t> &openSize() { return ctrl()->openSize; }
  ZuInline ZmAtomic<uint32_t> &rdrCount() { return ctrl()->rdrCount; }
  ZuInline ZmAtomic<uint64_t> &rdrMask() { return ctrl()->rdrMask; }
  ZuInline ZmAtomic<uint64_t> &attMask() { return ctrl()->attMask; }
  ZuInline ZmAtomic<uint64_t> &attSeqNo() { return ctrl()->attSeqNo; }

  // PIDs may be re-used by the OS, so readers are ID'd by PID + start time

  ZuInline uint32_t *rdrPID() { return ctrl()->rdrPID; }
  ZuInline ZmTime *rdrTime() { return ctrl()->rdrTime; }
 
public:
  ZuInline void *data() { return m_data.addr(); }

  int open(unsigned flags, ZeError *e) {
    if (!config().name()) goto einval;
    m_flags = flags;
    if (!config().ll() && ZiRing_::open(e) != Zi::OK) return Zi::IOError;
    {
      unsigned mmapFlags = ZiFile::Shm;
      if (flags & Create) mmapFlags |= ZiFile::Create;
      int r;
      if ((r = m_ctrl.mmap(config().name() + ".ctrl",
	      mmapFlags, sizeof(Ctrl), true, 0, 0777, e)) != Zi::OK)
	return r;
      if (config().size()) {
	uint32_t reqSize = (uint32_t)config().size() | (uint32_t)config().ll();
	// check that requested sizes and latency are consistent
	if (uint32_t openSize = this->openSize().cmpXch(reqSize, 0))
	  if (openSize != reqSize) {
	    m_ctrl.close();
	    goto einval;
	  }
      } else {
	uint32_t openSize = this->openSize();
	if (!(openSize & ~1)) {
	  m_ctrl.close();
	  goto einval;
	}
	config().size(openSize & ~1);
	config().ll(openSize & 1);
      }
      mmapFlags |= ZiFile::ShmDbl;
      if ((r = m_data.mmap(config().name() + ".data",
	      mmapFlags, config().size(), true, 0, 0777, e)) != Zi::OK) {
	m_ctrl.close();
	if (!config().ll()) ZiRing_::close();
	return r;
      }
      if (!!config().cpuset())
	hwloc_set_area_membind(
	    ZmTopology::hwloc(), m_data.addr(), (m_data.mmapLength())<<1,
	    config().cpuset(), HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_MIGRATE);
      if (flags & Read) ++rdrCount();
      if (flags & Write) {
	gc();
	this->head() = this->head().load_() & ~EndOfFile;
      }
      return Zi::OK;
    }

  einval:
#ifndef _WIN32
    if (e) *e = EINVAL;
#else
    if (e) *e = ERROR_INVALID_PARAMETER;
#endif
    return Zi::IOError;
  }

  void close() {
    if (!m_ctrl.addr()) return;
    if (m_flags & Read) {
      if (m_id >= 0) detach();
      --rdrCount();
    }
    m_ctrl.close();
    m_data.close();
    if (!config().ll()) ZiRing_::close();
  }

  int reset() {
    if (!m_ctrl.addr()) return Zi::IOError;
    if ((m_flags & Read) && m_id >= 0) detach();
    if (ZuUnlikely(this->rdrMask())) return Zi::NotReady;
    memset(m_ctrl.addr(), 0, sizeof(Ctrl));
    return Zi::OK;
  }

  ZuInline unsigned ctrlSize() const { return m_ctrl.mmapLength(); }
  ZuInline unsigned size() const { return m_data.mmapLength(); }

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

  ZuInline unsigned align(unsigned size) {
    return (size + 8 + CacheLineSize - 1) & ~(CacheLineSize - 1);
  }

  // writer

  ZuInline void *push(unsigned size = sizeof(T)) { return push_<1>(size); }
  ZuInline void *tryPush(unsigned size = sizeof(T)) { return push_<0>(size); }
  template <bool Wait> inline void *push_(unsigned size) {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Write);

    size = align(size);

  retry:
    uint64_t rdrMask = this->rdrMask().load_();
    if (!rdrMask) return 0; // no readers

    uint32_t head = this->head().load_();
    if (ZuUnlikely(head & EndOfFile)) return 0; // EOF
    uint32_t tail = this->tail(); // acquire
    uint32_t head_ = head & ~(Wrapped | Mask);
    uint32_t tail_ = tail & ~(Wrapped | Mask);
    if (ZuUnlikely((head_ < tail_ ?
	(head_ + size > tail_) :
	(head_ + size > tail_ + this->size())))) {
      int j = gc();
      if (ZuUnlikely(j < 0)) return 0;
      if (ZuUnlikely(j > 0)) goto retry;
      if constexpr (!Wait) return 0;
      if (ZuUnlikely(!config().ll()))
	if (ZiRing_wait(Tail, this->tail(), tail) != Zi::OK) return 0;
      goto retry;
    }

    uint8_t *ptr = &((uint8_t *)data())[head_];
    *(uint64_t *)ptr = rdrMask;
    return (void *)&ptr[8];
  }
  inline void push2() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    uint8_t *ptr = &((uint8_t *)data())[head & ~(Wrapped | Mask)];
    head += align(Traits::size(*(const T *)&ptr[8]));
    if ((head & ~(Wrapped | Mask)) >= size())
      head = (head ^ Wrapped) - size();

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->head().xch(head & ~Waiting) & Waiting))
	ZiRing_wake(Head, this->head(), rdrCount().load_());
    } else
      this->head() = head; // release
  }

  void eof(bool b = true) {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    if (b)
      head |= EndOfFile;
    else
      head &= ~EndOfFile;

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->head().xch(head & ~Waiting) & Waiting))
	ZiRing_wake(Head, this->head(), rdrCount().load_());
    } else
      this->head() = head; // release
  }

  // can be called by writer if ring is full to garbage collect
  // dead readers and any lingering messages intended exclusively for them;
  // returns space freed, or -1 if ring is empty and no readers
  int gc() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Write);

    // GC dead readers
 
    unsigned freed = 0;
    uint64_t dead;

    // below loop is a probe - as long as any concurrent attach() or
    // detach() overlap with our discovery of dead readers, the results
    // are unreliable and the probe must be re-attempted - after N attempts
    // give up and return 0
    for (unsigned i = 0;; ) {
      uint64_t attSeqNo = this->attSeqNo().load_();
      dead = this->rdrMask(); // assume all dead
      if (!dead) return -1;
      for (unsigned id = 0; id < 64; id++) {
	if (!(dead & (1ULL<<id))) continue;
	if (alive(rdrPID()[id], rdrTime()[id])) dead &= ~(1ULL<<id);
      }
      if (attSeqNo == this->attSeqNo()) break;
      ZmPlatform::yield();
      if (++i == config().spin()) return 0;
    }

    uint32_t tail_ = this->tail(); // acquire
    uint32_t tail = tail_ & ~Mask;
    uint32_t head = this->head().load_() & ~Mask;
    while (tail != head) {
      uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
      uint32_t n = align(Traits::size(*(const T *)&ptr[8]));
      tail += n;
      if (ZuUnlikely((tail & ~Wrapped) >= size()))
	tail = (tail ^ Wrapped) - size();
      uint64_t mask = (*(ZmAtomic<uint64_t> *)ptr).xchAnd(~dead);
      if (mask && !(mask & ~dead)) {
	freed += n;
	if (ZuUnlikely(!config().ll())) {
	  if (ZuUnlikely(
		this->tail().xch(tail | (tail_ & (Mask & ~Waiting))) & Waiting))
	    ZiRing_wake(Tail, this->tail(), 1);
	} else
	  this->tail() = tail | (tail_ & Mask); // release
      }
    }

    for (unsigned id = 0; id < 64; id++)
      if (dead & (1ULL<<id))
	if (rdrPID()[id]) {
	  this->rdrMask() &= ~(1ULL<<id);
	  rdrPID()[id] = 0, rdrTime()[id] = ZmTime();
	  --rdrCount();
	}

    if (!(attMask() &= ~dead)) return -1; // no readers left
    return freed;
  }

  // kills all stalled readers (following a timeout), sleeps, then runs gc()
  int kill() {
    uint64_t targets;
    {
      uint32_t tail = this->tail() & ~Mask;
      if (tail == (this->head() & ~Mask)) return 0;
      uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
      targets = *(ZmAtomic<uint64_t> *)ptr;
    }
    for (unsigned id = 0; id < 64; id++)
      if (targets & (1ULL<<id))
	ZiRing_::kill(rdrPID()[id], config().coredump());
    ZmPlatform::sleep(ZmTime((time_t)config().killWait()));
    return gc();
  }

  // can be called by writers after push returns 0; returns
  // NotReady (no readers), EndOfFile,
  // or amount of space remaining in ring buffer (>= 0)
  int writeStatus() {
    ZmAssert(m_flags & Write);

    if (ZuUnlikely(!m_ctrl.addr())) return Zi::IOError;
    if (ZuUnlikely(!rdrMask())) return Zi::NotReady;
    uint32_t head = this->head().load_();
    if (ZuUnlikely(head & EndOfFile)) return Zi::EndOfFile;
    head &= ~(Wrapped | Mask);
    uint32_t tail = this->tail() & ~(Wrapped | Mask);
    if (head < tail) return tail - head;
    return size() - (head - tail);
  }

  // reader

  ZuInline int id() { return m_id; } // -ve if not attached

  int attach() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Read);

    if (m_id >= 0) return Zi::IOError;

    // allocate an ID for this reader
    {
      uint64_t attMask;
      unsigned id;
      do {
	attMask = this->attMask().load_();
	for (id = 0; id < 64; id++)
	  if (!(attMask & (1ULL<<id))) break;
	if (id == 64) return Zi::IOError;
      } while (
	  this->attMask().cmpXch(attMask | (1ULL<<id), attMask) != attMask);

      m_id = id;
    }

    ++(this->attSeqNo());

    getpinfo(rdrPID()[m_id], rdrTime()[m_id]);
    /**/ZiRing_bp(attach1);
#ifndef ZiRing_FUNCTEST
    rdrMask() |= (1ULL<<m_id); // notifies the writer about an attach
#endif

    // skip any trailing messages not intended for us, since other readers
    // may be concurrently advancing the ring's tail; this must be
    // re-attempted as long as the head keeps moving and the writer remains
    // unaware of our attach
    /**/ZiRing_bp(attach2);
    uint32_t tail = this->tail().load_() & ~Mask;
    /**/ZiRing_bp(attach3);
    uint32_t head = this->head() & ~Mask, head_; // acquire
    do {
      while (tail != head) {
	uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
	if ((*(uint64_t *)ptr) & (1ULL<<m_id)) goto done; // writer aware
	tail += align(Traits::size(*(const T *)&ptr[8]));
	if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
      }
      head_ = head;
      /**/ZiRing_bp(attach4);
#ifdef ZiRing_FUNCTEST
      rdrMask() |= (1ULL<<m_id); // deferred notification for functional testing
#endif
      head = this->head() & ~Mask; // acquire
    } while (head != head_);

  done:
    m_tail = tail;

    ++(this->attSeqNo());

    return Zi::OK;
  }

  int detach() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Read);

    if (m_id < 0) return Zi::IOError;

    ++(this->attSeqNo());

#ifndef ZiRing_FUNCTEST
    rdrMask() &= ~(1ULL<<m_id); // notifies the writer about a detach
#endif

    /**/ZiRing_bp(detach1);
 
    // drain any trailing messages that are waiting to be read by us,
    // advancing ring's tail as needed; this must be
    // re-attempted as long as the head keeps moving and the writer remains
    // unaware of our detach
    uint32_t tail = m_tail;
    /**/ZiRing_bp(detach2);
    uint32_t head = this->head() & ~Mask, head_; // acquire
    do {
      while (tail != head) {
	uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
	if (!((*(uint64_t *)ptr) & (1ULL<<m_id))) goto done; // writer aware
	tail += align(Traits::size(*(const T *)&ptr[8]));
	if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
	if (*(ZmAtomic<uint64_t> *)ptr &= ~(1ULL<<m_id)) continue;
	/**/ZiRing_bp(detach3);
	if (ZuUnlikely(!config().ll())) {
	  if (ZuUnlikely(this->tail().xch(tail) & Waiting))
	    ZiRing_wake(Tail, this->tail(), 1);
	} else
	  this->tail() = tail; // release
      }
      head_ = head;
      /**/ZiRing_bp(detach4);
#ifdef ZiRing_FUNCTEST
      rdrMask() &= ~(1ULL<<m_id); // deferred notification for func. testing
#endif
      head = this->head() & ~Mask; // acquire
    } while (head != head_);
  done:
    m_tail = tail;

    // release ID for re-use by future attach
    /**/ZiRing_bp(detach5);
    rdrPID()[m_id] = 0, rdrTime()[m_id] = ZmTime();

    ++(this->attSeqNo());

    attMask() &= ~(1ULL<<m_id);
    m_id = -1;

    return Zi::OK;
  }

  inline T *shift() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Read);
    ZmAssert(m_id >= 0);

    uint32_t tail = m_tail;
    uint32_t head;
  retry:
    head = this->head(); // acquire
    /**/ZiRing_bp(shift1);
    if (tail == (head & ~Mask)) {
      if (ZuUnlikely(head & EndOfFile)) return 0;
      if (ZuUnlikely(!config().ll()))
	if (ZiRing_wait(Head, this->head(), head) != Zi::OK) return 0;
      goto retry;
    }

    uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
    return (T *)&ptr[8];
  }
  inline void shift2() {
    ZmAssert(m_ctrl.addr());
    ZmAssert(m_flags & Read);
    ZmAssert(m_id >= 0);

    uint32_t tail = m_tail;
    uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
    tail += align(Traits::size(*(const T *)&ptr[8]));
    if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
    m_tail = tail;
    if (*(ZmAtomic<uint64_t> *)ptr &= ~(1ULL<<m_id)) return;
    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->tail().xch(tail) & Waiting))
	ZiRing_wake(Tail, this->tail(), 1);
    } else
      this->tail() = tail; // release
  }

  // can be called by readers after push returns 0; returns
  // EndOfFile (< 0), or amount of data remaining in ring buffer (>= 0)
  int readStatus() {
    ZmAssert(m_flags & Read);

    if (ZuUnlikely(!m_ctrl.addr())) return Zi::IOError;
    uint32_t head = this->head();
    if (ZuUnlikely(head & EndOfFile)) return Zi::EndOfFile;
    head &= ~Mask;
    uint32_t tail = m_tail;
    if ((head ^ tail) == Wrapped) return size();
    head &= ~Wrapped;
    tail &= ~Wrapped;
    if (head >= tail) return head - tail;
    return size() - (tail - head);
  }

#ifndef ZiRing_FUNCTEST
private:
#endif
  uint32_t		m_flags;
  int			m_id;
  ZiFile		m_ctrl;
  ZiFile		m_data;
  uint32_t		m_tail;	// reader only

#ifdef ZiRing_FUNCTEST
public:
  ZiRing_Breakpoint	bp_attach1;
  ZiRing_Breakpoint	bp_attach2;
  ZiRing_Breakpoint	bp_attach3;
  ZiRing_Breakpoint	bp_attach4;
  ZiRing_Breakpoint	bp_detach1;
  ZiRing_Breakpoint	bp_detach2;
  ZiRing_Breakpoint	bp_detach3;
  ZiRing_Breakpoint	bp_detach4;
  ZiRing_Breakpoint	bp_detach5;
  ZiRing_Breakpoint	bp_shift1;
#endif
};

#endif /* ZiRing_HPP */
