#include <zlib/ZuArrayN.hpp>

#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkCallback.hpp>
#include <zlib/ZGtkTreeModel.hpp>

ZmSemaphore done;

void start();
void activate(GtkApplication *, gpointer);

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

static const auto rowsAtomName = "zdash/rows";
static GdkAtom rowsAtom()
{
  static GdkAtom atom = nullptr;
  if (!atom) atom = gdk_atom_intern_static_string(rowsAtomName);
  return atom;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings" 
static GtkTargetEntry rowsTargets_[] = {
  { (gchar *)rowsAtomName, GTK_TARGET_SAME_APP, 0 }
};
#pragma GCC diagnostic pop
static constexpr const GtkTargetEntry *rowsTargets()
{
  return rowsTargets_;
}
static constexpr const gint nRowsTargets()
{
  return sizeof(rowsTargets_) / sizeof(rowsTargets_[0]);
}

struct TreeModel : public ZGtk::TreeModel<TreeModel> {
  ~TreeModel() {
    // FIXME - iterate over columns and default column
    if (m_user_data && m_destroy) m_destroy(m_user_data);
  }

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
    iter->stamp = (uintptr_t)this;
    auto index = indices[0];
    if (m_order != GTK_SORT_ASCENDING) index = 9 - index;
    iter->user_data = (gpointer)(uintptr_t)index;
    iter->user_data2 = nullptr;
    iter->user_data3 = nullptr;
    return true;
  }
  GtkTreePath *get_path(GtkTreeIter *iter) {
    GtkTreePath *path;
    gint index = (uintptr_t)iter->user_data;
    path = gtk_tree_path_new();
    if (m_order != GTK_SORT_ASCENDING) index = 9 - index;
    gtk_tree_path_append_index(path, index);
    return path;
  }
  void get_value(GtkTreeIter *iter, gint i, GValue *value) {
    g_value_init(value, G_TYPE_STRING);
    unsigned j = (uintptr_t)iter->user_data;
    // if (m_order != GTK_SORT_ASCENDING) j = 9 - j;
    j *= j;
    static ZuStringN<24> v;
    v.null(); v << j;
    g_value_set_static_string(value, v);
  }
  gboolean iter_next(GtkTreeIter *iter) {
    uintptr_t index = (uintptr_t)iter->user_data;
    if (m_order != GTK_SORT_ASCENDING) {
      if (!index) return false;
      iter->user_data = (gpointer)(index - 1);
    } else {
      if (index >= 9) return false;
      iter->user_data = (gpointer)(index + 1);
    }
    return true;
  }
  gboolean iter_children(GtkTreeIter *iter, GtkTreeIter *parent) {
    if (parent) return false;
    iter->stamp = (uintptr_t)this;
    iter->user_data = (gpointer)(uintptr_t)0;
    puts("iter_children() 0\n");
    iter->user_data2 = nullptr;
    iter->user_data3 = nullptr;
    return true;
  }
  gboolean iter_has_child(GtkTreeIter *iter) {
    if (!iter) return true;
    return false;
  }
  gint iter_n_children(GtkTreeIter *iter) {
    if (!iter) return 10;
    return 0;
  }
  gboolean iter_nth_child(GtkTreeIter *iter, GtkTreeIter *parent, gint n) {
    if (parent) return false;
    iter->stamp = (uintptr_t)this;
    iter->user_data = (gpointer)(uintptr_t)n;
    iter->user_data2 = nullptr;
    iter->user_data3 = nullptr;
    return true;
  }
  gboolean iter_parent(GtkTreeIter *iter, GtkTreeIter *child) {
    return false;
  }

  // gint m_sortCol = GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;// DEFAULT/UNSORTED
  gint m_sortCol = 0;
  GtkSortType m_order = GTK_SORT_ASCENDING;// ASCENDING/DESCENDING

  // FIXME - below for each column, and one for the default (if we
  // want to support app-specified sort functions)
  GtkTreeIterCompareFunc m_fn = nullptr;
  gpointer m_user_data = nullptr;
  GDestroyNotify m_destroy = nullptr;

  gboolean get_sort_column_id(gint *column, GtkSortType *order) {
    if (column) *column = m_sortCol;
    if (order) *order = m_order;
    switch ((int)m_sortCol) {
      case GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID:
      case GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID:
	return false;
      default:
	return true;
    }
  }
  void set_sort_column_id(gint column, GtkSortType order) {
    bool reordered = m_order != order;
   
    // printf("set_sort_column_id() column=%d order=%d reordered=%d\n", (int)column, (int)order, (int)reordered);

    m_sortCol = column;
    m_order = order;

    // emit gtk_tree_sortable_sort_column_changed()
    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(this));

    if (!reordered) return;

    // emit gtk_tree_model_rows_reordered()
    // Note: new_order length must be same as number of rows
    ZuArrayN<gint, 10> new_order;
    auto path = gtk_tree_path_new();
    for (unsigned i = 0; i < 10; i++) new_order[i] = 9 - i;
    gtk_tree_model_rows_reordered(
	GTK_TREE_MODEL(this), path, nullptr, &new_order[0]);
    gtk_tree_path_free(path);
  }

  void set_sort_func(
      gint column, GtkTreeIterCompareFunc fn,
      gpointer user_data, GDestroyNotify destroy) {
    if (m_user_data && m_destroy) m_destroy(m_user_data);
    m_fn = fn;
    m_user_data = user_data;
    m_destroy = destroy;
  }
  void set_default_sort_func(
      GtkTreeIterCompareFunc fn, gpointer user_data, GDestroyNotify destroy) {
    if (m_user_data && m_destroy) m_destroy(m_user_data);
    m_fn = fn;
    m_user_data = user_data;
    m_destroy = destroy;
  }
  gboolean has_default_sort_func() {
    return false; // true;
  }

  gboolean row_draggable(GList *rows) {
    return true;
  }
  gboolean drag_data_get(GList *rows, GtkSelectionData *data) {
    gtk_selection_data_set(data, rowsAtom(), sizeof(rows)<<3,
	reinterpret_cast<const guchar *>(&rows), sizeof(rows));
    return true;
  }
  gboolean drag_data_delete(GList *rows) {
    return false;
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

  auto window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
  auto view = GTK_WIDGET(gtk_builder_get_object(builder, "treeview"));
  auto watchlist = GTK_WIDGET(gtk_builder_get_object(builder, "watchlist"));

  g_object_unref(G_OBJECT(builder));

  auto model = TreeModel::ctor();

#if 0
  gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(model),
      [](GtkTreeModel *, GtkTreeIter *a, GtkTreeIter *b, gpointer) -> gint {
	return (int)(uintptr_t)a->user_data - (int)(uintptr_t)b->user_data;
      }, nullptr, nullptr);
