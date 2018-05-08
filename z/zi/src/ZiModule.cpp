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

#include <ZiModule.hpp>

#ifdef _WIN32

int ZiModule::load(const Path &name, int flags, ZtString *e)
{
  Guard guard(m_lock);

  if (m_flags & GC) unload();
  m_flags = flags;
  if (m_handle = LoadLibrary(name)) return Zi::OK;
  if (e) *e = Error::last();
  return Zi::IOError;
}

int ZiModule::unload(ZtString *e)
{
  Guard guard(m_lock);

  if (!m_handle) return Zi::OK;
  if (FreeLibrary(m_handle)) {
    m_handle = 0;
    return Zi::OK;
  }
  m_handle = 0;
  if (e) *e = Error::last();
  return Zi::IOError;
}

void *ZiModule::resolve(const char *symbol, ZtString *e)
{
  Guard guard(m_lock);
  void *ptr;

  ptr = (void *)GetProcAddress(m_handle, symbol);
  if (!ptr && e) *e = Error::last();
  return ptr;
}

#else

#include <dlfcn.h>

int ZiModule::load(const Path &name, int flags, ZtString *e)
{
  Guard guard(m_lock);

  if (m_flags & GC) unload();
  m_flags = flags;
  int flags_ = RTLD_LAZY | RTLD_GLOBAL;
  if (flags & Pre) flags_ |= RTLD_DEEPBIND;
  if (m_handle = dlopen(name, flags_)) return Zi::OK;
  if (e) *e = Error::last();
  return Zi::IOError;
}

int ZiModule::unload(ZtString *e)
{
  Guard guard(m_lock);

  if (!m_handle) return Zi::OK;
  if (!dlclose(m_handle)) {
    m_handle = 0;
    return Zi::OK;
  }
  m_handle = 0;
  if (e) *e = Error::last();
  return Zi::IOError;
}

void *ZiModule::resolve(const char *symbol, ZtString *e)
{
  Guard guard(m_lock);
  void *ptr;

  if ((ptr = dlsym(m_handle, symbol)) && ptr != (void *)(uintptr_t)-1)
    return ptr;
  if (e) *e = Error::last();
  return 0;
}

#endif
