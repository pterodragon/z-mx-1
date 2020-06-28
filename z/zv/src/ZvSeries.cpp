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

// Data Series

#include <zlib/ZvSeries.hpp>

#include <zlib/ZeLog.hpp>

using namespace ZvSeries;

void Mgr::init(unsigned maxBufs)
{
  m_maxBufs = maxBufs;
}

unsigned Mgr::alloc(BufUnloadFn unloadFn)
{
  unsigned id = m_unloadFn.length();
  m_unloadFn.push(ZuMv(fn));
  return id;
}

void Mgr::free(unsigned seriesID) // caller unloads
{
  auto i = m_lru.iterator();
  while (BufLRUNode *node = i.iterateNode())
    if (node->seriesID == seriesID) i.del();
}

void Mgr::purge(unsigned seriesID, unsigned blkIndex) // caller unloads
{
  auto i = m_lru.iterator();
  ZmRef<Buf> 
  while (BufLRUNode *node = i.iterateNode())
    if (node->seriesID == seriesID && node->blkIndex < blkIndex) i.del();
}

void FileMgr::init(ZmScheduler *sched, FileMgr::Config config)
{
  m_sched = sched;
  m_dir = config.dir;
  m_coldDir = config.coldDir;
  m_writeTID = sched->tid(config.writeThread);
  if (!m_writeTID ||
      m_writeTID > sched->params().nThreads())
    throw ZtString() <<
      "ZvSeries::FileMgr writeThread misconfigured: " <<
      config.writeThread;
  m_files = new FileHash{};
  m_maxFileSize = config.maxFileSize;
  m_maxOpenFiles = m_files->size();
}

void FileMgr::final()
{
  m_files.clean();
}

bool FileMgr::open(
    unsigned seriesID, ZuString parent, ZuString name, OpenFn openFn)
{
  ZiFile::Path path = ZiFile::append(m_dir, parent);
  if (!ZiFile::isdir(path)) return true;
  path = ZiFile::append(path, name);
  if (!ZiFile::isdir(path)) return true;
  if (m_series.length() <= seriesID) m_series.length(seriesID + 1);
  m_series[seriesID] = SeriesData{
    .path = path;
    .fileBlks = (m_maxFileSize > BufSize) ? (m_maxFileSize / BufSize) : 1
  };
  ZiDir dir;
  if (dir.open(path) != Zi::OK) return false;
  ZiDir::Path fileName;
  unsigned minFileIndex = UINT_MAX;
  while (dir.read(fileName) == Zi::OK) {
#ifdef _WIN32
    ZtString fileName_{fileName};
#else
    auto &fileName_ = fileName;
#endif
    try {
      const auto &r = ZtStaticRegexUTF8("^[0-9a-f]{8}\\.sdb$");
      if (!r.m(fileName_)) continue;
    } catch (const ZtRegexError &e) {
      ZeLOG(Error, ZtString() << e);
      continue;
    } catch (...) {
      continue;
    }
    ZuBox<unsigned> fileIndex;
    fileIndex.scan(ZuFmt::Hex<>(), fileName_);
    if (fileIndex < minFileIndex) minFileIndex = fileIndex;
  }
  if (minFileIndex == UINT_MAX) minFileIndex = 0;
  m_series[seriesID].minFileIndex = minFileIndex;
  openFn(minFileIndex * m_series[seriesID].fileBlks);
  return true;
}

void FileMgr::close(unsigned seriesID)
{
  auto i = m_lru.iterator();
  ZmRef<File> fileRef; // need to keep ref count +ve during loop iteration
  while (auto file_ = i.iterateNode()) {
    auto file = static_cast<File *>(file_);
    if (file->id.seriesID() == seriesID) {
      i.del();
      fileRef = m_files.del(file->id); // see above comment
    }
  }
}

ZuRef<File> FileMgr::getFile(FileID fileID, bool create)
{
  ++m_fileLoads;
  ZuRef<Zdb_File> file;
  if (file = m_files->find(fileID)) {
    use(file.ptr());
    return file;
  }
  ++m_fileMisses;
  file = openFile(fileID, BufSize, create);
  if (ZuUnlikely(!file)) return nullptr;
  if (auto node = shift())
    m_files.del(static_cast<File *>(node)->id);
  m_files->add(file);
  push(file.ptr());
  if (index > m_lastFile) m_lastFile = index;
  return file;
}

