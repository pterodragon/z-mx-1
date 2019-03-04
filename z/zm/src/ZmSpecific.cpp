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

// Meyers / Phoenix TLS Multiton

#include <ZmSpecific.hpp>

#ifdef _WIN32

// Win32 voodoo to force TLS support in linked image
extern "C" { extern DWORD _tls_used; }
struct R {
  inline R() : m_value(_tls_used) { };
  volatile DWORD	m_value;
};

// use TLS API from within DLL to be safe on 2K and XP
struct K {
  inline K() { m_key = TlsAlloc(); }
  inline ~K() { TlsFree(m_key); }

  inline int set(void *value) const
    { return TlsSetValue(m_key, value) ? 0 : -1; }
  inline void *get() const
    { return TlsGetValue(m_key); }

  DWORD	m_key;
};

// per-instance cleanup context
struct C {
  inline C(C *next, ZmSpecific_cleanup_Fn fn, void *ptr) :
      m_next(next), m_fn(fn), m_ptr(ptr) { }

  inline static K &key() {
    static K key_;
    return key_;
  }
  inline static C *head() { return (C *)key().get(); }
  inline static void head(C *c) { key().set(c); }

  static R		m_ref;

  C			*m_next;
  ZmSpecific_cleanup_Fn	m_fn;
  void			*m_ptr;
};

R C::m_ref;	// TLS reference

void ZmSpecific_cleanup()
{
  while (C *c = C::head()) {
    C::head(0);
    while (c) {
      (*(c->m_fn))(c->m_ptr);
      C *n = c->m_next;
      delete c;
      c = n;
    }
  }
}

ZmAPI void ZmSpecific_cleanup_add(ZmSpecific_cleanup_Fn fn, void *ptr)
{
  C::head(new C(C::head(), fn, ptr));
}

ZmAPI void ZmSpecific_cleanup_del(void *ptr)
{
  C *c = C::head();
  if (!c) return;
  C *n = c->m_next;
  if (c->m_ptr == ptr) {
    C::head(n);
    goto cleanup;
  }
  while (n) {
    if (n->m_ptr == ptr) {
      c->m_next = n->m_next, c = n;
      goto cleanup;
    }
    c = n, n = c->m_next;
  }
  return;
cleanup:
  delete c;
}

extern "C" {
  typedef void (NTAPI *ZmSpecific_cleanup_ptr)(HINSTANCE, DWORD, void *);
  ZmExtern void NTAPI ZmSpecific_cleanup_(HINSTANCE, DWORD, void *);
}

void NTAPI ZmSpecific_cleanup_(HINSTANCE, DWORD d, void *)
{
  if (d == DLL_THREAD_DETACH) ZmSpecific_cleanup();
}

// more Win32 voodoo
#ifdef _MSC_VER
#pragma section(".CRT$XLC", long, read)
extern "C" {
  __declspec(allocate(".CRT$XLC")) ZmSpecific_cleanup_ptr
    __ZmSpecific_cleanup__ = ZmSpecific_cleanup_;
}
#else
extern "C" {
  PIMAGE_TLS_CALLBACK __ZmSpecific_cleanup__
    __attribute__((section(".CRT$XLC"))) =
      (PIMAGE_TLS_CALLBACK)ZmSpecific_cleanup_;
}
#endif

#endif
