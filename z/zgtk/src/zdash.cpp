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

// FIXME - css
//
// @define-color bg_color #f9a039;
//
// gtk_css_provider_load_from_file() // parse CSS file
// gtk_style_context_new()
// gtk_style_context_add_provider() // associate CSS with style context
// gtk_style_context_lookup_color() // look up color

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

namespace MainTree {
  template <typename T>
  class TelKey {
    static const T &data_();
  public:
    using Key = typename ZuDecay<decltype(data_().key())>::T;
    struct Accessor : public ZuAccessor<T, Key> {
      static auto value(const T &data) { return data.key(); }
    };
  };

  template <typename T>
  using TelTree =
    ZmRBTree<T,
      ZmRBTreeIndex<typename TelKey<T>::Accessor,
	ZmRBTreeLock<ZmNoLock> > >;

  template <unsigned Depth, typename Data>
  struct Child : public Data, public ZGtk::TreeNode::Child<Depth> {
    Child() = default;
    template <typename ...Args>
    Child(Args &&... args) : Data{ZuFwd<Args>(args)...} { }
  };

  template <
    unsigned Depth, typename Data, typename Child,
    typename Tree, typename XData = ZuNull>
  struct Parent :
      public Data,
      public ZGtk::TreeNode::Parent<Depth, Child>,
      public XData {
    Parent() = default;
    template <typename ...Args>
    Parent(Args &&... args) : Data{ZuFwd<Args>(args)...} { }

    Tree	tree;
  };

  template <
    unsigned Depth, typename Data, typename Tuple, typename XData = ZuNull>
  struct Branch :
      public Data,
      public ZGtk::TreeNode::Branch<Depth, Tuple>,
      public XData {
    Branch() = default;
    template <typename ...Args>
    Branch(Args &&... args) : Data{ZuFwd<Args>(args)...} { }
  };

  using Heap_ = Child<2, ZvTelemetry::Heap_load>
  using HeapTree = TelTree<Heap_>;
  using Heap = typename HeapTree::Node;

  using HashTbl_ = Child<2, ZvTelemetry::HashTbl_load>;
  using HashTblTree = TelTree<HashTbl_>;
  using HashTbl = typename HashTblTree_::Node;

  using Thread_ = Child<2, ZvTelemetry::Thread_load>;
  using ThreadTree = TelTree<Thread_>;
  using Thread = typename ThreadTree_::Node;

  using Socket_ = Child<3, ZvTelemetry::Socket_load>;
  using SocketTree = TelTree<Socket_>;
  using Socket = typename SocketTree_::Node;

  using Mx_ = Parent<2, ZvTelemetry::Mx_load, Socket, SocketTree>;
  using MxTree = TelTree<Mx_>;
  using Mx = typename MxTree::Node;

  using Link_ = Child<3, ZvTelemetry::Link_load>;
  using LinkTree = TelTree<Link_>;
  using Link = typename LinkTree::Node;

  using Engine_ = Parent<2, ZvTelemetry::Engine_load, Link, LinkTree>;
  using EngineTree = TelTree<Engine_>;
  using Engine = typename EngineTree::Node;

  using DBHost_ = Child<3, ZvTelemetry::DBHost_load>;
  using DBHostTree = TelTree<DBHost_>;
  using DBHost = typename DBHostTree::Node;

  using DB_ = Child<3, ZvTelemetry::DB_load>;
  using DBTree = TelTree<DB_>;
  using DB = typename DBTree::Node;

  // DB hosts
  struct DBHosts { auto key() const { return ZuMkTuple("hosts"); } };
  using DBHostParent = Parent<2, DBHosts, DBHost, DBHostTree>;
  // DBs
  struct DBs { auto key() const { return ZuMkTuple("dbs"); } };
  using DBParent = Parent<2, DBs, DB, DBTree>;

  // heaps
  struct Heaps { auto key() const { return ZuMkTuple("heaps"); } };
  using HeapParent = Parent<1, Heaps, Heap, HeapTree>;
  // hashTbls
  struct HashTbls { auto key() const { return ZuMkTuple("hashTbls"); } };
  using HashTblParent = Parent<1, HashTbls, HashTbl, HashTblTree>;
  // threads
  struct Threads { auto key() const { return ZuMkTuple("threads"); } };
  using ThreadParent = Parent<1, Threads, Thread, ThreadTree>;
  // multiplexers
  struct Mxs { auto key() const { return ZuMkTuple("multiplexers"); } };
  using MxParent = Parent<1, Mxs, Mx, MxTree>;
  // engines
  struct Engines { auto key() const { return ZuMkTuple("engines"); } };
  using EngineParent = Parent<1, Engines, Engine, EngineTree>;

