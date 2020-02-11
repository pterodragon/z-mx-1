#include <zlib/ZmScheduler.hpp>

#include <zlib/ZGtkApp.hpp>

int main(int argc, char **argv)
{
  ZmScheduler s{ZmSchedParams().id("sched").nThreads(2)};
  s.start();

  ZGtk::App app;

  app.attach(&s, 1);

  for (int i = 0; i < 5; i++) {
    s.run(1, []() {
      std::cout << ZmThreadContext::self() << '\n' << std::flush;
    }, ZmTimeNow(i));
  }
  ::sleep(6);

  s.stop();

  app.detach();
}
