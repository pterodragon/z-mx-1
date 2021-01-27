#include <zlib/ZuStringN.hpp>

#include <zlib/ZmSemaphore.hpp>

#include <zlib/ZrlEditor.hpp>

#define CHECK(x) ((x) ? puts("OK  " #x) : puts("NOK " #x))

int main()
{
  ZmScheduler s{ZmSchedParams{}.id("sched").nThreads(1)};
  s.start();
  Zrl::App app;
  Zrl::Config config;
  Zrl::Editor editor;
  ZmSemaphore done;
  app.enter = [&done](ZuString s) {
    std::cout << s << '\n';
    if (s == "quit") done.post();
  };
  app.end = [&done]() { done.post(); };
  app.sig = [&done](int sig) {
    switch (sig) {
      case SIGINT: std::cout << "SIGINT\n" << std::flush; break;
      case SIGQUIT: std::cout << "SIGQUIT\n" << std::flush; break;
      case SIGTSTP: std::cout << "SIGTSTP\n" << std::flush; break;
    }
    done.post();
  };
  app.error = [&done](ZeError e) {
    // FIXME - this is a little odd
    std::cout << e << '\n' << std::flush;
    done.post();
  };
  editor.init(config, app);
  std::cout << editor.dumpMaps();
  editor.open(&s, 1);
  editor.start("] ");
  done.wait();
  editor.stop();
  editor.close();
  editor.final();
  s.stop();
}
