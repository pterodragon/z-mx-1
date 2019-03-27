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

// utility class to capture a ring of recent backtraces and print them on demand

#ifndef ZmBackTracer_HPP
#define ZmBackTracer_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZmLib_HPP
#include <ZmLib.hpp>
#endif

#include <ZuTuple.hpp>
#include <ZuUnion.hpp>

#include <ZmBackTrace.hpp>
#include <ZmThread.hpp>
#include <ZmPLock.hpp>

template <unsigned N = 64> class ZmBackTracer {
public:
  inline ZmBackTracer() { }
  inline ~ZmBackTracer() { }

private:
  typedef ZuTuple<ZmThreadID, ZmThreadName, ZmBackTrace> Data;

public:
  inline void capture(unsigned skip = 0) {
    ZmGuard<ZmPLock> guard(m_lock);
    unsigned i = m_offset;
    m_offset = (i + 1) & 63;
    Data *data = new (m_captures[i].new1()) Data();
    ZmThreadContext *self = ZmThreadContext::self();
    data->p1() = self->tid();
    data->p2() = self->name();
    data->p3().capture(skip + 1);
  }

  template <typename S> void dump(S &s) const {
    ZmGuard<ZmPLock> guard(m_lock);
    bool first = true;
    for (unsigned i = 0; i < N; i++) {
      unsigned j = (m_offset + (N - 1) - i) % N;
      if (m_captures[j].type() == 1) {
	const Data &data = m_captures[j].p1();
	if (!first) s << "---\n";
	first = false;
	s << data.p2() << " (TID " << data.p1() << ")\n" << data.p3();
      }
    }
  }

private:
  ZmPLock		m_lock;
  ZuBox0(unsigned)	  m_offset;
  ZuUnion<Data>		  m_captures[64];
};

#endif /* ZmBackTracer_HPP */
