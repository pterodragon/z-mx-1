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

#ifndef ZvUserDB_HPP
#define ZvUserDB_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZvLib_HPP
#include <ZvLib.hpp>
#endif

#include <ZuBitmap.hpp>
#include <ZuObject.hpp>
#include <ZuPolymorph.hpp>
#include <ZuRef.hpp>
#include <ZuArrayN.hpp>

#include <ZmHash.hpp>
#include <ZmRBTree.hpp>
#include <ZmRWLock.hpp>

#include <ZtString.hpp>

#include <ZiFile.hpp>

#include <Zfb.hpp>

#include <ZtlsBase64.hpp>
#include <ZtlsHMAC.hpp>
#include <ZtlsRandom.hpp>

#include "userdb_generated.h"
#include "loginreq_generated.h"
#include "userdbreq_generated.h"
#include "userdback_generated.h"

namespace ZvUserDB {

using Bitmap = ZuBitmap<256>;

struct Role_ : public ZuObject {
public:
  using Flags = uint8_t;
  enum {
    Immutable =	0x01
  };

  Role_(ZuString name_, Flags flags_) :
      name(name_), flags(flags_) { }
  Role_(ZuString name_, Flags flags_,
	const Bitmap &perms_, const Bitmap &apiperms_) :
      name(name_), perms(perms_), apiperms(apiperms_), flags(flags_) { }

  ZtString		name;
  Bitmap		perms;
  Bitmap		apiperms;
  Flags			flags;

  Zfb::Offset<fbs::Role> save(Zfb::Builder &fbb) {
    using namespace Zfb::Save;
    return fbs::CreateRole(fbb, str(fbb, name),
	fbb.CreateVector(perms.data, Bitmap::Words),
	fbb.CreateVector(apiperms.data, Bitmap::Words));
  }
};
struct RoleNameAccessor : public ZuAccessor<Role_, ZtString> {
  inline static const ZtString &value(const Role_ &r) { return r.name; }
};
using RoleTree =
  ZmRBTree<Role_,
    ZmRBTreeObject<ZuNull,
      ZmRBTreeIndex<RoleNameAccessor,
	ZmRBTreeNodeIsKey<true,
	  ZmRBTreeLock<ZmNoLock> > > > >;
using Role = RoleTree::Node;
inline ZmRef<Role> loadRole(const fbs::Role *role_) {
  using namespace Zfb::Load;
  ZmRef<Role> role = new Role(str(role_->name()), role_->flags());
  all(role_->perms(), [role](unsigned i, uint64_t v) {
    if (i < Bitmap::Words) role->perms.data[i] = v;
  });
  all(role_->apiperms(), [role](unsigned i, uint64_t v) {
    if (i < Bitmap::Words) role->apiperms.data[i] = v;
  });
  return role;
}

struct Key_;
struct User__ : public ZuPolymorph {
  using Flags = uint8_t;
  enum {
    Immutable =	0x01,
    Enabled =	0x02,
    ChPass =	0x04	// user must change password
  };

  User__(uint64_t id_, ZuString name_, Flags flags_) :
      id(id_), name(name_), flags(flags_) { }

  static constexpr const mbedtls_md_type_t keyType() {
    return MBEDTLS_MD_SHA256;
  }
  using KeyData = ZuArrayN<uint8_t, 32>;	// 256 bit key

  uint64_t		id;
  ZtString		name;
  KeyData		hmac;		// HMAC-SHA256 of secret, password
  KeyData		secret;		// secret (random at user creation)
  ZtArray<Role *>	roles;
  Key_			*keyList = nullptr; // head of list of keys
  Bitmap		perms;		// permissions
  Bitmap		apiperms;	// API permissions
  Flags			flags;