  // dbEnv
  ZuDeclTuple(DBEnvTuple,
      (DBHostParent, hosts),
      (DBParent, dbs));
  using DBEnv = Branch<1, ZvTelemetry::DBEnv_load, DBEnvTuple>;
  // applications
  ZuDeclTuple(AppTuple,
      (HeapParent, heaps),
      (HashTblParent, hashTbls),
      (ThreadParent, threads),
      (MxParent, multiplexers),
      (EngineParent, engines),
      (DBEnv, dbEnv));
  struct AppXData { Link *link = nullptr; };
  using App = Branch<0, ZvTelemetry::App_load, AppTuple, AppXData>;

  using Root = ZGtk::TreeNode::Parent<0, App, ZGtk::TreeNode::Root>;

  enum { MaxDepth = 3 };

  // (*) - virtual parent node - not draggable
  using Iter = ZuUnion<
    App *,		// [app]

    // app children
    HeapParent *,	// app->heaps (*)
    HashTblParent *,	// app->hashTbls (*)
    ThreadParent *,	// app->threads (*)
    MxParent *,	// app->mxTbl (*)
    EngineParent *,	// app->engines (*)
    DBEnv *,		// app->dbEnv (*) !!! must be EngineParent * + 1 !!!

    // app grandchildren
    Heap *,		// app->heaps->[heap]
    HashTbl *,	// app->hashTbls->[hashTbl]
    Thread *,		// app->threads->[thread]
    Mx *,		// app->mxTbl->[mx]
    Engine *,		// app->engines->[engine]

    // app great-grandchildren
    Socket *,		// app->mxTbl->mx->[socket]
    Link *,		// app->engines->engine->[link]

    // DBEnv children
    DBHostParent *,	// app->dbEnv->hosts (*)
    DBParent *,	// app->dbEnv->dbs (*)

    // DBEnv grandchildren
    DBHost *,		// app->dbEnv->hosts->[host]
    DB *>;		// app->dbEnv->dbs->[db]

  class Model : public ZGtk::TreeHierarchy<Model, Iter, MaxDepth> {
  public:
    // root()
    Root *root() { return &m_root; }

    // parent() - child->parent type map
    template <typename T>
    static typename ZuSame<T, App, Root>::T *parent(T *ptr) {
      return ptr->parent<Root>();
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, HeapParent>::Same ||
	ZuConversion<T, HashTblParent>::Same ||
	ZuConversion<T, ThreadParent>::Same ||
	ZuConversion<T, MxParent>::Same ||
	ZuConversion<T, EngineParent>::Same ||
	ZuConversion<T, DBEnv>::Same, App *>::T *parent(T *ptr) {
      return ptr->parent<App>();
    }
    template <static typename T>
    static typename ZuSame<T, Heap, HeapParent>::T *parent(T *ptr) {
      return ptr->parent<HeapParent>();
    }
    template <static typename T>
    static typename ZuSame<T, HashTbl, HashTblParent>::T parent(T *ptr) {
      return ptr->parent<HashTblParent>();
    }
    template <static typename T>
    static typename ZuSame<T, Thread, ThreadParent>::T parent(T *ptr) {
      return ptr->parent<ThreadParent>();
    }
    template <static typename T>
    static typename ZuSame<T, Mx, MxParent>::T *parent(T *ptr) {
      return ptr->parent<MxParent>();
    }
    template <static typename T>
    static typename ZuSame<T, Engine, EngineParent>::T parent(T *ptr) {
      return ptr->parent<EngineParent>();
    }
    template <static typename T>
    static typename ZuSame<T, Socket, Mx>::T *parent(T *ptr) {
      return ptr->parent<Mx>();
    }
    template <static typename T>
    static typename ZuSame<T, Link, Engine>::T *parent(T *ptr) {
      return ptr->parent<Engine>();
    }
    template <static typename T>
    static typename ZuIfT<
	ZuConversion<T, DBHostParent>::Same ||
	ZuConversion<T, DBParent>::Same, DBEnv>::T *parent(T *ptr) {
      return ptr->parent<DBEnv>();
    }
    template <static typename T>
    static typename ZuSame<T, DBHost, DBHostParent>::T parent(T *ptr) {
      return ptr->parent<DBHostParent>();
    }
    template <static typename T>
    static typename ZuSame<T, DB, DBParent>::T *parent(T *ptr) {
      return ptr->parent<DBParent>();
    }

    // value() - get value for column i from ptr
    template <typename T>
    static void value(T *ptr, gint i, ZGtk::Value *value) {
      if (i) {
	m_value.length(0);
	value->init(G_TYPE_STRING);
	m_value << ptr->key().print("|");
	value->set_static_string(m_value);
      } else {
	value->init(G_TYPE_INT);
	value->set_int(ptr->rag());
      }
    }

  private:
    Root	m_root;	// root of tree
    ZtString	m_value;
  };

