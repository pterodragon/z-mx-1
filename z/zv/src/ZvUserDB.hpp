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

#include <ZmHash.hpp>
#include <ZmRBTree.hpp>

#include <ZtString.hpp>

#include <Zfb.hpp>

#include "userdb_generated.h"

namespace ZvUserDB {

using Bitmap = ZuBitmap<256>;

struct Role_ : public ZuObject {
private:
  inline void perm_() { }
  template <typename Arg1, typename ...Args>
  inline void perm_(Arg1 &&arg1, Args &&... args) {
    perms.set(ZuFwd<Arg1>(arg1));
    perm_(ZuFwd<Args>(args)...);
  }
public:
  template <typename ...Args>
  inline Role_(ZuString name_, Args &&... args) : name(name_) {
    perm_(ZuFwd<Args>(args)...);
  }

  ZtString		name;
  Bitmap		perms;

  template <typename B> auto save(B &b) {
    using namespace fbs;
    using namespace Save;
    return CreateRole(b, str(b, name),
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
  using namespace Load;
  ZmRef<Role> role = new Role(str(role_->name()));
  all(role_->perms(), [role](unsigned i, uint64_t v) {
    if (i < Bitmap::Words) role->perms.data[i] = v;
  });
  return role;
}

struct User__ : public ZuPolymorph {
  User__(uint64_t id_, ZuString name_) : id(id_), name(name_) { }

  uint64_t		id;
  ZtString		name;
  char			passwd[32];	// HMAC(passwdKey, passwdPlainTxt)
  char			passwdKey[32];	// TOTP and passwd hash secret
  ZtArray<Role *>	roles;
  Bitmap		perms;		// effective permisions

  template <typename B> auto save(B &b) {
    using namespace fbs;
    using namespace Save;
    return CreateUser(b, id,
	str(b, name), bytes(b, passwd, 32), bytes(b, passwdKey, 32),
	strVecIter(b, roles.length(), [this](B &b, unsigned k) {
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
  using namespace Load;
  ZmRef<User> user = new User(user_->id(), str(user_->name()));
  if (!cpy(user->passwd, user_->passwd())) return nullptr;
  if (!cpy(user->passwdKey, user_->passwdKey())) return nullptr;
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

  ZtString	id;
  char		secret[32];
  uint64_t	userID;

  template <typename B> auto save(B &b) {
    using namespace fbs;
    using namespace Save;
    return CreateKey(b, str(b, id), bytes(b, secret, 32), userID);
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
  using namespace Load;
  ZmRef<Key> key = new Key(str(key_->key()), key_->userID());
  if (!cpy(key->secret, key_->secret())) return nullptr;
  return key;
}

struct UserDB {
  ZtArray<ZtString>	perms; // indexed by permission ID
  RoleTree		roles; // name -> permissions
  UserIDHash		users;
  UserNameHash		userNames;
  KeyHash		keys;

  void permAdd() { }
  template <typename Arg1, typename ...Args>
  void permAdd(Arg1 &&arg1, Args &&... args) {
    // FIXME - idempotent
    perms.push(ZuFwd<Arg1>(arg1));
    permAdd(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void roleAdd(ZuString name, Args &&... args) {
    // FIXME - idempotent
    ZmRef<Role> role = new Role(name, ZuFwd<Args>(args)...);
    roles.add(role);
  }
  void userAdd(uint64_t id, ZuString name, ZuString role) {
    // FIXME - idempotent
    ZmRef<User> user = new User(id, name);
    memset(user->passwd, 0, 32); // FIXME
    memset(user->secret, 0, 32); // FIXME
    if (auto node = roles.find(role)) {
      user->roles.push(node);
      user->perms = node->perms;
    }
    users.add(user);
    userNames.add(user);
  }
  void keyAdd(ZuString id, uint64_t userID) {
    // FIXME - idempotent
    ZmRef<Key> key = new Key(id, userID);
    memset(key->secret, 0, 32); // FIXME
    keys.add(key);
  }

  // FIXME -
  // add login verify (passwd, totp)
  // add key verify (key, token, hmac) // hmac is hmac(secret, token)
  // Note: hmac is re-used during session, token is re-issued each session
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

  // FIXME - move to .cpp
  auto save(FlatBufferBuilder &b) const {
    using namespace Save;
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

  void load(const void *buf);

};

}

#endif /* ZvUserDB_HPP */
