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

// simple fast dynamic ring buffer (supports FIFO and LIFO) for types with
// a distinguished null value (defaults to ZuCmp<T>::null())

#ifndef ZmDRing_HPP
#define ZmDRing_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuNull.hpp>
#include <zlib/ZuArrayFn.hpp>
#include <zlib/ZuCmp.hpp>
#include <zlib/ZuIndex.hpp>

#include <zlib/ZmAssert.hpp>
#include <zlib/ZmGuard.hpp>
#include <zlib/ZmLock.hpp>
#include <zlib/ZmNoLock.hpp>

// defaults
#define ZmDRingInitial		8
#define ZmDRingIncrement	8
#define ZmDRingMaxFrag		50.0

class ZmDRingParams {
public:
  ZmDRingParams() :
    m_initial(ZmDRingInitial),
    m_increment(ZmDRingIncrement),
    m_maxFrag(ZmDRingMaxFrag) { }

  ZmDRingParams &initial(unsigned v) { m_initial = v; return *this; }
  ZmDRingParams &increment(unsigned v) { m_increment = v; return *this; }
  ZmDRingParams &maxFrag(double v) { m_maxFrag = v; return *this; }

  unsigned initial() const { return m_initial; }
  unsigned increment() const { return m_increment; }
  double maxFrag() const { return m_maxFrag; }

private:
  unsigned	m_initial;
  unsigned	m_increment;
  double	m_maxFrag;
};

// uses NTP (named template parameters):
//
// ZmDRing<ZtString,			// ring of ZtStrings
//   ZmDRingCmp<ZtICmp> >		// case-insensitive comparison

struct ZmDRing_Defaults {
  template <typename T> struct OpsT { using Ops = ZuArrayFn<T>; };
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct IndexT { using Index = T; };
  using Lock = ZmLock;
};

template <class Cmp_, class NTP = ZmDRing_Defaults>
struct ZmDRingCmp : public NTP {
  template <typename T> struct OpsT {
    using Ops = ZuArrayFn<T, Cmp_>;
  };
  template <typename T> struct CmpT {
    using Cmp = Cmp_;
  };
};

template <class ICmp_, class NTP = ZmDRing_Defaults>
struct ZmDRingICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

template <typename Accessor, class NTP = ZmDRing_Defaults>
struct ZmDRingIndex : public NTP {
  template <typename T> struct CmpT {
    using Cmp = typename ZuIndex<Accessor>::template CmpT<T>;
  };
  template <typename> struct ICmpT {
    using ICmp = typename ZuIndex<Accessor>::ICmp;
  };
  template <typename> struct IndexT {
    using Index = typename ZuIndex<Accessor>::I;
  };
};

template <class Lock_, class NTP = ZmDRing_Defaults>
struct ZmDRingLock : public NTP {
  using Lock = Lock_;
};

// only provide delPtr and findPtr methods to callers of unlocked ZmDRings
// since they are intrinsically not thread-safe

template <typename Val, class NTP> class ZmDRing;

template <class Ring> struct ZmDRing_Unlocked;
template <typename Val, class NTP>
struct ZmDRing_Unlocked<ZmDRing<Val, NTP> > {
  using Ring = ZmDRing<Val, NTP>;

  template <typename Index_>
  Val *findPtr(const Index_ &index) {
    return static_cast<Ring *>(this)->findPtr_(index);
  }
  void delPtr(Val *val) {
    return static_cast<Ring *>(this)->delPtr_(val);
  }
};

template <class Ring, class Lock> struct ZmDRing_Base { };
template <class Ring>
struct ZmDRing_Base<Ring, ZmNoLock> : public ZmDRing_Unlocked<Ring> { };

// derives from Ops so that a ZmDRing includes an *instance* of Ops

