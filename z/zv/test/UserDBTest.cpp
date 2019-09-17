#include <ZtlsRandom.hpp>

#include <ZvUserDB.hpp>

using namespace Zfb;
using namespace ZvUserDB;

int main()
{
  ZmRef<IOBuf> iobuf;

  {
    Ztls::Random rng;

    rng.init();

    UserDB userDB;

    ZtString passwd, secret;

    userDB.init(&rng, 12, passwd, secret);

    std::cout << "passwd: " << passwd << "\nsecret: " << secret << '\n';

    IOBuilder b;

    b.Finish(userDB.save(b));

    uint8_t *buf = b.GetBufferPointer();
    int len = b.GetSize();

    // std::cout << ZtHexDump("", buf, len);

    iobuf = b.buf();

    std::cout << ZtHexDump("\n", iobuf->data(), iobuf->length);

    if ((void *)buf != (void *)(iobuf->data()) ||
	len != iobuf->length) {
      std::cerr << "FAILED - inconsistent buffers\n" << std::flush;
      return 1;
    }
  }

  {
    using namespace Load;

    {
      using namespace ZvUserDB;

      auto db = fbs::GetUserDB(iobuf->data());

      auto perm = db->perms()->LookupByKey(UserDB::Perm::ChPass);

      if (!perm) {
	std::cerr << "READ FAILED - key lookup failed\n" << std::flush;
	return 1;
      }

      if (str(perm->name()) != "chPass") {
	std::cerr << "READ FAILED - wrong key\n" << std::flush;
	return 1;
      }
    }

    UserDB userDB;

    if (!userDB.load(iobuf->data(), iobuf->length)) {
      std::cerr << "LOAD FAILED - failed to verify\n" << std::flush;
      return 1;
    }

    if (userDB.perm(UserDB::Perm::ChPass) != "chPass") {
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
