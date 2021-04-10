//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>
#include <zlib/ZuDemangle.hpp>

#include <zlib/ZdfMem.hpp>
#include <zlib/ZdfFile.hpp>
#include <zlib/Zdf.hpp>
#include <zlib/ZdfStats.hpp>

void print(const char *s) {
  std::cout << s << '\n' << std::flush;
}
void print(const char *s, int64_t i) {
  std::cout << s << ' ' << i << '\n' << std::flush;
}
void ok(const char *s) { } // { print(s); }
void ok(const char *s, int64_t i) { } // { print(s, i); }
void fail(const char *s) { print(s); }
void fail(const char *s, int64_t i) { print(s, i); }
#define CHECK(x) ((x) ? ok("OK  " #x) : fail("NOK " #x))
#define CHECK2(x, y) ((x == y) ? ok("OK  " #x, x) : fail("NOK " #x, x))

struct Frame {
  static const ZvFieldArray fields() noexcept;
  uint64_t	v1;
  ZuFixedVal	v2;
};
inline const ZvFieldArray Frame::fields() noexcept {
  ZvFields(Frame,
      // (Int, v1, Series | Index | Delta2),
      (Int, v1, Series | Index | Delta),
      // (Int, v1, Series | Index),
      (Fixed, v2, Series | Delta, 9));
}
void usage() {
  std::cerr << "usage: ZdfTest mem|load|save\n" << std::flush;
  ::exit(1);
};
int main(int argc, char **argv)
{
  if (argc < 2) usage();
  enum { Mem = 0, Load, Save };
  int mode;
  if (!strcmp(argv[1], "mem"))
    mode = Mem;
  else if (!strcmp(argv[1], "load"))
    mode = Load;
  else if (!strcmp(argv[1], "save"))
    mode = Save;
  else {
    usage();
    return 1; // suppress compiler warning
  }
  using namespace Zdf;
  using namespace ZdfCompress;
  MemMgr memMgr;
  if (mode == Mem) memMgr.init(nullptr, nullptr);
  ZmScheduler sched(ZmSchedParams().nThreads(2));
  FileMgr fileMgr;
  if (mode != Mem) {
    ZmRef<ZvCf> cf = new ZvCf{};
    cf->fromString(
	"dir .\n"
	"coldDir .\n"
	"writeThread 1\n", false);
    fileMgr.init(&sched, cf);
  }
  DataFrame df(Frame::fields(), "frame");
  if (mode == Mem)
    df.init(&memMgr);
  else
    df.init(&fileMgr);
  sched.start();
  df.open();
  Frame frame;
  if (mode == Mem || mode == Save) {
    auto writer = df.writer();
    for (uint64_t i = 0; i < 300; i++) { // 1000
      frame.v1 = i;
      frame.v2 = i * 42;
      writer.write(&frame);
    }
  }
  if (mode == Mem || mode == Load) {
    AnyReader index, reader;
    df.find(index, 0, ZuFixed{20, 0});
    std::cout << "offset=" << index.offset() << '\n';
    df.seek(reader, 1, index.offset());
    ZuFixed v;
    CHECK(reader.read(v));
    CHECK(v.value == 20 * 42);
    CHECK(v.exponent == 9);
    index.findFwd(ZuFixed{200, 0});
    std::cout << "offset=" << index.offset() << '\n';
    reader.seekFwd(index.offset());
    CHECK(reader.read(v));
    CHECK(v.value == 200 * 42);
    CHECK(v.exponent == 9);
    index.findRev(ZuFixed{100, 0});
    std::cout << "offset=" << index.offset() << '\n';
    reader.seekRev(index.offset());
    AnyReader cleaner;
    {
      auto offset = reader.offset();
      offset = offset < 100 ? 0 : offset - 100;
      df.seek(cleaner, 1, offset);
    }
    Zdf::StatsTree<> w;
    while (reader.read(v)) {
      w.add(v);
      if (cleaner.read(v)) w.del(v);
      std::cout << "min=" << ZuBoxed(w.minimum()) <<
	" max=" << ZuBoxed(w.maximum()) <<
	" mean=" << ZuBoxed(w.mean()) <<
	" stddev=" << ZuBoxed(w.std()) <<
	" median=" << ZuBoxed(w.median()) <<
	" 95%=" << ZuBoxed(w.rank(0.95)) << '\n';
    }
    // for (auto k = w.begin(); k != w.end(); ++k) std::cout << *k << '\n';
    // for (auto k: w) std::cout << k.first << '\n';
    // std::cout << "stddev=" << w.std() << '\n';
  }
  df.close();
  sched.stop();
  std::cout << ZmHeapMgr::csv();
  // df.final();
  // mgr.final();
}
