//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

// Data Series File

#ifndef ZdfFile_HPP
#define ZdfFile_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZdfLib_HPP
#include <zlib/ZdfLib.hpp>
#endif

#include <zlib/ZuPtr.hpp>

#include <zlib/ZmHeap.hpp>
#include <zlib/ZmList.hpp>
#include <zlib/ZmNoLock.hpp>
#include <zlib/ZmHash.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZtArray.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiFile.hpp>

#include <zlib/ZvCf.hpp>

#include <zlib/ZdfBuf.hpp>
#include <zlib/ZdfSeries.hpp>

namespace Zdf {

using FileLRU =
  ZmList<ZuNull,
    ZmListObject<ZuNull,
      ZmListNodeIsItem<true,
	ZmListHeapID<ZuNull,
	  ZmListLock<ZmNoLock> > > > >;
using FileLRUNode = FileLRU::Node;

ZuDeclTuple(FileID, (unsigned, seriesID), (unsigned, index));
ZuDeclTuple(FilePos, (unsigned, index), (unsigned, offset));

struct File_ : public ZmObject, public ZiFile, public FileLRUNode {
  template <typename ...Args>
  File_(Args &&... args) : id{ZuFwd<Args>(args)...} { }

  FileID	id;
};
struct File_IDAccessor : public ZuAccessor<File_, FileID> {
  ZuInline static const FileID &value(const File_ &file) {
    return file.id;
  }
};

struct File_HeapID {
  static const char *id() { return "Zdf.File"; }
};
using FileHash =
  ZmHash<File_,
    ZmHashObject<ZmObject,
      ZmHashNodeIsKey<true,
	ZmHashIndex<File_IDAccessor,
	  ZmHashHeapID<File_HeapID,
	    ZmHashLock<ZmNoLock> > > > > >;

class ZdfAPI FileMgr : public Mgr {
private:
  struct Config {
    Config(const Config &) = delete;
    Config &operator =(const Config &) = delete;
    Config() = default;
    Config(Config &&) = default;
    Config &operator =(Config &&) = default;

    Config(ZvCf *cf) {
      dir = cf->get("dir", true);
      coldDir = cf->get("coldDir", true);
      writeThread = cf->get("writeThread", true);
      maxFileSize = cf->getInt("maxFileSize", 1, 1U<<30, false, 10<<20);
    }

    ZiFile::Path	dir;
    ZiFile::Path	coldDir;
    ZmThreadName	writeThread;
    unsigned		maxFileSize = 0;
  };

public:
  void init(ZmScheduler *sched, ZvCf *cf);
  void final();

  const ZiFile::Path &dir() const { return m_dir; }
  const ZiFile::Path &coldDir() const { return m_coldDir; }

  bool open(
      unsigned seriesID, ZuString parent, ZuString name, OpenFn openFn);
  void close(unsigned seriesID);

private:
  ZmRef<File_> getFile(FileID fileID, bool create);
  ZmRef<File_> openFile(FileID fileID, bool create);
  void archiveFile(FileID fileID);

public:
  bool loadHdr(unsigned seriesID, unsigned blkIndex, Hdr &hdr);
  bool load(unsigned seriesID, unsigned blkIndex, void *buf);
  void save(ZmRef<Buf> buf);

  void purge(unsigned seriesID, unsigned blkIndex);

  bool loadFile(
      ZuString name, Zfb::Load::LoadFn,
      unsigned maxFileSize, ZeError *e = nullptr);
  bool saveFile(
      ZuString name, Zfb::Builder &fbb, ZeError *e = nullptr);

private:
  void save_(unsigned seriesID, unsigned blkIndex, const void *buf);

  struct SeriesFile {
    ZiFile::Path	path;
    unsigned		minFileIndex = 0;	// earliest file index
    unsigned		fileBlks = 0;

    unsigned fileSize() const { return fileBlks * BufSize; }
  };

  const ZiFile::Path &pathName(unsigned seriesID) {
    return m_series[seriesID].path;
  }
  ZiFile::Path fileName(const FileID &fileID) {
    return fileName(pathName(fileID.seriesID()), fileID.index());
  }
  static ZiFile::Path fileName(const ZiFile::Path &path, unsigned index) {
    return ZiFile::append(path, ZuStringN<20>() <<
	ZuBox<unsigned>(index).hex(ZuFmt::Right<8>()) << ".sdb");
  }
  FilePos pos(unsigned seriesID, unsigned blkIndex) {
    auto fileBlks = m_series[seriesID].fileBlks;
    return FilePos{blkIndex / fileBlks, (blkIndex % fileBlks) * BufSize};
  }

  FileLRUNode *shift() {
    if (m_lru.count() >= m_maxOpenFiles) return m_lru.shiftNode();
    return nullptr;
  }
  void push(FileLRUNode *node) { m_lru.push(node); }
  void use(FileLRUNode *node) { m_lru.push(m_lru.del(node)); }
  void del(FileLRUNode *node) { m_lru.del(node); }

  void fileRdError_(unsigned seriesID, ZiFile::Offset, int, ZeError);
  void fileWrError_(unsigned seriesID, ZiFile::Offset, ZeError);

private:
  ZtArray<SeriesFile>	m_series;	// indexed by seriesID
  ZuPtr<FileHash>	m_files;
  FileLRU		m_lru;
  ZmScheduler		*m_sched = nullptr;
  ZiFile::Path		m_dir;
  ZiFile::Path		m_coldDir;
  unsigned		m_writeTID = 0;	// write thread ID
  unsigned		m_maxFileSize;	// maximum file size
  unsigned		m_maxOpenFiles;	// maximum #files open
  unsigned		m_fileLoads = 0;
  unsigned		m_fileMisses = 0;
};

} // namespace Zdf

#endif /* ZdfFile_HPP */
