#include <zlib/ZuStringN.hpp>

#include <zlib/ZmSemaphore.hpp>

#include <zlib/ZrlTerminal.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

bool process(Zrl::Terminal &tty, ZmSemaphore &done, int32_t key)
{
  using namespace Zrl;
  auto s = ZtString{} << VKey::print(key);
  tty.splice(tty.line().position(tty.pos()).mapping(), ZuUTFSpan{},
      s, ZuUTF<uint32_t, uint8_t>::span(s), true);
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
  tty.open(&s, 1,
      [&done](bool) { done.post(); },
      [&done](ZuString s) {
	std::cerr << s << "\r\n" << std::flush;
	done.post();
      });
  done.wait();
  tty.start(
      []() { std::cerr << "started\r\n" << std::flush; },
      [&tty, &done](int32_t key) -> bool { return process(tty, done, key); });
  done.wait();
  tty.stop();
  tty.close([]() { });
  s.stop();
}
