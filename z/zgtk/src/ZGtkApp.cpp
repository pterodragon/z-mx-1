#include <ZGtkApp.hpp>

extern "C" {
  gboolean dispatch(GSource *, GSourceFunc, gpointer);
}

gboolean dispatch(GSource *source, GSourceFunc, gpointer)
{
  gtk_main_quit();
  g_source_set_ready_time(source, -1);
  return G_SOURCE_CONTINUE;
}

namespace ZGtk {

void App::wake()
{
  g_source_set_ready_time(m_source, 0);
  g_main_context_wakeup(nullptr);
}

void App::run()
{
  gtk_main();
}

void App::attach(ZmScheduler &sched, unsigned tid)
{
  GSourceFuncs funcs = {
    .prepare = nullptr,
    .check = nullptr,
    .dispatch = dispatch,
    .finalize = nullptr
  };

  m_source = g_source_new(&funcs, sizeof(GSource));
  g_source_attach(m_source, nullptr);
  sched.wakeFn(tid, [this, &sched]() mutable {
    sched.run_(1, []() { run(); }); wake();
  });
  sched.run_(tid, []() { run(); });
}

void App::detach(ZmScheduler &sched, unsigned tid)
{
  // g_source_detach(m_source, nullptr); // FIXME
  // g_source_final(m_source); // FIXME
  sched.wakeFn(tid, ZmFn{});
}

}