#endif

  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
      // GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
      0,
      GTK_SORT_ASCENDING);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(model));

  g_object_unref(model); // view now owns model

  // the renderer, column and view all need to be referenced together
  // by a containing application view object, and unref'd in reverse
  // order in the dtor

  auto renderer = gtk_cell_renderer_text_new();
  auto col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, "number");
  gtk_tree_view_column_pack_start(col, renderer, true);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);

  gtk_tree_view_column_set_sort_column_id(col, 0);

  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  gtk_drag_source_set(view,
      GDK_BUTTON1_MASK, rowsTargets(), nRowsTargets(), GDK_ACTION_COPY);

  egg_tree_multi_drag_add_drag_support(GTK_TREE_VIEW(view));

  g_signal_connect(G_OBJECT(view), "drag-data-get",
      ZGtk::callback([](GObject *o, GdkDragContext *,
	  GtkSelectionData *data, guint, guint) {
	auto view = GTK_TREE_VIEW(o);
	g_return_if_fail(!!view);
	auto sel = gtk_tree_view_get_selection(view);
	g_return_if_fail(!!sel);
	GtkTreeModel *model = nullptr;
	auto rows = gtk_tree_selection_get_selected_rows(sel, &model);
	g_return_if_fail(!!rows);
	g_return_if_fail(!!model);
	egg_tree_multi_drag_source_drag_data_get(
	    EGG_TREE_MULTI_DRAG_SOURCE(model), rows, data);
      }), 0);

  gtk_drag_dest_set(watchlist,
      GTK_DEST_DEFAULT_ALL, rowsTargets(), nRowsTargets(), GDK_ACTION_COPY);

  g_signal_connect(G_OBJECT(watchlist), "drag-data-received",
      ZGtk::callback([](GtkWidget *widget, GdkDragContext *, int, int,
	  GtkSelectionData *data, guint, guint32, gpointer x) {
	if (gtk_selection_data_get_data_type(data) != rowsAtom()) return;
	gint length;
	auto ptr = gtk_selection_data_get_data_with_length(data, &length);
	auto rows = *reinterpret_cast<GList *const *>(ptr);
	for (GList *i = g_list_first(rows); i; i = g_list_next(i)) {
	  auto path = reinterpret_cast<GtkTreePath *>(i->data);
	  gint n;
	  auto indices = gtk_tree_path_get_indices_with_depth(path, &n);
	  for (gint j = 0; j < n; j++) {
	    if (j) std::cout << ':';
	    std::cout << indices[j];
	  }
	  std::cout << '\n';
	  gtk_tree_path_free(path);
	}
	g_list_free(rows);
	g_signal_stop_emission_by_name(widget, "drag-data-received");
      }), nullptr);

  g_signal_connect(G_OBJECT(window), "destroy",
      ZGtk::callback([](GObject *o, gpointer p) {
	// std::cerr << "destroy " << ZmPlatform::getTID() << '\n' << std::flush;
	reinterpret_cast<ZmSemaphore *>(p)->post();
      }), reinterpret_cast<gpointer>(&done));

  gtk_widget_show_all(window);

  gtk_window_present(GTK_WINDOW(window));
}
