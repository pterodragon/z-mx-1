#include <ZvUserDB.hpp>

using namespace Zfb;

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
