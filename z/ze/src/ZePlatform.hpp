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

// platform primitives

#ifndef ZePlatform_HPP
#define ZePlatform_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZeLib_HPP
#include <zlib/ZeLib.hpp>
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <zlib/ZuTraits.hpp>
#include <zlib/ZuFnName.hpp>
#include <zlib/ZuStringN.hpp>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmSingleton.hpp>
#include <zlib/ZmHeap.hpp>
#include <zlib/ZmStream.hpp>
#include <zlib/ZmList.hpp>

#include <zlib/ZtDate.hpp>
#include <zlib/ZtString.hpp>
#include <zlib/ZtEnum.hpp>

// ZePlatform::ErrNo is a regular native OS error code
//
// In addition to the native OS error code, need to handle the EAI_ error
// codes returned by getaddrinfo() (Windows - GetAddrInfo())
//
//   Windows	   - EAI_ codes are #defined identically to equiv. system codes
//   Linux/Solaris - EAI_ codes are negative, all errno codes are positive
//
// Both types of platform also use a signed integer type for system errors,
// so both sets of codes can be stored in the same type; However,
// strerror() on Unix will not work with EAI_ codes - on Unix we
// need to check for < 0 explicitly and call gai_strerror()

#define ZeLog_BUFSIZ (32<<10)	// caps individual log message size to 32k

namespace Ze {
  ZtEnumValues("Ze", Debug, Info, Warning, Error, Fatal);
}

struct ZeAPI ZePlatform_ {
#ifndef _WIN32
  using ErrNo = int;
  static constexpr ErrNo OK = 0;

  static ErrNo errNo() { return errno; }
  static ErrNo sockErrNo() { return errno; }
  static const char *strerror(ErrNo e) {
    return e >= 0 ? ::strerror(e) : gai_strerror(e);
  }
#else
  using ErrNo = DWORD;				// <= sizeof(int)
  static constexpr ErrNo OK = ERROR_SUCCESS;	// == 0

  static ErrNo errNo() { return GetLastError(); }
  static ErrNo sockErrNo() { return WSAGetLastError(); }
  static const char *strerror(ErrNo e);
#endif
};

class ZeError {
public:
  using ErrNo = ZePlatform_::ErrNo;
  static const ErrNo OK = ZePlatform_::OK;

  ZeError() : m_errNo(OK) { }

  ZeError(const ZeError &) = default;
  ZeError &operator =(const ZeError &) = default;
  ZeError(ZeError &&) = default;
  ZeError &operator =(ZeError &&) = default;

  ZeError(ErrNo e) : m_errNo(e) { }
  ZeError &operator =(ErrNo e) {
    m_errNo = e;
    return *this;
  }

  ErrNo errNo() const { return m_errNo; }
  const char *message() const {
    return ZePlatform_::strerror(m_errNo);
  }

  bool operator !() const { return m_errNo == OK; }
  ZuOpBool

  template <typename S> ZuInline void print(S &s) const { s << message(); }

  struct Traits : public ZuBaseTraits<ZeError> { enum { IsPOD = 1 }; };
  friend Traits ZuTraitsType(ZeError *);

private:
  ErrNo		m_errNo;
};

inline ZeError Ze_OK() { return ZeError(); }
#define ZeOK Ze_OK()

inline ZeError Ze_LastError() { return ZeError(ZePlatform_::errNo()); }
#define ZeLastError Ze_LastError()

inline ZeError Ze_LastSockError() { return ZeError(ZePlatform_::sockErrNo()); }
#define ZeLastSockError Ze_LastSockError()

template <> struct ZuPrint<ZeError> : public ZuPrintFn { };

template <typename Event>
class ZeMessageFn_ : public ZmFn<const Event &, ZmStream &> {
public:
  using Fn = ZmFn<const Event &, ZmStream &>;

  ZeMessageFn_() { }
  ZeMessageFn_(const ZeMessageFn_ &fn) : Fn(fn) { }
  ZeMessageFn_ &operator =(const ZeMessageFn_ &fn) {
    Fn::operator =(fn);
    return *this;
  }
  ZeMessageFn_(ZeMessageFn_ &&fn) noexcept :
    Fn(static_cast<Fn &&>(fn)) { }
  ZeMessageFn_ &operator =(ZeMessageFn_ &&fn) noexcept {
    Fn::operator =(static_cast<Fn &&>(fn));
    return *this;
  }

private:
  template <typename U> struct IsLiteral {
    enum { OK = ZuTraits<U>::IsArray &&
      ZuTraits<U>::IsPrimitive && ZuTraits<U>::IsCString &&
      ZuConversion<typename ZuTraits<U>::Elem, const char>::Same };
  };
  template <typename U, typename R = void>
  using MatchLiteral = ZuIfT<IsLiteral<U>::OK, R>;

