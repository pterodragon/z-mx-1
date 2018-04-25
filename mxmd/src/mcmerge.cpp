//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <stdio.h>

#include <ZmTime.hpp>
#include <ZmTrap.hpp>
#include <ZmRBTree.hpp>

#include <ZeLog.hpp>

#include <ZiFile.hpp>

#include <MxBase.hpp>
#include <MxMDFrame.hpp>

void usage()
{
  fputs("usage: mcmerge OUTFILE INFILE...\n"
	"\tOUTFILE\t- output capture file\n"
	"\tINFILE\t- input capture file\n\n"
        "Options:\n",
	stderr);
  exit(1);
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
      ZeLOG(Error, ZtZString() << '"' << m_path << "\": " << e);
      exit(1);
    }
  }
  void close() { m_file.close(); }
  ZmTime read() {
    ZeError e;
    int i;
    if ((i = m_file.read(&m_frame, sizeof(MxMDFrame), &e)) == Zi::IOError) {
      ZeLOG(Error, ZtZString() << '"' << m_path << "\": " << e);
      exit(1);
    }
    if (i == Zi::EndOfFile || (unsigned)i < sizeof(MxMDFrame)) {
      close();
      return ZmTime();
    }
    if (m_frame.len > MsgSize) {
      ZeLOG(Error, ZtZString() << "message length >" << ZuBoxed(MsgSize) <<
	  " at offset " << ZuBoxed(m_file.offset() - sizeof(MxMDFrame)));
      exit(1);
    }
    if ((i = m_file.read(m_buf, m_frame.len, &e)) == Zi::IOError) {
      ZeLOG(Error, ZtZString() << '"' << m_path << "\": " << e);
      exit(1);
    }
    if (i == Zi::EndOfFile || (unsigned)i < m_frame.len) {
      close();
      return ZmTime();
    }
    return ZmTime((time_t)m_frame.sec, (int32_t)m_frame.nsec);
  }
  int write(ZiFile *out, ZeError *e) {
    int i;
    if ((i = out->write(&m_frame, sizeof(MxMDFrame), e)) < 0) return i;
    if ((i = out->write(m_buf, m_frame.len, e)) < 0) return i;
    return Zi::OK;
  }

private:
  ZtString	m_path;
  ZiFile	m_file;
  MxMDFrame	m_frame;
  char		m_buf[MsgSize];
};

typedef ZmRBTree<ZmTime,
	  ZmRBTreeVal<ZmRef<File>,
	    ZmRBTreeLock<ZmNoLock> > > Files;

int main(int argc, const char *argv[])
{
  Files files;
  ZtZString outPath;

  {
    ZtArray<ZtZString> paths;

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
    ZeLOG(Error, ZtZString() << '"' << outPath << "\": " << e);
    exit(1);
  }

  for (;;) {
    ZmRef<File> file;
    {
      Files::Iterator i(files, Files::GreaterEqual);
      if (file = i.iterateVal()) i.del();
    }
    if (!file) break;
    if (file->write(&out, &e) != Zi::OK) {
      ZeLOG(Error, ZtZString() << '"' << outPath << "\": " << e);
      exit(1);
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
