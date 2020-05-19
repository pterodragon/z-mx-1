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
    template <unsigned, typename Data>
    class TelRoot {
    public:
      TelRoot() = default;
      template <typename ...Args>
      TelRoot(Args &&... args) : data{ZuFwd<Args>(args)...} { }

      Data			data;
    };

    class TelRow {
    public:
      gint row() const { return m_row; }
      void row(gint v) { m_row = v; }

    private:
      gint			m_row = -1;
    };

    template <int Depth> class TelChild;

    template <> class TelChild<-1> { };

    template <> class TelChild<0> : public TelRow {
    public:
      constexpr static unsigned depth() const { return 0; }
      void ascend(const gint *indices) const { indices[0] = this->row(); }
    };

    template <unsigned Depth> class TelChild : public TelRow {
    public:
      constexpr static unsigned depth() const { return Depth; }
      void ascend(const gint *indices) const {
	indices[Depth] = this->row();
	m_parent->ascend(indices);
      }

      template <typename Parent> Parent *parent() const {
	return reinterpret_cast<Parent *>(m_parent);
      }
      void parent(TelChild<Depth - 1> *p) { m_parent = p; }

    private:
      TelChild<Depth - 1>	*m_parent = nullptr;
    };

    template <unsigned Depth, typename Data>
    class TelItem : public TelChild<Depth> {
    public:
      TelItem() = default;
      template <typename ...Args>
      TelItem(Args &&... args) : data{ZuFwd<Args>(args)...} { }

      constexpr static bool hasChild() { return false; }
      constexpr static unsigned nChildren() { return 0; }
      template <typename L>
      bool child(gint, L l) const { return false; }
      template <typename L>
      auto descend(const gint *, unsigned, L &l) const { return l(this); }

      Data			data;
    };

    template <
      unsigned Depth, typename Data, typename Base, typename ChildPtr,
      template <unsigned, typename> Type = TelItem>
    class TelParent : public Type<Depth, Data>, public Base {
    public:
      TelParent() = default;
      template <typename ...Args>
      TelParent(Args &&... args) : Type<Depth, Data>{ZuFwd<Args>(args)...} { }

      constexpr bool hasChild() const { return true; }
      constexpr unsigned nChildren() const { return m_children.length(); }
      template <typename L>
      bool child(gint i, L l) const {
	if (ZuUnlikely(i < 0 || i >= m_children.length())) return false;
	l(m_children[i]);
        return true;
      }
      template <typename L>
      auto descend(const gint *indices, unsigned n, L &l) const {
	if (!n) return l(this);
	auto i = indices[0];
	if (i < 0 || i >= m_children.length())
	  return decltype(l(decltype(m_children[0]){})){};
	++indices, --n;
	return m_children[i]->descend(indices, n, l);
      }

    private:
      ZtArray<ChildPtr>		m_children;
    };

    template <
      unsigned Depth, typename Data, typename Base, typename Children,
      template <unsigned, typename> Type = TelItem>
    class TelBranch : public Type<Depth, Data>, public Base {
    public:
      TelBranch() = default;
      template <typename ...Args>
      TelBranch(Args &&... args) : Type<Depth, Data>{ZuFwd<Args>(args)...} { }

      constexpr bool hasChild() const { return true; }
      constexpr unsigned nChildren() const { return Children::N; }
      template <typename L>
      bool child(gint i, L l) const {
	if (ZuUnlikely(i < 0 || i >= Children::N)) return false;
	Children::dispatch(i, [&l](auto ptr) { l(ptr); });
	return true;
      }
      template <typename L>
      auto descend(const gint *indices, unsigned n, L &l) const {
	if (!n) return l(this);
	auto i = indices[0];
	++indices, --n;
	return Children::dispatch(i, [indices, n, &l](auto ptr) {
	  return ptr->descend(indices, n, l);
	});
      }

    private:
      Children			m_children;
    };

    template <typename T>
    struct TelKey {
      static const T &data_();
      using Key = typename ZuDecay<decltype(data_().data.key())>::T;
      struct Accessor : public ZuAccessor<T, Key> {
	static auto value(const T &data) { return data.key(); }
      };
    };

    template <typename T>
    using TelTree =
      ZmRBTree<T,
	ZmRBTreeIndex<typename TelKey<T>::Accessor,
	  ZmRBTreeBase<ZmObject,
	    ZmRBTreeLock<ZmNoLock> > > >;

    using HeapTree = TelTree<TelChild<2, ZvTelemetry::Heap_load>>;
    using HeapPtr = typename HeapTree_::Node *;

    using HashTblTree = TelTree<TelChild<2, ZvTelemetry::HashTbl_load>>;
    using HashTblPtr = typename HashTblTree_::Node *;

    using ThreadTree = TelTree<TelChild<2, ZvTelemetry::Thread_load>>;
    using ThreadPtr = typename ThreadTree_::Node *;

    using SocketTree = TelTree<TelChild<3, ZvTelemetry::Socket_load>>;
    using SocketPtr = typename SocketTree_::Node *;

    using Mx = TelParent<2, ZvTelemetry::Mx_load, SocketTree, SocketPtr>;
    using MxTree = TelTree<Mx>;
    using MxPtr = typename MxTree::Node *;

    using LinkTree = TelTree<TelChild<3, ZvTelemetry::Link_load>>;
    using LinkPtr = typename LinkTree::Node *;

    using Engine = TelParent<2, ZvTelemetry::Engine_load, LinkTree, LinkPtr>;
    using EngineTree = TelTree<Engine>;
    using EnginePtr = typename EngineTree::Node *;

    using DBHostTree = TelTree<TelChild<3, ZvTelemetry::DBHost_load>>;
    using DBHostPtr = typename DBHostTree::Node *;

    using DBTree = TelTree<TelChild<3, ZvTelemetry::DB_load>>;
    using DBPtr = typename DBTree::Node *;

    // DB hosts
    struct DBHosts { auto key() const { return ZuMkTuple("hosts"); } };
    using DBHostParent = TelParent<2, DBHosts, DBHostTree, DBHostPtr>;
    using DBHostParentPtr = DBHostParent *;
    // DBs
    struct DBs { auto key() const { return ZuMkTuple("dbs"); } };
    using DBParent = TelParent<2, DBs, DBTree, DBPtr>;
    using DBParentPtr = DBParent *;

    // heaps
    struct Heaps { auto key() const { return ZuMkTuple("heaps"); } };
    using HeapParent = TelParent<1, Heaps, HeapTree, HeapPtr>;
    using HeapParentPtr = HeapParent *;
    // hashTbls
    struct HashTbls { auto key() const { return ZuMkTuple("hashTbls"); } };
    using HashTblParent = TelParent<1, HashTbls, HashTblTree, HashTblPtr>;
    using HashTblParentPtr = HashTblParent *;
    // threads
    struct Threads { auto key() const { return ZuMkTuple("threads"); } };
    using ThreadParent = TelParent<1, Threads, ThreadTree, ThreadPtr>;
    using ThreadParentPtr = ThreadParent *;
    // multiplexers
    struct Mxs { auto key() const { return ZuMkTuple("multiplexers"); } };
    using MxParent = TelParent<1, Mxs, MxTree, MxPtr>;
    using MxParentPtr = MxParent *;
    // engines
    struct Engines { auto key() const { return ZuMkTuple("engines"); } };
    using EngineParent = TelParent<1, Engines, EngineTree, EnginePtr>;
    using EngineParentPtr = EngineParent *;

    // dbEnv
    ZuDeclTuple(DBEnvChildren,
	(DBHostParent, hosts),
	(DBParent, dbs));
    using DBEnv = TelBranch<1, ZvTelemetry::DBEnv_load, ZuNull, DBEnvChildren>;
    using DBEnvPtr = DBEnv *;
    // applications
    ZuDeclTuple(AppChildren,
	(HeapParent, heaps),
	(HashTblParent, hashTbls),
	(ThreadParent, threads),
	(MxParent, multiplexers),
	(EngineParent, engines),
	(DBEnv, dbEnv),
	(AppLink, link));
    struct AppBase { Link *link = nullptr; };
    using App = TelBranch<0, ZvTelemetry::App_load, AppBase, AppChildren>;
    using AppTree = TelTree<App>;
    using AppPtr = typename AppTree:Node *;

    using Root = TelParent<0, ZuNull, AppTree, AppPtr, TelRoot>;

    enum { MaxDepth = 3 };

    Root	m_root;	// root of tree

    // (*) - virtual parent node - not draggable
    using Iter = ZuUnion<
      AppPtr,		// [app]

      // app children
      HeapParentPtr,	// app->heaps (*)
      HashTblParentPtr,	// app->hashTbls (*)
      ThreadParentPtr,	// app->threads (*)
      MxParentPtr,	// app->mxTbl (*)
      EngineParentPtr,	// app->engines (*)
      DBEnvPtr,		// app->dbEnv (*) !!! must be EngineParentPtr + 1 !!!

      // app grandchildren
      HeapPtr,		// app->heaps->[heap]
      HashTblPtr,	// app->hashTbls->[hashTbl]
      ThreadPtr,	// app->threads->[thread]
      MxPtr,		// app->mxTbl->[mx]
      EnginePtr,	// app->engines->[engine]

      // app great-grandchildren
      SocketPtr,	// app->mxTbl->mx->[socket]
      LinkPtr,		// app->engines->engine->[link]

      // DBEnv children
      DBHostParentPtr,	// app->dbEnv->hosts (*)
      DBParentPtr,	// app->dbEnv->dbs (*)

      // DBEnv grandchildren
      DBHostPtr,	// app->dbEnv->hosts->[host]
      DBPtr>;		// app->dbEnv->dbs->[db]

    ZuAssert(sizeof(Iter) < sizeof(GtkTreeIter));

    // child->parent relationships
    template <typename T>
    static typename ZuSame<T, AppPtr, Root *>::T parent(T *ptr) {
      return ptr->parent<Root>();
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, HeapParentPtr>::Same ||
	ZuConversion<T, HashTblParentPtr>::Same ||
	ZuConversion<T, ThreadParentPtr>::Same ||
	ZuConversion<T, MxParentPtr>::Same ||
	ZuConversion<T, EngineParentPtr>::Same ||
	ZuConversion<T, DBEnvPtr>::Same, AppPtr>::T parent(T *ptr) {
      return ptr->parent<AppPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, HeapPtr, HeapParentPtr>::T parent(T *ptr) {
      return ptr->parent<HeapParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, HashTblPtr, HashTblParentPtr>::T
    parent(T *ptr) {
      return ptr->parent<HashTblParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, ThreadPtr, ThreadParentPtr>::T
    parent(T *ptr) {
      return ptr->parent<ThreadParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, MxPtr, MxParentPtr>::T parent(T *ptr) {
      return ptr->parent<MxParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, EnginePtr, EngineParentPtr>::T
    parent(T *ptr) {
      return ptr->parent<EngineParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, SocketPtr, MxPtr>::T parent(T *ptr) {
      return ptr->parent<MxPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, LinkPtr, EnginePtr>::T parent(T *ptr) {
      return ptr->parent<EnginePtr>();
    }
    template <static typename T>
    static typename ZuIfT<
	ZuConversion<T, DBHostParentPtr>::Same ||
	ZuConversion<T, DBParentPtr>::Same, DBEnvPtr>::T parent(T *ptr) {
      return ptr->parent<DBEnvPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, DBHostPtr, DBHostParentPtr>::T
    parent(T *ptr) {
      return ptr->parent<DBHostParentPtr>();
    }
    template <static typename T>
    static typename ZuSame<T, DBPtr, DBParentPtr>::T parent(T *ptr) {
      return ptr->parent<DBParentPtr>();
    }

    GtkTreeModelFlags get_flags() {
      return static_cast<GtkTreeModelFlags>(GTK_TREE_MODEL_ITERS_PERSIST);
    }
    gint get_n_columns() { return 1; }
    GType get_column_type(gint i) { return G_TYPE_STRING; }
    gboolean get_iter(GtkTreeIter *iter_, GtkTreePath *path) {
      gint depth = gtk_tree_path_get_depth(path);
      gint *indices = gtk_tree_path_get_indices(path);
      m_root.descend(indices, depth, [iter_](auto ptr) {
	*Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter_) = ptr;
      });
    }
    GtkTreePath *get_path(GtkTreeIter *iter_) {
      auto iter = reinterpret_cast<Iter *>(iter_);
      GtkTreePath *path = gtk_tree_path_new();
      gint depth;
      gint indices[MaxDepth + 1];
      iter->cdispatch([&depth, indices](auto ptr) {
	depth = ptr->depth();
	ptr->ascend(indices);
      });
      return gtk_tree_path_new_from_indicesv(indices, depth + 1);
    }
    void get_value(GtkTreeIter *iter_, gint i, ZGtk::Value *value) {
      auto iter = reinterpret_cast<Iter *>(iter_);
      static ZtString s; // ok - this is single-threaded
      s.length(0);
      value->init(G_TYPE_STRING);
      iter->cdispatch([&s](const auto ptr) { s << ptr->data.key(); });
      value->set_static_string(s);
    }
    gboolean iter_next(GtkTreeIter *iter_) {
      auto iter = reinterpret_cast<Iter *>(iter_);
      return iter->cdispatch([iter_](auto ptr) {
	unsigned i = ptr->row() + 1;
	return parent(ptr)->child(i, [iter_](auto ptr) {
	  *Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter_) = ptr;
	});
      });
    }
    gboolean iter_children(GtkTreeIter *iter_, GtkTreeIter *parent_) {
      if (!parent_)
	return m_root.child(0, [iter_](auto ptr)
	  *Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter_) = ptr;
      auto parent = reinterpret_cast<Iter *>(parent_);
      return parent->cdispatch([iter_](auto ptr) {
	return ptr->child(0, [iter_](auto ptr) {
	  *Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter_) = ptr;
	});
      });
    }
    gboolean iter_has_child(GtkTreeIter *iter_) {
      if (!iter_) return true;
      auto iter = reinterpret_cast<Iter *>(iter_);
      return iter->cdispatch([iter](auto ptr) { return ptr->hasChild(); });
    }
    gint iter_n_children(GtkTreeIter *iter_) {
      if (!iter_) return m_root.nChildren();
      auto iter = reinterpret_cast<Iter *>(iter_);
      return iter->cdispatch([iter](auto ptr) { return ptr->nChildren(); });
    }
    gboolean iter_nth_child(GtkTreeIter *iter_, GtkTreeIter *parent_, gint i) {
      if (!parent_)
	return m_root.child(i, [iter_](auto ptr)
	  *Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter) = ptr;
      auto parent = reinterpret_cast<Iter *>(parent_);
      return parent->cdispatch([iter_, i](auto ptr) {
	return ptr->child(i, [iter_](auto ptr) {
	  *Iter::new_<typename ZuDecay<decltype(ptr)>::T>(iter_) = ptr;
	});
      });
    }
    gboolean iter_parent(GtkTreeIter *iter_, GtkTreeIter *child_) {
      if (!child_) return false;
      auto child = reinterpret_cast<Iter *>(child_);
      return child->cdispatch([iter_](auto ptr) {
	auto parent = Tree::parent(ptr);
	if (!parent) return false;
	*Iter::new_<typename ZuDecay<decltype(parent)>::T>(iter_) = parent;
	return true;
      });
    }
    void ref_node(GtkTreeIter *) { }
    void unref_node(GtkTreeIter *) { }

    // FIXME from here - need (somewhat) generic add/del
    void add(const Iter &iter) {
      gint col;
      GtkSortType order;
      gint row;
      if (this->get_sort_column_id(&col, &order)) {
	row = ZuSearchPos(ZuInterSearch<false>(
	      &m_rows[0], m_rows.length(), iter,
    [](Iter i1, Iter i2) {
      return i1->data.key().cmp(i2->data.key());
    });
	impl()->row(iter, row);
	m_rows.splice(row, 0, iter);
      } else {
	row = m_rows.length();
	impl()->row(iter, row);
	m_rows.push(iter);
      }
      GtkTreeIter iter_;
      new (&iter_) Iter{iter};
      auto path = gtk_tree_path_new_from_indicesv(&row, 1);
      // emit #GtkTreeModel::row-inserted
      gtk_tree_model_row_inserted(GTK_TREE_MODEL(this), path, &iter_);
      gtk_tree_path_free(path);
    }

    void del(const Iter &iter) {
      gint row = impl()->row(iter);
      auto path = gtk_tree_path_new_from_indicesv(&row, 1);
      // emit #GtkTreeModel::row-deleted - invalidates iterators
      gtk_tree_model_row_deleted(GTK_TREE_MODEL(this), path);
      gtk_tree_path_free(path);
      m_rows.splice(row, 1);
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