template <typename Val, class NTP = ZmDRing_Defaults> class ZmDRing :
    public ZmDRing_Base<ZmDRing<Val, NTP>, typename NTP::Lock>,
    public NTP::template OpsT<Val>::Ops {
  ZmDRing(const ZmDRing &);
  ZmDRing &operator =(const ZmDRing &);	// prevent mis-use

friend ZmDRing_Unlocked<ZmDRing>;

public:
  using Ops = typename NTP::template OpsT<Val>::Ops;
  using Cmp = typename NTP::template CmpT<Val>::Cmp;
  using ICmp = typename NTP::template ICmpT<Val>::ICmp;
  using Index = typename NTP::template IndexT<Val>::Index;
  using Lock = typename NTP::Lock;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  template <typename ...Args>
  ZmDRing(ZmDRingParams params = ZmDRingParams(), Args &&... args) :
      m_data(0), m_offset(0), m_size(0), m_length(0), m_count(0),
      m_initial(params.initial()), m_increment(params.increment()),
      m_defrag(1.0 - (double)params.maxFrag() / 100.0) { }

  ~ZmDRing() {
    clean_();
    free(m_data);
  }

  ZuInline unsigned initial() const { return m_initial; }
  ZuInline unsigned increment() const { return m_increment; }
  ZuInline unsigned maxFrag() const {
    return (unsigned)((1.0 - m_defrag) * 100.0);
  }

  ZuInline unsigned size() const { ReadGuard guard(m_lock); return m_size; }
  ZuInline unsigned length() const { ReadGuard guard(m_lock); return m_length; }
  ZuInline unsigned count() const { ReadGuard guard(m_lock); return m_count; }
  ZuInline unsigned size_() const { return m_size; }
  ZuInline unsigned length_() const { return m_length; }
  ZuInline unsigned count_() const { return m_count; }
  ZuInline unsigned offset_() const { return m_offset; }

private:
  void lazy() {
    if (ZuUnlikely(!m_data)) extend(m_initial);
  }

  void clean_() {
    if (!m_data) return;
    unsigned o = m_offset + m_length;
    if (o > m_size) {
      int n = m_size - m_offset;
      Ops::destroyItems(m_data + m_offset, n);
      Ops::destroyItems(m_data, o - m_size);
    } else {
      Ops::destroyItems(m_data + m_offset, m_length);
    }
  }

  void extend(unsigned size) {
    Val *data = (Val *)malloc(size * sizeof(Val));
    ZmAssert(data);
    if (m_data) {
      unsigned o = m_offset + m_length;
      if (o > m_size) {
	int n = m_size - m_offset;
	Ops::moveItems(data, m_data + m_offset, n);
	Ops::moveItems(data + n, m_data, o - m_size);
      } else {
	Ops::moveItems(data, m_data + m_offset, m_length);
      }
      free(m_data);
    }
    m_data = data, m_size = size, m_offset = 0;
  }

public:
  void init(ZmDRingParams params = ZmDRingParams()) {
    Guard guard(m_lock);

    if ((m_initial = params.initial()) > m_size) extend(params.initial());
    m_increment = params.increment();
    m_defrag = 1.0 - (double)params.maxFrag() / 100.0;
  }

  void clean() {
    Guard guard(m_lock);

    clean_();
    m_offset = m_length = m_count = 0;
  }

private:
  void push() {
    if (m_count >= m_size) extend(m_size + m_increment);
  }
  unsigned offset(unsigned i) {
    if ((i += m_offset) >= m_size) i -= m_size;
    return i;
  }

public:
  template <typename Val_>
  void push(Val_ &&v) {
    Guard guard(m_lock);

    lazy();
    push();
    Ops::initItem(m_data + offset(m_length++), ZuFwd<Val_>(v));
    m_count++;
  }

  // idempotent push
  template <typename Val_>
  void findPush(Val_ &&v) {
    Guard guard(m_lock);

    for (int i = m_length; --i >= 0; ) {
      unsigned o = offset(i);
      if (Cmp::equals(m_data[o], v)) return;
    }
    lazy();
    push();
    Ops::initItem(m_data + offset(m_length++), ZuFwd<Val_>(v));
    m_count++;
  }

  Val pop() {
    Guard guard(m_lock);

    if (m_count <= 0) return Cmp::null();
    --m_count;
    unsigned o = offset(--m_length);
    Val v = ZuMv(m_data[o]);
    Ops::destroyItem(m_data + o);
    {
      int i = m_length;
      while (--i >= 0) {
	o = offset(i);
	if (!Cmp::null(m_data[o])) break;
	Ops::destroyItem(m_data + o);
      }
      m_length = ++i;
    }
    return v;
  }

  template <typename Val_>
  void unshift(Val_ &&v) {
    Guard guard(m_lock);

    lazy();
    push();
    unsigned o = offset(m_size - 1);
    m_offset = o, m_length++;
    Ops::initItem(m_data + o, ZuFwd<Val_>(v));
    m_count++;
  }

  // idempotent unshift
  template <typename Val_>
  void findUnshift(Val_ &&v) {
    Guard guard(m_lock);

    for (unsigned i = 0; i < m_length; i++) {
      unsigned o = offset(i);
      if (Cmp::equals(m_data[o], v)) return;
    }
    lazy();
    push();
    unsigned o = offset(m_size - 1);
    m_offset = o, m_length++;
    Ops::initItem(m_data + o, ZuFwd<Val_>(v));
    m_count++;
  }

  Val shift() {
    Guard guard(m_lock);

    if (m_count <= 0) return Cmp::null();
    --m_count;
    int i = m_length, j = 0, o = m_offset;
    Val v = ZuMv(m_data[o]);
    do {
      Ops::destroyItem(m_data + o);
    } while (--i > 0 && Cmp::null(m_data[o = offset(++j)]));
    m_offset = o, m_length = i;
    return v;
  }

  Val head() {
    Guard guard(m_lock);

    if (m_length <= 0) return Cmp::null();
    return m_data[m_offset];
  }
  Val tail() {
    Guard guard(m_lock);

    if (m_length <= 0) return Cmp::null();
    return m_data[offset(m_length - 1)];
  }

  template <typename Index_>
  Val find(const Index_ &index) {
    Guard guard(m_lock);

    for (int i = m_length; --i >= 0; ) {
      unsigned o = offset(i);
      if (ICmp::equals(m_data[o], index)) return m_data[o];
    }
    return Cmp::null();
  }

private:
  template <typename Index_>
  Val *findPtr_(const Index_ &index) {
    for (int i = m_length; --i >= 0; ) {
      unsigned o = offset(i);
      if (ICmp::equals(m_data[o], index)) return &m_data[o];
    }
    return 0;
  }

  void delPtr_(Val *val) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
    int i = val - m_data;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    m_count--;

    int o = offset(m_length - 1);
    if (i == o) {
      Ops::destroyItem(val);
      i = --m_length;
      while (--i >= 0) {
	if (!Cmp::null(m_data[o = offset(i)])) break;
	Ops::destroyItem(m_data + o);
      }
      m_length = ++i;
    } else if (i == (int)m_offset) {
      Ops::destroyItem(val);
      ++m_offset, --m_length, i = -1;
      while (++i < (int)m_length) {
	if (!Cmp::null(m_data[o = offset(i)])) break;
	Ops::destroyItem(m_data + o);
      }
      if (--i > 0) m_offset = offset(i), m_length -= i;
    } else {
      *val = Cmp::null();
      if ((double)m_count < (double)m_length * m_defrag) {
	i = m_length - 1;
	while (--i >= 0) {
	  if (Cmp::null(m_data[offset(i)])) {
	    int j = i;
	    while (--j >= 0 && Cmp::null(m_data[offset(j)])); ++i, ++j;
	    for (int n = j; n < i; n++)
	      Ops::destroyItem(m_data + offset(n));
	    for (int n = m_length - i; --n >= 0; i++, j++) {
	      Ops::initItem(m_data + offset(j), ZuMv(m_data[offset(i)]));
	      Ops::destroyItem(m_data + offset(i));
	    }
	    m_length -= (i - j);
	    i = j;
	  }
	}
      }
    }
  }

