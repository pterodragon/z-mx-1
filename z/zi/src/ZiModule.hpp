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

// dynamic loading of shared objects / DLLs

#ifndef ZiModule_HPP
#define ZiModule_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <ZiLib.hpp>
#endif

#include <ZmLock.hpp>

#include <ZePlatform.hpp>

#include <ZiPlatform.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class ZiAPI ZiModule {
  ZiModule(const ZiModule &);
  ZiModule &operator =(const ZiModule &);	// prevent mis-use

public:
#ifdef _WIN32
  typedef HMODULE Handle;
#else
  typedef void *Handle;
#endif

  typedef ZiPlatform::Path Path;

  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;

  enum Flags {
    GC	= 0x001,	// unload() handle in destructor
    Pre	= 0x002		// equivalent to LD_PRELOAD / RTLD_DEEPBIND
  };

  struct Error {
#ifdef _WIN32
    inline static void clear() { }
    inline static ZtZString last() {
      return ZePlatform_::strerror(GetLastError());
    }
#else
    inline static void clear() { dlerror(); }
    inline static ZtZString last() { return dlerror(); }
#endif
  };

  inline ZiModule() : m_handle(0), m_flags(0) { }
  inline ~ZiModule() { finalize(); }

  inline Handle handle() { return m_handle; }

  inline int flags() { return m_flags; }
  inline void setFlags(int f) { Guard guard(m_lock); m_flags |= f; }
  inline void clrFlags(int f) { Guard guard(m_lock); m_flags &= ~f; }

  int init(Handle handle, int flags, ZtZString *e = 0) {
    Guard guard(m_lock);

    if (m_flags & GC) unload();
    m_handle = handle;
    m_flags = flags;
    return Zi::OK;
  }

  inline void finalize() {
    Guard guard(m_lock);

    if (m_flags & GC)
      unload();
    else
      m_handle = 0;
  }

  int load(const Path &name, int flags, ZtZString *e = 0);
  int unload(ZtZString *e = 0);

  void *resolve(const char *symbol, ZtZString *e = 0);

private:
  ZmLock	m_lock;
  Handle	m_handle;
  int		m_flags;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* ZiModule_HPP */
