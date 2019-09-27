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

// stack backtrace

#ifndef ZmBackTrace__HPP
#define ZmBackTrace__HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <zlib/ZmLib.hpp>
#endif

#define ZmBackTrace_DEPTH 64
#define ZmBackTrace_BUFSIZ 32768	// 32k

#include <string.h>

class ZmAPI ZmBackTrace {
public:
  ZuInline ZmBackTrace() {
    memset(m_frames, 0, sizeof(void *) * ZmBackTrace_DEPTH);
  }
  inline ZmBackTrace(unsigned skip) {
    memset(m_frames, 0, sizeof(void *) * ZmBackTrace_DEPTH);
    capture(skip + 1);
  }

  ZuInline ZmBackTrace(const ZmBackTrace &t) {
    memcpy(m_frames, t.m_frames, sizeof(void *) * ZmBackTrace_DEPTH);
  }
  ZuInline ZmBackTrace &operator =(const ZmBackTrace &t) {
    if (this != &t)
      memcpy(m_frames, t.m_frames, sizeof(void *) * ZmBackTrace_DEPTH);
    return *this;
  }

  ZuInline bool equals(const ZmBackTrace &t) {
    return !memcmp(m_frames, t.m_frames, sizeof(void *) * ZmBackTrace_DEPTH);
  }

  ZuInline bool operator !=(const ZmBackTrace &t) { return !equals(t); }
  ZuInline bool operator ==(const ZmBackTrace &t) { return equals(t); }

  ZuInline bool operator !() const { return !m_frames[0]; }

  void capture() { capture(0); }
  void capture(unsigned skip);
#ifdef _WIN32
  void capture(EXCEPTION_POINTERS *exInfo, unsigned skip);
#endif

  ZuInline void *const *frames() const { return m_frames; }

private:
  void		*m_frames[ZmBackTrace_DEPTH];
};

#endif /* ZmBackTrace__HPP */
