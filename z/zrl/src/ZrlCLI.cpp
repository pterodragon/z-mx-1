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

// command line interface

#include <zlib/ZrlCLI.hpp>

using namespace Zrl;

namespace { static ZmSemaphore done; }

void CLI::init(App app)
{
  app.end = [end = ZuMv(app.end)]() { done.post(); end(); };
  app.sig = [sig = ZuMv(app.sig)](int i) { done.post(); sig(i); };
  Editor::init(ZuMv(app));
  m_sched = new ZmScheduler{ZmSchedParams{}.id("ZrlCLI").nThreads(1)};
  if (auto map = ::getenv("ZRL_MAP"))
    if (!loadMap(map, true)) std::cerr << loadError() << '\n' << std::flush;
  if (auto mapID = ::getenv("ZRL_MAPID"))
    map(mapID);
}

void CLI::final()
{
  Editor::final();
  m_sched = nullptr;
}

void CLI::open()
{
  Editor::open(m_sched, 0);
}

void CLI::start(ZuString prompt)
{
  m_sched->start();
  Editor::start([prompt = ZtArray<uint8_t>{prompt}](Editor &editor) mutable {
    editor.prompt(ZuMv(prompt));
  });
}

void CLI::stop()
{
  Editor::stop();
  m_sched->stop();
}

void CLI::join()
{
  done.wait();
}
