#include <Zfb.hpp>

#include "zfbtest_generated.h"

#include <ZtString.hpp>

using namespace Zfb;
using namespace zfbtest;

int main()
{
  ZmRef<IOBuf> iobuf;

  {
    IOBuilder b;

    {
      using namespace Zfb::Build;

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
