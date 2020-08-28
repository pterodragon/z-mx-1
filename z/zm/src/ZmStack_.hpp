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

// simple fast stack (LIFO array) for types with
// a distinguished null value (defaults to ZuCmp<T>::null())
//
// ZmStack_.hpp is used by other Zm components to avoid a circular dependency
// on ZmLock - do not include directly - use ZmStack.hpp instead

#ifndef ZmStack__HPP
#define ZmStack__HPP

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
#include <zlib/ZuConversion.hpp>
#include <zlib/ZuIfT.hpp>

#include <zlib/ZmAssert.hpp>
#include <zlib/ZmGuard.hpp>

class ZmLock;
class ZmNoLock;

// defaults
#define ZmStackInitial		4
#define ZmStackIncrement	8
#define ZmStackMaxFrag		50.0

class ZmStackParams {
public:
  ZmStackParams() :
    m_initial(ZmStackInitial),
    m_increment(ZmStackIncrement),
    m_maxFrag(ZmStackMaxFrag) { }

  ZmStackParams &initial(unsigned v) { m_initial = v; return *this; }
  ZmStackParams &increment(unsigned v) { m_increment = v; return *this; }
  ZmStackParams &maxFrag(double v) { m_maxFrag = v; return *this; }

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
// ZmStack<ZtString>			// stack of ZtStrings
//    ZmStackCmp<ZtICmp> >		// case-insensitive comparison

struct ZmStack_Defaults {
  template <typename T> struct OpsT { using Ops = ZuArrayFn<T>; };
  template <typename T> struct CmpT { using Cmp = ZuCmp<T>; };
  template <typename T> struct ICmpT { using ICmp = ZuCmp<T>; };
  template <typename T> struct IndexT { using Index = T; };
  using Lock = ZmLock;
};

template <class Cmp_, class NTP = ZmStack_Defaults>
struct ZmStackCmp : public NTP {
  template <typename T> struct OpsT { using Ops = ZuArrayFn<T, Cmp_>; };
  template <typename> struct CmpT { using Cmp = Cmp_; };
};

template <class ICmp_, class NTP = ZmStack_Defaults>
struct ZmStackICmp : public NTP {
  template <typename> struct ICmpT { using ICmp = ICmp_; };
};

template <typename Accessor, class NTP = ZmStack_Defaults>
struct ZmStackIndex : public NTP {
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

template <class Lock_, class NTP = ZmStack_Defaults>
struct ZmStackLock : public NTP {
  using Lock = Lock_;
};

// only provide delPtr and findPtr methods to callers of unlocked ZmStacks
// since they are intrinsically not thread-safe

template <typename Val, class NTP> class ZmStack;

template <class Stack> struct ZmStack_Unlocked;
template <typename Val, class NTP>
struct ZmStack_Unlocked<ZmStack<Val, NTP> > {
  using Stack = ZmStack<Val, NTP>;

