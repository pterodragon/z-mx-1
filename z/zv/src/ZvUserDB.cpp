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
#include <ZtlsTOTP.hpp>

namespace ZvUserDB {

bool UserDB::init(
    Ztls::Random *rng, unsigned passLen, ZtString &passwd, ZtString &secret)
{
  Guard guard(m_lock);
  if (!m_perms.length())
    permAdd_(
	"login",		// == UserDB::Perm::Login
	"access",		// == UserDB::Perm::Access
	"chPass",		// == UserDB::Perm::ChPass
	"userGet", "userAdd", "userMod", "resetPass", "keyClr", "userDel",
	"roleGet", "roleAdd", "roleMod", "roleDel",
	"permGet", "permAdd", "permMod", "permDel",
	"keyGet", "keyAdd", "keyDel");
  if (!m_roles.count())
    roleAdd_("admin", Role::Immutable, Bitmap().fill(), Bitmap());
  if (!m_users.count_()) {
    ZmRef<User> user = userAdd_(
	0, "admin", "admin", User::Immutable | User::Enabled | User::ChPass,
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
    Ztls::HMAC hmac(User::keyType());
    hmac.start(user->secret.data(), user->secret.length());
    hmac.update(passwd.data(), passwd.length());
    user->hmac.length(user->hmac.size());
    hmac.finish(user->hmac.data());
  }
  if (role)
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

ZmRef<User> UserDB::login(
    ZuString name, ZuString passwd, unsigned totp, unsigned totpRange)
{
  ReadGuard guard(m_lock);
  ZmRef<User> user = m_userNames.find(name);
  if (!user) return nullptr;
  if (!(user->flags & User::Enabled)) return nullptr;
  if (!(user->perms && Perm::Login)) return nullptr;
  {
    Ztls::HMAC hmac(User::keyType());
    User::KeyData verify;
    hmac.start(user->secret.data(), user->secret.length());
    hmac.update(passwd.data(), passwd.length());
    verify.length(verify.size());
    hmac.finish(verify.data());
    if (verify != user->hmac) return nullptr;
  }
  if (!Ztls::TOTP::verify(
	user->secret.data(), user->secret.length(), totp, totpRange))
    return nullptr;
  return user;
}

ZmRef<User> UserDB::access(
    ZuString keyID, ZuArray<uint8_t> data, ZuArray<uint8_t> hmac)
{
  ReadGuard guard(m_lock);
  Key *key = m_keys.find(keyID);
  if (!key) return nullptr;
  ZmRef<User> user = m_users.find(key->userID);
  if (!user) return nullptr; // LATER - log warning about key w/ invalid userID
  if (!(user->flags & User::Enabled)) return nullptr;
  if (!(user->perms && Perm::Access)) return nullptr;
  {
    Ztls::HMAC hmac_(Key::keyType());
    Key::KeyData verify;
    hmac_.start(user->secret.data(), user->secret.length());
    hmac_.update(data.data(), data.length());
    verify.length(verify.size());
    hmac_.finish(verify.data());
    if (verify != hmac) return nullptr;
  }
  return user;
}

template <typename T> using Offset = Zfb::Offset<T>;
template <typename T> using Vector = Zfb::Vector<T>;

Offset<Vector<Offset<fbs::User>>> UserDB::userGet(
    Zfb::Builder &b, fbs::UserID *id_)
{
  ReadGuard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto id = id_->id();
  if (id == nullID()) {
    auto i = m_users.readIterator();
    return keyVecIter<fbs::User>(b, i.count(),
	[&i](Builder &b, unsigned) { return i.iterate()->save(b); });
  } else {
    if (auto user = m_users.findPtr(id))
      return keyVec<fbs::User>(b, user->save(b));
    else
      return keyVec<fbs::User>(b);
  }
}

Offset<fbs::UserPass> UserDB::userAdd(Zfb::Builder &b, fbs::User *user_,
    Ztls::Random *rng, unsigned passLen)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  ZtString passwd;
  ZmRef<User> user = userAdd_(
      user_->id(), Load::str(user_->name()), ZuString(),
      user_->flags() | User::ChPass,
      rng, passLen, passwd);
  Load::all(user_->roles(), [this, &user](unsigned, auto roleName) {
    if (auto role = m_roles.findPtr(Load::str(roleName))) {
      user->roles.push(role);
      user->perms |= role->perms;
      user->apiperms |= role->apiperms;
    }
  });
  return fbs::CreateUserPass(b, user->save(b), str(b, passwd), 1);
}

Offset<fbs::UserPass> UserDB::resetPass(
    Zfb::Builder &b, fbs::UserID *id_,
    Ztls::Random *rng, unsigned passLen)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto id = id_->id();
  auto user = m_users.findPtr(id);
  if (!user) {
    fbs::UserPassBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  ZtString passwd;
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
  {
    Ztls::HMAC hmac(User::keyType());
    hmac.start(user->secret.data(), user->secret.length());
    hmac.update(passwd.data(), passwd.length());
    user->hmac.length(user->hmac.size());
    hmac.finish(user->hmac.data());
  }
  {
    auto i = m_keys.iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  return fbs::CreateUserPass(b, user->save(b), str(b, passwd), 1);
}

Offset<fbs::UserAck> UserDB::chPass(
    Zfb::Builder &b, User *user, fbs::UserChPass *userChPass_)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  ZuString oldPass = Load::str(userChPass_->oldpass());
  ZuString newPass = Load::str(userChPass_->newpass());
  Ztls::HMAC hmac(User::keyType());
  User::KeyData verify;
  hmac.start(user->secret.data(), user->secret.length());
  hmac.update(oldPass.data(), oldPass.length());
  verify.length(verify.size());
  hmac.finish(verify.data());
  if (verify != user->hmac)
    return fbs::CreateUserAck(b, 0);
  hmac.reset();
  hmac.update(newPass.data(), newPass.length());
  hmac.finish(user->hmac.data());
  return fbs::CreateUserAck(b, 1);
}

// only id, name, roles, flags are processed
Offset<fbs::UserUpdAck> UserDB::userMod(
    Zfb::Builder &b, fbs::User *user_)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto id = user_->id();
  auto user = m_users.findPtr(id);
  if (!user) {
    fbs::UserUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  if (auto name = Load::str(user_->name())) user->name = name;
  if (user_->roles()->size()) {
    user->roles.length(0);
    user->perms.zero();
    user->apiperms.zero();
    Load::all(user_->roles(), [this, &user](unsigned, auto roleName) {
      if (auto role = m_roles.findPtr(Load::str(roleName))) {
	user->roles.push(role);
	user->perms |= role->perms;
	user->apiperms |= role->apiperms;
      }
    });
  }
  if (Zfb::IsFieldPresent(user_, fbs::User::VT_FLAGS))
    user->flags = user_->flags();
  return fbs::CreateUserUpdAck(b, user->save(b), 1);
}

Offset<fbs::UserAck> UserDB::keyClr(
    Zfb::Builder &b, fbs::UserID *id_)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto id = id_->id();
  ZmRef<User> user = m_users.del(id);
  if (!user) return fbs::CreateUserAck(b, 0);
  {
    auto i = m_keys.iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  return fbs::CreateUserAck(b, 1);
}

Offset<fbs::UserUpdAck> UserDB::userDel(
    Zfb::Builder &b, fbs::UserID *id_)
{
  Guard guard(m_lock);
  using namespace Zfb;
  using namespace Save;
  auto id = id_->id();
  ZmRef<User> user = m_users.del(id);
  if (!user) {
    fbs::UserUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  {
    auto i = m_keys.iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  return fbs::CreateUserUpdAck(b, user->save(b), 1);
}

} // namespace ZvUserDB
