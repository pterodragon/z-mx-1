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

// intrusively reference-counted object (base class)
// - ZmAtomic<int> reference count (multithread-safe)

#ifndef ZmObject_HPP
#define ZmObject_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#include <stddef.h>

#include <zlib/ZmObject_.hpp>
#include <zlib/ZmAtomic.hpp>

class ZmObject : public ZmObject_Debug {
  ZmObject(const ZmObject &) = delete;
  ZmObject &operator =(const ZmObject &) = delete;

public:
  ZuInline ZmObject()  : m_refCount(0) { }

  ZuInline ~ZmObject() {
#ifdef ZmObject_DEBUG
    this->del_();
#endif
  }

  ZuInline int refCount() const { return m_refCount.load_(); }

#ifdef ZmObject_DEBUG
  inline void ref(const void *referrer = 0) const
#else
  ZuInline void ref() const
#endif
  {
#ifdef ZmObject_DEBUG
    if (ZuUnlikely(this->deleted_())) return;
    if (ZuUnlikely(this->debugging_())) ZmObject_ref(this, referrer);
#endif
    this->ref_();
  }
#ifdef ZmObject_DEBUG
  inline bool deref(const void *referrer = 0) const
#else
  ZuInline bool deref() const
#endif
  {
#ifdef ZmObject_DEBUG
    if (ZuUnlikely(this->deleted_())) return false;
    if (ZuUnlikely(this->debugging_())) ZmObject_deref(this, referrer);
#endif
    return this->deref_();
  }

#ifdef ZmObject_DEBUG
  void mvref(const void *prev, const void *next) const {
    if (ZuUnlikely(this->debugging_())) {
      ZmObject_ref(this, next);
      ZmObject_deref(this, prev);
    }
  }
#endif

  // apps occasionally need to manipulate the refCount directly
  ZuInline void ref_() const { ++m_refCount; }
  ZuInline void ref2_() const { m_refCount += 2; }
  ZuInline bool deref_() const { return !--m_refCount; }

private:
#ifdef ZmObject_DEBUG
  ZuInline bool deleted_() const { return m_refCount.load_() < 0; }
  ZuInline void del_() const { m_refCount.store_(-1); }
#endif

  mutable ZmAtomic<int>	m_refCount;
};

#endif /* ZmObject_HPP */
