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

/* test program */

#include <new>

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <alloca.h>
#endif

#include <ZmTime.hpp>
#include <ZmHeap.hpp>
#include <ZmThread.hpp>
#include <ZmFn.hpp>

static bool verbose = false;

template <typename Heap> struct S_ : public Heap {
  inline S_(int i) : m_i(i) { }
  inline ~S_() { m_i = -1; }
  inline void doit() {
    if (verbose) { printf("hello world %d\n", m_i); fflush(stdout); }
    if (m_i < 0) __builtin_trap();
  }
  int m_i;
};
struct ID {
  inline static const char *id() { return "S"; }
};
typedef S_<ZmHeap<ID, sizeof(S_<ZuNull>)> > S;

static unsigned count = 0;

void doit()
{
  for (unsigned i = 0; i < count; i++) {
    S *s = new S(i);
    s->doit();
    delete s;
  }
}

void usage()
{
  fputs(
"usage: ZmHeapTest COUNT SIZE NTHR [VERB]\n\n"
"    COUNT\t- number of iterations\n"
"    SIZE\t- size of heap\n"
"    NTHR\t- number of threads\n"
"    VERB\t- verbose (0 | 1 - defaults to 0)\n"
, stderr);
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc < 4 || argc > 5) usage();
  count = atoi(argv[1]);
  int size = atoi(argv[2]);
  int nthr = atoi(argv[3]);
  if (argc == 5) verbose = atoi(argv[4]);
  if (!count || !nthr) usage();
  ZmHeapMgr::init("S", 0, ZmHeapConfig{0, (unsigned)size});
  ZmThread *threads;
#ifdef __GNUC__
  threads = (ZmThread *)alloca(sizeof(ZmThread) * nthr);
#endif
#ifdef _MSC_VER
  __try {
    threads = (ZmThread *)_alloca(sizeof(ZmThread) * nthr);
  } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
    _resetstkoflw();
    threads = 0;
  }
#endif
  if (!threads) {
    fputs("alloca() failed\n", stderr);
    exit(1);
  }
  ZmTime start(ZmTime::Now);
  for (int i = 0; i < nthr; i++)
    new (&threads[i]) ZmThread(0, 0, ZmFn<>::Ptr<&doit>::fn());
  for (int i = 0; i < nthr; i++)
    threads[i].join();
  ZmTime end(ZmTime::Now);
  end -= start;
  printf("%u.%09u\n", (unsigned)end.sec(), (unsigned)end.nsec());
  std::cout << ZmHeapMgr::csv();
}
