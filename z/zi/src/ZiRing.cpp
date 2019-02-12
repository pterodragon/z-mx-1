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

// shared memory ring buffer IPC with fan-out to multiple readers (up to 64)

#include <ZuStringN.hpp>

#include <ZiRing.hpp>

void ZiRing_::getpinfo(uint32_t &pid, ZmTime &start)
{
#ifdef linux
  pid = getpid();
  ZuStringN<20> path; path << "/proc/" << ZuBox<uint32_t>(pid);
  struct stat s;
  start = (::stat(path, &s) < 0) ? ZmTime() : ZmTime(s.st_ctim);
#endif
#ifdef _WIN32
  pid = GetCurrentProcessId();
  FILETIME creation, exit, kernel, user;
  start = !GetProcessTimes(
      GetCurrentProcess(), &creation, &exit, &kernel, &user) ?
    ZmTime() : ZmTime(creation);
#endif
}

bool ZiRing_::alive(uint32_t pid, ZmTime start)
{
  if (!pid) return false;
#ifdef linux
  ZuStringN<20> path; path << "/proc/" << ZuBox<uint32_t>(pid);
  struct stat s;
  if (::stat(path, &s) < 0) return false;
  return !start || ZmTime(s.st_ctim) == start;
#endif
#ifdef _WIN32
  HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
  if (!h || h == INVALID_HANDLE_VALUE) return false;
  DWORD r = WaitForSingleObject(h, 0);
  if (r != WAIT_TIMEOUT) { CloseHandle(h); return false; }
  FILETIME creation, exit, kernel, user;
  if (!GetProcessTimes(h, &creation, &exit, &kernel, &user))
    { CloseHandle(h); return false; }
  return !start || ZmTime(creation) == start;
  CloseHandle(h);
#endif
}

bool ZiRing_::kill(uint32_t pid, bool coredump)
{
  if (!pid) return false;
#ifdef linux
  return ::kill(pid, coredump ? SIGQUIT : SIGKILL) >= 0;
#endif
#ifdef _WIN32
  HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (!h || h == INVALID_HANDLE_VALUE) return false;
  bool ok = TerminateProcess(h, (unsigned)-1) == TRUE;
  CloseHandle(h);
  return ok;
#endif
}

#ifdef linux

#include <sys/syscall.h>
#include <linux/futex.h>

int ZiRing_::open(ZeError *e) { return Zi::OK; }

int ZiRing_::close(ZeError *e) { return Zi::OK; }

int ZiRing_::wait(ZmAtomic<uint32_t> &addr, uint32_t val)
{
  if (addr.cmpXch(val | Waiting, val) != val) return Zi::OK;
  val |= Waiting;
  if (ZuUnlikely(params().timeout())) {
    ZmTime out(ZmTime::Now, (int)params().timeout());
    unsigned i = 0, n = params().spin();
    do {
      if (ZuUnlikely(i >= n)) {
	if (syscall(SYS_futex, (volatile int *)&addr,
	      FUTEX_WAIT_BITSET | FUTEX_CLOCK_REALTIME,
	      (int)val, &out, 0, FUTEX_BITSET_MATCH_ANY) >= 0) return Zi::OK;
	switch (errno) {
	  case EINTR:    break;
	  case EAGAIN:   addr.cmpXch(val & ~Waiting, val); return Zi::OK;
	  case ETIMEDOUT:addr.cmpXch(val & ~Waiting, val); return Zi::NotReady;
	  default:       addr.cmpXch(val & ~Waiting, val); return Zi::IOError;
	}
	i = 0;
      } else
	++i;
    } while (addr == val);
  } else {
    unsigned i = 0, n = params().spin();
    do {
      if (ZuUnlikely(i >= n)) {
	if (syscall(SYS_futex, (volatile int *)&addr,
	      FUTEX_WAIT, (int)val, 0, 0, 0) >= 0) return Zi::OK;
	switch (errno) {
	  case EINTR:		break;
	  case EAGAIN:	 addr.cmpXch(val & ~Waiting, val); return Zi::OK;
	  default:	 addr.cmpXch(val & ~Waiting, val); return Zi::IOError;
	}
	i = 0;
      } else
	++i;
    } while (addr == val);
  }
  return Zi::OK;
}

int ZiRing_::wake(ZmAtomic<uint32_t> &addr, uint32_t n)
{
  if (syscall(SYS_futex, (volatile int *)&addr,
	FUTEX_WAKE, (int)n, 0, 0, 0) < 0)
    return Zi::IOError;
  return Zi::OK;
}

#endif /* linux */

#ifdef _WIN32

int ZiRing_::open(ZeError *e)
{
  if (m_sem[Head]) return Zi::OK;
  ZiFile::Path path(m_params.name().length() + 16);
  path << L"Global\\" << m_params.name() << L".sem";
  if (!(m_sem[Head] = CreateSemaphore(0, 0, 0x7fffffff, path))) {
    if (e) *e = ZeLastError;
    return Zi::IOError;
  }
  if (!(m_sem[Tail] = CreateSemaphore(0, 0, 0x7fffffff, path))) {
    if (e) *e = ZeLastError;
    CloseHandle(m_sem[Head]);
    m_sem[Head] = 0;
    return Zi::IOError;
  }
  return Zi::OK;
}

int ZiRing_::close(ZeError *e)
{
  if (!m_sem[Head]) return Zi::OK;
  bool error = false;
  if (!CloseHandle(m_sem[Head])) { if (e) *e = ZeLastError; error = true; }
  if (!CloseHandle(m_sem[Tail])) { if (e) *e = ZeLastError; error = true; }
  m_sem[Head] = m_sem[Tail] = 0;
  return error ? Zi::IOError : Zi::OK;
}

int ZiRing_::wait(unsigned index, ZmAtomic<uint32_t> &addr, uint32_t val)
{
  if (addr.cmpXch(val | Waiting, val) != val) return Zi::OK;
  val |= Waiting;
  DWORD timeout = params().timeout() ? params().timeout() * 1000 : INFINITE;
  unsigned i = 0, n = params().spin();
  do {
    if (ZuUnlikely(i >= n)) {
      DWORD r = WaitForSingleObject(m_sem[index], timeout);
      addr.cmpXch(val & ~Waiting, val);
      switch ((int)r) {
	case WAIT_OBJECT_0:	return Zi::OK;
	case WAIT_TIMEOUT:	return Zi::NotReady;
	default:		return Zi::IOError;
      }
      i = 0; // not reached
    } else
      ++i;
  } while (addr == val);
  return Zi::OK;
}

int ZiRing_::wake(unsigned index, unsigned n)
{
  ReleaseSemaphore(m_sem[index], n, 0);
  return Zi::OK;
}

#endif /* _WIN32 */
