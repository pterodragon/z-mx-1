#include <zlib/ZuStringN.hpp>

#include <zlib/ZmSemaphore.hpp>

#include <zlib/ZrlTerminal.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

bool process(Zrl::Terminal &tty, ZmSemaphore &done, int32_t key)
{
  using namespace Zrl;
  tty.ins(ZtString{} << VKey::print(key));
  tty.write();
  if (key == 'q') {
    tty.crnl_();
    done.post();
    return true;
  }
  return false;
}

int main()
{
  ZmScheduler s{ZmSchedParams{}.id("sched").nThreads(1)};
  s.start();
  Zrl::Terminal tty;
  ZmSemaphore done;
  tty.open(&s, 1);
  tty.start(
      []() { },
      [&tty, &done](int32_t key) -> bool { return process(tty, done, key); });
  done.wait();
  tty.stop();
  tty.close();
  s.stop();
}
