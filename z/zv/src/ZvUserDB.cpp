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

#include <ZtlsBase32.hpp>

namespace ZvUserDB {

bool UserDB::init(
    Ztls::Random *rng, unsigned passLen, ZtString &passwd, ZtString &secret)
{
  Guard guard(m_lock);
  if (!m_perms.length())
    permAdd_(
	"login",
	"userList", "userAdd", "userMod", "pwReset", "userDel",
	"roleList", "roleAdd", "roleMod", "roleDel",
	"permList", "permAdd", "permMod", "permDel",
	"keyList", "keyAdd", "keyDel", "keyClr");
  if (!m_roles.count())
    roleAdd_("admin", Role::Immutable, 0x3ffff);
  if (!m_users.count_()) {
    ZmRef<User> user = userAdd_(
	0, "admin", "admin", User::Immutable | User::Enabled,
	rng, passLen, passwd);
    secret.length(Ztls::Base32::enclen(user->secret.length()));
    Ztls::Base32::encode(
	secret.data(), secret.length(),
	user->secret.data(), user->secret.length());
    return true;
  }
  return false;
}

ZmRef<User> UserDB::userAdd_(
    uint64_t id, ZuString name, ZuString role, User::Flags flags,
    Ztls::Random *rng, unsigned passLen, ZtString &passwd)
{
  ZmRef<User> user = new User(id, name, flags);
  {
    User::KeyData passwd_;
    passLen = Ztls::Base64::declen(passLen);
    if (passLen > passwd_.size()) passLen = passwd_.size();
    passwd_.length(passLen);
    rng->random(passwd_.data(), passLen);
    passwd.length(Ztls::Base64::enclen(passLen));
    Ztls::Base64::encode(
	passwd.data(), passwd.length(), passwd_.data(), passwd_.length());
  }
  user->secret.length(user->secret.size());
  rng->random(user->secret.data(), user->secret.length());
  {
    Ztls::HMAC hmac(MBEDTLS_MD_SHA256);
    hmac.start(passwd.data(), passwd.length());
    hmac.update(user->secret.data(), user->secret.length());
    user->hmac.length(user->hmac.size());
    hmac.finish(user->hmac.data());
  }
  if (auto node = m_roles.find(role)) {
    user->roles.push(node);
    user->perms = node->perms;
  }
  user->flags = flags;
  m_users.add(user);
  m_userNames.add(user);
  return user;
}

void UserDB::load(const void *buf)
{
  Guard guard(m_lock);
  using namespace Zfb::Load;
  auto userDB = fbs::GetUserDB(buf);
  all(userDB->perms(), [this](unsigned, auto perm_) {
    unsigned j = perm_->id();
    if (j >= Bitmap::Bits) return;
    if (m_perms.length() < j + 1) m_perms.length(j + 1);
    m_perms[j] = str(perm_->name());
  });
  all(userDB->roles(), [this](unsigned, auto role_) {
    if (auto role = loadRole(role_)) {
      m_roles.del(role->name);
      m_roles.add(ZuMv(role));
    }
  });
  all(userDB->users(), [this](unsigned, auto user_) {
    if (auto user = loadUser(m_roles, user_)) {
      m_users.del(user->id);
      m_users.add(ZuMv(user));
    }
  });
  all(userDB->keys(), [this](unsigned, auto key_) {
    if (auto key = loadKey(key_)) {
      m_keys.del(key->id);
      m_keys.add(ZuMv(key));
    }
  });
}

Zfb::Offset<fbs::UserDB> UserDB::save(Zfb::Builder &b) const
{
  ReadGuard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto perms_ = keyVecIter<fbs::Perm>(b, m_perms.length(),
      [this](Builder &b, unsigned i) {
	return fbs::CreatePerm(b, i, str(b, m_perms[i]));
      });
  Offset<Vector<Offset<fbs::Role>>> roles_;
  {
    auto i = m_roles.readIterator();
    roles_ = keyVecIter<fbs::Role>(b, i.count(),
	[&i](Builder &b, unsigned j) { return i.iterate()->save(b); });
  }
  Offset<Vector<Offset<fbs::User>>> users_;
  {
    auto i = m_users.readIterator();
    users_ = keyVecIter<fbs::User>(b, i.count(),
	[&i](Builder &b, unsigned) { return i.iterate()->save(b); });
  }
  Offset<Vector<Offset<fbs::Key>>> keys_;
  {
    auto i = m_keys.readIterator();
    keys_ = keyVecIter<fbs::Key>(b, i.count(),
	[&i](Builder &b, unsigned) { return i.iterate()->save(b); });
  }
  return fbs::CreateUserDB(b, perms_, roles_, users_, keys_);
}

}