  class View {
  public:
    void init(GtkTreeView *view, GtkStyleContext *context) :
	m_treeView(view) {
      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_red_fg", &m_rag_red_fg))
	m_rag_red_fg = { 0.0, 0.0, 0.0, 1.0 }; // #000000
      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_red_bg", &m_rag_red_bg))
	m_rag_red_bg = { 1.0, 0.0820, 0.0820, 1.0 }; // #ff1515

      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_amber_fg", &m_rag_amber_fg))
	m_rag_amber_fg = { 0.0, 0.0, 0.0, 1.0 }; // #000000
      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_amber_bg", &m_rag_amber_bg))
	m_rag_amber_bg = { 1.0, 0.5976, 0.0, 1.0 }; // #ff9900

      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_green_fg", &m_rag_green_fg))
	m_rag_green_fg = { 0.0, 0.0, 0.0, 1.0 }; // #000000
      if (!context || !gtk_style_context_lookup_color(
	    context, "rag_green_bg", &m_rag_green_bg))
	m_rag_green_bg = { 0.1835, 0.8789, 0.2304, 1.0 }; // #2fe13b

      auto col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title(col, "name");

      auto cell = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, cell, true);

      gtk_tree_view_column_set_cell_data_func(
	  col, cell, [](
	      GtkTreeViewColumn *col, GtkCellRenderer *cell,
	      GtkTreeModel *model, GtkTreeIter *iter, gpointer) {
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

	    gtk_tree_model_get_value(model, iter, 1, &values[0]);

	    gint rag;
	    {
	      ZGtk::Value rag_;
	      gtk_tree_model_get_value(model, iter, 0, &rag_);
	      rag = rag_.get_int();
	    }
	    switch (rag) {
	      case ZvTelemetry::RAG::Red: {
		values[1].set_static_boxed(&m_rag_red_bg);
		values[2].set_static_boxed(&m_rag_red_fg);
		g_object_setv(G_OBJECT(cell), 3, props, values);
	      } break;
	      case ZvTelemetry::RAG::Amber: {
		values[1].set_static_boxed(&m_rag_amber_bg);
		values[2].set_static_boxed(&m_rag_amber_fg);
		g_object_setv(G_OBJECT(cell), 3, props, values);
	      } break;
	      case ZvTelemetry::RAG::Green: {
		values[1].set_static_boxed(&m_rag_green_bg);
		values[2].set_static_boxed(&m_rag_green_fg);
		g_object_setv(G_OBJECT(cell), 3, props, values);
	      } break;
	      default:
		g_object_setv(G_OBJECT(cell), 1, props, values);
		break;
	    }
	  }, nullptr, nullptr);

      gtk_tree_view_append_column(m_treeView, col);
    }

    void bind(GtkTreeModel *model) {
      gtk_tree_view_set_model(m_treeView, GTK_TREE_MODEL(model));
      g_object_unref(G_OBJECT(model));
    }

  private:
    GtkTreeView	*m_treeView = nullptr;
    GdkRGBA	m_rag_red_fg;
    GdkRGBA	m_rag_red_bg;
    GdkRGBA	m_rag_amber_fg;
    GdkRGBA	m_rag_amber_bg;
    GdkRGBA	m_rag_green_fg;
    GdkRGBA	m_rag_green_bg;
  };
}

class ZDash : public ZmPolymorph, public ZvCmdClient<ZDash>, public ZCmdHost {
  using Base = ZvCmdClient<ZDash>;

  // FIXME - use ZGtk::TreeArray<> to define all watchlists
  // FIXME - figure out how to save/load tree view columns, sort, etc.

  class Link : public ZvCmdCliLink<ZDash, Link> {
  public:
    using Base = ZvCmdCliLink<ZDash, Link>;

    auto key() { return ZuMkTuple(this->server(), this->port()); }

    void loggedIn() {
      this->app()->loggedIn(this);
    }
    void disconnected() {
      this->app()->disconnected(this);
      Base::disconnected();
    }
    void connectFailed(bool transient) {
      this->app()->connectFailed(this);
      // Base::connectFailed(transient);
    }
    int processApp(ZuArray<const uint8_t> data) {
      return this->app()->processApp(this, data);
    }
    int processTel(const ZvTelemetry::fbs::Telemetry *data) {
      return this->app()->processTel(this, data);
    }
  };

  MainTree::View	m_view;
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
