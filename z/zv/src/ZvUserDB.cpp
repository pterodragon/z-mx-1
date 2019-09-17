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

bool UserDB::load(const uint8_t *buf, unsigned len)
{
  {
    Zfb::Verifier verifier(buf, len);
    if (!fbs::VerifyUserDBBuffer(verifier)) return false;
  }
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
    auto user = m_users.findPtr(key_->userID());
    if (!user) return;
    if (auto key = loadKey(key_, user->keyList)) {
      user->keyList = key;
      m_keys.del(key->id);
      m_keys.add(ZuMv(key));
    }
  });
  return true;
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

int UserDB::load(ZuString path, ZeError *e)
{
  ZiFile f;
  int i;
  
  if ((i = f.open(path, ZiFile::ReadOnly, 0, e)) != Zi::OK) return i;
  ZiFile::Offset len = f.size();
  if (!len || len >= (ZiFile::Offset)INT_MAX) {
    f.close();
    if (e) *e = ZiENOMEM;
    return Zi::IOError;
  }
  uint8_t *buf = (uint8_t *)::malloc(len);
  if (!buf) {
    f.close();
    if (e) *e = ZiENOMEM;
    return Zi::IOError;
  }
  if ((i = f.read(buf, len, e)) != Zi::OK) {
    ::free(buf);
    f.close();
    return i;
  }
  f.close();
  if (!load(buf, len)) {
    ::free(buf);
    if (e) *e = ZiEINVAL;
    return Zi::IOError;
  }
  ::free(buf);
  return Zi::OK;
}

int UserDB::save(ZuString path, unsigned maxAge, ZeError *e)
{
  Zfb::Builder b;
  b.Finish(save(b));

  ZiFile::age(path, maxAge);

  ZiFile f;
  int i;
  
  if ((i = f.open(path, ZiFile::WriteOnly, 0666, e)) != Zi::OK) return i;

  uint8_t *buf = b.GetBufferPointer();
  int len = b.GetSize();

  if (!buf || len <= 0) {
    f.close();
    if (e) *e = ZiENOMEM;
    return Zi::IOError;
  }
  if ((i = f.write(buf, len, e)) != Zi::OK) {
    f.close();
    return i;
  }
  f.close();
  return Zi::OK;
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
    ZuString keyID, ZuArray<uint8_t> token, ZuArray<uint8_t> hmac)
{
  ReadGuard guard(m_lock);
  Key *key = m_keys.findPtr(keyID);
  if (!key) return nullptr;
  ZmRef<User> user = m_users.find(key->userID);
  if (!user) return nullptr; // LATER - log warning about key w/ invalid userID
  if (!(user->flags & User::Enabled)) return nullptr;
  if (!(user->perms && Perm::Access)) return nullptr;
  {
    Ztls::HMAC hmac_(Key::keyType());
    Key::KeyData verify;
    hmac_.start(user->secret.data(), user->secret.length());
    hmac_.update(token.data(), token.length());
    verify.length(verify.size());
    hmac_.finish(verify.data());
    if (verify != hmac) return nullptr;
  }
  return user;
}

template <typename T> using Offset = Zfb::Offset<T>;
template <typename T> using Vector = Zfb::Vector<T>;

using namespace Zfb;
using namespace Save;

