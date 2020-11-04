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

// Gtk application wrapper

#ifndef ZGtkApp_HPP
#define ZGtkApp_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZGtkLib_HPP
#include <zlib/ZGtkLib.hpp>
#endif

#include <glib.h>

#include <gtk/gtk.h>

#include <zlib/ZuString.hpp>

#include <zlib/ZmScheduler.hpp>
#include <zlib/ZmLock.hpp>

#include <zlib/ZtString.hpp>

namespace ZGtk {

class ZGtkAPI App {
public:
  // e.g. "gimp20", "/usr/share" - initialize locale, libintl (gettext)
  void i18n(ZtString domain, ZtString dataDir);

  void attach(ZmScheduler *sched, unsigned tid);
  ZuInline void detach() { detach(ZmFn<>{}); }
  void detach(ZmFn<>);

  ZuInline ZmScheduler *sched() const { return m_sched; }
  ZuInline unsigned tid() const { return m_tid; }

  template <typename ...Args>
  ZuInline void run(Args &&... args)
    { m_sched->run(m_tid, ZuFwd<Args>(args)...); }
  template <typename ...Args>
  ZuInline void invoke(Args &&... args)
    { m_sched->invoke(m_tid, ZuFwd<Args>(args)...); }

private:
  void attach_();	// runs on Gtk thread
  void detach_(ZmFn<>);	// ''

  void wake();
  void wake_();		// ''
  static void run_();	// ''

private:
  GSource		*m_source = nullptr;
  ZmScheduler		*m_sched = nullptr;
  unsigned		m_tid = 0;
  ZtString		m_domain;	// libintl domain
  ZtString		m_dataDir;	// libintl data directory
};

} // ZGtk

#endif /* ZGtkApp_HPP */
