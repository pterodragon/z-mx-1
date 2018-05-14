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

#include <ZuBox.hpp>

#include <ZmPlatform.hpp>
#include <ZmSingleton.hpp>

#include <ZtArray.hpp>
#include <ZtDate.hpp>
#include <ZtString.hpp>
#include <ZtRegex.hpp>

#include <ZeLog.hpp>

void ZmCleanup<ZeLog>::final(ZeLog *log)
{
  try { log->stop_(); } catch (...) { }
}

ZeLog::ZeLog() : m_level(1) { }

ZeLog *ZeLog::instance()
{
  return ZmSingleton<ZeLog>::instance();
}

void ZeLog::add_(ZeSinkFn fn)
{
  {
    SinkList::ReadIterator i(m_sinks);
    while (ZeSinkFn fn_ = i.iterate()) if (fn == fn_) return;
  }
  m_sinks.add(fn);
}

void ZeLog::start_()
{
  ZmGuard<ZmLock> guard(m_lock);
  if (!!m_thread) return;
  start__();
}

void ZeLog::start__()
{
  m_thread = ZmThread(
      this, 0, ZmFn<>::Member<&ZeLog::work_>::fn(this),
      ZmThreadParams().priority(ZmThreadPriority::Low));
  m_started.wait();
}

void ZeLog::forked_()
{
  ZmGuard<ZmLock> guard(m_lock);
  start__();
}

void ZeLog::stop_()
{
  ZmThread thread;
  {
    ZmGuard<ZmLock> guard(m_lock);
    thread = m_thread;
    m_thread = 0;
  }
  if (!thread) return;
  m_work.post();
  thread.join();
}

void ZeLog::work_()
{
  m_started.post();
  for (;;) {
    m_work.wait();
    ZmRef<ZeEvent> event;
    {
      ZmGuard<ZmLock> guard(m_lock);
      event = m_queue.shift();
    }
    if (!event) return;
    log__(event);
  }
}

void ZeLog::log_(ZmRef<ZeEvent> e)
{
  if (!e) return;
  if ((int)e->severity() < m_level) return;
  {
    ZmGuard<ZmLock> guard(m_lock);
    m_queue.push(ZuMv(e));
  }
  m_work.post();
}

void ZeLog::log__(ZeEvent *e)
{
  if (ZuUnlikely(!m_sinks.count())) {
#ifdef _WIN32
    m_sinks.add(sysSink());	// on Windows, default to the event log
#else
    m_sinks.add(fileSink());	// on Unix, default to stderr
#endif
  }

  SinkList::ReadIterator i(m_sinks);
  while (const ZeSinkFn &fn = i.iterate()) fn(e);
}

struct ZeLog_Buf;
template <> struct ZmCleanup<ZeLog_Buf> {
  enum { Level = ZmCleanupLevel::Platform };
};
struct ZeLog_Buf : public ZmObject {
  ZuStringN<ZeLog_BUFSIZ>	s;
  ZtDate::CSVFmt		dateFmt;
};
static ZeLog_Buf *logBuf()
{
  ZeLog_Buf *buf = ZmSpecific<ZeLog_Buf>::instance();
  buf->s.null();
  return buf;
}

void ZeLog::FileSink::init(unsigned age, int tzOffset)
{
  m_tzOffset = tzOffset;

  if (!m_filename) m_filename << ZeLog::program() << ".log";

  if (m_filename != "&2") {
    // age log files
    unsigned size = m_filename.size() + ZuBoxed(age).length() + 1;
    for (unsigned i = age; i > 0; i--) {
      ZtString oldName(size);
      oldName << m_filename;
      if (i > 1) oldName << '.' << ZuBoxed(i - 1);
      ZtString newName(size);
      newName << m_filename << '.' << ZuBoxed(i);
      if (i == age) ::remove(newName);
      ::rename(oldName, newName);
    }
    m_file = fopen(m_filename, "w");
  }

  if (!m_file)
    m_file = stderr;
  else
    setvbuf(m_file, 0, _IOLBF, ZeLog_BUFSIZ);
}

ZeLog::FileSink::~FileSink()
{
  if (m_file) fclose(m_file);
}

// suppress security warning about fopen()
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

void ZeLog::FileSink::log(ZeEvent *e)
{
  log_(m_file, e);
}

void ZeLog::FileSink::log_(FILE *f, ZeEvent *e)
{
  ZeLog_Buf *buf = logBuf();

  buf->dateFmt.offset(m_tzOffset);
  buf->s.null();

  ZtDate d{e->time()};

  buf->s <<
    d.csv(buf->dateFmt) << ' ' <<
    ZuBoxed(e->tid()) << ' ' <<
    ZePlatform::severity(e->severity()) << ' ';
  if (e->severity() == Ze::Debug || e->severity() == Ze::Fatal)
    buf->s <<
      '\"' << ZePlatform::filename(e->filename()) << "\":" <<
      ZuBoxed(e->lineNumber()) << ' ';
  buf->s <<
    ZePlatform::function(e->function()) << "() " <<
    e->message() << '\n';

  unsigned len = buf->s.length();

  if (buf->s[len - 1] != '\n') buf->s[len - 1] = '\n';

  fwrite(buf->s.data(), 1, len, f);

  if (e->severity() > Ze::Debug) fflush(f);
}

void ZeLog::SysSink::log(ZeEvent *e) const
{
  ZePlatform::syslog(e);
}

void ZeLog::DebugSink::init()
{
  m_filename << ZeLog::program() << ".log." << ZuBoxed(ZmPlatform::getPID());

  if (m_filename != "&2") m_file = fopen(m_filename, "w");

  if (!m_file)
    m_file = stderr;
  else
    setvbuf(m_file, 0, _IOLBF, ZeLog_BUFSIZ);
}

ZeLog::DebugSink::~DebugSink()
{
  if (m_file) fclose(m_file);
}

void ZeLog::DebugSink::log(ZeEvent *e)
{
  log_(m_file, e);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void ZeLog::DebugSink::log_(FILE *f, ZeEvent *e)
{
  ZeLog_Buf *buf = logBuf();

  buf->s.null();

  ZmTime d = e->time() - m_started;

  buf->s <<
    '+' << ZuBoxed(d.dtime()).fmt(ZuFmt::FP<9>()) << ' ' <<
    ZuBoxed(e->tid()) << ' ' <<
    ZePlatform::severity(e->severity()) << ' ';
  if (e->severity() == Ze::Debug || e->severity() == Ze::Fatal)
    buf->s <<
      '\"' << ZePlatform::filename(e->filename()) << "\":" <<
      ZuBoxed(e->lineNumber()) << ' ';
  buf->s <<
    ZePlatform::function(e->function()) << "() " <<
    e->message() << '\n';

  unsigned len = buf->s.length();

  if (buf->s[len - 1] != '\n') buf->s[len - 1] = '\n';

  fwrite(buf->s.data(), 1, len, f);

  fflush(f);
}
