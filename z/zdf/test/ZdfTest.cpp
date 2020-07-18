//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <iostream>

#include <zlib/ZuStringN.hpp>
#include <zlib/ZuDemangle.hpp>

#include <zlib/ZdfMem.hpp>
#include <zlib/Zdf.hpp>

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
  static const ZvFields fields() noexcept;
  uint64_t	v1;
  ZuFixedVal	v2;
};
inline const ZvFields Frame::fields() noexcept {
  ZvMkFields(Frame,
      // (Int, v1, Series | Index | Delta2),
      (Int, v1, Series | Index | Delta),
      // (Int, v1, Series | Index),
      (Fixed, v2, Series | Delta, 9));
}
int main()
{
  using namespace Zdf;
  using namespace ZdfCompress;
  Zdf::MemMgr mgr;
  mgr.init(nullptr, nullptr);
  Zdf::DataFrame df(Frame::fields(), "frame");
  df.init(&mgr);
  df.open();
  Frame frame;
  {
    auto writer = df.writer();
    for (uint64_t i = 0; i < 1000; i++) {
      frame.v1 = i;
      frame.v2 = i * 42;
      writer.write(&frame);
    }
  }
  {
    AnyReader index, reader;
    df.index(index, 0, ZuFixed{20, 0});
    std::cout << "offset=" << index.offset() << '\n';
    df.reader(reader, 1, index.offset());
    ZuFixed v;
    CHECK(reader.read(v));
    CHECK(v.value == 20 * 42);
    CHECK(v.exponent == 9);
    index.indexFwd(ZuFixed{200, 0});
    std::cout << "offset=" << index.offset() << '\n';
    reader.seekFwd(index.offset());
    CHECK(reader.read(v));
    CHECK(v.value == 200 * 42);
    CHECK(v.exponent == 9);
  }
  df.close();
  // df.final();
  // mgr.final();
}
