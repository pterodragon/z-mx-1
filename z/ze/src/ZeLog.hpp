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

// singleton logger

// ZeLog::init("program", "daemon");	// for LOG_DAEMON
// ZeLog::add(ZeLog::sysSink());	// sysSink() is syslog / event log
// ZeLOG(Debug, "debug message");	// ZeLOG() is macro
// ZeLOG(Error, ZeLastError);		// errno / GetLastError()
// ZeLOG(Error,
//	 ZtSprintf("fopen(%s) failed: %s", file, ZeLastError.message().data()));
// try { ... } catch (ZeError &e) { ZeLOG(Error, e); }

// if no sink is added at initialization, the default sink is stderr
// on Unix and the Application event log on Windows

#ifndef ZeLog_HPP
#define ZeLog_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZeLib_HPP
#include <ZeLib.hpp>
#endif

#include <stdio.h>

#include <ZuCmp.hpp>
#include <ZuString.hpp>

#include <ZmBackTrace.hpp>
#include <ZmCleanup.hpp>
#include <ZmList.hpp>
#include <ZmRWLock.hpp>
#include <ZmSemaphore.hpp>
#include <ZmThread.hpp>
#include <ZmTime.hpp>
#include <ZmNoLock.hpp>

#include <ZtString.hpp>

#include <ZePlatform.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251 4231 4660)
#endif

typedef ZmFn<ZeEvent *> ZeSinkFn;

class ZeLog;

template <> struct ZmCleanup<ZeLog> {
  enum { Level = ZmCleanupLevel::Library };
  static void final(ZeLog *);
};

class ZeAPI ZeLog : public ZmThreadMgr {
  ZeLog(const ZeLog &);
  ZeLog &operator =(const ZeLog &);		// prevent mis-use

friend class ZmSingleton<ZeLog, 1>;
friend struct ZmCleanup<ZeLog>;

  struct EventQueueID {
    inline static const char *id() { return "ZeLog.EventQueue"; }
  };
  struct SinkListID {
    inline static const char *id() { return "ZeLog.SinkList"; }
  };

  typedef ZmList<ZmRef<ZeEvent>,
	    ZmListObject<ZuNull,
	      ZmListLock<ZmNoLock,
		ZmListHeapID<EventQueueID> > > > EventQueue;

  typedef ZmList<ZeSinkFn,
	    ZmListObject<ZuNull,
	      ZmListLock<ZmRWLock,
		ZmListHeapID<SinkListID> > > > SinkList;

  class ZeAPI FileSink : public ZmPolymorph {
    typedef ZmLock Lock;
    typedef ZmGuard<Lock> Guard;

  public:
    inline FileSink() : m_file(0) { init(8, 0); }
    template <typename Name>
    inline FileSink(Name &&name, unsigned age = 8, int tzOffset = 0) :
	m_filename(ZuFwd<Name>(name)), m_file(0) { init(age, tzOffset); }

    virtual ~FileSink();

    void log(ZeEvent *e);

  private:
    void init(unsigned age, int tzOffset);

    void log_(FILE *f, ZeEvent *e);

    Lock		m_lock;
    ZtString		m_filename;
    FILE *		m_file;
    int			m_tzOffset;	// timezone offset
  };

  class ZeAPI DebugSink : public ZmPolymorph {
    typedef ZmLock Lock;
    typedef ZmGuard<Lock> Guard;

  public:
    DebugSink() : m_file(0), m_started(ZmTime::Now) { init(); }
    template <typename Name> DebugSink(Name &&name) :
	m_filename(ZuFwd<Name>(name)), m_file(0), m_started(ZmTime::Now)
      { init(); }

    virtual ~DebugSink();

    void log(ZeEvent *e);

  private:
    void init();

    void log_(FILE *f, ZeEvent *e);

    Lock		m_lock;
    ZtString		m_filename;
    FILE *		m_file;
    ZmTime		m_started;
  };

  class ZeAPI SysSink : public ZmPolymorph {
  public:
    void log(ZeEvent *e) const;
  };

  ZeLog();
public:
  virtual ~ZeLog() { }

