#include <Zfb.hpp>

#include "userdb_generated.h"

#include <ZuBitmap.hpp>
#include <ZuObject.hpp>
#include <ZuPolymorph.hpp>
#include <ZuRef.hpp>

#include <ZmHash.hpp>
#include <ZmRBTree.hpp>

#include <ZtString.hpp>

using namespace Zfb;

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
    using namespace zfbtest;
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
  char			passwd[32];
  char			passwdKey[32];
  ZtArray<Role *>	roles;
  Bitmap		perms;		// effective permisions

  template <typename B> auto save(B &b) {
    using namespace zfbtest;
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
    using namespace zfbtest;
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
    perms.push(ZuFwd<Arg1>(arg1));
    permAdd(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void roleAdd(ZuString name, Args &&... args) {
    ZmRef<Role> role = new Role(name, ZuFwd<Args>(args)...);
    roles.add(role);
  }
  void userAdd(uint64_t id, ZuString name, ZuString role) {
    ZmRef<User> user = new User(id, name);
    memset(user->passwd, 0, 32);
    memset(user->passwdKey, 0, 32);
    if (auto node = roles.find(role)) {
      user->roles.push(node);
      user->perms = node->perms;
    }
    users.add(user);
    userNames.add(user);
  }
  void keyAdd(ZuString id, uint64_t userID) {
    ZmRef<Key> key = new Key(id, userID);
    memset(key->secret, 0, 32);
    keys.add(key);
  }

  template <typename B> auto save(B &b) const {
    using namespace Save;
    auto perms_ = keyVecIter<zfbtest::Perm>(b, perms.length(),
	[this](B &b, unsigned i) {
	  return zfbtest::CreatePerm(b, i, str(b, perms[i]));
	});
    Offset<Vector<Offset<zfbtest::Role>>> roles_;
    {
      auto i = roles.readIterator();
      roles_ = keyVecIter<zfbtest::Role>(b, i.count(),
	  [&i](B &b, unsigned j) { return i.iterate()->save(b); });
    }
    Offset<Vector<Offset<zfbtest::User>>> users_;
    {
      auto i = users.readIterator();
      users_ = keyVecIter<zfbtest::User>(b, i.count(),
	  [&i](B &b, unsigned) { return i.iterate()->save(b); });
    }
    Offset<Vector<Offset<zfbtest::Key>>> keys_;
    {
      auto i = keys.readIterator();
      keys_ = keyVecIter<zfbtest::Key>(b, i.count(),
	  [&i](B &b, unsigned) { return i.iterate()->save(b); });
    }
    return zfbtest::CreateUserDB(b, perms_, roles_, users_, keys_);
  }

  void load(const void *buf) {
    using namespace Load;
    auto userDB = zfbtest::GetUserDB(buf);
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
};

int main()
{
  ZmRef<IOBuf> iobuf;

  {
    UserDB userDB;

    userDB.permAdd("useradd", "usermod", "userdel", "login");
    userDB.roleAdd("admin", 0, 1, 2, 3);
    userDB.roleAdd("user", 3);
    userDB.userAdd(0, "user1", "admin");
    userDB.userAdd(0, "user2", "user");
    userDB.keyAdd("1", 0);
    userDB.keyAdd("2", 1);

    IOBuilder b;

    b.Finish(userDB.save(b));

    uint8_t *buf = b.GetBufferPointer();
    int len = b.GetSize();

    std::cout << ZtHexDump("", buf, len);

    iobuf = b.buf();

    std::cout << ZtHexDump("\n\n", iobuf->data + iobuf->skip, iobuf->length);

    if ((void *)buf != (void *)(iobuf->data + iobuf->skip) ||
	len != iobuf->length) {
      std::cerr << "FAILED - inconsistent buffers\n" << std::flush;
      return 1;
    }
  }

  {
    using namespace Load;

    {
      using namespace zfbtest;

      auto db = GetUserDB(iobuf->data + iobuf->skip);

      auto perm = db->perms()->LookupByKey(1);

      if (!perm) {
	std::cerr << "READ FAILED - key lookup failed\n" << std::flush;
	return 1;
      }

      if (str(perm->name()) != "usermod") {
	std::cerr << "READ FAILED - wrong key\n" << std::flush;
	return 1;
      }
    }

    UserDB userDB;

    userDB.load(iobuf->data + iobuf->skip);

    if (userDB.perms[1] != "usermod") {
      std::cerr << "LOAD FAILED - wrong key\n" << std::flush;
      return 1;
    }
  }

  return 0;
}

#if 0
void read(ZuString path, ZtArray<char> &buf) {
  ZiFile f;
  if (f.open(path, ZiFile::ReadOnly, 0666, &e) != Zi::OK) {
    logError("open(", path, "): ", e);
    return;
  }
  auto o = f.size();
  if (o > (ZiFile::Offset)(1<<20)) {
    logError("\"", path, "\" too large");
    return;
  }
  buf.size(o);

  FlatBufferBuilder b(f.size());
}
#endif
