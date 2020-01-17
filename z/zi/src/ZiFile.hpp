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

#ifndef ZiFile_HPP
#define ZiFile_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZiLib_HPP
#include <zlib/ZiLib.hpp>
#endif

#include <zlib/ZmLock.hpp>
#include <zlib/ZmGuard.hpp>

#include <zlib/ZePlatform.hpp>

#include <zlib/ZiPlatform.hpp>

#ifndef _WIN32
#include <sys/mman.h>
#endif

#ifndef _WIN32
#include <alloca.h>

#define ZiENOMEM ENOMEM
#else
#define ZiENOMEM ERROR_NOT_ENOUGH_MEMORY
#endif

class ZiAPI ZiFile {
  ZiFile(const ZiFile &) = delete;
  ZiFile &operator =(const ZiFile &) = delete;	// prevent mis-use

public:
  typedef ZiPlatform::Handle Handle;
  typedef ZiPlatform::Path Path;
  typedef ZiPlatform::Offset Offset;
  typedef ZiPlatform::MMapPtr MMapPtr;

  typedef ZmLock Lock;
  typedef ZmGuard<Lock> Guard;
  typedef ZmReadGuard<Lock> ReadGuard;

  enum Flags {
    ReadOnly	= 0x0001,
    WriteOnly	= 0x0002,
    Create	= 0x0004,
    Exclusive	= 0x0008,
    Truncate	= 0x0010,
    Append	= 0x0020,
    Direct	= 0x0040,// O_DIRECT (Unix) / FILE_FLAG_NO_BUFFERING  (Windows)
    Sync	= 0x0080,// O_DSYNC  (Unix) / FILE_FLAG_WRITE_THROUGH (Windows)
    GC		= 0x0100,// close() handle in destructor
    MMap	= 0x0200,// memory-mapped file (set internally by mmap())
    Shm		= 0x0400,// global named shared memory, not a real file
    ShmGC	= 0x0800,// remove shared memory on close()
    ShmDbl	= 0x1000,// map two adjacent copies of the same memory
    MMPopulate	= 0x2000,// MAP_POPULATE
    Shadow	= 0x4000 // shadow already opened file
  };

  // Note: Direct requires caller align all reads/writes to blkSize()

  ZiFile() :
      m_handle(ZiPlatform::nullHandle()), m_flags(0),
      m_offset(0), m_blkSize(0), m_addr(0), m_mmapLength(0)
#ifdef _WIN32
      , m_mmapHandle(ZiPlatform::nullHandle())
#endif
  { }

  ~ZiFile() { final(); }

  ZuInline Handle handle() { return m_handle; }
  ZuInline void *addr() const { return m_addr; }
  ZuInline Offset mmapLength() const { return m_mmapLength; }

  ZuInline unsigned flags() { return m_flags; }
  void setFlags(int f) { Guard guard(m_lock); m_flags |= f; }
  void clrFlags(int f) { Guard guard(m_lock); m_flags &= ~f; }

  int init(Handle handle, unsigned flags, ZeError *e = 0);
  void final() {
    Guard guard(m_lock);

    if (m_flags & GC) {
      close();
    } else {
      m_handle = ZiPlatform::nullHandle();
#ifdef _WIN32
      m_mmapHandle = ZiPlatform::nullHandle();
#endif
    }
  }

  ZuInline bool operator !() const {
    ReadGuard guard(m_lock);
    return ZiPlatform::nullHandle(m_handle);
  }
  ZuOpBool

  int open(const Path &name,
      unsigned flags, unsigned mode = 0666, ZeError *e = 0);
  int open(const Path &name,
      unsigned flags, unsigned mode, Offset length, ZeError *e = 0);
  int mmap(const Path &name,
      unsigned flags, Offset length, bool shared = true,
      int mmapFlags = 0, unsigned mode = 0666, ZeError *e = 0);
  int shadow(const ZiFile &file, ZeError *e = 0);
  void close();

  Offset size();
  int blkSize() { return m_blkSize; }

