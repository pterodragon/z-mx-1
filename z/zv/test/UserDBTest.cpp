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

    UserDB userDB(&rng, 12, 6);

    ZtString passwd, secret;

    userDB.bootstrap(passwd, secret);

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

      auto perm = db->perms()->LookupByKey(fbs::ReqData_ChPass + 1);

      if (!perm) {
	std::cerr << "READ FAILED - key lookup failed\n" << std::flush;
	return 1;
      }

      if (str(perm->name()) != "UserDB.ChPass") {
	std::cerr << "READ FAILED - wrong key\n" << std::flush;
	return 1;
      }
    }

    Ztls::Random rng;
    UserDB userDB(&rng, 12, 6);

    if (!userDB.load(iobuf->data(), iobuf->length)) {
      std::cerr << "LOAD FAILED - failed to verify\n" << std::flush;
      return 1;
    }

    if (userDB.perm(2) != "UserDB.ChPass") {
      std::cerr << "LOAD FAILED - wrong key\n" << std::flush;
      return 1;
    }
  }

  return 0;
}
