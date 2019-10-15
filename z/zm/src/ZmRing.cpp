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

// ring buffer

#include <zlib/ZmRing.hpp>

#ifdef linux

#include <sys/syscall.h>
#include <linux/futex.h>

int ZmRing_::open() { return OK; }

int ZmRing_::close() { return OK; }

int ZmRing_::wait(ZmAtomic<uint32_t> &addr, uint32_t val)
{
  // fprintf(stderr, "WAIT addr: %#.8x (%#.8x) val: %#.8x\n", (unsigned)(uintptr_t)(void *)&addr, (unsigned)addr.load_(), (unsigned)val);
  if (addr.cmpXch(val | Waiting, val) != val) return OK;
  val |= Waiting;
  if (ZuUnlikely(params().timeout())) {
    ZmTime out(ZmTime::Now, (int)params().timeout());
    unsigned i = 0, n = params().spin();
    do {
      if (ZuUnlikely(i >= n)) {
	if (syscall(SYS_futex, (volatile int *)&addr,
	      FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME,
	      (int)val, &out, 0, FUTEX_BITSET_MATCH_ANY) < 0)
	  if (errno == ETIMEDOUT) return NotReady;
	i = 0;
      } else
	++i;
    } while (addr == val);
  } else {
    unsigned i = 0, n = params().spin();
    do {
      if (ZuUnlikely(i >= n)) {
	syscall(SYS_futex, (volatile int *)&addr,
	    FUTEX_WAIT | FUTEX_PRIVATE_FLAG, (int)val, 0, 0, 0);
	i = 0;
      } else
	++i;
    } while (addr == val);
  }
  return OK;
}

int ZmRing_::wake(ZmAtomic<uint32_t> &addr, int n)
{
  // fprintf(stderr, "WAKE addr: %#.8x (%#.8x) n:%u\n", (unsigned)(uintptr_t)(void *)&addr, (unsigned)addr.load_(), (unsigned)n);
  addr &= ~Waiting;
  syscall(SYS_futex, (volatile int *)&addr,
      FUTEX_WAKE | FUTEX_PRIVATE_FLAG, n, 0, 0, 0);
  return OK;
}

#endif /* linux */

#ifdef _WIN32

int ZmRing_::open()
{
  if (m_sem[Head]) return OK;
  if (!(m_sem[Head] = CreateSemaphore(0, 0, 0x7fffffff, 0))) return Error;
  if (!(m_sem[Tail] = CreateSemaphore(0, 0, 0x7fffffff, 0))) {
    CloseHandle(m_sem[Head]);
    m_sem[Head] = 0;
    return Error;
  }
  return OK;
}

int ZmRing_::close()
{
  if (!m_sem[Head]) return OK;
  bool error = false;
  if (!CloseHandle(m_sem[Head])) error = true;
  if (!CloseHandle(m_sem[Tail])) error = true;
  m_sem[Head] = m_sem[Tail] = 0;
  return error ? Error : OK;
}

int ZmRing_::wait(unsigned index, ZmAtomic<uint32_t> &addr, uint32_t val)
{
  if (addr.cmpXch(val | Waiting, val) != val) return OK;
  val |= Waiting;
  DWORD timeout = params().timeout() ? params().timeout() * 1000 : INFINITE;
  unsigned i = 0, n = params().spin();
  do {
    if (ZuUnlikely(i >= n)) {
      DWORD r = WaitForSingleObject(m_sem[index], timeout);
      switch ((int)r) {
	case WAIT_OBJECT_0:	return OK;
	case WAIT_TIMEOUT:	return NotReady;
	default:		return Error;
      }
      i = 0; // not reached
    } else
      ++i;
  } while (addr == val);
  return OK;
}

int ZmRing_::wake(unsigned index, int n)
{
  addr &= ~Waiting;
  ReleaseSemaphore(m_sem[index], n, 0);
  return OK;
}

#endif /* _WIN32 */
