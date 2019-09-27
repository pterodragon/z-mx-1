#include <stdlib.h>
#include <alloca.h>

#include <iostream>
#include <string>

#include <zlib/ZuBox.hpp>
#include <zlib/ZuByteSwap.hpp>

#include <zlib/Zfb.hpp>

#include "zfbtest_fbs.h"

std::vector<ZmRef<ZiIOBuf>> bufs;

void build(Zfb::IOBuilder &fbb, unsigned n, bool detach)
{
  ZmRef<ZiIOBuf> buf;
  {
    uint8_t *zero = (uint8_t *)::malloc(n);
    memset(zero, 0, n);
    fbb.Clear();
    auto o = zfbtest::fbs::CreateBuf(fbb, Zfb::Save::bytes(fbb, zero, n));
    fbb.Finish(o);
    fbb.PushElement((uint32_t)42);
    fbb.PushElement((uint32_t)fbb.GetSize());
    ::free(zero);
    if (detach) { buf = fbb.buf(); bufs.push_back(buf); }
  }
  {
    uint8_t *ptr = detach ? buf->data() : fbb.GetBufferPointer();
    int len = detach ? buf->length : fbb.GetSize();
    uint32_t len_ = *(ZuLittleEndian<uint32_t> *)ptr;
    uint32_t type_ = *(ZuLittleEndian<uint32_t> *)(ptr + 4);
    auto buf_ = zfbtest::fbs::GetBuf(ptr + 8);
    auto data = Zfb::Load::bytes(buf_->data());
    std::cout
      << "ptr=" << ZuBoxPtr(ptr).hex() << " len=" << len
      << " len_=" << len_ << " type_=" << type_
      << " data=" << ZuBoxPtr(data.data()).hex()
      << " len__=" << data.length() << '\n' << std::flush;
  }
}

int main(int argc, char **argv)
{
  if (argc != 2) { std::cerr << "usage N\n" << std::flush; exit(1); }
  unsigned n = ZuBox<unsigned>(argv[1]);
  Zfb::IOBuilder fbb;
  build(fbb, n, false);
  build(fbb, n, true);
  build(fbb, n, true);
  build(fbb, n, false);
  build(fbb, n, true);
  build(fbb, n, true);
}
