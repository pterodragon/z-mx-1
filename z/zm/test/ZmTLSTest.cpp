#include <zlib/ZuBox.hpp>

#include <zlib/ZmPlatform.hpp>

ZmPlatform::ThreadID getTID() {
  return ZmPlatform::getTID();
}

int main()
{
  std::cout << ZuBoxed(getTID()) << '\n';
  return 0;
}
