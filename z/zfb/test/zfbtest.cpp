#include <Zfb.hpp>

#include "zfbtest_generated.h"

#include <ZtString.hpp>

using namespace Zfb;
using namespace zfbtest;

template <typename Vector, typename L>
inline void push_(Vector &, L &) { }
template <typename Vector, typename L, typename Arg1, typename ...Args>
inline void push_(Vector &v, L &l, Arg1 &&arg1, Args &&... args) {
  v.push_back(l(ZuFwd<Arg1>(arg1)));
  push_(v, l, ZuFwd<Args>(args)...);
}
template <typename T, typename B, typename L, typename ...Args>
inline auto vector_(B &b, L l, Args &&... args) {
  std::vector<Offset<T> > v;
  push_(v, l, ZuFwd<Args>(args)...);
  return b.CreateVector(v);
}
template <typename T, typename B, typename ...Args>
inline auto vector(B &b, Args &&... args) {
  return vector_<T>(b, [](auto p) { return p; }, ZuFwd<Args>(args)...);
}
template <typename T, typename B, typename L, typename ...Args>
inline auto keyVec_(B &b, L l, Args &&... args) {
  std::vector<Offset<T> > v;
  push_(v, l, ZuFwd<Args>(args)...);
  return b.CreateVectorOfSortedTables(&v);
}
template <typename T, typename B, typename ...Args>
inline auto keyVec(B &b, Args &&... args) {
  return keyVec_<T>(b, [](auto p) { return p; }, ZuFwd<Args>(args)...);
}
template <typename B, typename S>
inline auto str(B &b, S &&s) {
  return b.CreateString(ZuFwd<S>(s));
}
template <typename B, typename ...Args>
inline auto strVec(B &b, Args &&... args) {
  return vector_<String>(b, [&b](const auto &s) {
    return str(b, s);
  }, ZuFwd<Args>(args)...);
}
template <typename B>
inline auto byteVec(B &b, const void *data, unsigned len) {
  return b.CreateVector(static_cast<const uint8_t *>(data), len);
}
template <typename B>
inline auto byteVec(B &b, ZuString s) {
  return b.CreateVector(
      reinterpret_cast<const uint8_t *>(s.data()), s.length());
}

int main()
{
  ZmRef<IOBuf> iobuf;

  {
    IOBuilder b;

    auto perms = keyVec<Perm>(b,
	CreatePerm(b, 0, str(b, "useradd")),
	CreatePerm(b, 1, str(b, "usermod")),
	CreatePerm(b, 2, str(b, "userdel")),
	CreatePerm(b, 3, str(b, "login")));
    uint64_t userPerms = 0x80;
    uint64_t adminPerms = 0x8f;
    auto roles = keyVec<Role>(b,
	CreateRole(b, str(b, "user"), b.CreateVector(&userPerms, 1)),
	CreateRole(b, str(b, "admin"), b.CreateVector(&adminPerms, 1)));
    auto users = keyVec<User>(b,
	CreateUser(b, 0, str(b, "user1"),
	  byteVec(b, "", 0), byteVec(b, "", 0), strVec(b, "admin")),
	CreateUser(b, 1, str(b, "user2"),
	  byteVec(b, "", 0), byteVec(b, "", 0), strVec(b, "user", "admin")));
    auto keys = keyVec<Key>(b,
	CreateKey(b, str(b, "1"), byteVec(b, "2", 1), 0),
	CreateKey(b, str(b, "3"), byteVec(b, "4", 1), 1));

    auto db = CreateUserDB(b, perms, roles, users, keys);

    b.Finish(db);

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

  auto db = GetUserDB(iobuf->data + iobuf->skip);

  auto perm = db->perms()->LookupByKey(1);

  if (!perm) {
    std::cerr << "FAILED - key lookup failed\n" << std::flush;
    return 1;
  }

  if (string(perm->name()) != "usermod") {
    std::cerr << "FAILED - wrong key\n" << std::flush;
    return 1;
  }

  return 0;
}
