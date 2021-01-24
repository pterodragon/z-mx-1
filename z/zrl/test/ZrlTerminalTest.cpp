#include <zlib/ZuStringN.hpp>

#include <zlib/ZmSemaphore.hpp>

#include <zlib/ZrlTerminal.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

bool process(Zrl::Terminal &tty, ZmSemaphore &done, int32_t key)
{
  if (key < 0) {
    key = -key;
    using namespace Zrl::VKey;
    switch (static_cast<int>(key & Mask)) {
      case Continue: tty.ins("Continue"); break;
      case Error: tty.ins("Error"); break;
      case EndOfFile: tty.ins("EndOfFile"); break;
      case SigInt: tty.ins("SigInt"); break;
      case SigQuit: tty.ins("SigQuit"); break;
      case SigSusp: tty.ins("SigSusp"); break;
      case Enter: tty.ins("Enter"); break;
      case Erase: tty.ins("Erase"); break;
      case WErase: tty.ins("WErase"); break;
      case Kill: tty.ins("Kill"); break;
      case LNext: tty.ins("LNext"); break;
      case Up: tty.ins("Up"); break;
      case Down: tty.ins("Down"); break;
      case Left: tty.ins("Left"); break;
      case Right: tty.ins("Right"); break;
      case Home: tty.ins("Home"); break;
      case End: tty.ins("End"); break;
      case Insert: tty.ins("Insert"); break;
      case Delete: tty.ins("Delete"); break;
    }
    tty.ins("[");
    if (key & Shift) tty.ins("Shift");
    if (key & Ctrl) { if (key & Shift) tty.ins("|"); tty.ins("Ctrl"); }
    if (key & Alt) { if (key & (Shift|Ctrl)) tty.ins("|"); tty.ins("Alt"); }
    tty.ins("]");
  } else {
    ZuArrayN<uint8_t, 4> utf;
    if (key < 0x20) {
      utf << '^' << ('A' + (key - 1));
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
