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
#include <zlib/ZmLib.hpp>
#endif

#include <zlib/ZuTuple.hpp>
#include <zlib/ZuUnion.hpp>

#include <zlib/ZmBackTrace.hpp>
#include <zlib/ZmThread.hpp>
#include <zlib/ZmPLock.hpp>

template <unsigned N = 64> class ZmBackTracer {
public:
  ZmBackTracer() { }
  ~ZmBackTracer() { }

private:
  typedef ZuTuple<ZmThreadID, ZmThreadName, ZmBackTrace> Data;

  using Lock = ZmPLock;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

public:
  void capture(unsigned skip = 0) {
    Guard guard(m_lock);
    unsigned i = m_offset;
    m_offset = (i + 1) & 63;
    Data *data = new (m_captures[i].template new_<0>()) Data();
    ZmThreadContext *self = ZmThreadContext::self();
    data->p<0>() = self->tid();
    data->p<1>() = self->name();
    data->p<2>().capture(skip + 1);
  }

  template <typename S> void dump(S &s) const {
    ReadGuard guard(m_lock);
    bool first = true;
    for (unsigned i = 0; i < N; i++) {
      unsigned j = (m_offset + (N - 1) - i) % N;
      if (m_captures[j].type() == 1) {
	const Data &data = m_captures[j].template p<0>();
	if (!first) s << "---\n";
	first = false;
	s << data.template p<1>()
	  << " (TID " << data.template p<0>()
	  << ")\n" << data.template p<2>();
      }
    }
  }

private:
  ZmPLock		m_lock;
  ZuBox0(unsigned)	  m_offset;
  ZuUnion<Data>		  m_captures[64];
};

#endif /* ZmBackTracer_HPP */
