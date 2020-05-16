//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>

#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuPolymorph.hpp>
#include <zlib/ZuByteSwap.hpp>

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmTrap.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiMultiplex.hpp>
#include <zlib/ZiModule.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvCSV.hpp>
#include <zlib/ZvMultiplex.hpp>
#include <zlib/ZvCmdClient.hpp>
#include <zlib/ZvCmdHost.hpp>
#include <zlib/ZvUserDB.hpp>

#include <zlib/ZtlsBase32.hpp>
#include <zlib/ZtlsBase64.hpp>

#include <zlib/Zrl.hpp>
#include <zlib/ZCmd.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkCallback.hpp>
#include <zlib/ZGtkTreeModel.hpp>
#include <zlib/ZGtkValue.hpp>

// FIXME
// each node needs to include:
// index row (in left-hand tree)
// watch window and row (in watchlist)
// array of time series

// time series structure
// key frames per-packet (1.5K Ethernet MTU)
// delta compression
// values: floating point +1.0
// 

// each child needs a backpointer to parent, and a row
// each parent needs a container of child nodes
//   and an index of child iters (a rows[] array)

class ZDash : public ZmPolymorph, public ZvCmdClient<ZDash>, public ZCmdHost {
  using Base = ZvCmdClient<ZDash>;

  // FIXME - distinguish selection tree from watchlists
  // FIXME - use ZGtk::TreeArray<> to define all watchlists
  // FIXME - figure out how to save/load tree view columns, sort, etc.

  class Link;

  struct Tree : public ZGtk::TreeModel<Tree> {

    struct TelItem_ {
      gint			row = -1;
    };
    struct TelChild_ : public TelItem_ {
      TelItem_			*parent = nullptr;
    };
    template <typename Iter>
    struct TelParent_ {
      ZtArray<Iter>	children;

      template <typename Iter_>
      ZtArray<Iter_> &children_as() {
	ZuAssert(sizeof(Iter_) == sizeof(Iter));
	return *reinterpret_cast<ZtArray<Iter_> *>(&children);
      }
    };

    template <typename T>
    struct TelKey {
      static const T &data_();
      using Key = typename ZuTraits<decltype(data_().key())>::T;
      struct Accessor : public ZuAccessor<T, Key> {
	static Key value(const T &data) { return data.key(); }
      };
    };

    template <typename T>
    struct TelItem : public TelItem_, public T {
      TelItem() = default;
      template <typename ...Args>
      TelItem(Args &&... args) : T{ZuFwd<Args>(args)...} { }
    };
    template <typename T>
    struct TelChild : public TelChild_, public T {
      TelChild() = default;
      template <typename ...Args>
      TelChild(Args &&... args) : T{ZuFwd<Args>(args)...} { }
    };
    template <typename T, typename Iter, typename Data = ZuNull>
    struct TelParent : public TelItem_, public TelParent_<Iter>, public T {
      TelParent() = default;
      template <typename ...Args>
      TelParent(Args &&... args) : data{ZuFwd<Args>(args)...} { }
      Data	data;
    };
    template <typename T, typename Iter, typename Data = ZuNull>
    struct TelMidParent : public TelChild_, public TelParent_<Iter>, public T {
      TelMidParent() = default;
      template <typename ...Args>
      TelMidParent(Args &&... args) : data{ZuFwd<Args>(args)...} { }
      Data	data;
    };

    using AnyIter_ = ZuUnion<void *>;

    template <typename T>
    using TelTree =
      ZmRBTree<T,
	ZmRBTreeIndex<typename TelKey<T>::Accessor,
	  ZmRBTreeBase<ZmObject,
	    ZmRBTreeLock<ZmNoLock> > > >;

    using HeapTree = TelTree<TelChild<ZvTelemetry::Heap_load>>;
    using HeapIter = typename HeapTree_::Node *;

    using HashTblTree = TelTree<TelChild<ZvTelemetry::HashTbl_load>>;
    using HashTblIter = typename HashTblTree_::Node *;

    using ThreadTree = TelTree<TelChild<ZvTelemetry::Thread_load>>;
    using ThreadIter = typename ThreadTree_::Node *;

    using SocketTree = TelTree<TelChild<ZvTelemetry::Socket_load>>;
    using SocketIter = typename SocketTree_::Node *;

    using MxNode = TelMidParent<SocketTree, SocketIter, ZvTelemetry::Mx_load>;
    using MxTree = TelTree<MxNode>;
    using MxIter = typename MxTree::Node *;

    using LinkTree = TelTree<TelChild<ZvTelemetry::Link_load>>;
    using LinkIter = typename LinkTree::Node *;

    using Engine =
      TelMidParent<LinkTree, LinkIter, ZvTelemetry::Engine_load>;
    using EngineTree = TelTree<Engine>;
    using EngineIter = typename EngineTree::Node *;

    using DBHostTree = TelTree<TelChild<ZvTelemetry::DBHost_load>>;
    using DBHostIter = typename DBHostTree::Node *;

