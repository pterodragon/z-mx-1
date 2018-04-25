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

// FIFO ring buffer with broadcast fan-out to multiple readers (up to 64)

// Note: this is broadcast, not SPMC - every consumer processes every message,
// i.e. every message is processed as many times as there are consumers;
// with conventional SPMC, each message is processed once by a single
// consumer selected from amongst the pool of all consumers, typically
// non-deterministically chosen by availability/readiness/priority

#ifndef ZmBxRing_HPP
#define ZmBxRing_HPP

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
#include <ZmRing.hpp>

// uses NTP (named template parameters):
//
// ZmBxRing<ZmFn<>			// ring of functions
//   ZmBxRingBase<ZmObject> >		// reference-counted

// NTP defaults
struct ZmBxRing_Defaults {
  struct Base { };
};

// ZmBxRingBase - injection of a base class (e.g. ZmObject)
template <class Base_, class NTP = ZmBxRing_Defaults>
struct ZmBxRingBase : public NTP {
  typedef Base_ Base;
};

#define ZmBxRingAlign(x) (((x) + 8 + 15) & ~15)

template <typename T_, class NTP = ZmBxRing_Defaults>
class ZmBxRing : public NTP::Base, public ZmRing_ {
  ZmBxRing &operator =(const ZmBxRing &);	// prevent mis-use

public:
  enum { CacheLineSize = ZmPlatform::CacheLineSize };

  enum { // open() flags
    Read	= 0x00000001,
    Write	= 0x00000002,
    Shadow	= 0x00000004
  };

  typedef T_ T;

  enum { Size = ZmBxRingAlign(sizeof(T)) };

  template <typename ...Args>
  inline ZmBxRing(ZmRingParams params = ZmRingParams(0), Args &&... args) :
      NTP::Base(ZuFwd<Args>(args)...),
      ZmRing_(params),
      m_flags(0), m_id(-1), m_ctrl(0), m_data(0),
      m_tail(0), m_size(0), m_full(0) { }

  inline ZmBxRing(const ZmBxRing &ring) :
      ZmRing_(ring.m_params), m_flags(Shadow),
      m_id(-1), m_ctrl(ring.m_ctrl), m_data(ring.m_data),
      m_tail(0), m_full(0) { }

  inline ~ZmBxRing() { close(); }

private:
  struct Ctrl {
    ZmAtomic<uint32_t>		head;
    char			pad_1[CacheLineSize - 4];

    ZmAtomic<uint32_t>		tail;
    char			pad_2[CacheLineSize - 4];

    ZmAtomic<uint32_t>		rdrCount; // reader count
    ZmAtomic<uint64_t>		rdrMask;  // active readers
    ZmAtomic<uint64_t>		attMask;  // readers pending attach
    ZmAtomic<uint64_t>		attSeqNo; // attach/detach seqNo
  };

  ZuInline const Ctrl *ctrl() const { return (const Ctrl *)m_ctrl; }
  ZuInline Ctrl *ctrl() { return (Ctrl *)m_ctrl; }

  ZuInline const ZmAtomic<uint32_t> &head() const { return ctrl()->head; }
  ZuInline ZmAtomic<uint32_t> &head() { return ctrl()->head; }

  ZuInline const ZmAtomic<uint32_t> &tail() const { return ctrl()->tail; }
  ZuInline ZmAtomic<uint32_t> &tail() { return ctrl()->tail; }

