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

// file I/O

#include <zlib/ZiFile.hpp>

#include <zlib/ZtArray.hpp>

#include <zlib/ZtRegex.hpp>

#define ZiFile_CopyBufSize	(128<<10)	// 128k

#ifdef _WIN32

#include <ctype.h>
#include <stdlib.h>

#include <zlib/ZmRBTree.hpp>
#include <zlib/ZmSingleton.hpp>
#include <zlib/ZmNoLock.hpp>

extern "C" {
  typedef struct {
    WORD Length;
    WORD MaximumLength;
    wchar_t *Buffer;
  } UNICODE_STRING;

  typedef LONG (WINAPI *PNtQueryObject)(HANDLE, int, void *, ULONG, PULONG);
}

class ZiFile_WindowsDrives;

template <> struct ZmCleanup<ZiFile_WindowsDrives> {
  enum { Level = ZmCleanupLevel::Platform };
};

class ZiFile_WindowsDrives {
friend ZmSingletonCtor<ZiFile_WindowsDrives>;

public:
  static int blkSize(ZtWString path) {
    return instance()->blkSize_path(ZuMv(path));
  }
  static int blkSize(HANDLE handle) {
    return instance()->blkSize_handle(handle);
  }

#if 0
  static void dump() {
    return instance()->dump_();
  }
#endif

private:
  ZiFile_WindowsDrives();
public:
  ~ZiFile_WindowsDrives();

private:
  int blkSize_path(ZtWString path);
  int blkSize_handle(HANDLE handle);

#if 0
  void dump_();
#endif

  void refresh();

  static ZiFile_WindowsDrives *instance();

  using DriveLetters =
    ZmRBTree<ZtWString,
      ZmRBTreeVal<char,
	ZmRBTreeLock<ZmNoLock> > >;
  using DriveBlkSizes =
    ZmRBTree<char,
      ZmRBTreeVal<int,
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmNoLock> > > >;

  HMODULE		m_ntdll;
  PNtQueryObject	m_ntQueryObject;
  ZmLock		m_lock;
  ZmTime		m_lastRefresh;
  DriveLetters		m_driveLetters;
  DriveBlkSizes		m_driveBlkSizes;
};

ZiFile_WindowsDrives *ZiFile_WindowsDrives::instance()
{
  return ZmSingleton<ZiFile_WindowsDrives>::instance();
}

ZiFile_WindowsDrives::ZiFile_WindowsDrives()
{
  if (m_ntdll = LoadLibrary(L"ntdll.dll"))
    m_ntQueryObject = (PNtQueryObject)GetProcAddress(m_ntdll, "NtQueryObject");
  else
    m_ntQueryObject = 0;

  refresh();
}

ZiFile_WindowsDrives::~ZiFile_WindowsDrives()
{
  if (m_ntdll) FreeLibrary(m_ntdll);
}

void ZiFile_WindowsDrives::refresh()
{
  ZmTime now(ZmTime::Now);

  if ((now - m_lastRefresh).sec() < 1) return;

  m_driveLetters.clean();
  m_driveBlkSizes.clean();

  DWORD len = 0;
  ZtArray<wchar_t> buf;
  do {
    buf.length(len, false);
  } while ((len = GetLogicalDriveStrings(len, buf.data())) >
	     (DWORD)buf.length());
  unsigned i = 0;
  ZtWString drive;
  drive += L" :\\";
  ZtWString pathBuf(ZiPlatform::PathMax + 1);
  ZtWString path;
  int dl;
  do {
    dl = buf[i];
    if (islower(dl)) dl += 'A' - 'a';
    drive[0] = dl;
    drive[2] = 0;
    if (QueryDosDevice(drive, pathBuf, ZiPlatform::PathMax + 1) > 0) {
      pathBuf.calcLength();
      if (!pathBuf.cmp(L"\\\\?\\", 4))
	path.init(&pathBuf[4], pathBuf.length() - 4);
      else
	path = pathBuf;
      m_driveLetters.add(path, dl);
      if (!path.icmp(L"\\Device\\Harddisk", 16)) {
	drive[2] = '\\';
	{
	  DWORD sectorsPerCluster, bytesPerSector, d2, d3;

	  GetDiskFreeSpace(
	      drive, &sectorsPerCluster, &bytesPerSector, &d2, &d3);
	  m_driveBlkSizes.add(dl, sectorsPerCluster * bytesPerSector);
	}
      } else
	m_driveBlkSizes.add(dl, 512);
    }
    while (buf[i++] && i < len);
  } while (buf[i] && i < len);

  m_lastRefresh = now;
}