    using DBTree = TelTree<TelChild<ZvTelemetry::DB_load>>;
    using DBIter = typename DBTree::Node *;

    using DBHostParent = TelMidParent<DBHostTree>;
    using DBHostParentIter = DBHostParent *;

    using DBParent = TelMidParent<DBTree>;
    using DBParentIter = DBParent *;

    // heaps
    using HeapParent = TelMidParent<HeapTree, HeapIter>;
    using HeapParentIter = HeapParent *;
    // hashTbls
    using HashTblParent = TelMidParent<HashTblTree, HashTblIter>;
    using HashTblParentIter = HashTblParent *;
    // threads
    using ThreadParent = TelMidParent<ThreadTree, ThreadIter>;
    using ThreadParentIter = ThreadParent *;
    // multiplexers
    using MxParent = TelMidParent<MxTree, MxIter>;
    using MxParentIter = MxParent *;
    // engines
    using EngineParent = TelMidParent<EngineTree, EngineIter>;
    using EngineParentIter = EngineParent *;
    // dbEnv
    struct DBEnv_ {
      DBHostParent	hosts;
      DBParent		dbs;
    };
    using DBEnv = TelMidParent<DBEnv_, AnyIter_, ZvTelemetry::DBEnv_load> { };
    using DBEnvIter = DBEnv *;
    // applications
    struct App_ {
      Link		*link = nullptr;
      HeapParent	heaps;
      HashTblParent	hashTbls;
      ThreadParent	threads;
      MxParent		mxTbl;
      EngineParent	engines;
      DBEnv		dbEnv;
    };
    using App = TelParent<AppNode_, AnyIter_, ZvTelemetry::App_load>
    using AppTree = TelTree<App>;
    using AppIter = typename AppTree:Node *;

    AppTree		m_apps;
    ZtArray<AppIter *>	m_appRows;

    // (*) - not draggable
    using Iter = ZuUnion<
      AppIter,		// [app]
      HeapParentIter,	// app->heaps (*)
      HashTblParentIter,// app->hashTbls (*)
      ThreadParentIter,	// app->threads (*)
      MxParentIter,	// app->mxTbl (*)
      EngineParentIter,	// app->engines (*)
      DBEnvParentIter,	// app->dbEnv (*)
      HeapIter,		// app->heaps->[heap]
      HashTblIter,	// app->hashTbls->[hashTbl]
      ThreadIter,	// app->threads->[thread]
      MxIter,		// app->mxTbl->[mx]
      SocketIter,	// app->mxTbl->mx->[socket]
      EngineIter,	// app->engines->[engine]
      LinkIter,		// app->engines->engine->[link]
      DBEnvIter,	// app->[dbEnv]
      DBHostParentIter,	// app->dbEnv->hosts (*)
      DBParentIter,	// app->dbEnv->dbs (*)
      DBHostIter,	// app->dbEnv->hosts->[host]
      DBIter>;		// app->dbEnv->dbs->[db]

    ZuAssert(sizeof(Iter) < sizeof(GtkTreeIter));

    GtkTreeModelFlags get_flags() {
      return static_cast<GtkTreeModelFlags>(GTK_TREE_MODEL_ITERS_PERSIST);
    }
    gint get_n_columns() { return 1; }
    GType get_column_type(gint i) { return G_TYPE_STRING; }
    gboolean get_iter(GtkTreeIter *iter, GtkTreePath *path) {
      gint depth = gtk_tree_path_get_depth(path);
      gint *indices = gtk_tree_path_get_indices(path);
      gint i = indices[0];
      if (i < 0 || i >= m_appRows.length()) return false;
      auto app = m_appRows[i];
      if (depth == 1) {
	new (Iter::new_<AppIter>(iter)) AppIter{app};
	return true;
      }
      i = indices[1];
      // FIXME from here
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

  class Link : public ZvCmdCliLink<ZDash, Link> {
  public:
    using Base = ZvCmdCliLink<ZDash, Link>;

    void loggedIn() {
      this->app()->loggedIn();
    }
    void disconnected() {
      this->app()->disconnected();
      Base::disconnected();
    }
    void connectFailed(bool transient) {
      this->app()->connectFailed();
      // Base::connectFailed(transient);
    }
    int processApp(ZuArray<const uint8_t> data) {
      return this->app()->processApp(data);
    }
    int processTel(const ZvTelemetry::fbs::Telemetry *data) {
      return this->app()->processTel(data);
    }

  private:
    // FIXME - each link should have a name that defaults to host:port
    // FIXME - when creating/adding the link, user specified host:port and
    // optionally name
  };



ZmSemaphore done;

void start();

int main(int argc, char **argv)
{
  ZmScheduler s{ZmSchedParams{}.id("sched").nThreads(2)};
  s.start();

  ZGtk::App app;

  app.attach(&s, 1);

  s.run(app.tid(), [=]() { start(argc, argv); });

  done.wait();

  s.stop();

  app.detach();
}

// FIXME start with just heaps

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