Offset<Vector<Offset<fbs::User>>> UserDB::userGet(
    Zfb::Builder &b, fbs::UserID *id_)
{
  ReadGuard guard(m_lock);
  if (!Zfb::IsFieldPresent(id_, fbs::UserID::VT_ID)) {
    auto i = m_users.readIterator();
    return keyVecIter<fbs::User>(b, i.count(),
	[&i](Builder &b, unsigned) { return i.iterate()->save(b); });
  } else {
    auto id = id_->id();
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
  if (m_users.findPtr(user_->id())) {
    fbs::UserPassBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
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
  auto id = user_->id();
  auto user = m_users.findPtr(id);
  if (!user || (user->flags & User::Immutable)) {
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
  auto id = id_->id();
  ZmRef<User> user = m_users.del(id);
  if (!user) return fbs::CreateUserAck(b, 0);
  {
    auto i = m_keys.iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  user->keyList = nullptr;
  return fbs::CreateUserAck(b, 1);
}

Offset<fbs::UserUpdAck> UserDB::userDel(
    Zfb::Builder &b, fbs::UserID *id_)
{
  Guard guard(m_lock);
  auto id = id_->id();
  ZmRef<User> user = m_users.del(id);
  if (!user || (user->flags & User::Immutable)) {
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

Offset<Vector<Offset<fbs::Role>>> UserDB::roleGet(
    Zfb::Builder &b, fbs::RoleID *id_)
{
  ReadGuard guard(m_lock);
  auto name = Load::str(id_->name());
  if (!name) {
    auto i = m_roles.readIterator();
    return keyVecIter<fbs::Role>(b, i.count(),
	[&i](Builder &b, unsigned) { return i.iterate()->save(b); });
  } else {
    if (auto role = m_roles.findPtr(name))
      return keyVec<fbs::Role>(b, role->save(b));
    else
      return keyVec<fbs::Role>(b);
  }
}

Offset<fbs::RoleUpdAck> UserDB::roleAdd(
    Zfb::Builder &b, fbs::Role *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  if (m_roles.findPtr(name)) {
    fbs::RoleUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  auto role = loadRole(role_);
  m_roles.add(role);
  return fbs::CreateRoleUpdAck(b, role->save(b), 1);
}

// only perms, apiperms, flags are processed
Offset<fbs::RoleUpdAck> UserDB::roleMod(
    Zfb::Builder &b, fbs::Role *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  auto role = m_roles.findPtr(name);
  if (!role || (role->flags & Role::Immutable)) {
    fbs::RoleUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  if (role_->perms()->size()) {
    role->perms.zero();
    Load::all(role_->perms(), [role](unsigned i, uint64_t v) {
      if (i < Bitmap::Words) role->perms.data[i] = v;
    });
  }
  if (role_->apiperms()->size()) {
    role->apiperms.zero();
    Load::all(role_->apiperms(), [role](unsigned i, uint64_t v) {
      if (i < Bitmap::Words) role->apiperms.data[i] = v;
    });
  }
  if (Zfb::IsFieldPresent(role_, fbs::Role::VT_FLAGS))
    role->flags = role_->flags();
  return fbs::CreateRoleUpdAck(b, role->save(b), 1);
}

Offset<fbs::RoleUpdAck> UserDB::roleDel(
    Zfb::Builder &b, fbs::Role *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  auto role = m_roles.findPtr(name);
  if (!role || (role->flags & Role::Immutable)) {
    fbs::RoleUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  {
    auto i = m_users.iterator();
    while (auto user = i.iterate())
      user->roles.grep([role](Role *role_) { return role == role_; });
  }
  return fbs::CreateRoleUpdAck(b, role->save(b), 1);
}

Offset<Vector<Offset<fbs::Perm>>> UserDB::permGet(
    Zfb::Builder &b, fbs::PermID *id_)
{
  ReadGuard guard(m_lock);
  if (!Zfb::IsFieldPresent(id_, fbs::PermID::VT_ID)) {
    return keyVecIter<fbs::Perm>(b, m_perms.length(),
	[this](Builder &b, unsigned i) {
	  return fbs::CreatePerm(b, i, str(b, m_perms[i]));
	});
  } else {
    auto id = id_->id();
    if (id < m_perms.length())
      return keyVec<fbs::Perm>(b, fbs::CreatePerm(b, id, str(b, m_perms[id])));
    else
      return keyVec<fbs::Perm>(b);
  }
}

Offset<fbs::PermUpdAck> UserDB::permAdd(
    Zfb::Builder &b, fbs::Perm *perm_)
{
  Guard guard(m_lock);
  auto id = perm_->id();
  if (id < m_perms.length()) {
    fbs::PermUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  if (m_perms.length() < id + 1) m_perms.length(id + 1);
  m_perms[id] = Load::str(perm_->name());
  return fbs::CreatePermUpdAck(b,
      fbs::CreatePerm(b, id, str(b, m_perms[id])), 1);
}

Offset<fbs::PermUpdAck> UserDB::permMod(
    Zfb::Builder &b, fbs::Perm *perm_)
{
  Guard guard(m_lock);
  auto id = perm_->id();
  if (id >= m_perms.length()) {
    fbs::PermUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  m_perms[id] = Load::str(perm_->name());
  return fbs::CreatePermUpdAck(b,
      fbs::CreatePerm(b, id, str(b, m_perms[id])), 1);
}

Offset<fbs::PermUpdAck> UserDB::permDel(
    Zfb::Builder &b, fbs::PermID *id_)
{
  Guard guard(m_lock);
  auto id = id_->id();
  if (id >= m_perms.length()) {
    fbs::PermUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  ZtString name = ZuMv(m_perms[id]);
  if (id == m_perms.length() - 1) {
    unsigned i = id;
    do { m_perms.length(i); } while (!m_perms[--i]);
  }
  return fbs::CreatePermUpdAck(b,
      fbs::CreatePerm(b, id, str(b, name)), 1);
}

Offset<Vector<Offset<fbs::Key>>> UserDB::keyGet(
    Zfb::Builder &b, fbs::UserID *userID_)
{
  ReadGuard guard(m_lock);
  auto user = m_users.findPtr(userID_->id());
  if (!user) return keyVec<fbs::Key>(b);
  unsigned n = 0;
  for (auto key = user->keyList; key; key = key->nextKey) ++n;
  return keyVecIter<fbs::Key>(b, n,
      [key = user->keyList](Zfb::Builder &b, unsigned) mutable {
	auto key_ = key->save(b);
	key = key->nextKey;
	return key_;
      });
}

Offset<fbs::KeyUpdAck> UserDB::keyAdd(
    Zfb::Builder &b, fbs::UserID *userID_,
    Ztls::Random *rng)
{
  Guard guard(m_lock);
  auto user = m_users.findPtr(userID_->id());
  if (!user) {
    fbs::KeyUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  ZtString keyID;
  do {
    Key::IDData keyID_;
    keyID_.length(keyID_.size());
    rng->random(keyID_.data(), keyID_.length());
    keyID.length(Ztls::Base64::enclen(keyID_.length()));
    Ztls::Base64::encode(
	keyID.data(), keyID.length(), keyID_.data(), keyID_.length());
  } while (m_keys.findPtr(keyID));
  ZmRef<Key> key = new Key(ZuMv(keyID), user->keyList, user->id);
  key->secret.length(key->secret.size());
  rng->random(key->secret.data(), key->secret.length());
  user->keyList = key;
  m_keys.add(key);
  return fbs::CreateKeyUpdAck(b, key->save(b), 1);
}

Offset<fbs::KeyUpdAck> UserDB::keyDel(
    Zfb::Builder &b, fbs::KeyID *id_)
{
  Guard guard(m_lock);
  ZmRef<Key> key = m_keys.del(Load::str(id_->id()));
  if (!key) {
    fbs::KeyUpdAckBuilder b_(b);
    b_.add_ok(0);
    return b_.Finish();
  }
  auto user = m_users.findPtr(key->userID);
  if (user) {
    auto prev = user->keyList;
    if (prev == key)
      user->keyList = key->nextKey;
    else
      while (prev) {
	if (prev->nextKey == key) {
	  prev->nextKey = key->nextKey;
	  break;
	}
	prev = prev->nextKey;
      }
  }
  return fbs::CreateKeyUpdAck(b, key->save(b), 1);
}

} // namespace ZvUserDB
