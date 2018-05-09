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

// intrusive globals (singletons) - used by ZmSingleton / ZmSpecific

#ifndef _WIN32
#include <alloca.h>
#endif

// #include <stdio.h>

#include <ZmGlobal.hpp>

#ifdef ZDEBUG
#include <ZmStream.hpp>
#endif

// statically-initialized spinlock to guard initial singleton registration
// and cleanup at exit; little if any contention is anticipated; access
// intended to be exceptional, intermittent, and limited to application
// startup and shutdown
static uint32_t ZmGlobal_lock = 0;
#define lock() \
  ZmAtomic<uint32_t> *ZuMayAlias(lock) = \
    (ZmAtomic<uint32_t> *)&ZmGlobal_lock; \
  while (ZuUnlikely(lock->cmpXch(1, 0))) ZmPlatform::yield()
#define unlock() (*lock = 0)

static uint32_t ZmGlobal_atexit_ = 0;
typedef ZmGlobal *ZmGlobalPtr;
static ZmGlobalPtr ZmGlobal_list[ZmCleanupLevel::N] = { 0 };

// atexit handler
ZmExtern void ZmGlobal_atexit()
{
  lock();
  ZmGlobal_atexit_ = 0;
  unsigned n = 0;
  for (unsigned i = 0; i < ZmCleanupLevel::N; i++)
    for (ZmGlobal *g = ZmGlobal_list[i]; g; g = g->m_next) ++n;
  if (ZuUnlikely(!n)) { unlock(); return; }
  ZmGlobal **globals;
#ifdef __GNUC__
  globals = (ZmGlobal **)alloca(sizeof(ZmGlobal *) * n);
#endif
#ifdef _MSC_VER
  __try {
    globals = (ZmGlobal **)_alloca(sizeof(ZmGlobal *) * n);
  } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
    _resetstkoflw();
    globals = 0;
  }
#endif
  if (ZuUnlikely(!globals)) { unlock(); return; }
  unsigned o = 0;
  for (unsigned i = 0; i < ZmCleanupLevel::N; i++) {
    for (ZmGlobal *g = ZmGlobal_list[i]; g; g = g->m_next) {
      if (ZuUnlikely(o >= n)) { unlock(); return; }
      globals[o++] = g;
    }
    ZmGlobal_list[i] = 0;
  }
  unlock();
  // do not call dtor with lock held
  for (unsigned i = 0; i < n; i++) {
    ZmGlobal *g = globals[i];
    // std::cerr << "ZmGlobal_atexit() deleting " << g->m_name << ' ' << ZuBoxPtr(g).hex() << '\n' << std::flush;
    delete g;
  }
}

// singleton registration - normally only called once per type
// (on Windows, will be called once per type per DLL/EXE that uses it)
ZmGlobal *ZmGlobal::add(
    std::type_index type, unsigned level, ZmGlobal *(*ctor)())
{
  // printf("ZmGlobal::add()\n"); fflush(stdout);
  lock();
  if (ZuUnlikely(!ZmGlobal_atexit_)) {
    ZmGlobal_atexit_ = 1;
    ::atexit(ZmGlobal_atexit);
  }
  ZmGlobal *c = 0;
  ZmGlobal *g;
retry:
  for (g = ZmGlobal_list[level]; g; g = g->m_next)
    if (g->m_type == type) {
      unlock(); // do not call dtor with lock held
      // printf("ZmGlobal::add() returning existing instance %s %p\n", g->m_name, g); fflush(stdout);
      if (ZuUnlikely(c)) delete (ZmGlobal *)c;
      return g;
    }
  if (!c) {
    unlock(); // do not call ctor with lock held
    c = (*ctor)();
    lock();
    goto retry;
  }
  c->m_next = ZmGlobal_list[level];
  ZmGlobal_list[level] = c;
  // printf("ZmGlobal::add() returning new instance %s %p\n", c->m_name, c); fflush(stdout);
  unlock();
  return c;
}

#ifdef ZDEBUG
void ZmGlobal::dump(ZmStream &s)
{
  lock();
  for (unsigned i = 0; i < ZmCleanupLevel::N; i++)
    for (ZmGlobal *g = ZmGlobal_list[i]; g; g = g->m_next)
      s << g->m_name << ' ' << ZuBoxPtr(g).hex() << '\n';
  unlock();
}
#endif
