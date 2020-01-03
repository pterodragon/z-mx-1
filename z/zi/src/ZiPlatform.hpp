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

// platform primitives

#ifndef ZiPlatform_HPP
#define ZiPlatform_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <zlib/ZiLib.hpp>
#endif

#ifndef _WIN32

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <netdb.h>

#ifdef NETLINK
#include <linux/netlink.h>
#include <linux/genetlink.h>
#endif

#else /* !_WIN32 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#endif /* !_WIN32 */

#include <zlib/ZuInt.hpp>
#include <zlib/ZuTraits.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZtString.hpp>

struct ZiAPI ZiPlatform {
#ifndef _WIN32
  typedef int Handle;
  ZuInline static constexpr const Handle nullHandle() { return -1; }
  ZuInline static constexpr bool nullHandle(Handle i) { return i < 0; }
  typedef int Socket;
  ZuInline static constexpr const Socket nullSocket() { return -1; }
  ZuInline static constexpr bool nullSocket(Socket i) { return i < 0; }
  inline static void closeSocket(Socket s) { ::close(s); }
  typedef void *MMapPtr;
  typedef ZtString Path;
  typedef off_t Offset;
  typedef ZtString Hostname;
  typedef ZtString Username;
  enum {
    PathMax = PATH_MAX,
    NameMax = NAME_MAX,
    NVecMax = IOV_MAX
  };
#else
  typedef HANDLE Handle;
  ZuInline static const Handle nullHandle()
    { return INVALID_HANDLE_VALUE; }
  ZuInline static bool nullHandle(Handle i)
    { return !i || i == INVALID_HANDLE_VALUE; }
  typedef SOCKET Socket;
  ZuInline static constexpr const Socket nullSocket()
    { return INVALID_SOCKET; }
  ZuInline static constexpr bool nullSocket(Socket i)
    { return i == INVALID_SOCKET; }
  inline static void closeSocket(Socket s) { ::closesocket(s); }
  typedef LPVOID MMapPtr;
  typedef ZtWString Path;
  typedef int64_t Offset;		// 2x DWORD
  typedef ZtWString Hostname;
  typedef ZtWString Username;
  enum {
    PathMax = 32767,	// NTFS limit (MAX_PATH is 260 for FAT)
    NameMax = 255,	// NTFS & FAT limit
    NVecMax = 2048	// Undocumented system-wide limit
  };
#endif

  enum {
    HostnameMax = NI_MAXHOST,
    ServicenameMax = NI_MAXSERV
  };

  static Username username(ZeError *e = 0); /* effective username */
  static Hostname hostname(ZeError *e = 0);
};

#ifndef _WIN32
typedef struct iovec ZiVec;
typedef void *ZiVecPtr;
typedef size_t ZiVecLen;
#define ZiVec_ptr(x) (x).iov_base
#define ZiVec_len(x) (x).iov_len
#else
typedef WSABUF ZiVec;
typedef char *ZiVecPtr;
typedef u_long ZiVecLen;
#define ZiVec_ptr(x) (x).buf
#define ZiVec_len(x) (x).len
#endif

#ifndef _WIN32
#include <dlfcn.h>	// dlerror()
#include <zlib/ZePlatform.hpp>
#endif

#endif /*  ZiPlatform_HPP */
