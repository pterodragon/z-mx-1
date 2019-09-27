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

// daemon-ization

#ifndef ZvDaemon_HPP
#define ZvDaemon_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <zlib/ZvLib.hpp>
#endif

class ZvAPI ZvDaemon {
  ZvDaemon() = delete;
  ZvDaemon(const ZvDaemon &) = delete;
  ZvDaemon &operator =(const ZvDaemon &) = delete;

public:
  enum { OK = 0, Error = -1, Running = -2 };

  // * Unix
  //   - if daemonize is true the calling program is forked
  //   - if username is set the calling program must be running as root
  //   - password is ignored
  //
  // * Windows
  //   - the calling program is terminated and re-invoked regardless
  //   - if username is set, the calling user must have the
  //	 "Replace a process level token" right; this is granted by default
  //	 to local and network services but not to regular programs run by
  //	 Administrators; go to Control Panel / Administrative Tools /
  //	 Local Security Policy and add the users/groups needed
  //   - umask is ignored and has no effect
  //
  // * If forked (Unix) or re-invoked (Windows):
  //   - All running threads will be terminated
  //   - All open files and sockets will be closed
  //
  // * If daemonize is true
  //   - Standard input, output and error are closed and become unavailable

  static int init(
      const char *username,
      const char *password,
      int umask,
      bool daemonize,
      const char *pidFile);
};

#endif /* ZvDaemon_HPP */