ZuRef<File> FileMgr::openFile(FileID fileID, bool create)
{
  ZuRef<File> file = new File{fileID};
  unsigned fileSize = m_series[fileID.seriesID()].fileSize();
  ZiFile::Path path = fileName(fileID);
  if (file->open(path, ZiFile::GC, 0666, fileSize, nullptr) == Zi::OK) {
    scan(file);
    return file;
  }
  if (!create) return nullptr;
  bool retried = false;
retry:
  if (file->open(path, ZiFile::Create | ZiFile::GC,
	0666, fileSize, &e) != Zi::OK) {
    if (retried) {
      ZeLOG(Error, ZtString() <<
	  "ZvSeries::FileMgr could not open or create \"" <<
	  path << "\": " << e);
      return nullptr; 
    }
    ZiFile::Path dir = ZiFile::dirname(path);
    ZiFile::mkdir(ZiFile::dirname(dir));
    ZiFile::mkdir(ZuMv(dir));
    retried = true;
    goto retry;
  }
  return file;
}

void FileMgr::archiveFile(FileID fileID)
{
  ZiFile::Path name = fileName(dirName(fileID.seriesID()), fileID.fileIndex());
  ZiFile::Path coldName = ZiFile::append(m_coldDir, name);
  name = ZiFile::append(m_dir, name);
  ZeError e;
  if (ZiFile::rename(name, coldName, &e) != Zi::OK) {
    ZeLOG(Error, ZtString() <<
	"ZvSeries::FileMgr could not rename \"" << name << "\" to \"" <<
	coldName << "\": " << e);
  }
}

bool FileMgr::loadHdr(unsigned seriesID, unsigned blkIndex, Hdr &hdr)
{
  int r;
  ZeError e;
  FilePos pos = this->pos(seriesID, blkIndex);
  ZmRef<File> file = getFile(FileID{seriesID, pos.index}, false);
  if (!file) return false;
  if (ZuUnlikely((r = file.pread(
	    pos.offset, &hdr, sizeof(Hdr), &e)) < (int)sizeof(Hdr))) {
    fileRdError_(seriesID, pos.offset, r, e);
    return false;
  }
  return true;
}

bool FileMgr::load(unsigned seriesID, unsigned blkIndex, void *buf)
{
  int r;
  ZeError e;
  FilePos pos = this->pos(seriesID, blkIndex);
  ZmRef<File> file = getFile(FileID{seriesID, pos.index}, false);
  if (!file) return false;
  if (ZuUnlikely((r = file.pread(
	    pos.offset, buf, BufSize, &e)) < (int)BufSize)) {
    fileRdError_(seriesID, pos.offset, r, e);
    return false;
  }
  return true;
}

void FileMgr::save_(unsigned seriesID, unsigned blkIndex, const void *buf)
{
  int r;
  ZeError e;
  FilePos pos = this->pos(seriesID, blkIndex);
  ZmRef<File> file = getFile(FileID{seriesID, pos.index}, true);
  if (!file) return;
  if (ZuUnlikely((r = file.pwrite(
	    pos.offset, buf, BufSize, &e)) != (int)BufSize))
    fileWrError_(seriesID, pos.offset, r, e);
}

void FileMgr::purge(unsigned seriesID, unsigned blkIndex)
{
  Mgr::purge(seriesID, blkIndex);
  FilePos pos = this->pos(seriesID, blkIndex);
  {
    auto i = m_lru.iterator();
    ZmRef<File> fileRef; // need to keep ref count +ve during loop iteration
    while (auto file_ = i.iterateNode()) {
      auto file = static_cast<File *>(file_);
      if (file->id.seriesID() == seriesID &&
	  file->id.fileIndex() < pos.index) {
	i.del();
	fileRef = m_files.del(file->id); // see above comment
      }
    }
  }
  for (unsigned i = m_series[seriesID].minFileIndex; i < pos.index; i++)
    archiveFile(FileID{seriesID, i});
  m_series[seriesID].minFileIndex = pos.index;
}

void FileMgr::fileRdError_(
    unsigned seriesID, ZiFile::Offset off, int r, ZeError e)
{
  if (r < 0) {
    ZeLOG(Error, ZtString() <<
	"ZvSeries::FileMgr pread() failed on \"" <<
	m_series[seriesID].name <<
	"\" at offset " << ZuBoxed(off) <<  ": " << e);
  } else {
    ZeLOG(Error, ZtString() <<
	"ZvSeries::FileMgr pread() truncated on \"" <<
	m_series[seriesID].name <<
	"\" at offset " << ZuBoxed(off));
  }
}

void FileMgr::fileWrError_(unsigned seriesID, ZiFile::Offset off, ZeError e)
{
  ZeLOG(Error, ZtString() <<
      "ZvSeries::FileMgr pwrite() failed on \"" << m_series[seriesID].name <<
      "\" at offset " << ZuBoxed(off) <<  ": " << e);
}