  template <typename U> struct IsPrint {
    enum { OK = !IsLiteral<U>::OK &&
      (ZuTraits<U>::IsString || ZuPrint<U>::OK) };
  };
  template <typename U, typename R = void>
  using MatchPrint = ZuIfT<IsPrint<U>::OK, R>;

  template <typename U, typename R = void>
  using MatchFn = ZuIfT<!IsLiteral<U>::OK && !IsPrint<U>::OK, R>;

public:
  // from string literal
  template <typename P>
  ZeMessageFn_(P &&p, MatchLiteral<P> *_ = 0) :
    Fn([p = ZuString(p)](const Event &, ZmStream &s) { s << p; }) { }

  // from something printable (that's not a string literal)
  template <typename P>
  ZeMessageFn_(P &&p, MatchPrint<P> *_ = 0) :
    Fn([p = ZuFwd<P>(p)](const Event &, ZmStream &s) { s << p; }) { }

  // fwd anything else to ZmFn
  template <typename P>
  ZeMessageFn_(P &&p, MatchFn<P> *_ = 0) :
    Fn(ZuFwd<P>(p)) { }
  template <typename P1, typename P2, typename ...Args>
  ZeMessageFn_(P1 &&p1, P2 &&p2, Args &&... args) :
    Fn(ZuFwd<P1>(p1), ZuFwd<P2>(p2), ZuFwd<Args>(args)...) { }
};

using ZeEvent_Queue =
  ZmList<ZmObject,
    ZmListNodeIsItem<true,
      ZmListObject<ZmObject,
	ZmListLock<ZmNoLock,
	  ZmListHeapID<ZuNull> > > > >;
template <typename Heap>
class ZeEvent_ : public Heap, public ZeEvent_Queue::Node {
  ZeEvent_(const ZeEvent_ &) = delete;
  ZeEvent_ &operator =(const ZeEvent_ &) = delete;

  using ThreadID = ZmPlatform::ThreadID;

public:
  using MessageFn = ZeMessageFn_<ZeEvent_>;

  // from anything else
  template <typename Msg>
  ZeEvent_(int severity, const char *filename, int lineNumber,
      const char *function, Msg &&msg) :
    m_time{ZmTime::Now}, m_tid{ZmPlatform::getTID()},
    m_severity{severity}, m_filename{filename},
    m_lineNumber{lineNumber}, m_function{function},
    m_messageFn{ZuFwd<Msg>(msg)} { }

  ZmTime time() const { return m_time; }
  ZmPlatform::ThreadID tid() const { return m_tid; }
  int severity() const { return m_severity; }
  const char *filename() const { return m_filename; }
  int lineNumber() const { return m_lineNumber; }
  const char *function() const { return m_function; }
  MessageFn messageFn() const { return m_messageFn; }

  struct Message {
    void print(ZmStream &s) const { e.messageFn()(e, s); }
    template <typename S> void print(S &s_) const {
      ZmStream s(s_);
      e.messageFn()(e, s);
    }
    const ZeEvent_	&e;
    friend ZuPrintFn ZuPrintType(Message *);
  };

  Message message() const { return Message{*this}; }
  template <typename S> void print(S &s) const { message().print(s); }
  friend ZuPrintFn ZuPrintType(ZeEvent_ *);

private:
  ZmTime	m_time;
  ThreadID	m_tid;
  int		m_severity;	// Ze
  const char	*m_filename;
  int		m_lineNumber;
  const char	*m_function;
  MessageFn	m_messageFn;
};
struct ZeEvent_HeapID {
  static constexpr const char *id() { return "ZeEvent"; }
};
using ZeEvent_Heap = ZmHeap<ZeEvent_HeapID, sizeof(ZeEvent_<ZuNull>)>;
using ZeEvent = ZeEvent_<ZeEvent_Heap>;
using ZeMessageFn = ZeEvent::MessageFn;

class ZeAPI ZePlatform : public ZePlatform_ {
public:
  static void sysloginit(const char *program, const char *facility = 0);
  static void syslog(ZeEvent *e);

  static ZuString severity(unsigned i);
  static ZuString filename(ZuString s);
  static ZuString function(ZuString s);
};

#define ZeEVENT_(sev, msg) \
  (new ZeEvent(sev, __FILE__, __LINE__, ZuFnName, msg))
#define ZeEVENT(sev, msg) ZeEVENT_(Ze:: sev, msg)

#endif /* ZePlatform_HPP */