  Zfb::Offset<fbs::User> save(Zfb::Builder &fbb) {
    using namespace Zfb::Save;
    return fbs::CreateUser(fbb, id,
	str(fbb, name), bytes(fbb, hmac), bytes(fbb, secret),
	strVecIter(fbb, roles.length(), [this](Zfb::Builder &, unsigned k) {
	  return roles[k]->name;
	}));
  }
};
struct UserIDAccessor : public ZuAccessor<User__, uint64_t> {
  inline static uint64_t value(const User__ &u) { return u.id; }
};
using UserIDHash =
  ZmHash<User__,
    ZmHashObject<ZuNull,
      ZmHashIndex<UserIDAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashHeapID<ZuNull,
	    ZmHashLock<ZmNoLock> > > > > >;
using User_ = UserIDHash::Node;
struct UserNameAccessor : public ZuAccessor<User_, ZtString> {
  inline static ZtString value(const User_ &u) { return u.name; }
};
using UserNameHash =
  ZmHash<User_,
    ZmHashObject<ZuPolymorph,
      ZmHashIndex<UserNameAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashLock<ZmNoLock> > > > >;
using User = UserNameHash::Node;
template <typename Roles>
ZmRef<User> loadUser(const Roles &roles, const fbs::User *user_) {
  using namespace Zfb::Load;
  ZmRef<User> user = new User(user_->id(), str(user_->name()), user_->flags());
  user->hmac = bytes(user_->hmac());
  user->secret = bytes(user_->secret());
  all(user_->roles(), [&roles, &user](unsigned, auto roleName) {
    if (auto role = roles.findPtr(str(roleName))) {
      user->roles.push(role);
      user->perms |= role->perms;
      user->apiperms |= role->apiperms;
    }
  });
  return user;
}

struct Key_ : public ZuObject {
  inline Key_(ZuString id_, Key_ *nextKey_, uint64_t userID_) :
      id(id_), nextKey(nextKey_), userID(userID_) { }

  using IDData = ZuArrayN<uint8_t, 16>;
  static constexpr const mbedtls_md_type_t keyType() {
    return MBEDTLS_MD_SHA256;
  }
  using KeyData = ZuArrayN<uint8_t, 32>;	// 256 bit key

  ZtString	id;
  KeyData	secret;
  Key_		*nextKey;// next in per-user list
  uint64_t	userID;

  Zfb::Offset<fbs::Key> save(Zfb::Builder &fbb) {
    using namespace Zfb::Save;
    return fbs::CreateKey(fbb, str(fbb, id), bytes(fbb, secret), userID);
  }
};
struct KeyIDAccessor : public ZuAccessor<Key_, ZtString> {
  inline static ZtString value(const Key_ &k) { return k.id; }
};
using KeyHash =
  ZmHash<Key_,
    ZmHashObject<ZuObject,
      ZmHashIndex<KeyIDAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashLock<ZmNoLock> > > > >;
using Key = KeyHash::Node;
inline ZmRef<Key> loadKey(const fbs::Key *key_, Key_ *next) {
  using namespace Zfb::Load;
  ZmRef<Key> key = new Key(str(key_->id()), next, key_->userID());
  key->secret = bytes(key_->secret());
  return key;
}

class ZvAPI UserDB {
public:
  using Lock = ZmRWLock;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  struct Perm {
    enum {
      Login = 0,
      Access,
      N
    };
    enum {
      Offset = N - (fbs::ReqData_NONE + 1);
    };
  };

  UserDB(Ztls::Random *rng, unsigned passLen, unsigned totpRange);

  // one-time initialization (idempotent)
  bool bootstrap(ZtString &passwd, ZtString &secret);

  bool load(const uint8_t *buf, unsigned len);
  Zfb::Offset<fbs::UserDB> save(Zfb::Builder &) const;

  int load(ZuString path, ZeError *e = 0);
  int save(ZuString path, unsigned maxAge = 8, ZeError *e = 0);

  ZtString perm(unsigned i) {
    ReadGuard guard(m_lock);
    if (i >= m_perms.length()) return ZtString();
    return m_perms[i];
  }
  int findPerm(ZuString s) { // returns -1 if not found
    ReadGuard guard(m_lock);
    unsigned i, n = perms.length();
    for (i = 0; i < n; i++) if (perms[i] == s) return i;
    return -1;
  }

  template <typename T> using Offset = Zfb::Offset<T>;
  template <typename T> using Vector = Zfb::Vector<T>;

  // user, interactive
  ZuPair<ZmRef<User>, bool> loginReq(fbs::LoginReq *loginReq);

  Offset<fbs::ReqAck> request(User *user, bool interactive,
      Zfb::Builder &, fbs::Request *request);

private:
  // interactive login
  ZmRef<User> login(
      ZuString user, ZuString passwd, unsigned totp);
  // API access
  ZmRef<User> access(
      ZuString keyID, ZuArray<uint8_t> token, ZuArray<uint8_t> hmac);

public:
  // ok(user, interactive, perm)
  bool ok(User *user, bool interactive, unsigned perm) {
    if ((user->flags & User::ChPass) &&
	interactive && perm != Perm::ChPass)
      return false;
    return interactive ? (user->perms && perm) : (user->apiperms && perm);
  }

private:
  // change password
  Offset<fbs::UserAck> chPass(
      Zfb::Builder &, User *user, fbs::UserChPass *userChPass);

