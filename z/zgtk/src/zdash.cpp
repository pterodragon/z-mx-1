#include <zlib/ZmScheduler.hpp>

#include <zlib/ZGtkApp.hpp>

int main(int argc, char **argv)
{
  ZmScheduler s{ZmSchedParams().id("sched").nThreads(2)};
  s.start();

  ZGtk::App app;

  app.attach(&s, 1);

  for (int i = 0; i < 5; i++) {
    s.run(app.tid(), []() {
      std::cout << ZmThreadContext::self() << '\n' << std::flush;
    }, ZmTimeNow(i));
  }
  ::sleep(6);

  s.stop();

  app.detach();
}

// FIXME - key telemetry for deltas, do not use single static per type
//
// FIXME - need small framework to cast lambdas to raw function pointers
// with return type and args, so that G_CALLBACK() will work
//
// GtkBuilder *builder = gtk_builder_new();
//
// m_window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
//
// g_signal_connect(G_OBJECT(m_window), "destroy",
// G_CALLBACK([](GtkObject *o, gpointer p) {  ... }));
//
// gtk_widget_show(m_window);
//
// gtk_window_present(GTK_WINDOW(m_window)); // focus the main window
//
// g_object_unref(G_OBJECT(builder));
