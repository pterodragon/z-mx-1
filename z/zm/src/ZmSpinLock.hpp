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

// ck FAS spinlock wrapper

#ifndef ZmSpinLock_HPP
#define ZmSpinLock_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmLockTraits.hpp>

#ifdef _WIN32
typedef ZmPLock ZmSpinLock;
#endif

#ifndef _WIN32
#include <ck_spinlock.h>

class ZmSpinLock {
  ZmSpinLock(const ZmSpinLock &);
  ZmSpinLock &operator =(const ZmSpinLock &);	// prevent mis-use

public:
  ZuInline ZmSpinLock() { ck_spinlock_fas_init(&m_lock); }
  ZuInline void lock() { ck_spinlock_fas_lock(&m_lock); }
  ZuInline int trylock() { return -!ck_spinlock_fas_trylock(&m_lock); }
  ZuInline void unlock() { ck_spinlock_fas_unlock(&m_lock); }

private:
  ck_spinlock_fas_t	m_lock;
};

template <>
struct ZmLockTraits<ZmSpinLock> : public ZmGenericLockTraits<ZmSpinLock> {
  enum { CanTry = 1, Recursive = 0, RWLock = 0 };
};
#endif /* !_WIN32 */

#endif /* ZmSpinLock_HPP */