#if 0
#include <stdio.h>

void ZiFile_WindowsDrives::dump_()
{
  {
    DriveLetters::ReadIterator i(m_driveLetters);
    DriveLetters::NodeRef dln;

    while (dln = i.iterate()) {
      printf("%p %S -> %c\n",
	     (void *)dln->key().data(), dln->key().data(), (int)dln->val());
    }
  }
  {
    DriveBlkSizes::ReadIterator i(m_driveBlkSizes);
    DriveBlkSizes::NodeRef dbn;

    while (dbn = i.iterate()) {
      printf("%c -> %d\n", (int)dbn->key(), (int)dbn->val());
    }
  }
}
#endif

int ZiFile_WindowsDrives::blkSize_path(ZtWString path)
{
  if (!path.cmp(L"\\\\?\\", 4)) path.splice(4);

  int dl = 0;
  if (path[1] == ':')
    dl = path[0];
  else {
    if (path[0] == '\\') {
      if (path.icmp(L"\\Device\\Harddisk", 16)) return 0;

      ZmGuard<ZmLock> guard(m_lock);
      bool retried = false;

retry:
      auto i = m_driveLetters.readIterator<ZmRBTreeLessEqual>(path);
      DriveLetters::NodeRef dln = i.iterate();
      if (dln) {
	const ZtWString &drive = dln->key();
	if (!drive.cmp(path, drive.length())) dl = dln->val();
      }
      if (!dl) {
	if (!retried) { retried = true; refresh(); goto retry; }
	return 0;
      }
    } else {
      ZtWString dir_(ZiPlatform::PathMax + 1);
      dir_.length(GetCurrentDirectory(ZiPlatform::PathMax + 1, dir_));
      if (dir_.length()) {
	ZtWString dir(ZiPlatform::PathMax + 1);
	dir.length(GetFullPathName(dir_, ZiPlatform::PathMax + 1, dir, 0));
	if (dir.length()) dl = dir[0];
      }
      if (!dl) return 0;
    }
  }

  if (islower(dl)) dl += 'A' - 'a';

  {
    ZmGuard<ZmLock> guard(m_lock);
    int n = m_driveBlkSizes.findVal(dl);
    return ZuCmp<int>::null(n) ? 512 : n;
  }
}

int ZiFile_WindowsDrives::blkSize_handle(HANDLE handle)
{
  if (!m_ntQueryObject) return 0;
  LONG l;
  ULONG len = BUFSIZ;
  ZtArray<char> buf;
  do {
    buf.length(len, false);
    l = m_ntQueryObject(handle, 1, buf.data(), buf.length(), &len);
    if (l && len <= (ULONG)buf.length()) return 0;
  } while (len > (ULONG)buf.length());
  return blkSize_path(((UNICODE_STRING *)buf.data())->Buffer);
}

#endif /* _WIN32 */

int ZiFile::open(const Path &name, unsigned flags, unsigned mode, ZeError *e)
{
  Guard guard(m_lock);

  return open_(name, flags, mode, 0, e);
}

int ZiFile::open(
    const Path &name, unsigned flags, unsigned mode, Offset length, ZeError *e)
{
  Guard guard(m_lock);

  return open_(name, flags, mode, length, e);
}

