//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuStringN.hpp>

#include <ZmBxRing.hpp>
#include <ZmThread.hpp>
#include <ZmSpinLock.hpp>
#include <ZmTimeInterval.hpp>

void usage()
{
  std::cerr <<
"usage: ZmBxRingTest [OPTION]...\n"
"  test read/write ring buffer in shared memory\n\n"
"Options:\n"
"  -l N\t\t- loop N times\n"
"  -b BUFSIZE\t- set buffer size to BUFSIZE (default: 8192)\n"
"  -n COUNT\t- set number of messages to COUNT (default: 1)\n"
"  -r N\t\t- number of reader threads\n"
"  -w N\t\t- number of writer threads\n"
"  -i INTERVAL\t- set delay between messages in seconds (default: 0)\n"
"  -L\t\t- low-latency (readers spin indefinitely and do not yield)\n"
"  -s SPIN\t- set spin count to SPIN (default: 1000)\n"
"  -S\t\t- slow reader (sleep INTERVAL seconds in between reads)\n";
  exit(1);
}

struct Msg {
  ZuInline Msg() : m_p((uintptr_t)(void *)this) { }
  ZuInline ~Msg() { m_p = 0; }
  ZuInline bool ok() const { return m_p == (uintptr_t)(void *)this; }
  uintptr_t m_p;
};

typedef ZmBxRing<Msg> Ring;

struct App {
  inline App() :
    ring(0), count(1), readers(1), writers(1),
    interval((time_t)0), slow(false) { }

  int main(int, char **);
  void reader();
  void writer(unsigned);

  Ring				*ring;
  ZmTime			start, end;
  ZmTimeInterval<ZmSpinLock>	readTime, writeTime;
  unsigned			count;
  unsigned			readers;
  unsigned			writers;
  ZmTime			interval;
  bool				slow;
};

int main(int argc, char **argv)
{
  App a;
  return a.main(argc, argv);
}

int App::main(int argc, char **argv)
{
  unsigned bufsize = 8192;
  bool ll = false;
  unsigned spin = 1000;
  unsigned loop = 1;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') usage();
    switch (argv[i][1]) {
      case 'l':
	if (++i >= argc) usage();
	loop = atoi(argv[i]);
	break;
      case 'b':
	if (++i >= argc) usage();
	bufsize = atoi(argv[i]);
	break;
      case 'n':
	if (++i >= argc) usage();
	count = atoi(argv[i]);
	break;
      case 'i':
	if (++i >= argc) usage();
	interval = ZmTime((double)ZuBox<double>(argv[i]));
	break;
      case 'L':
	ll = true;
	break;
      case 's':
	if (++i >= argc) usage();
	spin = atoi(argv[i]);
	break;
      case 'S':
	slow = true;
	break;
      case 'r':
	if (++i >= argc) usage();
	readers = atoi(argv[i]);
	break;
      case 'w':
	if (++i >= argc) usage();
	writers = atoi(argv[i]);
	break;
      default:
	usage();
	break;
    }
  }

  ring = new Ring(ZmRingParams(bufsize).ll(ll).spin(spin));

  for (unsigned i = 0; i < loop; i++) {
    {
      if (ring->open(Ring::Read | Ring::Write) != Ring::OK) {
	std::cerr << "open failed\n";
	exit(1);
      }
    }

    {
      ZuStringN<80> s;
      s << "address: 0x" <<
	  ZuBoxed((uintptr_t)ring->data()).fmt(ZuFmt::Hex<>()) <<
	"  ctrlSize: " << ZuBoxed(ring->ctrlSize()) <<
	"  size: " << ZuBoxed(ring->size()) <<
	"  msgSize: " << ZuBoxed(sizeof(Msg)) << '\n';
      std::cerr << s;
    }

    {
      ZmThread r[readers], w[writers];

      for (unsigned i = 0; i < readers; i++)
	r[i] = ZmThread(0, 0, ZmFn<>::Member<&App::reader>::fn(this));
      for (unsigned i = 0; i < writers; i++)
	w[i] = ZmThread(0, 0, ZmFn<>([this, i]() { this->writer(i); }));
      for (unsigned i = 0; i < writers; i++)
	if (!!w[i]) w[i].join();
      ring->eof();
      for (unsigned i = 0; i < readers; i++)
	if (!!r[i]) r[i].join();
    }

    start = end - start;

    {
      ZuStringN<80> s;
      s << "total time: " <<
	ZuBoxed(start.sec()) << '.' <<
	  ZuBoxed(start.nsec()).fmt(ZuFmt::Frac<9>()) <<
	"  avg time: " <<
	ZuBoxed((double)((start.dtime() / (double)(count * writers)) *
	      (double)1000000)) <<
	" usec\n";
      std::cerr << s;
    }
    {
      ZuStringN<256> s;
      s << "shift: " << readTime << "\n"
	<< "push:  " << writeTime << "\n";
      std::cerr << s;
    }

    ring->close();
  }

  return 0;
}

void App::reader()
{
  std::cerr << "reader started\n";
  ring->attach();
  std::cerr << "reader attached\n";
  for (unsigned j = 0, n = count * writers; j < n; j++) {
    ZmTime readStart(ZmTime::Now);
    if (const Msg *msg = ring->shift()) {
      // puts("shift");
      // printf("shift: \"%s\"\n", msg->data());
      // fwrite("msg read\n", 1, 9, stderr);
      // assert(*msg == "hello world");
      if (ZuUnlikely(!msg->ok())) goto failed;
      // msg->~Msg();
      ring->shift2();
      ZmTime readEnd(ZmTime::Now);
      readTime.add(readEnd -= readStart);
    } else {
      int i = ring->readStatus();
      if (i == Ring::EndOfFile) {
	end.now();
	std::cerr << "reader EOF\n";
	break;
      } else if (!i)
	std::cerr << "ring empty\n";
      else {
	ZuStringN<32> s;
	s << "readStatus() returned " << ZuBoxed(i) << '\n';
	std::cerr << s;
      }
      ZmPlatform::sleep(.1);
      --j;
      continue;
    }
    if (slow && !!interval) ZmPlatform::sleep(interval);
  }
  end.now();
  ring->detach();
  return;
failed:
  end.now();
  std::cerr << "reader msg validation FAILED\n";
  return;
}

void App::writer(unsigned i)
{
  unsigned failed = 0;
  std::cerr << "writer started\n";
  if (!i) start.now();
  for (unsigned j = 0; j < count; j++) {
    unsigned full_ = ring->full();
    ZmTime writeStart(ZmTime::Now);
    if (void *ptr = ring->push()) {
      // puts("push");
      Msg *msg = new (ptr) Msg();
      // fwrite("msg written\n", 1, 12, stderr);
      ring->push2();
      // ring->push2(ptr);
      ZmTime writeEnd(ZmTime::Now);
      if (ring->full() == full_) writeTime.add(writeEnd -= writeStart);
    } else {
      int i = ring->writeStatus();
      if (i == Ring::EndOfFile) {
	end.now();
	std::cerr << "writer EOF\n";
	break;
      } else if (i >= (int)sizeof(Msg))
	std::cerr << "no readers\n";
      else {
	std::cerr << "push failed\n";
	++failed;
      }
      ZmPlatform::sleep(.1);
      --j;
      continue;
    }
    if (!!interval) ZmPlatform::sleep(interval);
  }
  {
    ZuStringN<64> s;
    s << "push failed " << ZuBoxed(failed) << " times\n"
      << "ring full " << ZuBoxed(ring->full()) << " times\n";
    std::cerr << s;
  }
}
