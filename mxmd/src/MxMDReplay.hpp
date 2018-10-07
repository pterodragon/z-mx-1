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

// MxMD replay

#ifndef MxMDReplay_HPP
#define MxMDReplay_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

class MxMDCore;

class MxMDAPI MxMDReplay {
  typedef ZmPLock Lock;
  typedef ZmGuard<ZmPLock> Guard;
  typedef ZuPair<ZuBox0(uint16_t), ZuBox0(uint16_t)> Version;

  MxMDReplay(MxMDCore *core);
  ~MxMDReplay();

  void start(
      ZtString path,
      MxDateTime begin = MxDateTime(),
      bool filter = true);
  void stop();

  inline ZtString path() const {
    Guard guard(m_lock);
    return m_path;
  }
  bool running() const { /* FIXME */ }

private:
  bool start_(Guard &guard, ZtString path);

  void replay();

private:
  MxMDCore	*m_core;
  Lock		m_lock;
    ZtString	  m_path;
    ZiFile	  m_file;
    ZmRef<Msg>	  m_msg;
    bool	  m_filter = 0;
    Version	  m_version;
    ZmTime	  m_timerNext;		// time of next timer event
};

#endif /* MxMDReplay_HPP */
