#include <zlib/ZuArrayN.hpp>

#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkCallback.hpp>
#include <zlib/ZGtkTreeModel.hpp>
#include <zlib/ZGtkValue.hpp>

ZmSemaphore done;

void start();

int main(int argc, char **argv)
{
  ZmScheduler s{ZmSchedParams().id("sched").nThreads(2)};
  s.start();

  ZGtk::App app;

  app.attach(&s, 1);

  s.run(app.tid(), []() { start(); });

#if 0
  for (int i = 0; i < 5; i++) {
    s.run(app.tid(), []() {
      std::cout << ZmThreadContext::self() << '\n' << std::flush;
    }, ZmTimeNow(i));
  }
  ::sleep(6);
#endif

  done.wait();

  s.stop();

  app.detach();
}

struct TreeModel : public ZGtk::TreeSortable<TreeModel, 1> {
  ~TreeModel() { }

  struct Iter { gint index; };

  ZuAssert(sizeof(Iter) < sizeof(GtkTreeIter));

  GtkTreeModelFlags get_flags() {
    return static_cast<GtkTreeModelFlags>(
	GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
  }
  gint get_n_columns() { return 1; }
  GType get_column_type(gint i) { return G_TYPE_STRING; }
  gboolean get_iter(GtkTreeIter *iter, GtkTreePath *path) {
    gint depth = gtk_tree_path_get_depth(path);
    if (depth != 1) return false;
    gint *indices = gtk_tree_path_get_indices(path);
    if (indices[0] < 0 || indices[0] > 9) return false;
    auto index = indices[0];
    if (m_order != GTK_SORT_ASCENDING) index = 9 - index;
    new (iter) Iter{index};
    return true;
  }
  GtkTreePath *get_path(GtkTreeIter *iter_) {
    auto iter = reinterpret_cast<Iter *>(iter_);
    GtkTreePath *path;
    auto index = iter->index;
    path = gtk_tree_path_new();
    if (m_order != GTK_SORT_ASCENDING) index = 9 - index;
    gtk_tree_path_append_index(path, index);
    return path;
  }
  void get_value(GtkTreeIter *iter_, gint i, ZGtk::Value *value) {
    auto iter = reinterpret_cast<Iter *>(iter_);
    auto index = iter->index;
    value->init(G_TYPE_LONG);
    value->set_long(index * index);
  }
  gboolean iter_next(GtkTreeIter *iter_) {
    auto iter = reinterpret_cast<Iter *>(iter_);
    auto &index = iter->index;
    if (m_order != GTK_SORT_ASCENDING) {
      if (!index) return false;
      --index;
    } else {
      if (index >= 9) return false;
      ++index;
    }
    return true;
  }
  gboolean iter_children(GtkTreeIter *iter, GtkTreeIter *parent_) {
    if (parent_) {
      // auto parent = reinterpret_cast<Iter *>(parent_);
      return false;
    }
    new (iter) Iter{0};
    return true;
  }
  gboolean iter_has_child(GtkTreeIter *iter_) {
    if (!iter_) return true;
    // auto iter = reinterpret_cast<Iter *>(iter_);
    return false;
  }
  gint iter_n_children(GtkTreeIter *iter_) {
    if (!iter_) return 10;
    // auto iter = reinterpret_cast<Iter *>(iter_);
    return 0;
  }
  gboolean iter_nth_child(GtkTreeIter *iter, GtkTreeIter *parent_, gint n) {
    if (parent_) {
      // auto parent = reinterpret_cast<Iter *>(parent_);
      return false;
    }
    new (iter) Iter{n};
    return true;
  }
  gboolean iter_parent(GtkTreeIter *iter, GtkTreeIter *child_) {
    // if (!child_) return false;
    // auto child = reinterpret_cast<Iter *>(child_);
    return false;
  }
  void ref_node(GtkTreeIter *) { }
  void unref_node(GtkTreeIter *) { }

  bool m_order = GTK_SORT_ASCENDING;
  void sort(gint col, GtkSortType order) { // , const ZGtk::TreeSortFn &
    if (m_order == order) return;
    m_order = order;
    // emit gtk_tree_model_rows_reordered()
    // Note: new_order length must be same as number of rows
    ZuArrayN<gint, 10> new_order;
    auto path = gtk_tree_path_new();
    for (unsigned i = 0; i < 10; i++) new_order[i] = 9 - i;
    gtk_tree_model_rows_reordered(
	GTK_TREE_MODEL(this), path, nullptr, new_order.data());
    gtk_tree_path_free(path);
  }
};

void start()
{
  // std::cerr << "start() " << ZmPlatform::getTID() << '\n' << std::flush;

  gtk_init(nullptr, nullptr);

  auto builder = gtk_builder_new();
  GError *e = nullptr;

  if (!gtk_builder_add_from_file(builder, "zdash.glade", &e)) {
    if (e) {
      ZeLOG(Error, e->message);
      g_error_free(e);
    }
    done.post();
    return;
  }

  auto window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
  auto view = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
  auto watchlist = GTK_TREE_VIEW(gtk_builder_get_object(builder, "watchlist"));
  auto model = TreeModel::ctor();

  // the cell, column and view all need to be referenced together
  // by a containing application view object, and unref'd in reverse
  // order in the dtor

  auto addCol = [&](bool reverse) {
    auto col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "number");

    auto cell = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, cell, true);

