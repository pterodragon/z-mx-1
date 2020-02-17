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

// Gtk wrapper

#ifndef ZGtk_HPP
#define ZGtk_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZGtkLib_HPP
#include <zlib/ZGtkLib.hpp>
#endif

#include <glib.h>

#include <gtk/gtk.h>

#include <zlib/ZmScheduler.hpp>

namespace ZGtk {

class App {
public:
  void attach(ZmScheduler *sched, unsigned tid);
  void detach();

  ZuInline ZmScheduler *sched() const { return m_sched; }
  ZuInline unsigned tid() const { return m_tid; }

private:
  void attach_();	// runs on Gtk thread
  void detach_();	// ''

  void wake();
  void wake_();		// ''
  static void run_();	// ''

private:
  GSource	*m_source = nullptr;
  ZmScheduler	*m_sched = nullptr;
  unsigned	m_tid = 0;
};

} // ZGtk

#endif /* ZGtk_HPP */
