//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZuStringN.hpp>

#include <zlib/ZeLog.hpp>

// #define ZiRing_STRESSTEST
#include <zlib/ZiRing.hpp>

void usage()
{
  std::cerr <<
    "usage: ZiRingTest2 [OPTION]... NAME\n"
    "  test read/write ring buffer in shared memory\n"
    "\tNAME\t- name of shared memory segment\n\n"
    "Options:\n"
    "  -r\t\t- read from buffer\n"
    "  -w\t\t- write to buffer (default)\n"
    "  -x\t\t- read and write in same process\n"
    "  -l N\t\t- loop N times\n"
    "  -g\t\t- test GC / attach / detach contention\n"
    "  -b BUFSIZE\t- set buffer size to BUFSIZE (default: 8192)\n"
    "  -m MSGSIZE\t- set message size to MSGSIZE (default: 1024)\n"
    "  -n COUNT\t- set number of messages to COUNT (default: 1)\n"
    "  -i INTERVAL\t- set delay between messages in seconds (default: 0)\n"
    "  -L\t\t- low-latency (readers spin indefinitely and do not yield)\n"
    "  -s SPIN\t- set spin count to SPIN (default: 1000)\n"
    "  -S\t\t- slow reader (sleep INTERVAL seconds in between reads)\n"
    "  -c CPUSET\t- bind memory to CPUSET\n"
    << std::flush;
  ZmPlatform::exit(1);
}

using Ring = ZiRing<ZiRingMsg>;

struct App {
  App() :
    flags(Ring::Write | Ring::Create), gc(false), ring(0),
    count(1), msgsize(1024), interval((time_t)0), slow(false) { }

  int main(int, char **);
  void reader();
  void writer();

  unsigned	flags;
  bool		gc;
  Ring		*ring;
  ZmTime	start, end;
  unsigned	count;
  unsigned	msgsize;
  ZmTime	interval;
  bool		slow;
  ZmBitmap	cpuset;
};

int main(int argc, char **argv)
{
  App a;
  return a.main(argc, argv);
}

int App::main(int argc, char **argv)
{
  const char *name = 0;
  unsigned bufsize = 8192;
  bool ll = false;
  unsigned spin = 1000;
  unsigned loop = 1;
  
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (name) usage();
      name = argv[i];
      continue;
    }
    switch (argv[i][1]) {
      case 'r':
	flags = Ring::Read | Ring::Create;
	break;
      case 'w':
	flags = Ring::Write | Ring::Create;
	break;
      case 'x':
	flags = Ring::Read | Ring::Write | Ring::Create;
	break;
      case 'l':
	if (++i >= argc) usage();
	loop = atoi(argv[i]);
	break;
      case 'g':
	gc = true;
	break;
      case 'b':
	if (++i >= argc) usage();
	bufsize = atoi(argv[i]);
	break;
      case 'm':
	if (++i >= argc) usage();
	msgsize = atoi(argv[i]);
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
      case 'c':
	if (++i >= argc) usage();
	cpuset = argv[i];
	break;
      default:
	usage();
	break;
    }
  }

  if (!name) usage();

  ring = new Ring(ZiRingParams(name).
      size(bufsize).ll(ll).spin(spin).coredump(true).cpuset(cpuset));

  for (unsigned i = 0; i < loop; i++) {
    {
      ZeError e;

      if (ring->open(flags, &e) != Zi::OK) {
	std::cerr << e << '\n' << std::flush;
	ZmPlatform::exit(1);
      }
    }

    std::cerr << 
      "address: 0x" << ZuBoxPtr(ring->data()).hex() <<
      "  ctrlSize: " << ZuBoxed(ring->ctrlSize()) <<
      "  size: " << ZuBoxed(ring->size()) << '\n';

    {
      ZmThread r, w;

      if (flags & Ring::Read)
	r = ZmThread(0, ZmFn<>::Member<&App::reader>::fn(this));
      if (flags & Ring::Write)
	w = ZmThread(0, ZmFn<>::Member<&App::writer>::fn(this));
      if (!!r) r.join();
      if (!!w) w.join();
    }

    start = end - start;

    std::cerr <<
      "total time: " <<
      ZuBoxed(start.sec()) << '.' <<
	ZuBoxed(start.nsec()).fmt(ZuFmt::Frac<9>()) <<
      "  avg time: " <<
      ZuBoxed((double)((start.dtime() / (double)count) * (double)1000000)) <<
      " usec\n";

    ring->close();
  }

  return 0;
}

void App::reader()
{
  std::cerr << "reader started\n";
  if (!gc) {
    ring->attach();
    std::cerr << "reader attached\n";
  }
  if (!(flags & Ring::Write)) start.now();
  for (unsigned j = 0; j < count; j++) {
    if (gc) {
      ring->attach();
      ring->detach();
      continue;
    }
    if (const ZiRingMsg *msg = ring->shift()) {
      // std::cerr << "shift: " << ZuBoxPtr(msg).hex() << " len: " << ZuBoxed(sizeof(ZiRingMsg) + msg->length()) << '\n';
      // for (unsigned i = 0, n = msg->length(); i < n; i++) assert(((const char *)(msg->ptr()))[i] == (char)(i & 0xff));
      // std::cerr << "msg read\n";
      ring->shift2();
    } else {
      int i = ring->readStatus();
      if (i == Zi::EndOfFile)
	std::cerr << "EOF\n";
      else if (!i)
	std::cerr << "ring empty\n";
      else
	std::cerr << "readStatus() returned " << ZuBoxed(i) << '\n';
      ZmPlatform::sleep(.1);
      --j;
      continue;
    }
    if (slow && !!interval) ZmPlatform::sleep(interval);
  }
  end.now();
  if (!gc) ring->detach();
}

void App::writer()
{
  unsigned full = 0;
  std::cerr << "writer started\n";
  start.now();
  for (unsigned j = 0; j < count; j++) {
    if (gc) {
      ring->gc();
      // int i = ring->gc();
      // { ZuStringN<32> s; s << "GC: " << ZuBoxed(i) << "\n"; std::cerr << s; }
      continue;
    }
    if (void *ptr = ring->push(msgsize)) {
      // std::cerr << "push: " << ZuBoxPtr(ptr).hex() << " len: " << ZuBoxed(msgsize) << '\n';
      ZiRingMsg *msg = new (ptr) ZiRingMsg(0, msgsize - sizeof(ZiRingMsg));
      // for (unsigned i = 0, n = msg->length(); i < n; i++) ((char *)(msg->ptr()))[i] = (char)(i & 0xff);
      // std::cerr << "msg written\n";
      ring->push2();
    } else {
      int i = ring->writeStatus();
      if (i == Zi::EndOfFile)
	std::cerr << "writer EOF\n";
      else if (i == Zi::IOError)
	std::cerr << "writer I/O Error\n";
      else if (i == Zi::NotReady)
	std::cerr << "writer Not Ready - no readers\n";
      else if (i > (int)msgsize)
	std::cerr << "writer OK!\n";
      else {
	std::cerr << "Ring Full\n";
	++full;
      }
      ZmPlatform::sleep(.1);
      --j;
      continue;
    }
    if (!!interval) ZmPlatform::sleep(interval);
  }
  if (!(flags & Ring::Read)) end.now();
  ring->eof();
  std::cerr << "ring full " << ZuBoxed(full) << " times\n";
}
