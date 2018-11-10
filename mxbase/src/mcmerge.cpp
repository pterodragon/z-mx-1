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

// multicast capture file merge tool

#include <stdio.h>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>
#include <ZmRBTree.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>

#include <MxBase.hpp>
#include <MxMCapHdr.hpp>

void usage()
{
  std::cerr <<
    "usage: mcmerge OUTFILE INFILE...\n"
    "\tOUTFILE\t- output capture file\n"
    "\tINFILE\t- input capture file\n\n"
    "Options:\n"
    << std::flush;
  ZmPlatform::exit(1);
}

class File : public ZuObject {
public:
  // UDP over Ethernet maximum payload is 1472 without Jumbo frames
  enum { MsgSize = 1472 };

  template <typename Path>
  inline File(const Path &path) : m_path(path) { }

  void open() {
    ZeError e;
    if (m_file.open(m_path, ZiFile::ReadOnly, 0, &e) != Zi::OK) {
      ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
      ZmPlatform::exit(1);
    }
  }
  void close() { m_file.close(); }
  ZmTime read() {
    ZeError e;
    int i;
    if ((i = m_file.read(&m_hdr, sizeof(MxMCapHdr), &e)) == Zi::IOError) {
      ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
      ZmPlatform::exit(1);
    }
    if (i == Zi::EndOfFile || (unsigned)i < sizeof(MxMCapHdr)) {
      close();
      return ZmTime();
    }
    if (m_hdr.len > MsgSize) {
      ZeLOG(Error, ZtString() << "message length >" << ZuBoxed(MsgSize) <<
	  " at offset " << ZuBoxed(m_file.offset() - sizeof(MxMCapHdr)));
      ZmPlatform::exit(1);
    }
    if ((i = m_file.read(m_buf, m_hdr.len, &e)) == Zi::IOError) {
      ZeLOG(Error, ZtString() << '"' << m_path << "\": " << e);
      ZmPlatform::exit(1);
    }
    if (i == Zi::EndOfFile || (unsigned)i < m_hdr.len) {
      close();
      return ZmTime();
    }
    return ZmTime((time_t)m_hdr.sec, (int32_t)m_hdr.nsec);
  }
  int write(ZiFile *out, ZeError *e) {
    int i;
    if ((i = out->write(&m_hdr, sizeof(MxMCapHdr), e)) < 0) return i;
    if ((i = out->write(m_buf, m_hdr.len, e)) < 0) return i;
    return Zi::OK;
  }

private:
  ZtString	m_path;
  ZiFile	m_file;
  MxMCapHdr	m_hdr;
  char		m_buf[MsgSize];
};

typedef ZmRBTree<ZmTime,
	  ZmRBTreeVal<ZmRef<File>,
	    ZmRBTreeLock<ZmNoLock> > > Files;

int main(int argc, const char *argv[])
{
  Files files;
  ZuString outPath;

  {
    ZtArray<ZuString> paths;

    for (int i = 1; i < argc; i++) {
      if (argv[i][0] != '-') {
	paths.push(argv[i]);
	continue;
      }
      switch (argv[i][1]) {
	default:
	  usage();
	  break;
      }
    }
    if (paths.length() < 2) usage();
    outPath = paths[0];
    
    ZeLog::init("mcmerge");
    ZeLog::level(0);
    ZeLog::add(ZeLog::fileSink("&2"));
    ZeLog::start();

    for (int i = 1, n = paths.length(); i < n; i++) {
      ZmRef<File> file = new File(paths[i]);
      file->open();
      ZmTime t = file->read();
      if (t) files.add(t, file);
    }
  }

  ZiFile out;
  ZeError e;

  if (out.open(outPath, ZiFile::Create | ZiFile::Append, 0666, &e) < 0) {
    ZeLOG(Error, ZtString() << '"' << outPath << "\": " << e);
    ZmPlatform::exit(1);
  }

  for (;;) {
    ZmRef<File> file;
    {
      auto i = files.iterator<ZmRBTreeGreaterEqual>();
      if (file = i.iterateVal()) i.del();
    }
    if (!file) break;
    if (file->write(&out, &e) != Zi::OK) {
      ZeLOG(Error, ZtString() << '"' << outPath << "\": " << e);
      ZmPlatform::exit(1);
    }
    ZmTime t = file->read();
    if (t)
      files.add(t, file);
    else
      if (!files.count()) break;
  }

  ZeLog::stop();
  return 0;
}