  template <typename Index_>
  Val *findPtr(const Index_ &index) {
    return static_cast<Stack *>(this)->findPtr_(index);
  }
  void delPtr(Val *val) {
    return static_cast<Stack *>(this)->delPtr_(val);
  }
};

template <class Stack, class Lock> struct ZmStack_Base { };
template <class Stack>
struct ZmStack_Base<Stack, ZmNoLock> : public ZmStack_Unlocked<Stack> { };

// derives from Ops so that a ZmStack includes an *instance* of Ops

template <typename Val, class NTP = ZmStack_Defaults> class ZmStack :
    public ZmStack_Base<ZmStack<Val, NTP>, typename NTP::Lock>,
    public NTP::template OpsT<Val>::Ops {
  ZmStack(const ZmStack &);
  ZmStack &operator =(const ZmStack &);	// prevent mis-use

friend ZmStack_Unlocked<ZmStack>;

public:
  using Ops = typename NTP::template OpsT<Val>::Ops;
  using Cmp = typename NTP::template CmpT<Val>::Cmp;
  using ICmp = typename NTP::template ICmpT<Val>::ICmp;
  using Index = typename NTP::template IndexT<Val>::Index;
  using Lock = typename NTP::Lock;

  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  template <typename ...Args>
  ZmStack(ZmStackParams params = ZmStackParams(), Args &&... args) :
      m_data(0), m_size(0), m_length(0), m_count(0),
      m_initial(params.initial()),
      m_increment(params.increment()),
      m_defrag(1.0 - (double)params.maxFrag() / 100.0) { }

  ~ZmStack() {
    clean_();
    free(m_data);
  }

  unsigned initial() const { return m_initial; }
  unsigned increment() const { return m_increment; }
  unsigned maxFrag() const {
    return (unsigned)((1.0 - m_defrag) * 100.0);
  }

  unsigned size() const { ReadGuard guard(m_lock); return m_size; }
  unsigned length() const { ReadGuard guard(m_lock); return m_length; }
  unsigned count() const { ReadGuard guard(m_lock); return m_count; }
  unsigned size_() const { return m_size; }
  unsigned length_() const { return m_length; }
  unsigned count_() const { return m_count; }

private:
  void lazy() {
    if (ZuUnlikely(!m_data)) extend(m_initial);
  }

  void clean_() {
    if (!m_data) return;
    Ops::destroyItems(m_data, m_length);
  }

  void extend(unsigned size) {
    Val *data = (Val *)malloc((m_size = size) * sizeof(Val));
    ZmAssert(data);
    if (m_data) {
      Ops::moveItems(data, m_data, m_length);
      free(m_data);
    }
    m_data = data;
  }

public:
  void init(ZmStackParams params = ZmStackParams()) {
    Guard guard(m_lock);

    if ((m_initial = params.initial()) > m_size) extend(params.initial());
    m_increment = params.increment();
    m_defrag = 1.0 - (double)params.maxFrag() / 100.0;
  }

  void clean() {
    Guard guard(m_lock);

    clean_();
    m_length = m_count = 0;
  }

  template <typename Val_>
  void push(Val_ &&v) {
    Guard guard(m_lock);

    lazy();
    if (m_length >= m_size) {
      Val *data = (Val *)malloc((m_size += m_increment) * sizeof(Val));
      ZmAssert(data);
      Ops::moveItems(data, m_data, m_length);
      free(m_data);
      m_data = data;
    }
    Ops::initItem(m_data + m_length++, ZuFwd<Val_>(v));
    m_count++;
  }

  Val pop() {
    Guard guard(m_lock);

    if (m_length <= 0) return Cmp::null();
    --m_count;
    Val v = ZuMv(m_data[--m_length]);
    {
      int i = m_length;
      while (--i >= 0 && Cmp::null(m_data[i])); ++i;
      Ops::destroyItems(m_data + i, m_length + 1 - i);
      m_length = i;
    }
    return v;
  }

  Val head() {
    Guard guard(m_lock);

    for (unsigned i = 0; i < m_length; i++)
      if (!Cmp::null(m_data[i])) return m_data[i];
    return Cmp::null();
  }
  Val tail() {
    Guard guard(m_lock);

    if (m_length <= 0) return Cmp::null();
    return m_data[m_length - 1];
  }

  template <typename Index_>
  Val find(const Index_ &index) {
    Guard guard(m_lock);

    for (int i = m_length; --i >= 0; )
      if (ICmp::equals(m_data[i], index)) return m_data[i];
    return Cmp::null();
  }

private:
  template <typename Index_>
  Val *findPtr_(const Index_ &index) {
    for (int i = m_length; --i >= 0; )
      if (ICmp::equals(m_data[i], index)) return &m_data[i];
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
    if (m_defrag >= 1.0) {
      Ops::destroyItems(m_data + i, 1);
      if (i < (int)--m_length)
	Ops::moveItems(m_data + i, m_data + i + 1, m_length - i);
    } else if (i == (int)m_length - 1) {
      while (--i >= 0 && Cmp::null(m_data[i])); ++i;
      Ops::destroyItems(m_data + i, m_length - i);
      m_length = i;
    } else {
      *val = Cmp::null();
      if ((double)m_count < (double)m_length * m_defrag) {
	i = m_length - 1;
	while (--i >= 0) {
	  if (Cmp::null(m_data[i])) {
	    int j;
	    for (j = i; --j >= 0 && Cmp::null(m_data[j]); ); ++i, ++j;
	    Ops::destroyItems(m_data + j, i - j);
	    Ops::moveItems(m_data + j, m_data + i, m_length - i);
	    m_length -= (i - j);
	    i = j;
	  }
	}
      }
    }
  }

public:
  template <typename Index_>
  Val del(const Index_ &index) {
    Guard guard(m_lock);
    Val *vptr = findPtr_(index);

    if (!vptr) return Cmp::null();
    Val v = ZuMv(*vptr);
    delPtr_(vptr);
    return v;
  }

  class Iterator;
friend Iterator;
  class Iterator : private Guard {
    Iterator(const Iterator &);
    Iterator &operator =(const Iterator &);	// prevent mis-use

    using Stack = ZmStack<Val, NTP>;

  public:
    Iterator(Stack &stack) :
      Guard(stack.m_lock), m_stack(stack), m_i(stack.m_length) { }
    Val *iteratePtr() {
      do {
	if (m_i <= 0) return 0;
      } while (Cmp::null(m_stack.m_data[--m_i]));
      return m_stack.m_data + m_i;
    }
    const Val &iterate() {
      do {
	if (m_i <= 0) return Cmp::null();
      } while (Cmp::null(m_stack.m_data[--m_i]));
      return m_stack.m_data[m_i];
    }

  private:
    Stack	&m_stack;
    int		m_i;
  };
  auto iterator() { return Iterator(*this); }

private:
  Lock		m_lock;
  Val		*m_data;
  unsigned	m_size;
  unsigned	m_length;
  unsigned	m_count;
  unsigned	m_initial;
  unsigned	m_increment;
  double	m_defrag;
};

#endif /* ZmStack__HPP */
