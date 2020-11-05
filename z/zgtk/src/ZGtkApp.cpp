#include <zlib/ZGtkApp.hpp>

#include <zlib/ZtString.hpp>

#include <locale.h>
#include <libintl.h>

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

void App::i18n(ZtString domain, ZtString dataDir)
{
  dataDir <<
      // libintl uses UTF-8 directory paths on Windows
#ifndef _WIN32
    "/locale"
#else
    "\\locale"
#endif
    ;
  m_domain = ZuMv(domain);
  m_dataDir = ZuMv(dataDir);
}

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

    if (m_domain) {
      // setlocale(LC_ALL, ""); // gtk_init() calls setlocale()
      bindtextdomain(m_domain, m_dataDir);
      bind_textdomain_codeset(m_domain, "UTF-8");
      textdomain(m_domain);
    }
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

void App::detach(ZmFn<> fn)
{
  m_sched->wakeFn(m_tid, ZmFn{});
  m_sched->run_(m_tid, [this, fn = ZuMv(fn)]() mutable { detach_(ZuMv(fn)); });
  wake_();
}

void App::detach_(ZmFn<> fn)
{
  if (m_source) {
    g_source_destroy(m_source);
    g_source_unref(m_source);
    m_source = nullptr;
  }
  fn();
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
