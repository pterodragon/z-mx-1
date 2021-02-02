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
  app.enter = [&done](ZuString s) -> bool {
    std::cout << s << '\n';
    if (s == "quit") {
      done.post();
      return true;
    }
    return false;
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
  ZtArray<ZtString> history;
  app.histSave = [&history](unsigned i, ZuString s) {
    if (history.length() <= i) history.grow(i + 1);
    history[i] = s;
  };
  app.histLoad = [&history](unsigned i, ZuString &s) -> bool {
    if (i >= history.length()) return false;
    s = history[i];
    return true;
  };
  editor.init(config, app);
  if (!editor.loadMap("vi.map"))
    std::cerr << editor.loadError() << '\n' << std::flush;
  editor.open(&s, 1);
  editor.start([](Zrl::Editor &editor) {
    std::cout << editor.dumpVKeys();
    std::cout << editor.dumpMaps();
    editor.prompt("-->] ");
  });
  done.wait();
  editor.stop();
  editor.close();
  editor.final();
  s.stop();
}
