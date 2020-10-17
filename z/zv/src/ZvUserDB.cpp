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

// server-side user DB with MFA, API keys, etc.

#include <zlib/ZvUserDB.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZtlsBase32.hpp>
#include <zlib/ZtlsTOTP.hpp>

namespace ZvUserDB {

Mgr::Mgr(Ztls::Random *rng, unsigned passLen, unsigned totpRange,
    unsigned maxSize) :
  m_rng(rng),
  m_passLen(passLen),
  m_totpRange(totpRange),
  m_maxSize(maxSize)
{
  m_users = new UserIDHash();
  m_userNames = new UserNameHash();
  m_keys = new KeyHash();
  m_permNames = new PermNames();
}

Mgr::~Mgr()
{
  m_users->clean();
}

bool Mgr::bootstrap(
    ZtString name, ZtString role, ZtString &passwd, ZtString &secret)
{
  Guard guard(m_lock);
  if (!m_nPerms) {
    permAdd_("UserDB.Login", "UserDB.Access");
    m_permIndex[Perm::Login] = 0;
    m_permIndex[Perm::Access] = 1;
    for (unsigned i = fbs::ReqData_NONE + 1; i <= fbs::ReqData_MAX; i++) {
      if (ZuUnlikely(m_nPerms >= Bitmap::Bits)) break;
      unsigned id = m_nPerms++;
      m_perms[id] = ZtString{"UserDB."} + fbs::EnumNamesReqData()[i];
      m_permNames->add(m_perms[id], id);
      m_permIndex[id] = id;
    }
  }
  if (!m_roles.count())
    roleAdd_(role, Role::Immutable, Bitmap().fill(), Bitmap().fill());
  if (!m_users->count_()) {
    ZmRef<User> user = userAdd_(
	0, name, role, User::Immutable | User::Enabled | User::ChPass,
	passwd);
    secret.length(Ztls::Base32::enclen(user->secret.length()));
    Ztls::Base32::encode(
	secret.data(), secret.length(),
	user->secret.data(), user->secret.length());
    return true;
  }
  return false;
}

ZmRef<User> Mgr::userAdd_(
    uint64_t id, ZuString name, ZuString role, User::Flags flags,
    ZtString &passwd)
{
  ZmRef<User> user = new User(id, name, flags);
  {
    KeyData passwd_;
    unsigned passLen_ = Ztls::Base64::declen(m_passLen);
    if (passLen_ > passwd_.size()) passLen_ = passwd_.size();
    passwd_.length(passLen_);
    m_rng->random(passwd_.data(), passLen_);
    passwd.length(m_passLen);
    Ztls::Base64::encode(
	passwd.data(), passwd.length(), passwd_.data(), passwd_.length());
  }
  user->secret.length(user->secret.size());
  m_rng->random(user->secret.data(), user->secret.length());
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
  m_users->add(user);
  m_userNames->add(user);
  return user;
}

bool Mgr::load_(const uint8_t *buf, unsigned len)
{
  {
    Zfb::Verifier verifier(buf, len);
    if (!fbs::VerifyUserDBBuffer(verifier)) return false;
  }
  Guard guard(m_lock);
  m_modified = false;
  using namespace Zfb::Load;
  auto userDB = fbs::GetUserDB(buf);
  all(userDB->perms(), [this](unsigned, auto perm_) {
    unsigned id = perm_->id();
    if (ZuUnlikely(id >= Bitmap::Bits)) return;
    if (id <= m_nPerms) m_nPerms = id + 1;
    if (ZuUnlikely(m_perms[id])) m_permNames->del(m_perms[id]);
    m_perms[id] = str(perm_->name());
    m_permNames->add(m_perms[id], id);
  });
  m_permIndex[Perm::Login] = findPerm_("UserDB.Login");
  m_permIndex[Perm::Access] = findPerm_("UserDB.Access");
  for (unsigned i = fbs::ReqData_NONE + 1; i <= fbs::ReqData_MAX; i++)
    m_permIndex[Perm::Offset + i] =
      findPerm_(ZtString{"UserDB."} + fbs::EnumNamesReqData()[i]);
  all(userDB->roles(), [this](unsigned, auto role_) {
    if (auto role = loadRole(role_)) {
      m_roles.del(role->name);
      m_roles.add(ZuMv(role));
    }
  });
  all(userDB->users(), [this](unsigned, auto user_) {
    if (auto user = loadUser(m_roles, user_)) {
      m_users->del(user->id);
      m_users->add(user);
      m_userNames->del(user->name);
      m_userNames->add(ZuMv(user));
    }
  });
  all(userDB->keys(), [this](unsigned, auto key_) {
    auto user = m_users->findPtr(key_->userID());
    if (!user) return;
    if (auto key = loadKey(key_, user->keyList)) {
      user->keyList = key;
      m_keys->del(key->id);
      m_keys->add(ZuMv(key));
    }
  });
  return true;
}

Zfb::Offset<fbs::UserDB> Mgr::save_(Zfb::Builder &fbb) const
{
  Guard guard(m_lock); // not ReadGuard
  m_modified = false;
  using namespace Zfb;
  using namespace Save;
  auto perms_ = keyVecIter<fbs::Perm>(fbb, m_nPerms,
      [this](Builder &fbb, unsigned i) {
	return fbs::CreatePerm(fbb, i, str(fbb, m_perms[i]));
      });
  Offset<Vector<Offset<fbs::Role>>> roles_;
  {
    auto i = m_roles.readIterator();
    roles_ = keyVecIter<fbs::Role>(fbb, i.count(),
	[&i](Builder &fbb, unsigned j) { return i.iterate()->save(fbb); });
  }
  Offset<Vector<Offset<fbs::User>>> users_;
  {
    auto i = m_users->readIterator();
    users_ = keyVecIter<fbs::User>(fbb, i.count(),
	[&i](Builder &fbb, unsigned) { return i.iterate()->save(fbb); });
  }
  Offset<Vector<Offset<fbs::Key>>> keys_;
  {
    auto i = m_keys->readIterator();
    keys_ = keyVecIter<fbs::Key>(fbb, i.count(),
	[&i](Builder &fbb, unsigned) { return i.iterate()->save(fbb); });
  }
  return fbs::CreateUserDB(fbb, perms_, roles_, users_, keys_);
}

int Mgr::load(const ZiFile::Path &path, ZeError *e)
{
  using namespace Zfb::Load;
  return Zfb::Load::load(path,
      LoadFn{this, [](Mgr *this_, const uint8_t *buf, unsigned len) {
	return this_->load_(buf, len);
      }}, m_maxSize, e);
}

int Mgr::save(const ZiFile::Path &path, unsigned maxAge, ZeError *e)
{
  Zfb::Builder fbb;
  fbb.Finish(save_(fbb));

  if (maxAge) ZiFile::age(path, maxAge);

  return Zfb::Save::save(path, fbb, 0600, e);
}

bool Mgr::modified() const
{
  ReadGuard guard(m_lock);
  return m_modified;
}

int Mgr::loginReq(
    const fbs::LoginReq *loginReq_, ZmRef<User> &user, bool &interactive)
{
  using namespace Zfb::Load;
  int failures;
  switch ((int)loginReq_->data_type()) {
    case fbs::LoginReqData_Access: {
      auto access = static_cast<const fbs::Access *>(loginReq_->data());
      user = this->access(failures,
	  str(access->keyID()), bytes(access->token()), bytes(access->hmac()));
      interactive = false;
    } break;
    case fbs::LoginReqData_Login: {
      auto login = static_cast<const fbs::Login *>(loginReq_->data());
      user = this->login(failures,
	  str(login->user()), str(login->passwd()), login->totp());
      interactive = true;
    } break;
    default:
      failures = -1;
      user = nullptr;
      break;
  }
  return failures;
}

Zfb::Offset<fbs::ReqAck> Mgr::request(User *user, bool interactive,
    const fbs::Request *request_, Zfb::Builder &fbb)
{
  uint64_t seqNo = request_->seqNo();
  const void *reqData = request_->data();
  fbs::ReqAckData ackType = fbs::ReqAckData_NONE;
  Offset<void> ackData = 0;

  int reqType = request_->data_type();

  {
    ReadGuard guard(m_lock);
    auto perm = m_permIndex[Perm::Offset + reqType];
    if (ZuUnlikely(perm < 0)) {
      ZtString permName;
      permName << "UserDB." << fbs::EnumNamesReqData()[reqType];
      perm = m_permIndex[Perm::Offset + reqType] = findPerm_(permName);
      guard.unlock();
      if (ZuUnlikely(perm < 0)) {
	using namespace Zfb::Save;
	auto text_ = str(fbb, ZtString() <<
	    "permission denied (\"" << permName << "\" missing)");
	fbs::ReqAckBuilder fbb_(fbb);
	fbb_.add_seqNo(seqNo);
	fbb_.add_rejCode(__LINE__);
	fbb_.add_rejText(text_);
	return fbb_.Finish();
      }
    }
    guard.unlock();
    if (ZuUnlikely(!ok(user, interactive, perm))) {
      using namespace Zfb::Save;
      ZtString text = "permission denied";
      if (user->flags & User::ChPass) text << " (user must change password)";
      auto text_ = str(fbb, text);
      fbs::ReqAckBuilder fbb_(fbb);
      fbb_.add_seqNo(seqNo);
      fbb_.add_rejCode(__LINE__);
      fbb_.add_rejText(text_);
      return fbb_.Finish();
    }
  }

  switch (reqType) {
    case fbs::ReqData_ChPass:
      ackType = fbs::ReqAckData_ChPass;
      ackData = chPass(
	  fbb, user, static_cast<const fbs::UserChPass *>(reqData)).Union();
      break;
    case fbs::ReqData_OwnKeyGet:
      ackType = fbs::ReqAckData_OwnKeyGet;
      ackData = fbs::CreateKeyIDList(fbb, ownKeyGet(
	    fbb, user, static_cast<const fbs::UserID *>(reqData))).Union();
      break;
    case fbs::ReqData_OwnKeyAdd:
      ackType = fbs::ReqAckData_KeyAdd;
      ackData = ownKeyAdd(
	  fbb, user, static_cast<const fbs::UserID *>(reqData)).Union();
      break;
    case fbs::ReqData_OwnKeyClr:
      ackType = fbs::ReqAckData_KeyClr;
      ackData = ownKeyClr(
	  fbb, user, static_cast<const fbs::UserID *>(reqData)).Union();
      break;
    case fbs::ReqData_OwnKeyDel:
      ackType = fbs::ReqAckData_KeyDel;
      ackData = ownKeyDel(
	  fbb, user, static_cast<const fbs::KeyID *>(reqData)).Union();
      break;

    case fbs::ReqData_UserGet:
      ackType = fbs::ReqAckData_UserGet;
      ackData = fbs::CreateUserList(fbb,
	  userGet(fbb, static_cast<const fbs::UserID *>(reqData))).Union();
      break;
    case fbs::ReqData_UserAdd:
      ackType = fbs::ReqAckData_UserAdd;
      ackData =
	userAdd(fbb, static_cast<const fbs::User *>(reqData)).Union();
      break;
    case fbs::ReqData_ResetPass:
      ackType = fbs::ReqAckData_ResetPass;
      ackData =
	resetPass(fbb, static_cast<const fbs::UserID *>(reqData)).Union();
      break;
    case fbs::ReqData_UserMod:
      ackType = fbs::ReqAckData_UserMod;
      ackData =
	userMod(fbb, static_cast<const fbs::User *>(reqData)).Union();
      break;
    case fbs::ReqData_UserDel:
      ackType = fbs::ReqAckData_UserDel;
      ackData =
	userDel(fbb, static_cast<const fbs::UserID *>(reqData)).Union();
      break;

    case fbs::ReqData_RoleGet:
      ackType = fbs::ReqAckData_RoleGet;
      ackData = fbs::CreateRoleList(fbb,
	  roleGet(fbb, static_cast<const fbs::RoleID *>(reqData))).Union();
      break;
    case fbs::ReqData_RoleAdd:
      ackType = fbs::ReqAckData_RoleAdd;
      ackData =
	roleAdd(fbb, static_cast<const fbs::Role *>(reqData)).Union();
      break;
    case fbs::ReqData_RoleMod:
      ackType = fbs::ReqAckData_RoleMod;
      ackData =
	roleMod(fbb, static_cast<const fbs::Role *>(reqData)).Union();
      break;
    case fbs::ReqData_RoleDel:
      ackType = fbs::ReqAckData_RoleDel;
      ackData =
	roleDel(fbb, static_cast<const fbs::RoleID *>(reqData)).Union();
      break;

    case fbs::ReqData_PermGet:
      ackType = fbs::ReqAckData_PermGet;
      ackData = fbs::CreatePermList(fbb,
	  permGet(fbb, static_cast<const fbs::PermID *>(reqData))).Union();
      break;
    case fbs::ReqData_PermAdd:
      ackType = fbs::ReqAckData_PermAdd;
      ackData =
	permAdd(fbb, static_cast<const fbs::PermAdd *>(reqData)).Union();
      break;
    case fbs::ReqData_PermMod:
      ackType = fbs::ReqAckData_PermMod;
      ackData =
	permMod(fbb, static_cast<const fbs::Perm *>(reqData)).Union();
      break;
    case fbs::ReqData_PermDel:
      ackType = fbs::ReqAckData_PermDel;
      ackData =
	permDel(fbb, static_cast<const fbs::PermID *>(reqData)).Union();
      break;

    case fbs::ReqData_KeyGet:
      ackType = fbs::ReqAckData_KeyGet;
      ackData = fbs::CreateKeyIDList(fbb,
	  keyGet(fbb, static_cast<const fbs::UserID *>(reqData))).Union();
      break;
    case fbs::ReqData_KeyAdd:
      ackType = fbs::ReqAckData_KeyAdd;
      ackData =
	keyAdd(fbb, static_cast<const fbs::UserID *>(reqData)).Union();
      break;
    case fbs::ReqData_KeyClr:
      ackType = fbs::ReqAckData_KeyClr;
      ackData =
	keyClr(fbb, static_cast<const fbs::UserID *>(reqData)).Union();
      break;
    case fbs::ReqData_KeyDel:
      ackType = fbs::ReqAckData_KeyDel;
      ackData =
	keyDel(fbb, static_cast<const fbs::KeyID *>(reqData)).Union();
      break;
  }

  fbs::ReqAckBuilder fbb_(fbb);
  fbb_.add_seqNo(seqNo);
  fbb_.add_data_type(ackType);
  fbb_.add_data(ackData);
  return fbb_.Finish();
}

ZmRef<User> Mgr::login(int &failures,
    ZuString name, ZuString passwd, unsigned totp)
{
  Guard guard(m_lock);
  ZmRef<User> user = m_userNames->find(name);
  if (!user) {
    failures = -1;
    return nullptr;
  }
  if (!(user->flags & User::Enabled)) {
    if (++user->failures < 3) {
      ZeLOG(Warning, ZtString() << "authentication failure: "
	  "disabled user \"" << user->name << "\" attempted login");
    }
    failures = user->failures;
    return nullptr;
  }
  if (!(user->perms && Perm::Login)) {
    if (++user->failures < 3) {
      ZeLOG(Warning, ZtString() << "authentication failure: "
	  "user without login permission \"" << user->name <<
	  "\" attempted login");
    }
    failures = user->failures;
    return nullptr;
  }
  {
    Ztls::HMAC hmac(User::keyType());
    KeyData verify;
    hmac.start(user->secret.data(), user->secret.length());
    hmac.update(passwd.data(), passwd.length());
    verify.length(verify.size());
    hmac.finish(verify.data());
    if (verify != user->hmac) {
      if (++user->failures < 3) {
	ZeLOG(Warning, ZtString() << "authentication failure: "
	    "user \"" << user->name << "\" provided invalid password");
      }
      failures = user->failures;
      return nullptr;
    }
  }
  if (!Ztls::TOTP::verify(
	user->secret.data(), user->secret.length(), totp, m_totpRange)) {
    if (++user->failures < 3) {
      ZeLOG(Warning, ZtString() << "authentication failure: "
	  "user \"" << user->name << "\" provided invalid OTP");
    }
    failures = user->failures;
    return nullptr;
  }
  failures = 0;
  return user;
}

ZmRef<User> Mgr::access(int &failures,
    ZuString keyID, ZuArray<uint8_t> token, ZuArray<uint8_t> hmac)
{
  ReadGuard guard(m_lock);
  Key *key = m_keys->findPtr(keyID);
  if (!key) {
    failures = -1;
    return nullptr;
  }
  ZmRef<User> user = m_users->find(key->userID);
  if (!user) {
    failures = -1;
    return nullptr;
  }
  if (!(user->flags & User::Enabled)) {
    if (++user->failures < 3) {
      ZeLOG(Warning, ZtString() << "authentication failure: "
	  "disabled user \"" << user->name << "\" attempted login");
    }
    failures = user->failures;
    return nullptr;
  }
  if (!(user->perms && Perm::Access)) {
    if (++user->failures < 3) {
      ZeLOG(Warning, ZtString() << "authentication failure: "
	  "user without API access permission \"" << user->name <<
	  "\" attempted access");
    }
    failures = user->failures;
    return nullptr;
  }
  {
    Ztls::HMAC hmac_(Key::keyType());
    KeyData verify;
    hmac_.start(key->secret.data(), key->secret.length());
    hmac_.update(token.data(), token.length());
    verify.length(verify.size());
    hmac_.finish(verify.data());
    if (verify != hmac) {
      if (++user->failures < 3) {
	ZeLOG(Warning, ZtString() << "authentication failure: "
	    "user \"" << user->name << "\" provided invalid API key secret");
      }
      failures = user->failures;
      return nullptr;
    }
  }
  failures = 0;
  return user;
}

template <typename T> using Offset = Zfb::Offset<T>;
template <typename T> using Vector = Zfb::Vector<T>;

using namespace Zfb;
using namespace Save;

Offset<fbs::UserAck> Mgr::chPass(
    Zfb::Builder &fbb, User *user, const fbs::UserChPass *userChPass_)
{
  Guard guard(m_lock);
  ZuString oldPass = Load::str(userChPass_->oldpass());
  ZuString newPass = Load::str(userChPass_->newpass());
  Ztls::HMAC hmac(User::keyType());
  KeyData verify;
  hmac.start(user->secret.data(), user->secret.length());
  hmac.update(oldPass.data(), oldPass.length());
  verify.length(verify.size());
  hmac.finish(verify.data());
  if (verify != user->hmac)
    return fbs::CreateUserAck(fbb, 0);
  user->flags &= ~User::ChPass;
  m_modified = true;
  hmac.reset();
  hmac.update(newPass.data(), newPass.length());
  hmac.finish(user->hmac.data());
  return fbs::CreateUserAck(fbb, 1);
}

Offset<Vector<Offset<fbs::User>>> Mgr::userGet(
    Zfb::Builder &fbb, const fbs::UserID *id_)
{
  ReadGuard guard(m_lock);
  if (!Zfb::IsFieldPresent(id_, fbs::UserID::VT_ID)) {
    auto i = m_users->readIterator();
    return keyVecIter<fbs::User>(fbb, i.count(),
	[&i](Builder &fbb, unsigned) { return i.iterate()->save(fbb); });
  } else {
    auto id = id_->id();
    if (auto user = m_users->findPtr(id))
      return keyVec<fbs::User>(fbb, user->save(fbb));
    else
      return keyVec<fbs::User>(fbb);
  }
}

Offset<fbs::UserPass> Mgr::userAdd(Zfb::Builder &fbb, const fbs::User *user_)
{
  Guard guard(m_lock);
  if (m_users->findPtr(user_->id())) {
    fbs::UserPassBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  ZtString passwd;
  ZmRef<User> user = userAdd_(
      user_->id(), Load::str(user_->name()), ZuString(),
      user_->flags() | User::ChPass, passwd);
  Load::all(user_->roles(), [this, &user](unsigned, auto roleName) {
    if (auto role = m_roles.findPtr(Load::str(roleName))) {
      user->roles.push(role);
      user->perms |= role->perms;
      user->apiperms |= role->apiperms;
    }
  });
  return fbs::CreateUserPass(fbb, user->save(fbb), str(fbb, passwd), 1);
}

Offset<fbs::UserPass> Mgr::resetPass(
    Zfb::Builder &fbb, const fbs::UserID *id_)
{
  Guard guard(m_lock);
  auto id = id_->id();
  auto user = m_users->findPtr(id);
  if (!user) {
    fbs::UserPassBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  ZtString passwd;
  {
    KeyData passwd_;
    unsigned passLen_ = Ztls::Base64::declen(m_passLen);
    if (passLen_ > passwd_.size()) passLen_ = passwd_.size();
    passwd_.length(passLen_);
    m_rng->random(passwd_.data(), passLen_);
    passwd.length(m_passLen);
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
    auto i = m_keys->iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
    user->keyList = nullptr;
  }
  return fbs::CreateUserPass(fbb, user->save(fbb), str(fbb, passwd), 1);
}

// only id, name, roles, flags are processed
Offset<fbs::UserUpdAck> Mgr::userMod(
    Zfb::Builder &fbb, const fbs::User *user_)
{
  Guard guard(m_lock);
  auto id = user_->id();
  auto user = m_users->findPtr(id);
  if (!user || (user->flags & User::Immutable)) {
    fbs::UserUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
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
  return fbs::CreateUserUpdAck(fbb, user->save(fbb), 1);
}

Offset<fbs::UserUpdAck> Mgr::userDel(
    Zfb::Builder &fbb, const fbs::UserID *id_)
{
  Guard guard(m_lock);
  auto id = id_->id();
  ZmRef<User> user = m_users->del(id);
  if (!user || (user->flags & User::Immutable)) {
    fbs::UserUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  {
    auto i = m_keys->iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  return fbs::CreateUserUpdAck(fbb, user->save(fbb), 1);
}

Offset<Vector<Offset<fbs::Role>>> Mgr::roleGet(
    Zfb::Builder &fbb, const fbs::RoleID *id_)
{
  ReadGuard guard(m_lock);
  auto name = Load::str(id_->name());
  if (!name) {
    auto i = m_roles.readIterator();
    return keyVecIter<fbs::Role>(fbb, i.count(),
	[&i](Builder &fbb, unsigned) { return i.iterate()->save(fbb); });
  } else {
    if (auto role = m_roles.findPtr(name))
      return keyVec<fbs::Role>(fbb, role->save(fbb));
    else
      return keyVec<fbs::Role>(fbb);
  }
}

Offset<fbs::RoleUpdAck> Mgr::roleAdd(
    Zfb::Builder &fbb, const fbs::Role *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  if (m_roles.findPtr(name)) {
    fbs::RoleUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  auto role = loadRole(role_);
  m_roles.add(role);
  return fbs::CreateRoleUpdAck(fbb, role->save(fbb), 1);
}

// only perms, apiperms, flags are processed
Offset<fbs::RoleUpdAck> Mgr::roleMod(
    Zfb::Builder &fbb, const fbs::Role *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  auto role = m_roles.findPtr(name);
  if (!role || (role->flags & Role::Immutable)) {
    fbs::RoleUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
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
  return fbs::CreateRoleUpdAck(fbb, role->save(fbb), 1);
}

Offset<fbs::RoleUpdAck> Mgr::roleDel(
    Zfb::Builder &fbb, const fbs::RoleID *role_)
{
  Guard guard(m_lock);
  auto name = Load::str(role_->name());
  auto role = m_roles.findPtr(name);
  if (!role || (role->flags & Role::Immutable)) {
    fbs::RoleUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  {
    auto i = m_users->iterator();
    while (auto user = i.iterate())
      user->roles.grep([role](Role *role_) { return role == role_; });
  }
  m_roles.del(role);
  return fbs::CreateRoleUpdAck(fbb, role->save(fbb), 1);
}

Offset<Vector<Offset<fbs::Perm>>> Mgr::permGet(
    Zfb::Builder &fbb, const fbs::PermID *id_)
{
  ReadGuard guard(m_lock);
  if (!Zfb::IsFieldPresent(id_, fbs::PermID::VT_ID)) {
    return keyVecIter<fbs::Perm>(fbb, m_nPerms,
	[this](Builder &fbb, unsigned i) {
	  return fbs::CreatePerm(fbb, i, str(fbb, m_perms[i]));
	});
  } else {
    auto id = id_->id();
    if (id < m_nPerms)
      return keyVec<fbs::Perm>(fbb,
	  fbs::CreatePerm(fbb, id, str(fbb, m_perms[id])));
    else
      return keyVec<fbs::Perm>(fbb);
  }
}

Offset<fbs::PermUpdAck> Mgr::permAdd(
    Zfb::Builder &fbb, const fbs::PermAdd *permAdd_)
{
  Guard guard(m_lock);
  if (m_nPerms >= Bitmap::Bits) {
    fbs::PermUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  auto name = Load::str(permAdd_->name());
  auto id = m_nPerms++;
  m_perms[id] = name;
  m_permNames->add(m_perms[id], id);
  m_modified = true;
  return fbs::CreatePermUpdAck(fbb,
      fbs::CreatePerm(fbb, id, str(fbb, m_perms[id])), 1);
}

Offset<fbs::PermUpdAck> Mgr::permMod(
    Zfb::Builder &fbb, const fbs::Perm *perm_)
{
  Guard guard(m_lock);
  auto id = perm_->id();
  if (id >= m_nPerms) {
    fbs::PermUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  m_permNames->del(m_perms[id]);
  m_permNames->add(m_perms[id] = Load::str(perm_->name()), id);
  return fbs::CreatePermUpdAck(fbb,
      fbs::CreatePerm(fbb, id, str(fbb, m_perms[id])), 1);
}

Offset<fbs::PermUpdAck> Mgr::permDel(
    Zfb::Builder &fbb, const fbs::PermID *id_)
{
  Guard guard(m_lock);
  auto id = id_->id();
  if (id >= m_nPerms) {
    fbs::PermUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  m_permNames->del(m_perms[id]);
  ZtString name = ZuMv(m_perms[id]);
  if (id == m_nPerms - 1) {
    unsigned i = id;
    do { m_nPerms = i; } while (i && !m_perms[--i]);
  }
  return fbs::CreatePermUpdAck(fbb,
      fbs::CreatePerm(fbb, id, str(fbb, name)), 1);
}

Offset<Vector<Offset<Zfb::String>>> Mgr::ownKeyGet(
    Zfb::Builder &fbb, const User *user, const fbs::UserID *userID_)
{
  ReadGuard guard(m_lock);
  if (user->id != userID_->id()) user = nullptr;
  return keyGet_(fbb, user);
}
Offset<Vector<Offset<Zfb::String>>> Mgr::keyGet(
    Zfb::Builder &fbb, const fbs::UserID *userID_)
{
  ReadGuard guard(m_lock);
  return keyGet_(fbb,
      static_cast<const User *>(m_users->findPtr(userID_->id())));
}
Offset<Vector<Offset<Zfb::String>>> Mgr::keyGet_(
    Zfb::Builder &fbb, const User *user)
{
  if (!user) return strVec(fbb);
  unsigned n = 0;
  for (auto key = user->keyList; key; key = key->nextKey) ++n;
  return strVecIter(fbb, n,
      [key = user->keyList](unsigned) mutable {
	auto id = key->id;
	key = key->nextKey;
	return id;
      });
}

Offset<fbs::KeyUpdAck> Mgr::ownKeyAdd(
    Zfb::Builder &fbb, User *user, const fbs::UserID *userID_)
{
  Guard guard(m_lock);
  if (user->id != userID_->id()) user = nullptr;
  return keyAdd_(fbb, user);
}
Offset<fbs::KeyUpdAck> Mgr::keyAdd(
    Zfb::Builder &fbb, const fbs::UserID *userID_)
{
  Guard guard(m_lock);
  return keyAdd_(fbb,
      static_cast<User *>(m_users->findPtr(userID_->id())));
}
Offset<fbs::KeyUpdAck> Mgr::keyAdd_(
    Zfb::Builder &fbb, User *user)
{
  if (!user) {
    fbs::KeyUpdAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  m_modified = true;
  ZtString keyID;
  do {
    Key::IDData keyID_;
    keyID_.length(keyID_.size());
    m_rng->random(keyID_.data(), keyID_.length());
    keyID.length(Ztls::Base64::enclen(keyID_.length()));
    Ztls::Base64::encode(
	keyID.data(), keyID.length(), keyID_.data(), keyID_.length());
  } while (m_keys->findPtr(keyID));
  ZmRef<Key> key = new Key(ZuMv(keyID), user->keyList, user->id);
  key->secret.length(key->secret.size());
  m_rng->random(key->secret.data(), key->secret.length());
  user->keyList = key;
  m_keys->add(key);
  return fbs::CreateKeyUpdAck(fbb, key->save(fbb), 1);
}

Offset<fbs::UserAck> Mgr::ownKeyClr(
    Zfb::Builder &fbb, User *user, const fbs::UserID *userID_)
{
  Guard guard(m_lock);
  if (user->id != userID_->id()) user = nullptr;
  return keyClr_(fbb, user);
}
Offset<fbs::UserAck> Mgr::keyClr(
    Zfb::Builder &fbb, const fbs::UserID *userID_)
{
  Guard guard(m_lock);
  return keyClr_(fbb,
      static_cast<User *>(m_users->findPtr(userID_->id())));
}
Offset<fbs::UserAck> Mgr::keyClr_(
    Zfb::Builder &fbb, User *user)
{
  if (!user) return fbs::CreateUserAck(fbb, 0);
  m_modified = true;
  auto id = user->id;
  {
    auto i = m_keys->iterator();
    while (auto key = i.iterate())
      if (key->userID == id) i.del();
  }
  user->keyList = nullptr;
  return fbs::CreateUserAck(fbb, 1);
}

Offset<fbs::UserAck> Mgr::ownKeyDel(
    Zfb::Builder &fbb, User *user, const fbs::KeyID *id_)
{
  Guard guard(m_lock);
  auto keyID = Load::str(id_->id());
  Key *key = m_keys->findPtr(keyID);
  if (!key || user->id != key->userID) {
    fbs::UserAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  return keyDel_(fbb, user, keyID);
}
Offset<fbs::UserAck> Mgr::keyDel(
    Zfb::Builder &fbb, const fbs::KeyID *id_)
{
  Guard guard(m_lock);
  auto keyID = Load::str(id_->id());
  Key *key = m_keys->findPtr(keyID);
  if (!key) {
    fbs::UserAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
  return keyDel_(fbb,
      static_cast<User *>(m_users->findPtr(key->userID)), keyID);
}
Offset<fbs::UserAck> Mgr::keyDel_(
    Zfb::Builder &fbb, User *user, ZuString keyID)
{
  m_modified = true;
  ZmRef<Key> key = m_keys->del(keyID);
  if (!key) {
    fbs::UserAckBuilder fbb_(fbb);
    fbb_.add_ok(0);
    return fbb_.Finish();
  }
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
  return fbs::CreateUserAck(fbb, 1);
}

} // namespace ZvUserDB
