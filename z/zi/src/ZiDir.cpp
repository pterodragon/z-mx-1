//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ZiFile::(at your option) any later version.
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

// directory scanning

#include <ZiDir.hpp>

int ZiDir::open(const Path &name, ZeError *e)
{
  Guard guard(m_lock);

  close();

#ifdef _WIN32

  if (!isdir(name, e)) {
    if (e && !*e) *e = ZeError(ENOTDIR);
    return Zi::IOError;
  }
  m_match = name + L"\\*";

#else /* _WIN32 */

  if (!(m_dir = opendir(name))) {
    if (e) *e = ZeError(errno);
    return Zi::IOError;
  }

#endif /* _WIN32 */

  return Zi::OK;
}

ZiDir::Path ZiDir::read(ZeError *e)
{
  Guard guard(m_lock);

#ifdef _WIN32

  WIN32_FIND_DATA wfd;

  if (m_handle == INVALID_HANDLE_VALUE) {
    if (!m_match) {
      if (e) *e = ZeError(EBADF);
      return Path();
    }
    m_handle = FindFirstFileEx(m_match, FindExInfoStandard, &wfd,
			       FindExSearchNameMatch, 0, 0);
    if (m_handle == INVALID_HANDLE_VALUE) goto error;
  } else {
    if (!FindNextFile(m_handle, &wfd)) goto error;
  }
  return Path(Path::Copy, wfd.cFileName);

error:
  DWORD errNo;
  switch (errNo = GetLastError()) {
    case ERROR_NO_MORE_FILES:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      break;
    default:
      if (e) *e = ZeError(errNo);
      break;
  }
  close();
  return Path();

#else /* _WIN32 */

  if (!m_dir) {
    if (e) *e = ZeError(EBADF);
    return Path();
  }

  struct dirent d, *r;
  int i;

  if (i = readdir_r(m_dir, &d, &r)) goto error;
  if (!r) return Path();
  return Path(Path::Copy, d.d_name);

error:
  if (e) *e = ZeError(errno);
  close();
  return Path();

#endif /* _WIN32 */
}

void ZiDir::close()
{
  Guard guard(m_lock);

#ifdef _WIN32
  m_match = Path();
  if (m_handle != INVALID_HANDLE_VALUE) {
    FindClose(m_handle);
    m_handle = INVALID_HANDLE_VALUE;
  }
#else
  if (m_dir) {
    closedir(m_dir);
    m_dir = 0;
  }
#endif
}

bool ZiDir::isdir(const Path &name, ZeError *e)
{
#ifdef _WIN32
  DWORD a = GetFileAttributes(name);
  if (a == INVALID_FILE_ATTRIBUTES) {
    int errNo = GetLastError();
    if (e) *e = ZeError(errNo);
    return false;
  }
  if (!(a & FILE_ATTRIBUTE_DIRECTORY)) {
    if (e) *e = ZeError(ENOTDIR);
    return false;
  }
  return true;
#else
  struct stat s;
  if (stat(name, &s)) {
    if (e) *e = ZeError(errno);
    return false;
  }
  return S_ISDIR(s.st_mode);
#endif
}
