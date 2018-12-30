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

// scheduler test program

#include <ZuLib.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <ZmFn.hpp>
#include <ZmScheduler.hpp>
#include <ZmBackoff.hpp>
#include <ZmTimeout.hpp>

class Job : public ZmPolymorph {
public:
  inline Job(const char *message, ZmTime timeout) :
	m_message(message), m_timeout(timeout) { }
  inline ~Job() {
    printf("~%s [%d]\n", m_message, (int)ZmThreadContext::self()->id());
    ::free((void *)m_message);
  }

  void *operator()() {
    printf("%s [%d]\n", m_message, (int)ZmThreadContext::self()->id());
    return 0;
  }

  inline ZmTime timeout() { return m_timeout; }

private:
  const char	*m_message;
  ZmTime	m_timeout;
};

class Timer : public ZmTimeout, public ZmObject {
public:
  inline Timer(ZmScheduler *s, const ZmBackoff &t) : ZmTimeout(s, t, -1) { }

  void retry() {
    ZmTime now = ZmTimeNow();

    printf("%d %ld\n", (int)now.sec(), (long)now.nsec());
  }
};

void dumpThread(ZmThreadContext *c)
{
  ZmThreadName s;
  c->name(s);
  if (!s)
    printf("%u\n", (unsigned)c->tid());
  else
    printf("%u %.*s\n", (unsigned)c->tid(), s.length(), s.data());
}

#include <signal.h>

void segv(int s)
{
  printf("%d/%d: SEGV\n", (int)ZmPlatform::getPID(), (int)ZmPlatform::getTID());
  fflush(stdout);
  while (-1);
}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

void usage()
{
  fputs("usage: ZmSchedTest [OPTIONS]...\n\nOptions:\n"
"    -n N\tset number of threads to N\n"
    , stderr);
  ZmPlatform::exit(1);
}

void fail(const char *s) 
{
  printf("FAIL: %s\n", s);
}

#define test(t, x) \
  (((ZuStringN<32>() << t(x)) == x) ? (void)0 : fail(#t " \"" x "\""))
#define test2(t, x, y) \
  (((ZuStringN<32>() << t(x)) == y) ? (void)0 : \
   fail(#t " \"" x "\" != \"" y "\""))

int main(int argc, char **argv)
{
  {
    test(ZmBitmap, "");
    test2(ZmBitmap, ",", "");
    test2(ZmBitmap, ",,", "");
    test(ZmBitmap, "0-");
    test(ZmBitmap, "0,3-");
    test(ZmBitmap, "3-");
    test(ZmBitmap, "3-5,7");
    test(ZmBitmap, "3-5,7,9-");
    test(ZmAffinity, "");
    test2(ZmAffinity, ":", "");
    test2(ZmAffinity, "::", "");
    test(ZmAffinity, "0=0-");
    test(ZmAffinity, "1=0-:2=3-5,7:4=3-");
  }

  ZuBox<unsigned> nThreads = 1;
  ZmAffinity affinity;
  ZmBitmap isolation;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') usage();
    switch (argv[i][1]) {
      default:
	usage();
	break;
      case 'n':
	if (++i >= argc) usage();
	nThreads = argv[i];
	break;
      case 'a':
	if (++i >= argc) usage();
	affinity = argv[i];
	break;
      case 'i':
	if (++i >= argc) usage();
	isolation = argv[i];
	break;
    }
  }

  signal(SIGSEGV, segv);

  ZmScheduler s(ZmSchedParams().
      id("scheduler").
      nThreads(nThreads).
      affinity(affinity).
      isolation(isolation));
  // ZmRef<Job> jobs[10];
  // ZmFn<> fns[10];
  ZmScheduler::Timer timers[10];

  s.start();
  ZmTime t(ZmTime::Now);
  int i;

  for (i = 0; i < 10; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    char *buf = (char *)malloc(32);
    sprintf(buf, "Goodbye World %d", j);
    // jobs[j - 1] = new Job(buf, t + ZmTime(((double)j) / 10.0));
    // fns[j - 1] = ZmFn<>::Member<&Job::operator()>::fn(jobs[j - 1].ptr());
    // s.add(&timers[j - 1], fns[j - 1], jobs[j - 1]->timeout());
    ZmTime out = t + ZmTime(((double)j) / 10.0);
    s.add(ZmFn<>::Member<&Job::operator()>::fn(ZmMkRef(new Job(buf, out))),
	out, &timers[j - 1]);
    printf("%u refCount: %u\n", (unsigned)j, (unsigned)timers[j - 1]->refCount());
    printf("Hello World %d\n", j);
  }

  for (i = 5; i < 10; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    if (timers[j - 1])
      printf("%u refCount: %u\n", (unsigned)j, (unsigned)timers[j - 1]->refCount());
    timers[j - 1] = 0;
    // fns[j - 1] = ZmFn<>();
    // jobs[j - 1] = 0;
  }

  for (i = 0; i < 5; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    if (timers[j - 1])
      printf("%u refCount: %u\n", (unsigned)j, (unsigned)timers[j - 1]->refCount());
    printf("Delete World %d\n", j);
    s.del(&timers[j - 1]);
    // timers[j - 1] = 0;
    // fns[j - 1] = ZmFn<>();
    // jobs[j - 1] = 0;
    ZmPlatform::sleep(ZmTime(.1));
  }

  ZmPlatform::sleep(ZmTime(.6));

  puts("threads:");
  ZmSpecific<ZmThreadContext>::all(
      ZmFn<ZmThreadContext *>::Ptr<&dumpThread>::fn());

  s.stop();

  s.start();

  t.now();

  for (i = 0; i < 10; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    char *buf = (char *)malloc(32);
    sprintf(buf, "Goodbye World %d", j);
    // jobs[j - 1] = new Job(buf, t + ZmTime(((double)j) / 10.0));
    // fns[j - 1] = ZmFn<>::Member<&Job::operator()>::fn(jobs[j - 1].ptr());
    ZmTime out = t + ZmTime(((double)j) / 10.0);
    s.add(ZmFn<>::Member<&Job::operator()>::fn(ZmMkRef(new Job(buf, out))),
	out, &timers[j - 1]);
    printf("Hello World %d\n", j);
  }

  for (i = 0; i < 5; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    timers[j - 1] = 0;
    // fns[j - 1] = ZmFn<>();
    // jobs[j - 1] = 0;
  }

  for (i = 5; i < 10; i++) {
    int j = (i & 1) ? ((i>>1) + 6) : (5 - (i>>1));
    printf("Delete World %d\n", j);
    s.del(&timers[j - 1]);
    // timers[j - 1] = 0;
    // fns[j - 1] = ZmFn<>();
    // jobs[j - 1] = 0;
    ZmPlatform::sleep(ZmTime(.1));
  }

  ZmPlatform::sleep(ZmTime(.6));

  puts("threads:");
  ZmSpecific<ZmThreadContext>::all(
      ZmFn<ZmThreadContext *>::Ptr<&dumpThread>::fn());

  s.stop();

  ZmBackoff o(ZmTime(.25), ZmTime(5), 1.25, .25);

  s.start();

  ZmRef<Timer> r = new Timer(&s, o);

  r->retry();
  r->start(ZmFn<>::Member<&Timer::retry>::fn(r.ptr()));

  ZmPlatform::sleep(ZmTime(8));

  r->stop();

  puts("threads:");
  ZmSpecific<ZmThreadContext>::all(
      ZmFn<ZmThreadContext *>::Ptr<&dumpThread>::fn());

  s.stop();
}
