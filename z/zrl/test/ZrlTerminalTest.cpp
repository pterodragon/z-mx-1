#include <zlib/ZuStringN.hpp>

#include <zlib/ZmSemaphore.hpp>

#include <zlib/ZrlTerminal.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

bool process(Zrl::Terminal &tty, ZmSemaphore &done, int32_t key)
{
  if (key < 0) {
    key = -key;
    using namespace Zrl::VKey;
    tty.ins(name(key & Mask));
    tty.ins("[");
    if (key & Shift) tty.ins("Shift");
    if (key & Ctrl) { if (key & Shift) tty.ins("|"); tty.ins("Ctrl"); }
    if (key & Alt) { if (key & (Shift|Ctrl)) tty.ins("|"); tty.ins("Alt"); }
    tty.ins("]");
  } else {
    ZuArrayN<uint8_t, 4> utf;
    if (key < 0x20) {
      utf << '^' << static_cast<char>('@' + key);
    } else if (key >= 0x7f && key < 0x100) {
      utf << "\\x" << ZuBoxed(key).hex(ZuFmt::Right<2, '0'>{});
    } else {
      utf.length(ZuUTF8::out(utf.data(), 4, key));
    }
    tty.ins(utf);
  }
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