  Offset offset() { ReadGuard guard(m_lock); return m_offset; }
  void seek(Offset offset) { Guard guard(m_lock); m_offset = offset; }

  int sync(ZeError *e = 0);
  int msync(void *addr = 0, Offset length = 0, ZeError *e = 0);

  int read(void *ptr, unsigned len, ZeError *e = 0);
  int readv(const ZiVec *vecs, unsigned nVecs, ZeError *e = 0);

  int write(const void *ptr, unsigned len, ZeError *e = 0);
  int writev(const ZiVec *vecs, unsigned nVecs, ZeError *e = 0);

  int pread(Offset offset, void *ptr, unsigned len, ZeError *e = 0);
  int preadv(Offset offset, const ZiVec *vecs, unsigned nVecs, ZeError *e = 0);

  int pwrite(Offset offset, const void *ptr, unsigned len, ZeError *e = 0);
  int pwritev(Offset offset, const ZiVec *vecs, unsigned nVecs, ZeError *e = 0);

  // Note: unbuffered!
  template <typename V> ZiFile &operator <<(V &&v) {
    append(ZuFwd<V>(v));
    return *this;
  }

  static ZmTime mtime(const Path &name, ZeError *e = 0);
  static bool isdir(const Path &name, ZeError *e = 0);

  static int remove(const Path &name, ZeError *e = 0);
  static int rename(const Path &oldName, const Path &newName, ZeError *e = 0);
  static int copy(const Path &oldName, const Path &newName, ZeError *e = 0);
  static int mkdir(const Path &name, ZeError *e = 0);
  static int rmdir(const Path &name, ZeError *e = 0);

  static Path cwd();

  static bool absolute(const Path &name);

  static Path leafname(const Path &name);
  static Path dirname(const Path &name);
  static Path append(const Path &dir, const Path &name);

  static void age(const Path &name, unsigned max);

private:
  int open_(const Path &name,
      unsigned flags, unsigned mode, Offset length, ZeError *e);

  void init_(Handle handle, unsigned flags, int blkSize, Offset mmapLength = 0);

  template <typename U, typename R = void,
	   bool B = ZuPrint<U>::Delegate && !ZuTraits<U>::IsString>
  struct MatchPDelegate;
  template <typename U, typename R>
  struct MatchPDelegate<U, R, 1> { typedef R T; };
  template <typename U, typename R = void,
	   bool B = ZuPrint<U>::Buffer && !ZuTraits<U>::IsString>
  struct MatchPBuffer;
  template <typename U, typename R>
  struct MatchPBuffer<U, R, 1> { typedef R T; };

  template <typename S> typename ZuIsString<S>::T append(S &&s_) {
    ZuString s(ZuFwd<S>(s_));
    if (ZuUnlikely(!s)) return;
    ZeError e;
    if (ZuUnlikely(write(s.data(), s.length(), &e) != Zi::OK))
      throw e;
  }
  template <typename P> typename MatchPDelegate<P>::T append(P &&p) {
    ZuPrint<P>::print(*this, ZuFwd<P>(p));
  }
  template <typename P> typename MatchPBuffer<P>::T append(const P &p) {
    unsigned len = ZuPrint<P>::length(p);
    char *buf;
#ifdef _MSC_VER
    __try {
      buf = (char *)_alloca(len);
    } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
      _resetstkoflw();
      buf = 0;
    }
#else
    buf = (char *)alloca(len);
#endif
    if (ZuUnlikely(!buf)) throw ZeError(ZiENOMEM);
    ZeError e;
    if (ZuUnlikely(write(buf, ZuPrint<P>::print(buf, len, p), e) != Zi::OK))
      throw e;
  }

  Lock		m_lock;
    Handle	  m_handle;
    unsigned	  m_flags;
    Offset	  m_offset;
    int		  m_blkSize;
    void	  *m_addr;
    Offset	  m_mmapLength;
#ifndef _WIN32
    ZtString	  m_shmName;
#else
    Handle	  m_mmapHandle;
#endif
};

#endif /* ZiFile_HPP */
