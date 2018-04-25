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

/* disabled lock */

#ifndef ZmNoLock_HPP
#define ZmNoLock_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZmLockTraits.hpp>

class ZmNoLock {
  ZmNoLock(const ZmNoLock &);
  ZmNoLock &operator =(const ZmNoLock &);	// prevent mis-use

public:
  inline ZmNoLock() { }
  inline ~ZmNoLock() { }

  inline void lock() { }
  inline int trylock() { return 0; }
  inline void unlock() { }
};

template <>
struct ZmLockTraits<ZmNoLock> : public ZmGenericLockTraits<ZmNoLock> {
  inline static void lock(ZmNoLock &l) { }
  inline static int trylock(ZmNoLock &l) { return 0; }
  inline static void unlock(ZmNoLock &l) { }
  inline static void readlock(ZmNoLock &l) { }
  inline static int readtrylock(ZmNoLock &l) { return 0; }
  inline static void readunlock(ZmNoLock &l) { }
};

template <class Lock> class ZmGuard;
template <class Lock> class ZmReadGuard;
template <> class ZmGuard<ZmNoLock> {
public:
  inline ZmGuard(ZmNoLock &l) { }
  inline ZmGuard(const ZmGuard &guard) { }
  inline ~ZmGuard() { }

  inline void unlock() { }

  inline ZmGuard &operator =(const ZmGuard &guard) { return *this; }
};

template <> class ZmReadGuard<ZmNoLock> {
public:
  inline ZmReadGuard(const ZmNoLock &l) { }
  inline ZmReadGuard(const ZmReadGuard &guard) { }
  inline ~ZmReadGuard() { }

  inline void unlock() { }

  inline ZmReadGuard &operator =(const ZmReadGuard &guard) { return *this; }
};

#endif /* ZmNoLock_HPP */
