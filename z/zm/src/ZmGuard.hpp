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

// guard template for locks

#ifndef ZmGuard_HPP
#define ZmGuard_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZmLockTraits.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4800)
#endif

template <class Lock> class ZmGuard : private ZmLockTraits<Lock> {
  typedef ZmLockTraits<Lock> Traits;

public:
  enum Try_ { Try };	// disambiguator

  ZuInline ZmGuard() noexcept : m_lock(0) { }

  ZuInline ZmGuard(Lock &l) noexcept : m_lock(&l) { this->lock(l); }

  ZuInline explicit ZmGuard(Lock &l, Try_ _) noexcept : m_lock(&l) {
    if (this->trylock(l)) m_lock = 0;
  }
  ZuInline explicit ZmGuard(Lock &l, Try_ _, int &r) noexcept : m_lock(&l) {
    if (r = this->trylock(l)) m_lock = 0;
  }

  ZuInline ZmGuard(ZmGuard &&guard) noexcept : m_lock(guard.m_lock) {
    guard.m_lock = 0;
  }
  ZuInline ZmGuard &operator =(ZmGuard &&guard) noexcept {
    if (m_lock) Traits::unlock(*m_lock);
    m_lock = guard.m_lock;
    guard.m_lock = 0;
    return *this;
  }

  ZuInline ~ZmGuard() noexcept { if (m_lock) Traits::unlock(*m_lock); }

  ZuInline bool locked() const noexcept { return m_lock; }

  ZuInline void unlock() noexcept {
    if (m_lock) { Traits::unlock(*m_lock); m_lock = 0; }
  }

private:
  Lock		*m_lock;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <class Lock> class ZmReadGuard : private ZmLockTraits<Lock> {
  typedef ZmLockTraits<Lock> Traits;

public:
  enum Try_ { Try };	// disambiguator

  ZuInline ZmReadGuard(const Lock &l) noexcept :
    m_lock(&(const_cast<Lock &>(l))) {
    this->readlock(const_cast<Lock &>(l));
  }
  ZuInline explicit ZmReadGuard(const Lock &l, Try_ _) noexcept :
      m_lock(&(const_cast<Lock &>(l))) {
    if (this->tryreadlock(const_cast<Lock &>(l))) m_lock = 0;
  }
  ZuInline explicit ZmReadGuard(const Lock &l, Try_ _, int &r) noexcept :
      m_lock(&(const_cast<Lock &>(l))) {
    if (r = this->tryreadlock(const_cast<Lock &>(l))) m_lock = 0;
  }
  ZuInline ZmReadGuard(ZmReadGuard &&guard) noexcept : m_lock(guard.m_lock) {
    guard.m_lock = 0;
  }
  ZuInline ~ZmReadGuard() noexcept { if (m_lock) Traits::readunlock(*m_lock); }

  ZuInline void unlock() noexcept {
    if (m_lock) { Traits::readunlock(*m_lock); m_lock = 0; }
  }

  ZuInline ZmReadGuard &operator =(ZmReadGuard &&guard) noexcept {
    if (m_lock) Traits::readunlock(*m_lock);
    m_lock = guard.m_lock;
    guard.m_lock = 0;
    return *this;
  }

private:
  Lock		*m_lock;
};

#endif /* ZmGuard_HPP */
