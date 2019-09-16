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

#include <Zfb.hpp>

#include <ZtlsBase64.hpp>
#include <ZtlsHMAC.hpp>
#include <ZtlsRandom.hpp>

#include "userdb_generated.h"

namespace ZvUserDB {

using Bitmap = ZuBitmap<256>;

struct Role_ : public ZuObject {
private:
  template <unsigned> inline void perm_() { }
  template <unsigned I> struct PermIndex { enum { OK = I < Bitmap::Words }; };
  template <unsigned I, typename ...Args>
  inline typename ZuIfT<!PermIndex<I>::OK>::T perm_(Args &&...) { }
  template <unsigned I, typename Arg1, typename ...Args>
  inline typename ZuIfT<PermIndex<I>::OK>::T
  perm_(Arg1 &&arg1, Args &&... args) {
    perms.data[I] = ZuFwd<Arg1>(arg1);
    perm_<I + 1>(ZuFwd<Args>(args)...);
  }

public:
  // flags
  using Flags = uint8_t;
  enum {
    Immutable =	0x01
  };

  template <typename ...Args>
  inline Role_(ZuString name_, Flags flags_, Args &&... args) :
      name(name_), flags(flags_) {
    perm_<0>(ZuFwd<Args>(args)...);
  }

  ZtString		name;
  Bitmap		perms;
  Flags			flags;

  Zfb::Offset<fbs::Role> save(Zfb::Builder &b) {
    using namespace Zfb::Save;
    return fbs::CreateRole(b, str(b, name),
	b.CreateVector(perms.data, Bitmap::Words));
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
template <typename T> inline ZmRef<Role> loadRole(T *role_) {
  using namespace Zfb::Load;
  ZmRef<Role> role = new Role(str(role_->name()), role_->flags());
  all(role_->perms(), [role](unsigned i, uint64_t v) {
    if (i < Bitmap::Words) role->perms.data[i] = v;
  });
  return role;
}

struct User__ : public ZuPolymorph {
  // flags
  using Flags = uint8_t;
  enum {
    Immutable =	0x01,
    Enabled =	0x02
  };

  User__(uint64_t id_, ZuString name_, Flags flags_) :
      id(id_), name(name_), flags(flags_) { }

  using KeyData = ZuArrayN<uint8_t, 32>;	// 256 bit key

  uint64_t		id;
  ZtString		name;
  KeyData		hmac;		// HMAC-SHA256 of secret, password
  KeyData		secret;		// secret (random at user creation)
  ZtArray<Role *>	roles;
  Bitmap		perms;		// effective permisions
  Flags			flags;

  Zfb::Offset<fbs::User> save(Zfb::Builder &b) {
    using namespace Zfb::Save;
    return fbs::CreateUser(b, id,
	str(b, name), bytes(b, hmac), bytes(b, secret),
	strVecIter(b, roles.length(), [this](auto &b, unsigned k) {
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
template <typename Roles, typename T>
inline ZmRef<User> loadUser(const Roles &roles, T *user_) {
  using namespace Zfb::Load;
  ZmRef<User> user = new User(user_->id(), str(user_->name()), user_->flags());
  user->hmac = bytes(user_->hmac());
  user->secret = bytes(user_->secret());
  all(user_->roles(), [&roles, &user](unsigned, auto roleName) {
    if (auto role = roles.findPtr(str(roleName))) {
      user->roles.push(role);
      user->perms |= role->perms;
    }
  });
  return user;
}

struct Key_ : public ZuObject {
  inline Key_(ZuString id_, uint64_t userID_) : id(id_), userID(userID_) { }

  using KeyData = ZuArrayN<uint8_t, 32>;	// 256 bit key

  ZtString	id;
  KeyData	secret;
  uint64_t	userID;

  Zfb::Offset<fbs::Key> save(Zfb::Builder &b) {
    using namespace Zfb::Save;
    return fbs::CreateKey(b, str(b, id), bytes(b, secret), userID);
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
template <typename T> inline ZmRef<Key> loadKey(T *key_) {
  using namespace Zfb::Load;
  ZmRef<Key> key = new Key(str(key_->key()), key_->userID());
  key->secret = bytes(key_->secret());
  return key;
}

class ZvAPI UserDB {
public:
  using Lock = ZmRWLock;
  using Guard = ZmGuard<Lock>;
  using ReadGuard = ZmReadGuard<Lock>;

  void load(const void *buf);
  Zfb::Offset<fbs::UserDB> save(Zfb::Builder &b) const;

  bool init(Ztls::Random *rng,
      unsigned passLen,
      ZtString &passwd,
      ZtString &secret);

  ZtString perm(unsigned i) {
    ReadGuard guard(m_lock);
    if (i >= m_perms.length()) return ZtString();
    return m_perms[i];
  }

  // interactive login
  ZmRef<User> login(ZuString user, ZuString passwd, unsigned totp);
  // API key access
  ZmRef<User> access(
      ZuString keyID, ZuArray<uint8_t> data, ZuArray<uint8_t> hmac);

  // FIXME - each role needs two perms - interactive/non-interactive
  // '' for each user (OR of all roles)
  // ok(user, interactive, perm) gives true/false can proceed
  //
  //userList

private:
  void permAdd_() { }
  template <typename Arg1, typename ...Args>
  void permAdd_(Arg1 &&arg1, Args &&... args) {
    m_perms.push(ZuFwd<Arg1>(arg1));
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

  // add login verify (passwd, totp)
  // add key verify (key, token, hmac) // hmac is hmac(secret, token)
  // Note: hmac is re-used during session, token is re-issued each session
  //
  // FIXME - encode below ops into fbs, provide apply() function?
  //
  // UserGet:UserID, -> UserList
  // UserAdd:User,
  // UserMod:User,
  // UserDel:UserID,
  
  // RoleGet:RoleID, -> RoleList
  // RoleAdd:Role,
  // RoleMod:Role,
  // RoleDel:RoleID,
  
  // PermGet:PermID, -> PermList
  // PermAdd:Perm,
  // PermMod:Perm,
  // PermDel:PermID,

  // KeyGet:UserID,	// returns KeyIDs valid for user
  // KeyAdd:UserID,	// adds new key to user (returns key+secret)
  // KeyClr:UserID,	// deletes all keys for user
  // KeyDel:KeyID		// deletes specific key
  //

private:
  Lock			m_lock;
    ZtArray<ZtString>	  m_perms; // indexed by permission ID
    RoleTree		  m_roles; // name -> permissions
    UserIDHash		  m_users;
    UserNameHash	  m_userNames;
    KeyHash		  m_keys;
};

}

#endif /* ZvUserDB_HPP */
