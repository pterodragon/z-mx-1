//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ZiIP::(at your option) any later version.
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

// IP address

#include <zlib/ZiIP.hpp>

#include <zlib/ZmSingleton.hpp>

#ifdef _WIN32

#if defined(__GNUC__) && !defined(GetAddrInfo)

extern "C" {
  typedef struct {
    int			ai_flags;
    int			ai_family;
    int			ai_socktype;
    int			ai_protocol;
    size_t		ai_addrlen;
    wchar_t		**ai_canonname;
    struct sockaddr	*ai_addr;
    struct addrinfoW	*ai_next;
  } ADDRINFOT;
  typedef int (WSAAPI *PGetAddrInfoW)(const wchar_t *, const wchar_t *,
				      const ADDRINFOT *, ADDRINFOT **);
  typedef void (WSAAPI *PFreeAddrInfoW)(ADDRINFOT *);
  typedef int (WSAAPI *PGetNameInfoW)(const struct sockaddr *, socklen_t,
				      wchar_t *, DWORD, wchar_t *, DWORD, int);
}

class ZiIP_WSDLL;

template <> struct ZmCleanup<ZiIP_WSDLL> {
  enum { Level = ZmCleanupLevel::Platform };
};

class ZiIP_WSDLL {
friend struct ZmSingletonCtor<ZiIP_WSDLL>;

public:
  int getAddrInfo(const wchar_t *, const wchar_t *, const ADDRINFOT *,
		  ADDRINFOT **);
  void freeAddrInfo(ADDRINFOT *);
  int getNameInfo(const struct sockaddr *, socklen_t,
		   wchar_t *, DWORD, wchar_t *, DWORD, int);

  static ZiIP_WSDLL *instance();

private:
  ZiIP_WSDLL();
public:
  ~ZiIP_WSDLL();

private:
  HMODULE		m_wsdll;
  bool			m_wsaCleanup;
  PGetAddrInfoW		m_getAddrInfoW;
  PFreeAddrInfoW	m_freeAddrInfoW;
  PGetNameInfoW		m_getNameInfoW;
};

ZiIP_WSDLL *ZiIP_WSDLL::instance()
{
  return ZmSingleton<ZiIP_WSDLL>::instance();
}

ZiIP_WSDLL::ZiIP_WSDLL()
{
  if (m_wsdll = LoadLibrary(L"ws2_32.dll")) {
    m_getAddrInfoW = (PGetAddrInfoW)GetProcAddress(m_wsdll, "GetAddrInfoW");
    m_freeAddrInfoW = (PFreeAddrInfoW)GetProcAddress(m_wsdll, "FreeAddrInfoW");
    m_getNameInfoW = (PGetNameInfoW)GetProcAddress(m_wsdll, "GetNameInfoW");
  } else {
    m_getAddrInfoW = 0;
    m_freeAddrInfoW = 0;
    m_getNameInfoW = 0;
  }
}

ZiIP_WSDLL::~ZiIP_WSDLL()
{
  FreeLibrary(m_wsdll);
}

int ZiIP_WSDLL::getAddrInfo(
  const wchar_t *node,
  const wchar_t *service,
  const ADDRINFOT *hints,
  ADDRINFOT **result)
{
  if (!m_getAddrInfoW) return WSASYSNOTREADY;
  return (*m_getAddrInfoW)(node, service, hints, result);
}

void ZiIP_WSDLL::freeAddrInfo(ADDRINFOT *ai)
{
  if (!m_freeAddrInfoW) return;
  (*m_freeAddrInfoW)(ai);
}

int ZiIP_WSDLL::getNameInfo(const struct sockaddr *sa, socklen_t salen,
		   wchar_t *host, DWORD hostlen, wchar_t *serv, DWORD servlen,
		   int flags)
{
  if (!m_getNameInfoW) return WSASYSNOTREADY;
  return (*m_getNameInfoW)(sa, salen, host, hostlen, serv, servlen, flags);
}

#define GetAddrInfo(node, service, hints, result) \
  (ZiIP_WSDLL::instance()->getAddrInfo(node, service, hints, result))
#define FreeAddrInfo(ai) (ZiIP_WSDLL::instance()->freeAddrInfo(ai))
#define GetNameInfo(sa, salen, host, hostlen, serv, servlen, flags) \
  (ZiIP_WSDLL::instance()->getNameInfo(sa, salen, host, hostlen, \
				       serv, servlen, flags))

