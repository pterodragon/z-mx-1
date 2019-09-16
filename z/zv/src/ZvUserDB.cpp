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

// server-side user DB with MFA, API keys, etc.

#include <ZvUserDB.hpp>

namespace ZvUserDB {

Zfb::Offset<fbs::UserDB> UserDB::save(Zfb::FlatBufferBuilder &b) const
{
  using namespace Zfb;
  using namespace Save;
  using B = FlatBufferBuilder;
  auto perms_ = keyVecIter<fbs::Perm>(b, perms.length(),
      [this](B &b, unsigned i) {
	return fbs::CreatePerm(b, i, str(b, perms[i]));
      });
  Offset<Vector<Offset<fbs::Role>>> roles_;
  {
    auto i = roles.readIterator();
    roles_ = keyVecIter<fbs::Role>(b, i.count(),
	[&i](B &b, unsigned j) { return i.iterate()->save(b); });
  }
  Offset<Vector<Offset<fbs::User>>> users_;
  {
    auto i = users.readIterator();
    users_ = keyVecIter<fbs::User>(b, i.count(),
	[&i](B &b, unsigned) { return i.iterate()->save(b); });
  }
  Offset<Vector<Offset<fbs::Key>>> keys_;
  {
    auto i = keys.readIterator();
    keys_ = keyVecIter<fbs::Key>(b, i.count(),
	[&i](B &b, unsigned) { return i.iterate()->save(b); });
  }
  return fbs::CreateUserDB(b, perms_, roles_, users_, keys_);
}

void UserDB::load(const void *buf)
{
  using namespace Zfb::Load;
  auto userDB = fbs::GetUserDB(buf);
  all(userDB->perms(), [this](unsigned, auto perm_) {
    unsigned j = perm_->id();
    if (j >= Bitmap::Bits) return;
    if (perms.length() < j + 1) perms.length(j + 1);
    perms[j] = str(perm_->name());
  });
  all(userDB->roles(), [this](unsigned, auto role_) {
    if (auto role = loadRole(role_)) roles.add(ZuMv(role));
  });
  all(userDB->users(), [this](unsigned, auto user_) {
    if (auto user = loadUser(roles, user_)) users.add(ZuMv(user));
  });
  all(userDB->keys(), [this](unsigned, auto key_) {
    if (auto key = loadKey(key_)) keys.add(ZuMv(key));
  });
}

}