    gtk_tree_view_column_set_cell_data_func(
	col, cell, [](
	    GtkTreeViewColumn *col, GtkCellRenderer *cell,
	    GtkTreeModel *model, GtkTreeIter *iter, gpointer reverse) {
	  static const gchar *props[] = {
	    "text", "background-rgba", "foreground-rgba"
	  };
	  static ZGtk::Value values[3];
	  static bool init = true;
	  if (init) {
	    init = false;
	    values[0].init(G_TYPE_STRING);
	    values[1].init(GDK_TYPE_RGBA);
	    values[2].init(GDK_TYPE_RGBA);
	  }

	  gint i;
	  {
	    GtkTreePath *path = gtk_tree_model_get_path(model, iter);
	    gint *indices = gtk_tree_path_get_indices(path);
	    i = indices[0];
	    gtk_tree_path_free(path);
	  }

	  gint j;
	  {
	    ZGtk::Value value;
	    gtk_tree_model_get_value(model, iter, 0, &value);
	    j = value.get_long();
	  }

	  {
	    static ZuStringN<24> text;
	    text.null(); text << j;
	    values[0].set_static_string(text);
	  }
	  {
	    static GdkRGBA bg = { 0.0, 0.0, 0.0, 1.0 };
	    if (reverse) {
	      bg.red = (gdouble)i / (gdouble)9;
	      bg.blue = (gdouble)(9 - i) / (gdouble)9;
	    } else {
	      bg.red = (gdouble)(9 - i) / (gdouble)9;
	      bg.blue = (gdouble)i / (gdouble)9;
	    }
	    values[1].set_static_boxed(&bg);
	  }
	  {
	    static GdkRGBA fg = { 1.0, 1.0, 1.0, 1.0 };
	    values[2].set_static_boxed(&fg);
	  }

	  g_object_setv(G_OBJECT(cell), 3, props, values);
	}, (gpointer)(uintptr_t)reverse, nullptr);

    // makes column sortable, and causes model->sort(0, order) to be called
    gtk_tree_view_column_set_sort_column_id(col, 0);

    gtk_tree_view_column_set_reorderable(col, true);

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  };

  addCol(false);
  addCol(true);

  model->drag(view, GTK_WIDGET(watchlist),
      [](TreeModel *model, GList *rows, GtkSelectionData *data) -> gboolean {
	gtk_selection_data_set(data, TreeModel::rowsAtom(), sizeof(rows)<<3,
	    reinterpret_cast<const guchar *>(&rows), sizeof(rows));
	return true;
      },
      [](TreeModel *model, GtkWidget *widget, GtkSelectionData *data) {
	if (gtk_selection_data_get_data_type(data) != TreeModel::rowsAtom())
	  return;
	gint length;
	auto ptr = gtk_selection_data_get_data_with_length(data, &length);
	if (length != sizeof(GList *)) return;
	auto rows = *reinterpret_cast<GList *const *>(ptr);
	for (GList *i = g_list_first(rows); i; i = g_list_next(i)) {
	  auto path = reinterpret_cast<GtkTreePath *>(i->data);
	  GtkTreeIter iter;
	  if (gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path)) {
	    ZGtk::Value value;
	    gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, 0, &value);
	    std::cout << value.get_long() << '\n';
	  }
	  gtk_tree_path_free(path);
	}
	g_list_free(rows);
      });

  model->view(view);

  g_signal_connect(G_OBJECT(window), "destroy",
      ZGtk::callback([](GObject *o, gpointer p) {
      // std::cerr << "destroy " << ZmPlatform::getTID() << '\n' << std::flush;
	reinterpret_cast<ZmSemaphore *>(p)->post();
      }), reinterpret_cast<gpointer>(&done));

  gtk_widget_show_all(GTK_WIDGET(window));

  gtk_window_present(GTK_WINDOW(window));
}
