#include <Zfb.hpp>

#include "userdb_generated.h"

#include <ZuBitmap.hpp>

#include <ZtString.hpp>

using namespace Zfb;

#if 0
using Bitmap = ZuBitmap<256>;

using RoleTree =
  ZmRBTree<ZtString,
    ZmRBTreeVal<Bitmap,
      ZmRBTreeLock<ZmNoLock> > >;

struct User__ : ZuObject {
  uint64_t		id;
  ZtString		name;
  char			passwd[32];
  char			passwdKey[32];
  ZtArray<Role *>	roles;
  Bitmap		perms;		// effective permisions
};
struct UserIDAccessor : public ZuAccessor<User__ *, uint64_t> {
  inline static uint64_t value(const User__ *u) { return u->id; }
};
using UserIDHash =
  ZmHash<User__,
    ZmHashObject<ZuNull,
      ZmHashIndex<UserIDAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashLock<ZmNoLock> > > > >;
using User_ = UserIDHash::Node;
struct UserNameAccessor : public ZuAccessor<User_ *, ZtString> {
  inline static ZtString value(const User_ *u) { return u->name; }
};
using UserNameHash =
  ZmHash<User_,
    ZmHashObject<ZuObject,
      ZmHashIndex<UserNameAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashLock<ZmNoLock> > > > >;
using User = UserNameHash::Node;

struct Key_ : ZuObject {
  ZtString	id;
  char		secret[32];
  uint64_t	userID;
};
struct KeyIDAccessor : public ZuAccessor<Key_ *, ZtString> {
  inline static ZtString value(const Key_ *k) { return k->id; }
};
using KeyHash =
  ZmHash<Key_,
    ZmHashObject<ZuObject,
      ZmHashIndex<KeyIDAccessor,
	ZmHashNodeIsKey<true,
	  ZmHashLock<ZmNoLock> > > > >;
using Key = KeyHash::Node;

struct UserDB {
  ZtArray<ZtString>	perms; // indexed by permission ID
  RoleTree		roles; // name -> permissions
  UserIDHash		users;
  UserNameHash		userNames;
  KeyHash		keys;

  template <typename B>
  void build(B &b) const {
    using namespace Zfb::Build;
    auto perms = keyVecIter<zfbtest::Perm>(b, perms.count(),
	[&b](unsigned i) {
	  return zfbtest::CreatePerm(b, i, str(b, perms[i]));
	});
    {
      RoleTree::ReadIterator i(roles);
      auto roles = keyVecIter<zdbtest::Role>(b, i.count(),
	  [&b, &i](unsigned j) {
	    auto node = i.iterate();
	    return zfbtest::CreateRole(b, str(b, node->key()),
		b.CreateVector(node->val().data(), Bitmap::Words));
	  });
    }
  }
};
#endif

int main()
{
  ZmRef<IOBuf> iobuf;

  {
    IOBuilder b;

    {
      using namespace Zfb::Build;
      using namespace zfbtest;

      auto perms = keyVec<Perm>(b,
	  CreatePerm(b, 0, str(b, "useradd")),
	  CreatePerm(b, 1, str(b, "usermod")),
	  CreatePerm(b, 2, str(b, "userdel")),
	  CreatePerm(b, 3, str(b, "login")));
      auto roles = keyVec<Role>(b,
	  CreateRole(b, str(b, "user"), pvector<uint64_t>(b, 0x80)),
	  CreateRole(b, str(b, "admin"), pvector<uint64_t>(b, 0x8f)));
      auto users = keyVec<User>(b,
	  CreateUser(b, 0, str(b, "user1"),
	    bytes(b, "", 0), bytes(b, "", 0), strVec(b, "admin")),
	  CreateUser(b, 1, str(b, "user2"),
	    bytes(b, "", 0), bytes(b, "", 0), strVec(b, "user", "admin")));
      auto keys = keyVec<Key>(b,
	  CreateKey(b, str(b, "1"), bytes(b, "2", 1), 0),
	  CreateKey(b, str(b, "3"), bytes(b, "4", 1), 1));

      auto db = CreateUserDB(b, perms, roles, users, keys);

      b.Finish(db);
    }

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
    using namespace Parse;
    using namespace zfbtest;

    auto db = GetUserDB(iobuf->data + iobuf->skip);

    auto perm = db->perms()->LookupByKey(1);

    if (!perm) {
      std::cerr << "FAILED - key lookup failed\n" << std::flush;
      return 1;
    }

    if (str(perm->name()) != "usermod") {
      std::cerr << "FAILED - wrong key\n" << std::flush;
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