public:
  template <typename Index_> Val del(const Index_ &index) {
    Guard guard(m_lock);
    Val *vptr = findPtr_(index);

    if (!vptr) return Cmp::null();
    Val v = ZuMv(*vptr);
    delPtr_(vptr);
    return v;
  }

  class Iterator_;
friend Iterator_;
  class Iterator_ : private Guard {
    Iterator_(const Iterator_ &);
    Iterator_ &operator =(const Iterator_ &);	// prevent mis-use

  protected:
    using Ring = ZmDRing<Val, NTP>;

    Iterator_(Ring &ring, unsigned i) :
	Guard(ring.m_lock), m_ring(ring), m_i(i) { }

    Ring	&m_ring;
    int		m_i;
  };
  class Iterator;
friend Iterator;
  class Iterator : private Iterator_ {
  public:
    Iterator(typename Iterator_::Ring &ring) : Iterator_(ring, 0) { }
    Val *iteratePtr() {
      unsigned o;
      do {
	if (this->m_i >= this->m_ring.m_length) return 0;
	o = this->m_ring.offset(this->m_i++);
      } while (Cmp::null(this->m_ring.m_data[o]));
      return this->m_ring.m_data + o;
    }
    const Val &iterate() {
      unsigned o;
      do {
	if (this->m_i >= (int)this->m_ring.m_length) return Cmp::null();
	o = this->m_ring.offset(this->m_i++);
      } while (Cmp::null(this->m_ring.m_data[o]));
      return this->m_ring.m_data[o];
    }
  };
  auto iterator() { return Iterator(*this); }
  class RevIterator;
friend RevIterator;
  class RevIterator : private Iterator_ {
  public:
    RevIterator(typename Iterator_::Ring &ring) :
	Iterator_(ring, ring.m_length) { }
    Val *iteratePtr() {
      unsigned o;
      do {
	if (this->m_i <= 0) return 0;
	o = this->m_ring.offset(--this->m_i);
      } while (Cmp::null(this->m_ring.m_data[o]));
      return this->m_ring.m_data + o;
    }
    const Val &iterate() {
      unsigned o;
      do {
	if (this->m_i <= 0) return Cmp::null();
	o = this->m_ring.offset(--this->m_i);
      } while (Cmp::null(this->m_ring.m_data[o]));
      return this->m_ring.m_data[o];
    }
  };
  auto revIterator() { return RevIterator(*this); }

private:
  Lock		m_lock;
  Val		*m_data;
  unsigned	m_offset;
  unsigned	m_size;
  unsigned	m_length;
  unsigned	m_count;
  unsigned	m_initial;
  unsigned	m_increment;
  double	m_defrag;
};

#endif /* ZmDRing_HPP */
