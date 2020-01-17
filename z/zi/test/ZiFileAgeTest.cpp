//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

#include <zlib/ZeLog.hpp>
#include <zlib/ZiFile.hpp>

int main()
{
  {
    ZiFile f;
    f.open("foo", ZiFile::Create | ZiFile::WriteOnly | ZiFile::GC, 0777);
  }
  ZiFile::age("foo", 8);
}
