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

void App::attach(ZmScheduler *sched, unsigned tid)
{
  static GSourceFuncs funcs = {
    .prepare = nullptr,
    .check = nullptr,
    .dispatch = dispatch,
    .finalize = nullptr
  };

  m_sched = sched;
  m_tid = tid;

  m_source = g_source_new(&funcs, sizeof(GSource));
  g_source_attach(m_source, nullptr);

  m_sched->wakeFn(m_tid, ZmFn{this, [](App *app) {
    app->m_sched->run_(app->m_tid, []() { run(); });
    app->wake();
  }});
  m_sched->run_(m_tid, ZmFn{this, [](App *app) {
    run();
  }});
}

void App::detach()
{
  // g_source_detach(m_source, nullptr); // FIXME
  // g_source_final(m_source); // FIXME
  m_sched->wakeFn(m_tid, ZmFn{});
}

}
