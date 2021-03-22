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

#include <zlib/ZvCmdDispatcher.hpp>

void ZvCmdDispatcher::init()
{
  m_fnMap = new FnMap{};
}
void ZvCmdDispatcher::final()
{
  m_fnMap->clean();
  m_fnMap = {};
}

void ZvCmdDispatcher::deflt(DefltFn fn)
{
  m_defltFn = fn;
}

void ZvCmdDispatcher::map(ZuID id, Fn fn)
{
  Guard guard(m_lock);
  if (auto kv = m_fnMap->find(id))
    const_cast<FnMap::KV *>(kv)->val() = ZuMv(fn);
  else
    m_fnMap->add(id, ZuMv(fn));
}

int ZvCmdDispatcher::dispatch(
    ZuID id, void *link, const uint8_t *data, unsigned len)
{
  if (auto node = m_fnMap->find(id))
    return (node->val())(link, data, len);
  if (m_defltFn) return m_defltFn(link, id, data, len);
  return -1;
}
