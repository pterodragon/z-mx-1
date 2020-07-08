#include <zlib/ZGtkApp.hpp>

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

void App::attach(ZmScheduler *sched, unsigned tid)
{
  m_sched = sched;
  m_tid = tid;

  m_sched->run_(m_tid, ZmFn{this, [](App *app) { app->attach_(); }});
}

void App::attach_()
{
  static bool initialized = false;

  if (!initialized) {
#ifdef _WIN32
    putenv("GTK_CSD=0");
    putenv("GTK_THEME=win32");
#endif
    gtk_init(nullptr, nullptr);
    initialized = true;
  }

  static GSourceFuncs funcs = {
    .prepare = nullptr,
    .check = nullptr,
    .dispatch = dispatch,
    .finalize = nullptr
  };

  m_source = g_source_new(&funcs, sizeof(GSource));
  g_source_attach(m_source, nullptr);

  m_sched->wakeFn(m_tid, ZmFn{this, [](App *app) { app->wake(); }});
  m_sched->run_(m_tid, ZmFn{this, [](App *app) { app->run_(); }});
}

void App::detach() // FIXME - completion / continuation?
{
  m_sched->wakeFn(m_tid, ZmFn{});
  m_sched->run_(m_tid, ZmFn{this, [](App *app) { app->detach_(); }});
}

void App::detach_() // FIXME - app-specific Gtk thread cleanup / finalization?
{
  g_source_destroy(m_source);
  g_source_unref(m_source);
  m_source = nullptr;
}

void App::wake()
{
  m_sched->run_(m_tid, []() { run_(); });
  wake_();
}

void App::wake_()
{
  g_source_set_ready_time(m_source, 0);
  g_main_context_wakeup(nullptr);
}

void App::run_()
{
  gtk_main();
}

}