  ZuInline ZmAtomic<uint32_t> &rdrCount() { return ctrl()->rdrCount; }
  ZuInline ZmAtomic<uint64_t> &rdrMask() { return ctrl()->rdrMask; }
  ZuInline ZmAtomic<uint64_t> &attMask() { return ctrl()->attMask; }
  ZuInline ZmAtomic<uint64_t> &attSeqNo() { return ctrl()->attSeqNo; }
 
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
      goto ret;
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
  ret:
    if (flags & Read) ++rdrCount();
    return OK;
  }

  void close() {
    if (!m_ctrl) return;
    if (m_flags & Read) {
      if (m_id >= 0) detach();
      --rdrCount();
    }
    if (m_flags & Shadow) return;
    hwloc_free(ZmTopology::hwloc(), m_ctrl, sizeof(Ctrl));
    hwloc_free(ZmTopology::hwloc(), m_data, size());
    m_ctrl = m_data = 0;
    if (!config().ll()) ZmRing_::close();
  }

  int reset() {
    if (!m_ctrl) return Error;
    if ((m_flags & Read) && m_id >= 0) detach();
    if (ZuUnlikely(rdrMask())) return NotReady;
    memset(m_ctrl, 0, sizeof(Ctrl));
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
    uint64_t rdrMask = this->rdrMask().load_();
    if (!rdrMask) return 0; // no readers

    uint32_t head = this->head().load_();
    if (ZuUnlikely(head & EndOfFile_)) return 0; // EOF
    head &= ~Mask;
    uint32_t tail = this->tail() & ~Mask; // acquire
    if (ZuUnlikely((head ^ tail) == Wrapped)) {
      ++m_full;
      if (ZuUnlikely(!config().ll()))
	if (ZmRing_wait(Tail, this->tail(), tail) != OK) return 0;
      goto retry;
    }

    uint8_t *ptr = &((uint8_t *)data())[head & ~Wrapped];
    *(uint64_t *)ptr = rdrMask;
    return (void *)&ptr[8];
  }
  inline void push2() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    head += Size;
    if ((head & ~(Wrapped | Mask)) >= size()) head = (head ^ Wrapped) - size();

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->head().xch(head & ~Waiting) & Waiting))
	this->ZmRing_wake(Head, this->head(), rdrCount().load_());
    } else
      this->head() = head; // release
  }

  void eof(bool b = true) {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    uint32_t head = this->head().load_();
    if (b)
      head |= EndOfFile_;
    else
      head &= ~EndOfFile_;

    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->head().xch(head & ~Waiting) & Waiting))
	this->ZmRing_wake(Head, this->head(), rdrCount().load_());
    } else
      this->head() = head; // release
  }

  // can be called by writers after push() returns 0; returns
  // NotReady (no readers), EndOfFile,
  // or amount of space remaining in ring buffer (>= 0)
  int writeStatus() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Write);

    if (ZuUnlikely(!rdrMask())) return NotReady;
    uint32_t head = this->head().load_();
    if (ZuUnlikely(head & EndOfFile_)) return EndOfFile;
    head &= ~(Wrapped | Mask);
    uint32_t tail = this->tail() & ~(Wrapped | Mask);
    if (head < tail) return tail - head;
    return size() - (head - tail);
  }

  // reader

  ZuInline int id() { return m_id; } // -ve if not attached

  int attach() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    if (m_id >= 0) return Error;

    // allocate an ID for this reader
    {
      uint64_t attMask;
      unsigned id;
      do {
	attMask = this->attMask().load_();
	for (id = 0; id < 64; id++)
	  if (!(attMask & (1ULL<<id))) break;
	if (id == 64) return Error;
      } while (
	  this->attMask().cmpXch(attMask | (1ULL<<id), attMask) != attMask);

      m_id = id;
    }

    ++(this->attSeqNo());

    rdrMask() |= (1ULL<<m_id); // notifies the writer about an attach

    // skip any trailing messages not intended for us, since other readers
    // may be concurrently advancing the ring's tail; this must be
    // re-attempted as long as the head keeps moving and the writer remains
    // unaware of our attach
    uint32_t tail = this->tail().load_() & ~Mask;
    uint32_t head = this->head() & ~Mask, head_; // acquire
    do {
      while (tail != head) {
	uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
	if ((*(uint64_t *)ptr) & (1ULL<<m_id)) goto done; // writer aware
	tail += Size;
	if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
      }
      head_ = head;
      head = this->head() & ~Mask; // acquire
    } while (head != head_);

  done:
    m_tail = tail;

    ++(this->attSeqNo());

    return OK;
  }

  int detach() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    if (m_id < 0) return Error;

    ++(this->attSeqNo());

    rdrMask() &= ~(1ULL<<m_id); // notifies the writer about a detach

    // drain any trailing messages that are waiting to be read by us,
    // advancing ring's tail as needed; this must be
    // re-attempted as long as the head keeps moving and the writer remains
    // unaware of our detach
    uint32_t tail = m_tail;
    uint32_t head = this->head() & ~Mask, head_; // acquire
    do {
      while (tail != head) {
	uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
	if (!((*(uint64_t *)ptr) & (1ULL<<m_id))) goto done; // writer aware
	tail += Size;
	if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
	if (*(ZmAtomic<uint64_t> *)ptr &= ~(1ULL<<m_id)) continue;

	if (ZuUnlikely(!config().ll())) {
	  if (ZuUnlikely(this->tail().xch(tail) & Waiting))
	    this->ZmRing_wake(Tail, this->tail(), 1);
	} else
	  this->tail() = tail; // release
      }
      head_ = head;
      head = this->head() & ~Mask; // acquire
    } while (head != head_);
  done:
    m_tail = tail;

    ++(this->attSeqNo());

    attMask() &= ~(1ULL<<m_id);
    m_id = -1;

    return OK;
  }

  inline T *shift() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);
    ZmAssert(m_id >= 0);

    uint32_t tail = m_tail;
    uint32_t head;
  retry:
    head = this->head(); // acquire
    if (tail == (head & ~Mask)) {
      if (ZuUnlikely(head & EndOfFile_)) return 0;
      if (ZuUnlikely(!config().ll()))
	if (ZmRing_wait(Head, this->head(), head) != OK) return 0;
      goto retry;
    }

    uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
    return (T *)&ptr[8];
  }
  inline void shift2() {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);
    ZmAssert(m_id >= 0);

    uint32_t tail = m_tail;
    uint8_t *ptr = &((uint8_t *)data())[tail & ~Wrapped];
    tail += Size;
    if ((tail & ~Wrapped) >= size()) tail = (tail ^ Wrapped) - size();
    m_tail = tail;
    if (*(ZmAtomic<uint64_t> *)ptr &= ~(1ULL<<m_id)) return;
    if (ZuUnlikely(!config().ll())) {
      if (ZuUnlikely(this->tail().xch(tail) & Waiting))
	this->ZmRing_wake(Tail, this->tail(), 1);
    } else
      this->tail() = tail; // release
  }

  // can be called by readers after shift() returns 0; returns
  // EndOfFile (< 0), or amount of data remaining in ring buffer (>= 0)
  int readStatus() const {
    ZmAssert(m_ctrl);
    ZmAssert(m_flags & Read);

    uint32_t head = const_cast<ZmBxRing *>(this)->head();
    if (ZuUnlikely(head & EndOfFile_)) return EndOfFile;
    head &= ~Mask;
    uint32_t tail = m_tail;
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
  int			m_id;
  void			*m_ctrl;
  void			*m_data;
  uint32_t		m_tail;
  uint32_t		m_size;
  uint32_t		m_full;
};

#endif /* ZmBxRing_HPP */