  // query users
  Offset<Vector<Offset<fbs::User>>> userGet(
      Zfb::Builder &, fbs::UserID *id);
  // add a new user
  Offset<fbs::UserPass> userAdd(
      Zfb::Builder &, fbs::User *user);
  // reset password (also clears all API keys)
  Offset<fbs::UserPass> resetPass(
      Zfb::Builder &, fbs::UserID *id);
  // modify user name, roles, flags
  Offset<fbs::UserUpdAck> userMod(
      Zfb::Builder &, fbs::User *user);
  // delete user
  Offset<fbs::UserUpdAck> userDel(
      Zfb::Builder &, fbs::UserID *id);
  
  // query roles
  Offset<Vector<Offset<fbs::Role>>> roleGet(
      Zfb::Builder &, fbs::RoleID *id);
  // add role
  Offset<fbs::RoleUpdAck> roleAdd(
      Zfb::Builder &, fbs::Role *role);
  // modify role perms, apiperms, flags
  Offset<fbs::RoleUpdAck> roleMod(
      Zfb::Builder &, fbs::Role *role);
  // delete role
  Offset<fbs::RoleUpdAck> roleDel(
      Zfb::Builder &, fbs::RoleID *role);
  
  // query permissions 
  Offset<Vector<Offset<fbs::Perm>>> permGet(
      Zfb::Builder &, fbs::PermID *perm);
  // add permission
  Offset<fbs::PermUpdAck> permAdd(
      Zfb::Builder &, fbs::Perm *perm);
  // modify permission name
  Offset<fbs::PermUpdAck> permMod(
      Zfb::Builder &, fbs::Perm *perm);
  // delete permission
  Offset<fbs::PermUpdAck> permDel(
      Zfb::Builder &, fbs::PermID *id);

  // query API keys for user
  Offset<Vector<Offset<fbs::Key>>> ownKeyGet(
      Zfb::Builder &, User *user, fbs::UserID *userID);
  Offset<Vector<Offset<fbs::Key>>> keyGet(
      Zfb::Builder &, fbs::UserID *userID);
  Offset<Vector<Offset<fbs::Key>>> keyGet_(
      Zfb::Builder &, User *user);
  // add API key for user
  Offset<fbs::KeyUpdAck> ownKeyAdd(
      Zfb::Builder &, User *user, fbs::UserID *userID);
  Offset<fbs::KeyUpdAck> keyAdd(
      Zfb::Builder &, fbs::UserID *userID);
  Offset<fbs::KeyUpdAck> keyAdd_(
      Zfb::Builder &, User *user);
  // clear all API keys for user
  Offset<fbs::UserAck> ownKeyClr(
      Zfb::Builder &, User *user, fbs::UserID *id);
  Offset<fbs::UserAck> keyClr(
      Zfb::Builder &, fbs::UserID *id);
  Offset<fbs::UserAck> keyClr_(
      Zfb::Builder &, User *user_);
  // delete API key
  Offset<fbs::KeyUpdAck> ownKeyDel(
      Zfb::Builder &, User *user, fbs::KeyID *id);
  Offset<fbs::KeyUpdAck> keyDel(
      Zfb::Builder &, fbs::KeyID *id);
  Offset<fbs::KeyUpdAck> keyDel_(
      Zfb::Builder &, User *user, fbs::KeyID *id);

private:
  void permAdd_() { }
  template <typename Arg0, typename ...Args>
  void permAdd_(Arg0 &&arg0, Args &&... args) {
    m_perms.push(ZuFwd<Arg0>(arg0));
    permAdd_(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void roleAdd_(ZuString name, Role::Flags flags, Args &&... args) {
    ZmRef<Role> role = new Role(name, flags, ZuFwd<Args>(args)...);
    m_roles.add(role);
  }
  ZmRef<User> userAdd_(
      uint64_t id, ZuString name, ZuString role, User::Flags flags,
      Ztls::Random *rng, unsigned passLen, ZtString &passwd);

private:
  Ztls::Random		*m_rng;
  unsigned		m_passLen;
  unsigned		m_totpRange;

  Lock			m_lock;
    ZtArray<ZtString>	  m_perms; // indexed by permission ID
    unsigned		  m_permIndex[Perm::Offset + fbs::ReqData_MAX + 1];
    RoleTree		  m_roles; // name -> permissions
    UserIDHash		  m_users;
    UserNameHash	  m_userNames;
    KeyHash		  m_keys;
};

}

#endif /* ZvUserDB_HPP */