int ZiFile::open_(
    const Path &name, unsigned flags, unsigned mode, Offset length, ZeError *e)
{
  if (m_handle != ZiPlatform::nullHandle()) {
#ifndef _WIN32
    if (e) *e = EINVAL;
#else
    if (e) *e = ERROR_INVALID_PARAMETER;
#endif
    return Zi::IOError;
  }

  Handle h;
  unsigned blkSize;

#ifndef _WIN32
  int openFlags;
  openFlags = (flags & ReadOnly) ? O_RDONLY :
	      (flags & WriteOnly) ? O_WRONLY : O_RDWR;
  if (flags & Create)	 openFlags |= O_CREAT;	// create if not existing
  if (flags & Exclusive) openFlags |= O_EXCL;	// do not open existing file
  if (flags & Direct)	 openFlags |= O_DIRECT;	// direct I/O - bypass OS cache
  if (flags & Sync)	 openFlags |= O_DSYNC;	// synchronize all writes
  if (flags & Shm) {
    if (length <= 0) goto einval;
    Path name_(name.length() + 2);
    name_ << '/' << name;
    h = shm_open(name_, openFlags, mode);
    if (h < 0) goto error;
    m_shmName = name_;
    blkSize = ::sysconf(_SC_PAGESIZE);
    length = ((length + blkSize - 1) / blkSize) * blkSize;
  } else {
    h = ::open(name, openFlags, mode);
    if (h < 0) goto error;
    {
      struct stat s;

      if (fstat(h, &s) < 0) { ::close(h); goto error; }
      blkSize = s.st_blksize;
    }
  }
  if (length >= 0 && (size() < length || (flags & Truncate))) {
    if (ftruncate(h, length) < 0) { ::close(h); goto error; }
  }
#else
  if (flags & Shm) {
    if (length <= 0) goto einval;
    Path name_(name.length() + 8);
    name_ << "Local\\" << name; // was Global
    DWORD protectFlags = (flags & ReadOnly) ? PAGE_READONLY : PAGE_READWRITE;
    if (flags & ShmDbl)
      blkSize = 64<<10; // Windows - 64k, not the system page size
    else
      { SYSTEM_INFO si; GetSystemInfo(&si); blkSize = si.dwPageSize; }
    length = ((length + blkSize - 1) / blkSize) * blkSize;
    h = CreateFileMapping(
	INVALID_HANDLE_VALUE, 0, protectFlags, 0, (DWORD)length, name_);
  } else {
    blkSize = ZiFile_WindowsDrives::blkSize(name);
    DWORD accessFlags = (flags & ReadOnly) ? GENERIC_READ :
      (flags & WriteOnly) ? GENERIC_WRITE : GENERIC_READ | GENERIC_WRITE;
    DWORD shareFlags = (flags & ReadOnly) ? FILE_SHARE_READ :
      FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD createFlags = !(flags & Create) ? OPEN_EXISTING :
      (flags & Exclusive) ? CREATE_NEW : OPEN_ALWAYS;
    DWORD fileFlags = FILE_FLAG_OVERLAPPED;
    if (flags & Direct) fileFlags |= FILE_FLAG_NO_BUFFERING;
    if (flags & Sync) fileFlags |= FILE_FLAG_WRITE_THROUGH;
    h = CreateFile(
	name, accessFlags, shareFlags, nullptr, createFlags, fileFlags, NULL);
    if (h == INVALID_HANDLE_VALUE) goto error;
    if ((length > 0 && size() < length) || (flags & Truncate)) {
      LONG high = length>>32;
      if ((SetFilePointer(h, length & 0xffffffffU, &high, FILE_BEGIN) ==
	    INVALID_SET_FILE_POINTER &&
	  GetLastError() != NO_ERROR) || !SetEndOfFile(h)) {
	CloseHandle(h);
	goto error;
      }
    }
  }
#endif

  init_(h, flags | GC, blkSize, (flags & (MMap | Shm)) ? length : (Offset)0);
  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;

einval:
#ifndef _WIN32
  if (e) *e = EINVAL;
#else
  if (e) *e = ERROR_INVALID_PARAMETER;
#endif
  return Zi::IOError;
}