  template <typename ...Args>
  inline static ZeSinkFn fileSink(Args &&... args) {
    return ZeSinkFn::Member<&FileSink::log>::fn(
	ZmMkRef(new FileSink(ZuFwd<Args>(args)...)));
  }
  template <typename ...Args>
  inline static ZeSinkFn debugSink(Args &&... args) {
    return ZeSinkFn::Member<&DebugSink::log>::fn(
	ZmMkRef(new DebugSink(ZuFwd<Args>(args)...)));
  }
  template <typename ...Args>
  inline static ZeSinkFn sysSink(Args &&... args) {
    return ZeSinkFn::Member<&SysSink::log>::fn(
	ZmMkRef(new SysSink(ZuFwd<Args>(args)...)));
  }

  template <typename ...Args>
  inline static void init(Args &&... args) {
    instance()->init_(ZuFwd<Args>(args)...);
  }

  inline static ZuString program() { return instance()->program_(); }

  inline static int level() { return instance()->level_(); }
  inline static void level(int l) { instance()->level_(l); }

  inline static void add(ZeSinkFn sink) { instance()->add_(sink); }
  inline static void del(ZeSinkFn sink) { instance()->del_(sink); }

  inline static void start() { instance()->start_(); }
  inline static void stop() { instance()->stop_(); }
  inline static void forked() { instance()->forked_(); }

  inline static void log(ZmRef<ZeEvent> e) { instance()->log_(ZuMv(e)); }

private:
  static ZeLog *instance();

  template <typename Program>
  inline void init_(Program &&program) {
    m_program = ZuFwd<Program>(program);
    ZePlatform::sysloginit(m_program, 0);
  }
  template <typename Program, typename Facility>
  inline void init_(Program &&program, Facility &&facility) {
    m_program = ZuFwd<Program>(program);
    ZePlatform::sysloginit(m_program, ZuFwd<Facility>(facility));
  }

  ZuString program_();

  inline int level_() const { return m_level; }
  inline void level_(int l) { m_level = l; }

  void add_(ZeSinkFn sink);
  inline void del_(ZeSinkFn sink) { m_sinks.del(sink); }

  void start_();
  void start__();
  void stop_();
  void forked_();

  void work_();
  void log_(ZmRef<ZeEvent> e);
  void log__(ZeEvent *e);

  void threadName(ZmThreadName &s, unsigned) { s = "log"; }

private:
  SinkList	m_sinks;
  ZtString	m_program;
  int		m_level;
  ZmSemaphore	m_work;
  ZmSemaphore	m_started;
  ZmLock	m_lock;
    EventQueue	  m_queue;
    ZmThread	  m_thread;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template <typename Msg>
auto ZuInline Ze_BackTrace_fn(ZmBackTrace bt, Msg &&msg) {
  return [bt, fn = ZeMessageFn(ZuFwd<Msg>(msg))](
      const ZeEvent &e, ZmStream &s) {
    fn(e, s);
    s << '\n' << bt;
  };
}

#define Ze_ERROR_(sev, e) ZeLog::log(ZeEVENT_(sev, (e)))
#define Ze_LOG_(sev, msg) ZeLog::log(ZeEVENT_(sev, msg))
#define Ze_BackTrace_(sev, msg) \
  do { \
    ZmBackTrace bt__; \
    bt__.capture(0); \
    ZeLog::log(ZeEVENT_(sev, Ze_BackTrace_fn(bt__, msg))); \
  } while (0)

#ifndef ZDEBUG

// filter out DEBUG messages in production builds
#define ZeERROR_(sev, e) \
  ((sev > Ze::Debug) ? (void)Ze_ERROR_(sev, e) : (void)0)
#define ZeLOG_(sev, msg) \
  ((sev > Ze::Debug) ? (void)Ze_LOG_(sev, msg) : (void)0)
#define ZeBackTrace_(sev, msg) \
  do { if (sev > Ze::Debug) Ze_BackTrace_(sev, msg); } while (0)

#else /* !ZEBUG */

#define ZeERROR_(sev, e) Ze_ERROR_(sev, e)
#define ZeLOG_(sev, msg) Ze_LOG_(sev, msg)
#define ZeBackTrace_(sev, msg) Ze_BackTrace_(sev, msg)

#endif /* !ZEBUG */

#define ZeERROR(sev, e) ZeERROR_(Ze:: sev, e)
#define ZeLOG(sev, msg) ZeLOG_(Ze:: sev, msg)
#define ZeBackTrace(sev, msg) ZeBackTrace_(Ze:: sev, msg)

#endif /* ZeLog_HPP */