#endif /* defined(__GNUC__) && !defined(GetAddrInfo) */

class ZiIP_WSAStartup;

template <> struct ZmCleanup<ZiIP_WSAStartup> {
  enum { Level = ZmCleanupLevel::Platform };
};

class ZiIP_WSAStartup {
friend struct ZmSingletonCtor<ZiIP_WSAStartup>;

public:
  static ZiIP_WSAStartup *instance();

private:
  ZiIP_WSAStartup();
public:
  ~ZiIP_WSAStartup();

private:
  bool		m_wsaCleanup;
};

ZiIP_WSAStartup *ZiIP_WSAStartup::instance()
{
  return ZmSingleton<ZiIP_WSAStartup>::instance();
}

ZiIP_WSAStartup::ZiIP_WSAStartup()
{
  WSADATA wd;
  m_wsaCleanup = !WSAStartup(MAKEWORD(2, 2), &wd);
}

ZiIP_WSAStartup::~ZiIP_WSAStartup()
{
  if (m_wsaCleanup) WSACleanup();
}

typedef ADDRINFOT ZiIP_AddrInfo;
#define ZiIP_GetAddrInfo GetAddrInfo
#define ZiIP_FreeAddrInfo FreeAddrInfo
#define ZiIP_GetNameInfo GetNameInfo

#define ZiIP_InitOnce() (ZiIP_WSAStartup::instance())

#else

typedef struct addrinfo ZiIP_AddrInfo;
#define ZiIP_GetAddrInfo getaddrinfo
#define ZiIP_FreeAddrInfo freeaddrinfo
#define ZiIP_GetNameInfo getnameinfo

#define ZiIP_InitOnce() ((void)0)

#endif /* _WIN32 */

int ZiIP::resolve_(ZuString host_, ZeError *e)
#ifdef _WIN32
{
  ZuWStringN<ZiPlatform::NameMax + 1> host;
  host.length(ZuUTF<wchar_t, char>::cvt(
	ZuArray<wchar_t>(host.data(), host.size() - 1), host_));
  return resolve_(ZuWString{host}, e);
}
int ZiIP::resolve_(ZuWString host_, ZeError *e)
#endif /* _WIN32 */
{
  ZiIP_InitOnce();

  ZiIP_AddrInfo hints;
  ZiIP_AddrInfo *result;
  int errno_;
 
  // ensure host is null terminated
#ifndef _WIN32
  ZuStringN<ZiPlatform::HostnameMax> host(host_);
#else
  ZuWStringN<ZiPlatform::HostnameMax> host(host_);
#endif

  memset(&hints, 0, sizeof(ZiIP_AddrInfo));
  hints.ai_family = AF_INET;
  hints.ai_protocol = PF_INET;
  while (errno_ = ZiIP_GetAddrInfo(host, 0, &hints, &result)) {
    if (errno_ == EAI_AGAIN) continue;
    if (e) *e =
#ifdef EAI_SYSTEM
      errno_ == EAI_SYSTEM ? ZeLastSockError :
#endif
      ZeError(errno_);
    return IOError;
  }
  if (!result || !result->ai_addr ||
      result->ai_addrlen < sizeof(struct sockaddr_in)) {
    if (e) *e = ZeError(EAI_NONAME);
    if (result) ZiIP_FreeAddrInfo(result);
    return IOError;
  }
  s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
  ZiIP_FreeAddrInfo(result);
  return OK;
}

ZiIP::Hostname ZiIP::name(ZeError *e)
{
  ZiIP_InitOnce();

  Hostname ret;
  struct sockaddr_in sai;
  memset(&sai, 0, sizeof(struct sockaddr_in));
  sai.sin_family = AF_INET;
  sai.sin_addr.s_addr = s_addr;
  ret.size(ZiPlatform::HostnameMax);
  int errno_;
  while (errno_ = ZiIP_GetNameInfo((struct sockaddr *)&sai,
				   sizeof(struct sockaddr_in),
				   ret, ZiPlatform::HostnameMax,
				   0, 0, 0)) {
    if (errno_ == EAI_AGAIN) continue;
    if (e) *e =
#ifdef EAI_SYSTEM
      errno_ == EAI_SYSTEM ? ZeLastSockError :
#endif
      ZeError(errno_);
    ret.null();
    return ret;
  }
  ret[ZiPlatform::HostnameMax - 1] = 0;
  ret.calcLength();
  ret.truncate();
  return ret;
}