int ZiFile::mmap(
    const Path &name, unsigned flags, Offset length, bool shared,
    int mmapFlags, unsigned mode, ZeError *e)
{
  if (length <= 0) {
#ifndef _WIN32
    if (e) *e = EINVAL;
#else
    if (e) *e = ERROR_INVALID_PARAMETER;
#endif
    return Zi::IOError;
  }

  Guard guard(m_lock);

  int r;

  if ((r = open_(name, flags | MMap, mode, length, e)) != Zi::OK) return r;

#ifndef _WIN32
  int prot = ((flags & ReadOnly) ? PROT_READ :
	      (flags & WriteOnly) ? PROT_WRITE : PROT_READ | PROT_WRITE);
  mmapFlags |= shared ? MAP_SHARED : MAP_PRIVATE;
#ifdef MAP_POPULATE
  if (flags & MMPopulate) mmapFlags |= MAP_POPULATE;
#endif
  if (flags & ShmDbl) {
    m_addr = ::mmap(
	0, m_mmapLength<<1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (m_addr == MAP_FAILED || !m_addr) goto error;
    void *addr = ::mmap(
	m_addr, m_mmapLength, prot, mmapFlags | MAP_FIXED, m_handle, 0);
    if (addr != m_addr) goto error;
    addr = ::mmap(
	(char *)m_addr + m_mmapLength, m_mmapLength,
	prot, mmapFlags | MAP_FIXED, m_handle, 0);
    if (addr != (void *)((char *)m_addr + m_mmapLength)) goto error;
  } else {
    m_addr = ::mmap(0, m_mmapLength, prot, mmapFlags, m_handle, 0);
    if (m_addr == MAP_FAILED || !m_addr) goto error;
  }
  *((char *)m_addr + (m_mmapLength - 1)) = (char)0;
#else
  if (flags & Shm)
    m_mmapHandle = m_handle;
  else {
    DWORD protectFlags = (flags & ReadOnly) ? PAGE_READONLY : PAGE_READWRITE;
    m_mmapHandle = CreateFileMapping(m_handle, 0, protectFlags, 0, 0, 0);
    if (!m_mmapHandle || m_mmapHandle == INVALID_HANDLE_VALUE) goto error;
  }
  {
    DWORD accessFlags = (flags & ReadOnly) ? FILE_MAP_READ : FILE_MAP_WRITE;
    if (flags & ShmDbl) {
retry:
      m_addr = VirtualAlloc(
	  0, (DWORD)(m_mmapLength<<1), MEM_RESERVE, PAGE_NOACCESS);
      if (!m_addr) goto error;
      if (!VirtualFree(m_addr, 0, MEM_RELEASE)) goto error;
      void *addr = MapViewOfFileEx(
	  m_mmapHandle, (DWORD)accessFlags, 0, 0, (DWORD)m_mmapLength, m_addr);
      if (!addr) goto retry;
      if (addr != m_addr) { UnmapViewOfFile(addr); goto retry; }
      addr = MapViewOfFileEx(
	  m_mmapHandle, (DWORD)accessFlags, 0, 0,
	  (DWORD)m_mmapLength, (char *)m_addr + m_mmapLength);
      if (!addr) goto retry;
      if (addr != (void *)((char *)m_addr + m_mmapLength)) {
	UnmapViewOfFile(m_addr);
	UnmapViewOfFile(addr);
	goto retry;
      }
    } else {
      m_addr = MapViewOfFile(m_mmapHandle, accessFlags, 0, 0, 0);
      if (!m_addr) goto error;
    }
  }
#endif

  return Zi::OK;

error:
  close();
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

int ZiFile::shadow(const ZiFile &file, ZeError *e)
{
  Guard guard(m_lock);

  if (m_handle != ZiPlatform::nullHandle()) {
    *e = ZiEINVAL;
    return Zi::IOError;
  }

  m_handle = file.m_handle;
  m_flags = file.m_flags | Shadow;
  m_offset = file.m_offset;
  m_blkSize = file.m_blkSize;
  m_addr = file.m_addr;
  m_mmapLength = file.m_mmapLength;
#ifndef _WIN32
  m_shmName = file.m_shmName;
#else
  m_mmapHandle = file.m_mmapHandle;
#endif

  return Zi::OK;
}

void ZiFile::close()
{
  Guard guard(m_lock);

  if (m_handle == ZiPlatform::nullHandle()) return;

  if (m_flags & Shadow) goto closed;

  if (m_addr) {
#ifndef _WIN32
    munmap(m_addr, m_mmapLength);
    if (m_flags & ShmDbl)
      munmap((char *)m_addr + m_mmapLength, m_mmapLength);
    if ((m_flags & ShmGC) && m_shmName)
      shm_unlink(m_shmName);
    m_shmName = ZtString();
#else
    if (m_mmapHandle != m_handle) CloseHandle(m_mmapHandle);
    m_mmapHandle = ZiPlatform::nullHandle();
    UnmapViewOfFile(m_addr);
    if (m_flags & ShmDbl)
      UnmapViewOfFile((char *)m_addr + m_mmapLength);
#endif
  }

#ifndef _WIN32
  ::close(m_handle);
#else
  CloseHandle(m_handle);
#endif

closed:
  m_handle = ZiPlatform::nullHandle();
  m_flags = 0;
  m_offset = 0;
  m_addr = 0;
  m_mmapLength = 0;
#ifndef _WIN32
  m_shmName.null();
#else
  m_mmapHandle = ZiPlatform::nullHandle();
#endif
}

int ZiFile::init(Handle handle, unsigned flags, ZeError *e)
{
  int blkSize;

#ifndef _WIN32
  struct stat s;

  if (fstat(handle, &s) < 0) goto error;
  blkSize = s.st_blksize;
#else
  blkSize = ZiFile_WindowsDrives::blkSize(handle);
#endif

  {
    Guard guard(m_lock);

    if (m_flags & GC) close();
    init_(handle, flags, blkSize);
    return Zi::OK;
  }

#ifndef _WIN32
error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
#endif
}

void ZiFile::init_(
    Handle handle, unsigned flags, int blkSize, Offset mmapLength)
{
  Guard guard(m_lock);

  m_handle = handle;
  m_flags = flags;
  m_blkSize = blkSize;
  m_offset = (flags & Append) ? size() : 0;
  m_mmapLength = mmapLength;
}

ZiFile::Offset ZiFile::size()
{
  Guard guard(m_lock);

#ifndef _WIN32
  off_t o;
  if ((o = lseek(m_handle, 0, SEEK_END)) == (off_t)-1) return 0;
  return o;
#else
  DWORD l, h;

  l = GetFileSize(m_handle, &h);
  return (static_cast<Offset>(h)<<32) | l;
#endif
}

#if 0
ZiFile::Offset ZiFile::offset()
{
#ifndef _WIN32
  return lseek(m_handle, 0, SEEK_CUR);
#else
  return SetFilePointer(m_handle, 0, 0, FILE_CURRENT);
#endif
}

int ZiFile::seek(Offset offset)
{
  Guard guard(m_lock);
#ifndef _WIN32
  if (lseek(m_handle, offset, SEEK_BEG) == (off_t)-1) goto error;
#else
  if (SetFilePointer(m_handle, offset, 0, FILE_BEGIN) ==
      INVALID_SET_FILE_POINTER)
    goto error;
#endif

  m_offset = offset;

  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}
#endif

int ZiFile::read(void *ptr, unsigned len, ZeError *e)
{
  Guard guard(m_lock);
  int r = pread(m_offset, ptr, len, e);
  if (r > 0) m_offset += r;
  return r;
}

int ZiFile::readv(const ZiVec *vecs, unsigned nVecs, ZeError *e)
{
  Guard guard(m_lock);
  int r = preadv(m_offset, vecs, nVecs, e);
  if (r > 0) m_offset += r;
  return r;
}

int ZiFile::preadv(Offset offset, const ZiVec *vecs, unsigned nVecs, ZeError *e)
{
#if 0
  unsigned len = 0;
  unsigned i;

  for (i = 0; i < nVecs; i++) len += ZiVec_len(vecs[i]);

  ZePlatform::ErrNo errNo;
  int r;

retry:
  r = ::preadv(m_handle, vecs, nVecs, offset);
  if (r < 0) {
    errNo = errno;
    switch (errNo) {
      case EINTR:
      case EAGAIN:
	goto retry;
      default:
	goto error;
    }
  }

  if (!r) return Zi::EndOfFile;

  offset += r;

  if (r < len) {
    // adjust r downwards to the nearest Vec boundary
    {
      int r_ = r;
      int n;
      r = 0;
      for (i = 0; i < nVecs; i++) {
	n = ZiVec_len(vecs[i]);
	if (r_ < n) break;
	r_ -= n;
	r += n;
      }
    }
    vecs += i;
    nVecs -= i;
    len -= r;
    goto retry;
  }

  return Zi::OK;

error:
  if (e) *e = ZeError(errNo);
  return Zi::IOError;
#else
  // Windows	- ReadFileScatter() cannot be used since it only accepts
  //		  page-sized and page-aligned buffers
  // Linux	- preadv() is missing

  int total = 0, r = 0;

  for (unsigned i = 0; i < nVecs; i++) {
    void *ptr = ZiVec_ptr(vecs[i]);
    unsigned len = ZiVec_len(vecs[i]);
    r = pread(offset, ptr, len, e);
    if (r < 0) return total ? total : r;
    total += r, offset += r;
    if (r < (int)len) break;
  }
  return total;
#endif
}

int ZiFile::pread(Offset offset, void *ptr, unsigned len, ZeError *e)
{
  if (!len) return 0;

  ZePlatform::ErrNo errNo;
  int total = 0;
#ifndef _WIN32
  int r;
#else
  DWORD r;
#endif

retry:

#ifndef _WIN32
  r = ::pread(m_handle, ptr, len, offset);
  if (r < 0) {
    errNo = errno;
    switch (errNo) {
      case EINTR:
      case EAGAIN:
	goto retry;
      default:
	goto error;
    }
  }
#else
  OVERLAPPED o{0};

  o.Offset = static_cast<DWORD>(offset);
  o.OffsetHigh = static_cast<DWORD>(offset>>32);
  if (!ReadFile(m_handle, ptr, len, &r, &o)) {
    errNo = GetLastError();
    if (errNo == ERROR_HANDLE_EOF) return total ? total : Zi::EndOfFile;
    if (errNo != ERROR_IO_PENDING) goto error;
    if (!GetOverlappedResult(m_handle, &o, &r, TRUE)) {
      errNo = GetLastError();
      if (errNo == ERROR_HANDLE_EOF) return total ? total : Zi::EndOfFile;
      goto error;
    }
  }
#endif

  if (!r) return total ? total : Zi::EndOfFile;

  total += r, offset += r;

  if ((unsigned)r < len) {
    ptr = (void *)((char *)ptr + r);
    len -= r;
    goto retry;
  }

  return total;

error:
  if (e) *e = ZeError(errNo);
  return total ? total : Zi::IOError;
}

int ZiFile::write(const void *ptr, unsigned len, ZeError *e)
{
  Guard guard(m_lock);
  int r = pwrite(m_offset, ptr, len, e);
  if (r == Zi::OK) m_offset += len;
  return r;
}

int ZiFile::writev(const ZiVec *vecs, unsigned nVecs, ZeError *e)
{
  Guard guard(m_lock);
  int r = pwritev(m_offset, vecs, nVecs, e);
  if (r == Zi::OK)
    for (unsigned i = 0; i < nVecs; i++) m_offset += ZiVec_len(vecs[i]);
  return r;
}

int ZiFile::pwritev(Offset offset, const ZiVec *vecs, unsigned nVecs, ZeError *e)
{
#if 0
  unsigned len = 0;
  unsigned i;

  for (i = 0; i < nVecs; i++) len += ZiVec_len(vecs[i]);

  ZePlatform::ErrNo errNo;
  int r;

retry:
  r = ::pwritev(m_handle, vecs, nVecs, offset);
  if (r < 0) {
    errNo = errno;
    switch (errNo) {
      case EINTR:
      case EAGAIN:
	goto retry;
      default:
	goto error;
    }
  }

  offset += r;

  if (r < len) {
    // adjust r downwards to the nearest Vec boundary
    {
      int r_ = r;
      int n;
      r = 0;
      for (i = 0; i < nVecs; i++) {
	n = ZiVec_len(vecs[i]);
	if (r_ < n) break;
	r_ -= n;
	r += n;
      }
    }
    vecs += i;
    nVecs -= i;
    len -= r;
    goto retry;
  }

  return Zi::OK;

error:
  if (e) *e = ZeError(errNo);
  return Zi::IOError;
#else
  // Windows	- WriteFileGather() cannot be used since it only accepts
  //		  page-sized and page-aligned buffers
  // Linux	- pwritev() is missing

  int r = 0;

  for (unsigned i = 0; i < nVecs; i++) {
    const void *ptr = ZiVec_ptr(vecs[i]);
    unsigned len = ZiVec_len(vecs[i]);
    r = pwrite(offset, ptr, len, e);
    if (r != Zi::OK) return r;
    offset += len;
  }
  return r;
#endif
}

int ZiFile::pwrite(Offset offset, const void *ptr, unsigned len, ZeError *e)
{
  if (!len) return Zi::OK;

  ZePlatform::ErrNo errNo;
#ifndef _WIN32
  int r;
#else
  DWORD r;
#endif

retry:

#ifndef _WIN32
  r = ::pwrite(m_handle, ptr, len, offset);
  if (r <= 0) {
    errNo = errno;
    switch (errNo) {
      case EINTR:
      case EAGAIN:
	goto retry;
      default:
	goto error;
    }
  }
#else
  OVERLAPPED o;

  o.Internal = 0;
  o.InternalHigh = 0;
  o.Offset = (DWORD)offset;
  o.OffsetHigh = (DWORD)(offset>>32);
  o.hEvent = 0;

  if (!WriteFile(m_handle, ptr, len, &r, &o)) {
    errNo = GetLastError();
    if (errNo != ERROR_IO_PENDING) goto error;
    if (!GetOverlappedResult(m_handle, &o, &r, TRUE)) r = 0;
  }
  if (!r) {
    errNo = GetLastError();
    goto error;
  }
#endif

  offset += r;

  if ((unsigned)r < len) {
    ptr = (void *)((char *)ptr + r);
    len -= r;
    goto retry;
  }

  return Zi::OK;

error:
  if (e) *e = ZeError(errNo);
  return Zi::IOError;
}

int ZiFile::sync(ZeError *e)
{
#ifndef _WIN32
  if (fsync(m_handle) < 0) goto error;
#else
  if (!FlushFileBuffers(m_handle)) goto error;
#endif

  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

int ZiFile::msync(void *addr, Offset length, ZeError *e)
{
  if (!m_addr) {
#ifndef _WIN32
    if (e) *e = EBADF;
#else
    if (e) *e = ERROR_INVALID_HANDLE;
#endif
    return Zi::IOError;
  }
#ifndef _WIN32
  if (::msync(addr ? addr : m_addr, length ? length : m_mmapLength,
	      MS_SYNC | MS_INVALIDATE) < 0) goto error;
#else
  if (!FlushViewOfFile(addr ? addr : m_addr,
		       (SIZE_T)(length ? length : m_mmapLength))) goto error;
#endif

  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

ZmTime ZiFile::mtime(const Path &name, ZeError *e)
{
#ifndef _WIN32
  struct stat s;
  if (::stat(name, &s) < 0) goto error;
  return ZmTime{s.st_mtime};
#else
  Handle h;
  FILETIME mtime;
  h = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (h == INVALID_HANDLE_VALUE) goto error;
  if (!GetFileTime(h, 0, 0, &mtime)) { CloseHandle(h); goto error; }
  CloseHandle(h);
  return ZmTime{mtime};
#endif

error:
  if (e) *e = ZeLastError;
  return ZmTime{};
}

bool ZiFile::isdir(const Path &name, ZeError *e)
{
#ifndef _WIN32
  struct stat s;
  if (::stat(name, &s) < 0) goto error;
  return S_ISDIR(s.st_mode);
#else
  DWORD a = GetFileAttributes(name);
  if (a == INVALID_FILE_ATTRIBUTES) goto error;
  return a & FILE_ATTRIBUTE_DIRECTORY;
#endif

error:
  if (e) *e = ZeLastError;
  return false;
}

int ZiFile::remove(const Path &name, ZeError *e)
{
#ifndef _WIN32
  if (::remove(name) < 0) goto error;
#else
  if (!DeleteFile(name)) goto error;
#endif

  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

int ZiFile::rename(const Path &oldName, const Path &newName, ZeError *e)
{
#ifndef _WIN32
  if (::rename(oldName, newName) < 0) goto error;
#else
  if (!MoveFileEx(oldName, newName, MOVEFILE_REPLACE_EXISTING)) goto error;
#endif

  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

int ZiFile::copy(const Path &oldName, const Path &newName, ZeError *e_)
{
  ZeError e;

#ifndef _WIN32
  {
    ZiFile oldHandle, newHandle;
    if (oldHandle.open(oldName, ReadOnly, 0777, &e) != Zi::OK) goto error;
    if (newHandle.open(
	  newName, WriteOnly | Create | Truncate, 0777, &e) != Zi::OK)
      goto error;
    unsigned oldBlkSize = oldHandle.blkSize();
    unsigned newBlkSize = newHandle.blkSize();
    unsigned maxBlkSize = oldBlkSize > newBlkSize ? oldBlkSize : newBlkSize;
    unsigned bufSize = ZiFile_CopyBufSize;
    bufSize += maxBlkSize - 1;
    bufSize -= (bufSize % maxBlkSize);
    Offset size = oldHandle.size();
    if ((Offset)bufSize > size) bufSize = size;
    ZtArray<char> buf(bufSize);
    for (Offset o = 0; o < size; o += bufSize) {
      Offset n = size - o;
      if (n > (Offset)bufSize) n = bufSize;
      if (oldHandle.pread(o, buf.data(), (int)n, &e) != Zi::OK) goto error;
      if (newHandle.pwrite(o, buf.data(), (int)n, &e) != Zi::OK) goto error;
    }
  }
#else
  if (!CopyFileEx(oldName, newName, 0, 0, 0, 0)) {
    e = ZeLastError;
    goto error;
  }
#endif

  return Zi::OK;

error:
  if (e_) *e_ = e;
  return Zi::IOError;
}

int ZiFile::mkdir(const Path &name, ZeError *e)
{
#ifndef _WIN32
  if (::mkdir(name, 0777) < 0) goto error;
#else
  if (!CreateDirectory(name, 0)) goto error;
#endif
  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

int ZiFile::rmdir(const Path &name, ZeError *e)
{
#ifndef _WIN32
  if (::rmdir(name) < 0) goto error;
#else
  if (!RemoveDirectory(name)) goto error;
#endif
  return Zi::OK;

error:
  if (e) *e = ZeLastError;
  return Zi::IOError;
}

void ZiFile::age(const Path &name_, unsigned max)
{
  ZtString name = name_;
  unsigned size = name.size() + ZuBoxed(max).length() + 4;

  ZtString prevName_(size), nextName_(size), sideName_(size);
  ZtString *prevName = &prevName_;
  ZtString *nextName = &nextName_;
  ZtString *sideName = &sideName_;

  *prevName << name;
  bool last = false;
  unsigned i;
  for (i = 0; i < max && !last; i++) {
    nextName->length(0);
    *nextName << name << '.' << ZuBoxed(i + 1);
    sideName->length(0);
    *sideName << *nextName << '_';
    last = (rename(*nextName, *sideName) == Zi::IOError);
    rename(*prevName, *nextName);
    ZtString *oldName = prevName;
    prevName = sideName;
    sideName = oldName;
  }
  if (i == max) remove(*prevName);
}

ZiFile::Path ZiFile::cwd()
{
  Path ret(ZiPlatform::PathMax + 1);

#ifndef _WIN32
  if (!getcwd(ret.data(), ZiPlatform::PathMax + 1))
    ret.null();
  else {
    ret.calcLength();
    ret.truncate();
  }
#else
  ZtWString dir(ZiPlatform::PathMax + 1);
  dir.length(GetCurrentDirectory(ZiPlatform::PathMax + 1, dir));
  if (!dir.length())
    ret.null();
  else {
    ret.length(GetFullPathName(dir, ZiPlatform::PathMax + 1, ret.data(), 0));
    ret.truncate();
  }
#endif
  return ret;
}

bool ZiFile::absolute(const Path &name)
{
#ifndef _WIN32
  return name[0] == '/';
#else
  return name[0] == L'\\' || name[0] == L'/' ||
	 (((name[0] >= L'a' && name[0] <= L'z') ||
	   (name[0] >= L'A' && name[0] <= L'Z')) && name[1] == L':');
#endif
}

ZiFile::Path ZiFile::leafname(const Path &name)
{
  int o, n = name.length();

  for (o = n; --o >= 0; )
#ifndef _WIN32
    if (name[o] == '/') break;
#else
    if (name[o] == L'\\' || name[o] == L'/') break;
#endif
  if (o < 0) return name;
  return name.splice(o + 1);
}

ZiFile::Path ZiFile::dirname(const Path &name)
{
  int o, n = name.length();

  for (o = n; --o >= 0; )
#ifndef _WIN32
    if (name[o] == '/') break;
  if (o < 0) return '.';
#else
    if (name[o] == L'\\' || name[o] == L'/') break;
  if (o < 0) return L".";
#endif
  return name.splice(0, o);
}

ZiFile::Path ZiFile::append(const Path &dir, const Path &name)
{
  Path ret(dir.length() + 1 + name.length() + 1);

  ret << dir;
#ifndef _WIN32
  ret << '/';
#else
  ret << L"\\"; // not single wchar_t due to type ambiguity
#endif
  ret << name;
  return ret;
}
