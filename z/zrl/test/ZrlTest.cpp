#include <zlib/Zrl.hpp>

int main()
{
  while (auto s = Zrl::readline("ZrlTest] ")) {
    puts(s);
    ::free(s);
  }
}
