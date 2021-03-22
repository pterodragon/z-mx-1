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

#include <libintl.h>

#include <zlib/ZuArrayN.hpp>
#include <zlib/ZuPolymorph.hpp>
#include <zlib/ZuByteSwap.hpp>
#include <zlib/ZuVersion.hpp>

#include <zlib/ZmPlatform.hpp>
#include <zlib/ZmTrap.hpp>
#include <zlib/ZmSemaphore.hpp>
#include <zlib/ZmScheduler.hpp>

#include <zlib/ZeLog.hpp>

#include <zlib/ZiMultiplex.hpp>
#include <zlib/ZiModule.hpp>
#include <zlib/ZiRing.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvCSV.hpp>
#include <zlib/ZvRingCf.hpp>
#include <zlib/ZvMultiplex.hpp>
#include <zlib/ZvUserDB.hpp>
#include <zlib/ZvCmdClient.hpp>
#include <zlib/ZvCmdServer.hpp>

#include <zlib/Zdf.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkCallback.hpp>
#include <zlib/ZGtkTreeModel.hpp>
#include <zlib/ZGtkValue.hpp>

// FIXME - css
//
// @define-color rag_red_bg #ff1515;

// FIXME
// - right-click row selection in tree -> watch
// - drag/drop rowset in watchlist -> graph
// - field select in graph
//   - defaults to no field selected
//   - until field selected, no trace

static void usage()
{
  static const char *usage =
    "usage: zdash\n";
  std::cerr << usage << std::flush;
  ZeLog::stop();
  ZmPlatform::exit(1);
}

namespace Telemetry {
  struct Watch {
    void	*ptr_ = nullptr;

    template <typename T> ZuInline const T *ptr() const {
      return static_cast<const T *>(ptr_);
    }
    template <typename T> ZuInline T *ptr() {
      return static_cast<T *>(ptr_);
    }
  };
  struct Watch_Accessor : public ZuAccessor<Watch, void *> {
    ZuInline static auto value(const Watch &v) { return v.ptr_; }
  };
  struct Watch_HeapID {
    static constexpr const char *id() { return "zdash.Telemetry.Watch"; }
  };
  template <typename T>
  using WatchList =
    ZmList<T,
      ZmListIndex<Watch_Accessor,
	ZmListNodeIsItem<true,
	  ZmListHeapID<Watch_HeapID,
	    ZmListLock<ZmNoLock> > > > >;

  // display - contains pointer to tree array
  struct Display_ : public Watch {
    unsigned		row = 0;
  };
  using DispList = WatchList<Display_>;
  using Display = DispList::Node;

  // graph - contains pointer to graph - graph contains field selection
  struct Graph_ : public Watch { };
  using GraphList = WatchList<Graph_>;
  using Graph = GraphList::Node;

  using TypeList = ZvTelemetry::TypeList;

  template <typename T_> struct FBSType { using T = typename T_::FBS; };
  using FBSTypeList = ZuTypeMap<FBSType, TypeList>::T;

  template <typename Data> struct Item__ {
    using TelKey = typename Data::Key;
    ZuInline static TelKey telKey(const Data &data) { return data.key(); }
    ZuInline static int rag(const Data &data) { return data.rag(); }
  };
  template <> struct Item__<ZvTelemetry::App> {
    using TelKey = ZuTuple<const ZtString &>;
    void initTelKey(const ZtString &server, uint16_t port) {
      telKey_ << server << ':' << port;
    }
    TelKey telKey(const ZvTelemetry::App &) const {
      return TelKey{telKey_};
    }
    ZuInline static int rag(const ZvTelemetry::App &data) {
      return data.rag();
    }
    ZtString	telKey_;
  };
  template <> struct Item__<ZvTelemetry::DBEnv> {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey(const ZvTelemetry::DBEnv &) {
      return TelKey{"dbenv"};
    }
    ZuInline static int rag(const ZvTelemetry::DBEnv &) {
      return ZvTelemetry::RAG::Off;
    }
  };
  template <typename Data_> class Item_ : public Item__<Data_> {
  public:
    using Data = Data_;
    using Load = ZvTelemetry::load<Data>;
    using TelKey = typename Item__<Data_>::TelKey;

    ZuInline Item_(void *link__) : link_{link__} { }
    template <typename FBS>
    ZuInline Item_(void *link__, FBS *fbs) : link_{link__}, data{fbs} { }

  private:
    Item_(const Item_ &) = delete;
    Item_ &operator =(const Item_ &) = delete;
    Item_(Item_ &&) = delete;
    Item_ &operator =(Item_ &&) = delete;
  public:
    ~Item_() {
      if (dataFrame) {
	dfWriter.final();
	dataFrame->close();
	dataFrame = nullptr;
      }
    }

    template <typename Link>
    ZuInline Link *link() const { return static_cast<Link *>(link_); }

    template <typename T>
    ZuInline T *gtkRow() const { return static_cast<T *>(gtkRow_); }
    template <typename T>
    ZuInline void gtkRow(T *node) { gtkRow_ = node; }

    ZuInline TelKey telKey() const { return Item__<Data_>::telKey(data); }
    ZuInline int rag() const { return Item__<Data_>::rag(data); }

    bool record(ZuString name, Zdf::Mgr *mgr, ZeError *e = nullptr) {
      dataFrame = new Zdf::DataFrame{Data::fields(), name, true};
      dataFrame->init(mgr);
      if (!dataFrame->open(e)) return false;
      dfWriter = dataFrame->writer();
      return true;
    }

    void			*link_;
    Load			data;

    void			*gtkRow_ = nullptr;
    DispList			dispList;
    GraphList			graphList;

    ZuPtr<Zdf::DataFrame>	dataFrame;
    Zdf::DataFrame::Writer	dfWriter;
  };

  struct ItemTree_HeapID {
    static constexpr const char *id() { return "zdash.Telemetry.Tree"; }
  };
  template <typename T>
  struct KeyAccessor : public ZuAccessor<T, typename T::TelKey> {
    ZuInline static auto value(const T &v) { return v.telKey(); }
  };
  template <typename T>
  using ItemTree_ =
    ZmRBTree<Item_<T>,
      ZmRBTreeIndex<KeyAccessor<Item_<T>>,
	ZmRBTreeObject<ZuNull,
	  ZmRBTreeNodeIsKey<true,
	    ZmRBTreeUnique<true,
	      ZmRBTreeLock<ZmNoLock,
		ZmRBTreeHeapID<ItemTree_HeapID> > > > > > >;
  template <typename T>
  class ItemTree : public ItemTree_<T> {
  public:
    using Node = typename ItemTree_<T>::Node;
    template <typename FBS>
    Node *lookup(const FBS *fbs) const {
      return this->find(T::key(fbs));
    }
  };
  template <typename T> class ItemSingleton {
    ItemSingleton(const ItemSingleton &) = delete;
    ItemSingleton &operator =(const ItemSingleton &) = delete;
    ItemSingleton(ItemSingleton &&) = delete;
    ItemSingleton &operator =(ItemSingleton &&) = delete;
  public:
    using Node = Item_<T>;
    ItemSingleton() = default;
    ~ItemSingleton() { if (m_node) delete m_node; }
    template <typename FBS>
    Node *lookup(const FBS *) const { return m_node; }
    void add(Node *node) { if (m_node) delete m_node; m_node = node; }
  private:
    Node	*m_node = nullptr;
  };
  class AlertArray {
    AlertArray(const AlertArray &) = delete;
    AlertArray &operator =(const AlertArray &) = delete;
    AlertArray(AlertArray &&) = delete;
    AlertArray &operator =(AlertArray &&) = delete;
  public:
    using T = ZvTelemetry::Alert;
    using Node = T;
    AlertArray() = default;
    ~AlertArray() = default;
    ZtArray<T>	data;
  };

  template <typename T_> struct Container { // default
    using T = ItemTree<T_>;
  };
  template <> struct Container<ZvTelemetry::App> {
    using T = ItemSingleton<ZvTelemetry::App>;
  };
  template <> struct Container<ZvTelemetry::DBEnv> {
    using T = ItemSingleton<ZvTelemetry::DBEnv>;
  };
  template <> struct Container<ZvTelemetry::Alert> {
    using T = AlertArray;
  };

  using ContainerList = ZuTypeMap<Container, TypeList>::T;
  using Containers = ZuTypeApply<ZuTuple, ContainerList>::T;

  template <typename T> using Item = typename Container<T>::T::Node;
}

namespace GtkTree {
  template <typename Impl, typename Item>
  class Row {
    Impl *impl() { return static_cast<Impl *>(this); }
    const Impl *impl() const { return static_cast<const Impl *>(this); }

  public:
    Item	*item = nullptr;

    Row() = default;
    Row(Item *item_) : item{item_} { init_(); }
    void init(Item *item_) { item = item_; init_(); }
    void init_() { item->gtkRow(impl()); }

    using TelKey = typename Item::TelKey;
    ZuInline TelKey telKey() const { return item->telKey(); }
    ZuInline int rag() const { return item->rag(); }
    ZuInline int cmp(const Impl &v) const {
      return item->telKey().cmp(v.telKey());
    }
  };

  template <unsigned Depth, typename Item>
  struct Leaf :
      public Row<Leaf<Depth, Item>, Item>,
      public ZGtk::TreeHierarchy::Leaf<Leaf<Depth, Item>, Depth> {
    Leaf() = default;
    Leaf(Item *item) : Row<Leaf, Item>{item} { }
    using Row<Leaf, Item>::cmp;
  };

  template <unsigned Depth, typename Item, typename Child>
  struct Parent :
      public Row<Parent<Depth, Item, Child>, Item>,
      public ZGtk::TreeHierarchy::Parent<
	Parent<Depth, Item, Child>, Depth, Child> {
    Parent() = default;
    Parent(Item *item) : Row<Parent, Item>{item} { }
    using Row<Parent, Item>::cmp;
    using Base = ZGtk::TreeHierarchy::Parent<Parent, Depth, Child>;
    using Base::add;
    using Base::del;
  };

  template <unsigned Depth, typename Item, typename Tuple>
  struct Branch :
      public Row<Branch<Depth, Item, Tuple>, Item>,
      public ZGtk::TreeHierarchy::Branch<
	Branch<Depth, Item, Tuple>, Depth, Tuple> {
    Branch() = default; 
    Branch(Item *item) : Row<Branch, Item>{item} { }
    using Row<Branch, Item>::cmp;
    using Base = ZGtk::TreeHierarchy::Branch<Branch, Depth, Tuple>;
    using Base::add;
    using Base::del;
  };
  template <typename TelKey_>
  struct BranchChild {
    using TelKey = TelKey_;
    static int rag() { return ZvTelemetry::RAG::Off; }
  };

  template <typename T> using TelItem = Telemetry::Item<T>;

  using Heap = Leaf<3, TelItem<ZvTelemetry::Heap>>;
  using HashTbl = Leaf<3, TelItem<ZvTelemetry::HashTbl>>;
  using Thread = Leaf<3, TelItem<ZvTelemetry::Thread>>;
  using Socket = Leaf<4, TelItem<ZvTelemetry::Socket>>;
  using Mx = Parent<3, TelItem<ZvTelemetry::Mx>, Socket>;
  using Queue = Leaf<3, TelItem<ZvTelemetry::Queue>>;
  using Link = Leaf<4, TelItem<ZvTelemetry::Link>>;
  using Engine = Parent<3, TelItem<ZvTelemetry::Engine>, Link>;
  using DBHost = Leaf<4, TelItem<ZvTelemetry::DBHost>>;
  using DB = Leaf<4, TelItem<ZvTelemetry::DB>>;

  // DB hosts
  struct DBHosts : public BranchChild<ZuTuple<const char *>> {
    static auto telKey() { return TelKey{"hosts"}; }
  };
  using DBHostParent = Parent<3, DBHosts, DBHost>;
  // DBs
  struct DBs : public BranchChild<ZuTuple<const char *>> {
    static auto telKey() { return TelKey{"dbs"}; }
  };
  using DBParent = Parent<3, DBs, DB>;

  // heaps
  struct Heaps :
      public BranchChild<ZuTuple<const char *, const char *, const char *>> {
    static auto telKey() { return TelKey{"heaps", "partition", "size"}; }
  };
  using HeapParent = Parent<2, Heaps, Heap>;
  // hashTbls
  struct HashTbls : public BranchChild<ZuTuple<const char *, const char *>> {
    static auto telKey() { return TelKey{"hashTbls", "addr"}; }
  };
  using HashTblParent = Parent<2, HashTbls, HashTbl>;
  // threads
  struct Threads : public BranchChild<ZuTuple<const char *>> {
    static auto telKey() { return TelKey{"threads"}; }
  };
  using ThreadParent = Parent<2, Threads, Thread>;
  // multiplexers
  struct Mxs : public BranchChild<ZuTuple<const char *>> {
    static auto telKey() { return TelKey{"multiplexers"}; }
  };
  using MxParent = Parent<2, Mxs, Mx>;
  // queues
  struct Queues : public BranchChild<ZuTuple<const char *, const char *>> {
    static auto telKey() { return TelKey{"queues", "type"}; }
  };
  using QueueParent = Parent<2, Queues, Queue>;
  // engines
  struct Engines : public BranchChild<ZuTuple<const char *>> {
    static auto telKey() { return TelKey{"engines"}; }
  };
  using EngineParent = Parent<2, Engines, Engine>;

  // dbEnv
  ZuDeclTuple(DBEnvTuple,
      (DBHostParent, hosts),
      (DBParent, dbs));
  using DBEnv = Branch<2, TelItem<ZvTelemetry::DBEnv>, DBEnvTuple>;
  // applications
  ZuDeclTuple(AppTuple,
      (HeapParent, heaps),
      (HashTblParent, hashTbls),
      (ThreadParent, threads),
      (MxParent, mxs),
      (QueueParent, queues),
      (EngineParent, engines),
      (DBEnv, dbEnv));
  using App = Branch<1, TelItem<ZvTelemetry::App>, AppTuple>;

  struct Root : public ZGtk::TreeHierarchy::Parent<Root, 0, App> { };

  // map telemetry items to corresponding tree nodes
  inline Heap *row(TelItem<ZvTelemetry::Heap> *item) {
    return item->template gtkRow<Heap>();
  }
  inline HashTbl *row(TelItem<ZvTelemetry::HashTbl> *item) {
    return item->template gtkRow<HashTbl>();
  }
  inline Thread *row(TelItem<ZvTelemetry::Thread> *item) {
    return item->template gtkRow<Thread>();
  }
  inline Mx *row(TelItem<ZvTelemetry::Mx> *item) {
    return item->template gtkRow<Mx>();
  }
  inline Socket *row(TelItem<ZvTelemetry::Socket> *item) {
    return item->template gtkRow<Socket>();
  }
  inline Queue *row(TelItem<ZvTelemetry::Queue> *item) {
    return item->template gtkRow<Queue>();
  }
  inline Engine *row(TelItem<ZvTelemetry::Engine> *item) {
    return item->template gtkRow<Engine>();
  }
  inline Link *row(TelItem<ZvTelemetry::Link> *item) {
    return item->template gtkRow<Link>();
  }
  inline DB *row(TelItem<ZvTelemetry::DB> *item) {
    return item->template gtkRow<DB>();
  }
  inline DBHost *row(TelItem<ZvTelemetry::DBHost> *item) {
    return item->template gtkRow<DBHost>();
  }
  inline DBEnv *row(TelItem<ZvTelemetry::DBEnv> *item) {
    return item->template gtkRow<DBEnv>();
  }
  inline App *row(TelItem<ZvTelemetry::App> *item) {
    return item->template gtkRow<App>();
  }

  enum { Depth = 5 };

  // (*) - branch
  using Iter = ZuUnion<
    // Root *,
    App *,		// [app]

    // app children
    HeapParent *,	// app->heaps (*)
    HashTblParent *,	// app->hashTbls (*)
    ThreadParent *,	// app->threads (*)
    MxParent *,		// app->mxs (*)
    QueueParent *,	// app->queues (*)
    EngineParent *,	// app->engines (*)
    DBEnv *,		// app->dbEnv (*)

    // app grandchildren
    Heap *,		// app->heaps->[heap]
    HashTbl *,		// app->hashTbls->[hashTbl]
    Thread *,		// app->threads->[thread]
    Mx *,		// app->mxs->[mx]
    Queue *,		// app->queues->[queue]
    Engine *,		// app->engines->[engine]

    // app great-grandchildren
    Socket *,		// app->mxs->mx->[socket]
    Link *,		// app->engines->engine->[link]

    // DBEnv children
    DBHostParent *,	// app->dbEnv->hosts (*)
    DBParent *,		// app->dbEnv->dbs (*)

    // DBEnv grandchildren
    DBHost *,		// app->dbEnv->hosts->[host]
    DB *>;		// app->dbEnv->dbs->[db]

  class Model : public ZGtk::TreeHierarchy::Model<Model, Iter, Depth> {
  public:
    enum { RAGCol = 0, IDCol0, IDCol1, IDCol2, NCols };

    // root()
    Root *root() { return &m_root; }

    // parent() - child->parent type map
    template <typename T>
    static typename ZuSame<T, App, Root>::T *parent(void *ptr) {
      return static_cast<Root *>(ptr);
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, HeapParent>::Same ||
	ZuConversion<T, HashTblParent>::Same ||
	ZuConversion<T, ThreadParent>::Same ||
	ZuConversion<T, MxParent>::Same ||
	ZuConversion<T, QueueParent>::Same ||
	ZuConversion<T, EngineParent>::Same ||
	ZuConversion<T, DBEnv>::Same, App>::T *parent(void *ptr) {
      return static_cast<App *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Heap, HeapParent>::T *parent(void *ptr) {
      return static_cast<HeapParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, HashTbl, HashTblParent>::T *parent(void *ptr) {
      return static_cast<HashTblParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Thread, ThreadParent>::T *parent(void *ptr) {
      return static_cast<ThreadParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Mx, MxParent>::T *parent(void *ptr) {
      return static_cast<MxParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Queue, QueueParent>::T *parent(void *ptr) {
      return static_cast<QueueParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Engine, EngineParent>::T *parent(void *ptr) {
      return static_cast<EngineParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Socket, Mx>::T *parent(void *ptr) {
      return static_cast<Mx *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, Link, Engine>::T *parent(void *ptr) {
      return static_cast<Engine *>(ptr);
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, DBHostParent>::Same ||
	ZuConversion<T, DBParent>::Same, DBEnv>::T *parent(void *ptr) {
      return static_cast<DBEnv *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, DBHost, DBHostParent>::T *parent(void *ptr) {
      return static_cast<DBHostParent *>(ptr);
    }
    template <typename T>
    static typename ZuSame<T, DB, DBParent>::T *parent(void *ptr) {
      return static_cast<DBParent *>(ptr);
    }

    // key printing
    template <typename Impl, typename Key>
    class KeyPrint_ {
      Impl *impl() { return static_cast<Impl *>(this); }
      const Impl *impl() const { return static_cast<const Impl *>(this); }
    public:
      Key key;
      template <typename Key_>
      KeyPrint_(Key_ &&key_) : key{ZuFwd<Key_>(key_)} { }
      auto p0() const { return key.template p<0>(); }
      template <unsigned N = Key::N>
      auto p1(typename ZuIfT<(N <= 1)>::T *_ = 0) const { return ""; }
      template <unsigned N = Key::N>
      auto p1(typename ZuIfT<(N > 1)>::T *_ = 0) const {
	return key.template p<1>();
      }
      template <unsigned N = Key::N>
      auto p2(typename ZuIfT<(N <= 2)>::T *_ = 0) const { return ""; }
      template <unsigned N = Key::N>
      auto p2(typename ZuIfT<(N > 2)>::T *_ = 0) const {
	return key.template p<2>();
      }
    };
    // generic key printing
    template <typename Key>
    struct KeyPrint : public KeyPrint_<KeyPrint<Key>, Key> {
      using KeyPrint_<KeyPrint, Key>::KeyPrint_;
    };
    template <typename T, typename Key>
    static typename ZuIfT<
      !ZuConversion<T, HashTbl>::Same &&
      !ZuConversion<T, Queue>::Same,
      KeyPrint<Key> >::T keyPrintType();
    // override addr for hash tables
    template <typename Key>
    struct HashTblKeyPrint : public KeyPrint_<HashTblKeyPrint<Key>, Key> {
      using KeyPrint_<HashTblKeyPrint<Key>, Key>::KeyPrint_;
      auto p1() { return ZuBoxed(this->key.template p<1>()).hex(); }
    };
    template <typename T, typename Key>
    static typename ZuIfT<
      ZuConversion<T, HashTbl>::Same, HashTblKeyPrint<Key> >::T
    keyPrintType();
    // override type for queues
    template <typename Key>
    struct QueueKeyPrint : public KeyPrint_<QueueKeyPrint<Key>, Key> {
      using KeyPrint_<QueueKeyPrint<Key>, Key>::KeyPrint_;
      auto p1() {
	return ZvTelemetry::QueueType::name(this->key.template p<1>());
      }
    };
    template <typename T, typename Key>
    static typename ZuIfT<
      ZuConversion<T, Queue>::Same, QueueKeyPrint<Key> >::T
    keyPrintType();

    gint get_n_columns() { return NCols; }
    GType get_column_type(gint i) {
      switch (i) {
	case RAGCol: return G_TYPE_INT;
	case IDCol0: return G_TYPE_STRING;
	case IDCol1: return G_TYPE_STRING;
	case IDCol2: return G_TYPE_STRING;
	default: return G_TYPE_NONE;
      }
    }
    template <typename T>
    void value(const T *ptr, gint i, ZGtk::Value *v) {
      using Key = decltype(ptr->telKey());
      using KeyPrint = decltype(keyPrintType<T, Key>());
      KeyPrint print{ptr->telKey()};
      switch (i) {
	case RAGCol: 
	  v->init(G_TYPE_INT);
	  v->set_int(ptr->rag());
	  break;
	case IDCol0:
	  m_value.length(0);
	  v->init(G_TYPE_STRING);
	  m_value << print.p0();
	  v->set_static_string(m_value);
	  break;
	case IDCol1:
	  m_value.length(0);
	  v->init(G_TYPE_STRING);
	  m_value << print.p1();
	  v->set_static_string(m_value);
	  break;
	case IDCol2:
	  m_value.length(0);
	  v->init(G_TYPE_STRING);
	  m_value << print.p2();
	  v->set_static_string(m_value);
	  break;
	default:
	  v->init(G_TYPE_NONE);
	  break;
      }
    }

  private:
    Root	m_root;		// root of tree
    ZtString	m_value;	// re-used string buffer
  };

  class View {
    View(const View &) = delete;
    View &operator =(const View &) = delete;
    View(View &&) = delete;
    View &operator =(View &&) = delete;
  public:
    View() = default;
    ~View() = default;

  private:
    template <unsigned RagCol, unsigned TextCol>
    void addCol(const char *id) {
      auto col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title(col, gettext(id));

      auto cell = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, cell, true);

      gtk_tree_view_column_set_cell_data_func(col, cell, [](
	    GtkTreeViewColumn *col, GtkCellRenderer *cell,
	    GtkTreeModel *model, GtkTreeIter *iter, gpointer this_) {
	reinterpret_cast<View *>(this_)->render<RagCol, TextCol>(
	    col, cell, model, iter);
      }, reinterpret_cast<gpointer>(this), nullptr);

      gtk_tree_view_append_column(m_treeView, col);

      // normally would add columns in order of saved column order;
      // and not add columns unselected by user
      //
      // need
      // - array of available columns (including ID)
      // - array of selected columns in display order
    }

    template <unsigned RagCol, unsigned TextCol>
    void render(
	GtkTreeViewColumn *col, GtkCellRenderer *cell,
	GtkTreeModel *model, GtkTreeIter *iter) {
      m_values[0].unset();
      gtk_tree_model_get_value(model, iter, TextCol, &m_values[0]);

      gint rag;
      {
	ZGtk::Value rag_;
	gtk_tree_model_get_value(model, iter, RagCol, &rag_);
	rag = rag_.get_int();
      }
      switch (rag) {
	case ZvTelemetry::RAG::Red:
	  m_values[1].set_static_boxed(&m_rag_red_bg);
	  m_values[2].set_static_boxed(&m_rag_red_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	  break;
	case ZvTelemetry::RAG::Amber:
	  m_values[1].set_static_boxed(&m_rag_amber_bg);
	  m_values[2].set_static_boxed(&m_rag_amber_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	  break;
	case ZvTelemetry::RAG::Green:
	  m_values[1].set_static_boxed(&m_rag_green_bg);
	  m_values[2].set_static_boxed(&m_rag_green_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	  break;
	default:
	  m_values[1].set_static_boxed(&m_rag_off_bg);
	  m_values[2].set_static_boxed(&m_rag_off_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	  break;
      }
    }

  public:
    void init(GtkTreeView *view, GtkStyleContext *context) {
      m_treeView = view;

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

      addCol<Model::RAGCol, Model::IDCol0>("ID");
      addCol<Model::RAGCol, Model::IDCol1>("");
      addCol<Model::RAGCol, Model::IDCol2>("");

      static const gchar *props[] = {
	"text", "background-rgba", "foreground-rgba"
      };

      m_props = props;
      m_values[1].init(GDK_TYPE_RGBA);
      m_values[2].init(GDK_TYPE_RGBA);

      {
	auto cell = gtk_cell_renderer_text_new();
	g_object_getv(G_OBJECT(cell), 2, &m_props[1], &m_values[1]);
	m_rag_off_bg =
	  *reinterpret_cast<const GdkRGBA *>(m_values[1].get_boxed());
	m_rag_off_fg =
	  *reinterpret_cast<const GdkRGBA *>(m_values[2].get_boxed());
	g_object_unref(G_OBJECT(cell));
      }

      g_signal_connect(
	  G_OBJECT(m_treeView), "destroy",
	  ZGtk::callback([](GObject *, gpointer this_) {
	    reinterpret_cast<View *>(this_)->destroyed();
	  }), reinterpret_cast<gpointer>(this));
    }

    void destroyed() {
      m_treeView = nullptr;
    }

    void final() {
      if (m_treeView) g_object_unref(G_OBJECT(m_treeView));
    }

    void bind(GtkTreeModel *model) {
      gtk_tree_view_set_model(m_treeView, model);
    }

  private:
    GtkTreeView	*m_treeView = nullptr;
    GdkRGBA	m_rag_red_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_red_bg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_amber_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_amber_bg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_green_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_green_bg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_off_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_off_bg = { 0.0, 0.0, 0.0, 0.0 };
    const gchar	**m_props;
    ZGtk::Value	m_values[3];
  };
}

class ZDash_Cli;
class ZDash_Srv;

class CliLink_ : public ZvCmdCliLink<ZDash_Cli, CliLink_> {
public:
  using Base = ZvCmdCliLink<ZDash_Cli, CliLink_>;
  using Key = ZuPair<ZuString, uint16_t>;
  Key key() const { return {this->server(), this->port()}; }

  CliLink_(ZDash_Cli *);

  void loggedIn();
  void disconnected();
  void connectFailed(bool transient);

  int processTelemetry(const uint8_t *data, unsigned len);
  int processDeflt(ZuID id, const uint8_t *data, unsigned len);

  ZvSeqNo		seqNo = 0;
  Telemetry::Containers	telemetry;
};
struct CliLink_KeyAccessor : public ZuAccessor<CliLink_ *, CliLink_::Key> {
  static CliLink_::Key value(const CliLink_ *link) { return link->key(); }
};
struct CliLink_HeapID {
  static constexpr const char *id() { return "CliLink"; }
};
using CliLinks =
  ZmRBTree<CliLink_,
    ZmRBTreeNodeIsKey<true,
      ZmRBTreeIndex<CliLink_KeyAccessor,
	ZmRBTreeUnique<true,
	  ZmRBTreeObject<ZmPolymorph,
	    ZmRBTreeLock<ZmPLock,
	      ZmRBTreeHeapID<CliLink_HeapID>>>>>>>;
using CliLink = CliLinks::Node;

class SrvLink : public ZvCmdSrvLink<ZDash_Srv, SrvLink> {
public:
  using Base = ZvCmdSrvLink<ZDash_Srv, SrvLink>;
  SrvLink(ZDash_Srv *app);

  int processApp(const uint8_t *data, unsigned len);
  int processDeflt(ZuID id, const uint8_t *data, unsigned len);

  // FIXME - selected CliLink
};

class ZDash;

class ZDash_Cli : public ZvCmdClient<ZDash_Cli, CliLink_> { };
class ZDash_Srv : public ZvCmdServer<ZDash_Srv, SrvLink> {
public:
  void telemetry(ZvTelemetry::App &data);
};

class ZDash :
    public ZmPolymorph,
    public ZDash_Cli,
    public ZDash_Srv,
    public ZGtk::App {
public:
  using Client = ZvCmdClient<ZDash_Cli, CliLink_>;
  using Server = ZvCmdServer<ZDash_Srv, SrvLink>;
  using User = Server::User;

  class TelRing : public ZiRing {
    TelRing() = delete;
    TelRing(const TelRing &) = delete;
    TelRing &operator =(const TelRing &) = delete;
    TelRing(TelRing &&) = delete;
    TelRing &operator =(TelRing &&) = delete;
  public:
    ~TelRing() = default;
    TelRing(const ZiRingParams &params) :
	ZiRing{[](const void *ptr) -> unsigned {
	  return *static_cast<const uint16_t *>(ptr) + sizeof(uintptr_t) + 2;
	}, params} { }
    bool push(CliLink_ *link, ZuArray<const uint8_t> msg) {
      unsigned n = msg.length();
      if (void *ptr = ZiRing::push(n + sizeof(uintptr_t) + 2)) {
	*static_cast<uintptr_t *>(ptr) = reinterpret_cast<uintptr_t>(link);
	ptr = static_cast<uint8_t *>(ptr) + sizeof(uintptr_t);
	*static_cast<uint16_t *>(ptr) = n;
	ptr = static_cast<uint8_t *>(ptr) + 2;
	memcpy(static_cast<uint8_t *>(ptr), msg.data(), n);
	push2();
	return true;
      }
      auto i = writeStatus();
      if (i < 0)
	ZeLOG(Error, ZtString{} <<
	    "ZiRing::push() failed - " << Zi::resultName(i));
      else
	ZeLOG(Error, ZtString{} <<
	    "ZiRing::push() failed - writeStatus=" << i);
      return false;
    }
    template <typename L>
    bool shift(L l) {
      if (const void *ptr = ZiRing::shift()) {
	CliLink_ *link =
	  reinterpret_cast<CliLink_ *>(*static_cast<const uintptr_t *>(ptr));
	ptr = static_cast<const uint8_t *>(ptr) + sizeof(uintptr_t);
	unsigned n = *static_cast<const uint16_t *>(ptr);
	ptr = static_cast<const uint8_t *>(ptr) + 2;
	l(link, ZuArray<const uint8_t>{static_cast<const uint8_t *>(ptr), n});
	shift2();
	return true;
      }
      return false;
    }
  };

  template <typename T> using TelItem = Telemetry::Item<T>;

  using AppItem = TelItem<ZvTelemetry::App>;
  using DBEnvItem = TelItem<ZvTelemetry::DBEnv>;

  void init(ZiMultiplex *mx, const ZvCf *cf) {
    if (ZmRef<ZvCf> telRingCf = cf->subset("telRing"))
      m_telRingParams.init(telRingCf);
    else
      m_telRingParams.name("zdash").size(131072);
    // 131072 is ~100mics at 1Gbit/s
    m_telRing = new TelRing{m_telRingParams};
    {
      ZeError e;
      if (m_telRing->open(
	    ZiRing::Read | ZiRing::Write | ZiRing::Create, &e) != Zi::OK)
	throw ZtString{} << m_telRingParams.name() << ": " << e;
      int r;
      if ((r = m_telRing->reset()) != Zi::OK)
	throw ZtString{} << m_telRingParams.name() <<
	  ": reset failed - " << Zi::resultName(r);
    }

    m_role = cf->getEnum<ZvTelemetry::AppRole::Map>(
	"appRole", false, ZvTelemetry::AppRole::Dev);

    m_gladePath = cf->get("gtkGlade", true);
    m_stylePath = cf->get("gtkStyle");

    {
      int64_t refreshRate =
	cf->getInt64("gtkRefresh", 1, 60000, false, 1) * (int64_t)1000000;
      m_refreshQuantum = ZmTime{ZmTime::Nano, refreshRate>>1};
      if (m_refreshQuantum < mx->params().quantum()) {
	m_refreshQuantum = mx->params().quantum();
	m_refreshRate = m_refreshQuantum + m_refreshQuantum;
      } else
	m_refreshRate = ZmTime{ZmTime::Nano, refreshRate};
    }
    unsigned gtkTID;
    {
      unsigned nThreads = mx->params().nThreads();
      m_tid = cf->getInt("thread", 1, nThreads, true);
      gtkTID = cf->getInt("gtkThread", 1, nThreads, true);
    }

    Server::init(mx, cf);
    static_cast<Server *>(this)->Dispatcher::map("zdash",
	[](void *link, const uint8_t *data, unsigned len) {
	  return static_cast<SrvLink *>(link)->processApp(data, len);
	});
    static_cast<Server *>(this)->Dispatcher::deflt(
	[](void *link, ZuID id, const uint8_t *data, unsigned len) {
	  return static_cast<SrvLink *>(link)->processDeflt(id, data, len);
	});

    Client::init(mx, cf);
    static_cast<Client *>(this)->Dispatcher::deflt(
	[](void *link, ZuID id, const uint8_t *data, unsigned len) {
	  return static_cast<CliLink_ *>(link)->processDeflt(id, data, len);
	});

    ZmTrap::sigintFn(ZmFn<>{this, [](ZDash *this_) { this_->post(); }});
    ZmTrap::trap();

    m_uptime.now();

    i18n(
	cf->get("i18n_domain", false, "zdash"),
	cf->get("dataDir", false, DATADIR));

    attach(mx, gtkTID);
    mx->run(gtkTID, [this]() { gtkInit(); });
  }

  void final() {
    detach(ZmFn<>{this, [](ZDash *this_) {
      this_->gtkFinal();
      this_->m_executed.post();
    }});
    m_executed.wait();

    m_telRing->close();

    Client::final();
    Server::final();
  }

  void gtkInit() {
    gtk_init(nullptr, nullptr);

    auto builder = gtk_builder_new();
    GError *e = nullptr;

    if (!gtk_builder_add_from_file(builder, m_gladePath, &e)) {
      if (e) {
	ZeLOG(Error, e->message);
	g_error_free(e);
      }
      post();
      return;
    }

    m_mainWindow = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
    auto view_ = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
    // m_watchlist = GTK_TREE_VIEW(gtk_builder_get_object(builder, "watchlist"));
    g_object_unref(G_OBJECT(builder));

    if (m_stylePath) {
      auto file = g_file_new_for_path(m_stylePath);
      auto provider = gtk_css_provider_new();
      g_signal_connect(G_OBJECT(provider), "parsing-error",
	  ZGtk::callback([](
	      GtkCssProvider *, GtkCssSection *,
	      GError *e, gpointer) { ZeLOG(Error, e->message); }), 0);
      gtk_css_provider_load_from_file(provider, file, nullptr);
      g_object_unref(G_OBJECT(file));
      m_styleContext = gtk_style_context_new();
      gtk_style_context_add_provider(
	  m_styleContext, GTK_STYLE_PROVIDER(provider), G_MAXUINT);
      g_object_unref(G_OBJECT(provider));
    }

    m_gtkModel = GtkTree::Model::ctor();
    m_gtkView.init(view_, m_styleContext);
    m_gtkView.bind(GTK_TREE_MODEL(m_gtkModel));

    m_mainDestroy = g_signal_connect(
	G_OBJECT(m_mainWindow), "destroy",
	ZGtk::callback([](GObject *, gpointer this_) {
	  reinterpret_cast<ZDash *>(this_)->gtkDestroyed();
	}), reinterpret_cast<gpointer>(this));

    gtk_widget_show_all(GTK_WIDGET(m_mainWindow));

    gtk_window_present(m_mainWindow);

    m_telRing->attach();
  }

  void gtkDestroyed() {
    m_mainWindow = nullptr;
    post();
  }

  void gtkFinal() {
    m_telRing->detach();

    ZGtk::App::sched()->del(&m_refreshTimer);

    if (m_mainWindow) {
      if (m_mainDestroy)
	g_signal_handler_disconnect(G_OBJECT(m_mainWindow), m_mainDestroy);
      gtk_window_close(m_mainWindow);
      gtk_widget_destroy(GTK_WIDGET(m_mainWindow));
      m_mainWindow = nullptr;
    }
    m_gtkView.final();
    if (m_gtkModel) g_object_unref(G_OBJECT(m_gtkModel));
    if (m_styleContext) g_object_unref(G_OBJECT(m_styleContext));
  }

  void post() { m_done.post(); }
  void wait() { m_done.wait(); }

  void telemetry(ZvTelemetry::App &data) {
    using namespace ZvTelemetry;
    data.id = "ZDash";
    data.version = ZuVerName();
    data.uptime = m_uptime;
    data.role = m_role;
    data.rag(RAG::Green);
  }

  template <typename ...Args>
  ZuInline void gtkRun(Args &&... args) {
    ZGtk::App::run(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  ZuInline void gtkInvoke(Args &&... args) {
    ZGtk::App::invoke(ZuFwd<Args>(args)...);
  }

  void loggedIn(CliLink_ *link) {
    // FIXME
  }

  void disconnected(CliLink_ *link) {
    // FIXME
    m_telRing->push(link, ZuArray<const uint8_t>{});
  }
  void disconnected2(CliLink_ *link) {
    // FIXME - update App RAG to red (in caller)
    // FIXME - delete link ?
    m_executed.post();
  }

  void connectFailed(CliLink_ *, bool transient) {
    // FIXME - delete link ?
    // FIXME
  }

  int processTelemetry(CliLink_ *link, const uint8_t *data, unsigned len) {
    using namespace ZvTelemetry;
    {
      flatbuffers::Verifier verifier(data, len);
      if (!fbs::VerifyTelemetryBuffer(verifier)) return -1;
    }
    if (m_telRing->push(link, {data, len}))
      if (!m_telCount++)
	gtkRun(ZmFn<>{this, [](ZDash *this_) { this_->gtkRefresh(); }},
	    ZmTimeNow() + m_refreshRate, ZmScheduler::Advance, &m_refreshTimer);
    return len;
  }

  int processApp(SrvLink *link, const uint8_t *data, unsigned len) {
    // FIXME - process version, connect, disconnect, links, select, etc.
    // - connect auto-selects
    // - disconnect de-selects
    return 0;
  }

  // fwd unknown app messages client <-> server using client-selected
  // server-side link (from zdash perspective, SrvLink selects CliLink)
  int processDeflt(CliLink_ *link, ZuID id, const uint8_t *data, unsigned len) {
    // FIXME - fwd to server links selecting this client-side link
    return 0;
  }
  int processDeflt(SrvLink *link, ZuID id, const uint8_t *data, unsigned len) {
    // FIXME - fwd to selected client link
    return 0;
  }

private:
  void gtkRefresh() {
    // FIXME - freeze, save sort col, unset sort col

    bool busy = false;
    ZmTime deadline = ZmTimeNow() + m_refreshQuantum;
    unsigned i = 0, n;
    while (m_telRing->shift([](
	    CliLink_ *link, const ZuArray<const uint8_t> &msg) {
      static_cast<ZDash *>(link->app())->processTel2(link, msg);
    })) {
      n = --m_telCount;
      if (ZuUnlikely(n == ~0U)) n = m_telCount = 0;
      if (!(++i & 0xf) && ZmTimeNow() >= deadline) {
	busy = true;
	break;
      }
    }

    // FIXME - restore sort col, thaw

    if (busy && n)
      gtkRun(ZmFn<>{this, [](ZDash *this_) { this_->gtkRefresh(); }},
	  ZmTimeNow() + m_refreshRate, ZmScheduler::Defer, &m_refreshTimer);
  }
  void processTel2(CliLink_ *link, const ZuArray<const uint8_t> &msg_) {
    if (ZuUnlikely(!msg_)) {
      disconnected2(link);
      return;
    }
    using namespace ZvTelemetry;
    auto msg = fbs::GetTelemetry(msg_);
    int i = msg->data_type();
    if (ZuUnlikely(i < TelData::First)) return;
    if (ZuUnlikely(i > TelData::MAX)) return;
    ZuSwitch::dispatch<TelData::N - TelData::First>(i - TelData::First,
	[this, link, msg](auto i) {
      using FBS = TelData::Type<i + TelData::First>;
      auto fbs = static_cast<const FBS *>(msg->data());
      processTel3(link, fbs);
    });
  }
  template <typename FBS>
  typename ZuIsNot<ZvTelemetry::fbs::Alert, FBS>::T
  processTel3(CliLink_ *link, const FBS *fbs) {
    ZuConstant<ZuTypeIndex<FBS, Telemetry::FBSTypeList>::I> i;
    using T = typename ZuType<i, Telemetry::TypeList>::T;
    auto &container = link->telemetry.p<i>();
    using Item = TelItem<T>;
    Item *item;
    if (item = container.lookup(fbs)) {
      item->data.loadDelta(fbs);
      m_gtkModel->updated(GtkTree::row(item));
    } else {
      item = new Item{link, fbs};
      container.add(item);
      addGtkRow(link, item);
    }
  }
  void addGtkRow(CliLink_ *link, AppItem *item) {
    item->initTelKey(link->server(), link->port());
    m_gtkModel->add(new GtkTree::App{item}, m_gtkModel->root());
  }
  AppItem *appItem(CliLink_ *link) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::App, ZvTelemetry::TypeList>::I> i;
    auto &container = link->telemetry.p<i>();
    auto item = container.lookup(
	static_cast<const ZvTelemetry::fbs::App *>(nullptr));
    if (!item) {
      item = new TelItem<ZvTelemetry::App>{link};
      container.add(item);
      addGtkRow(link, item);
    }
    return item;
  }
  DBEnvItem *dbEnvItem(CliLink_ *link) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::DBEnv, ZvTelemetry::TypeList>::I> i;
    auto &container = link->telemetry.p<i>();
    auto item = container.lookup(
	static_cast<const ZvTelemetry::fbs::DBEnv *>(nullptr));
    if (!item) {
      item = new TelItem<ZvTelemetry::DBEnv>{link};
      container.add(item);
      addGtkRow(link, item);
    }
    return item;
  }
  template <typename Item, typename ParentFn>
  void addGtkRow_(AppItem *appItem, Item *item, ParentFn parentFn) {
    auto appGtkRow = GtkTree::row(appItem);
    auto &parent = parentFn(appGtkRow);
    if (parent.row() < 0) m_gtkModel->add(&parent, appGtkRow);
    using GtkRow = typename ZuDecay<decltype(*GtkTree::row(item))>::T;
    m_gtkModel->add(new GtkRow{item}, &parent);
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Heap> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::HeapParent & {
	  return _->heaps();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::HashTbl> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::HashTblParent & {
	  return _->hashTbls();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Thread> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::ThreadParent & {
	  return _->threads();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Mx> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::MxParent & { return _->mxs(); });
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Socket> *item) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::Mx, ZvTelemetry::TypeList>::I> i;
    auto &mxContainer = link->telemetry.p<i>();
    auto mxItem = mxContainer.find(ZvTelemetry::Mx::Key{item->data.mxID});
    if (!mxItem) {
      auto mxItem = new TelItem<ZvTelemetry::Mx>{link};
      mxItem->data.id = item->data.mxID;
      mxContainer.add(mxItem);
      addGtkRow(link, mxItem);
    }
    m_gtkModel->add(new GtkTree::Socket{item}, GtkTree::row(mxItem));
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Queue> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::QueueParent & {
	  return _->queues();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Engine> *item) {
    addGtkRow_(appItem(link), item,
	[](GtkTree::App *_) -> GtkTree::EngineParent & {
	  return _->engines();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::Link> *item) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::Engine, ZvTelemetry::TypeList>::I> i;
    auto &engContainer = link->telemetry.p<i>();
    auto engItem =
      engContainer.find(ZvTelemetry::Engine::Key{item->data.engineID});
    if (!engItem) {
      auto engItem = new TelItem<ZvTelemetry::Mx>{link};
      engItem->data.id = item->data.engineID;
      engContainer.add(engItem);
      addGtkRow(link, engItem);
    }
    m_gtkModel->add(new GtkTree::Link{item}, GtkTree::row(engItem));
  }
  void addGtkRow(CliLink_ *link, DBEnvItem *item) {
    auto appGtkRow = GtkTree::row(appItem(link));
    auto &dbEnv = appGtkRow->dbEnv();
    dbEnv.init(item);
    m_gtkModel->add(&dbEnv, appGtkRow);
  }
  template <typename Item, typename ParentFn>
  void addGtkRow_(DBEnvItem *dbEnvItem, Item *item, ParentFn parentFn) {
    auto dbEnvGtkRow = GtkTree::row(dbEnvItem);
    auto &parent = parentFn(dbEnvGtkRow);
    if (parent.row() < 0) m_gtkModel->add(&parent, dbEnvGtkRow);
    using GtkRow = typename ZuDecay<decltype(*GtkTree::row(item))>::T;
    m_gtkModel->add(new GtkRow{item}, &parent);
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::DBHost> *item) {
    addGtkRow_(dbEnvItem(link), item,
	[](GtkTree::DBEnv *_) -> GtkTree::DBHostParent & {
	  return _->hosts();
	});
  }
  void addGtkRow(CliLink_ *link, TelItem<ZvTelemetry::DB> *item) {
    addGtkRow_(dbEnvItem(link), item,
	[](GtkTree::DBEnv *_) -> GtkTree::DBParent & {
	  return _->dbs();
	});
  }
  template <typename FBS>
  typename ZuIs<ZvTelemetry::fbs::Alert, FBS, bool>::T
  processTel3(CliLink_ *link, const FBS *fbs) {
    ZuConstant<ZuTypeIndex<FBS, Telemetry::FBSTypeList>::I> i;
    using T = typename ZuType<i, Telemetry::TypeList>::T;
    auto &container = link->telemetry.p<i>();
    alert(new (container.data.push()) ZvTelemetry::load<T>{fbs});
    return true;
  }

  void alert(const ZvTelemetry::Alert *) {
    // FIXME - update alerts in UX
  }

private:
  ZmSemaphore		m_done;
  ZmSemaphore		m_executed;

  CliLinks		m_links;

  Zfb::IOBuilder	m_fbb;

  int			m_role;	// ZvTelemetry::AppRole
  ZtDate		m_uptime;
  unsigned		m_tid = 0;

  ZvRingParams		m_telRingParams;
  ZuPtr<TelRing>	m_telRing;
  ZmAtomic<unsigned>	m_telCount = 0;

  // FIXME - need Zdf in-memory manager (later, file manager)

  ZtString		m_gladePath;
  ZtString		m_stylePath;
  GtkStyleContext	*m_styleContext = nullptr;
  GtkWindow		*m_mainWindow = nullptr;
  gulong		m_mainDestroy = 0;

  ZmTime		m_refreshQuantum;
  ZmTime		m_refreshRate;
  ZmScheduler::Timer	m_refreshTimer;

  GtkTree::View		m_gtkView;
  GtkTree::Model	*m_gtkModel;
};

inline void ZDash_Srv::telemetry(ZvTelemetry::App &data)
{
  static_cast<ZDash *>(this)->telemetry(data);
}

inline CliLink_::CliLink_(ZDash_Cli *app) : Base{app} { }

inline void CliLink_::loggedIn()
{
  static_cast<ZDash *>(this->app())->loggedIn(this);
}
inline void CliLink_::disconnected()
{
  static_cast<ZDash *>(this->app())->disconnected(this);
  Base::disconnected();
}
inline void CliLink_::connectFailed(bool transient)
{
  static_cast<ZDash *>(this->app())->connectFailed(this, transient);
}

inline int CliLink_::processTelemetry(const uint8_t *data, unsigned len)
{
  return static_cast<ZDash *>(this->app())->processTelemetry(this, data, len);
}
inline int CliLink_::processDeflt(ZuID id, const uint8_t *data, unsigned len)
{
  return static_cast<ZDash *>(this->app())->processDeflt(this, id, data, len);
}

inline SrvLink::SrvLink(ZDash_Srv *app) : Base{app} { }

inline int SrvLink::processApp(const uint8_t *data, unsigned len)
{
  return static_cast<ZDash *>(this->app())->processApp(this, data, len);
}
inline int SrvLink::processDeflt(ZuID id, const uint8_t *data, unsigned len)
{
  return static_cast<ZDash *>(this->app())->processDeflt(this, id, data, len);
}

int main(int argc, char **argv)
{
  if (argc != 1) usage();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::lambdaSink(
	[](ZeEvent *e) { std::cerr << e->message() << '\n' << std::flush; }));
  ZeLog::start();

  ZiMultiplex *mx = new ZiMultiplex(
      ZiMxParams()
	.scheduler([](auto &s) {
	  s.nThreads(5)
	  .thread(1, [](auto &t) { t.isolated(1); })
	  .thread(2, [](auto &t) { t.isolated(1); })
	  .thread(3, [](auto &t) { t.isolated(1); })
	  .thread(4, [](auto &t) { t.isolated(1); }); })
	.rxThread(1).txThread(2));

  mx->start();

  ZmRef<ZDash> app = new ZDash();

  {
    ZmRef<ZvCf> cf = new ZvCf();
    cf->set("timeout", "1");
    cf->set("thread", "3");
    cf->set("gtkThread", "4");
    cf->set("gtkGlade", "zdash.glade");
    if (auto caPath = ::getenv("ZCMD_CAPATH"))
      cf->set("caPath", caPath);
    else
      cf->set("caPath", "/etc/ssl/certs");
    try {
      app->init(mx, cf);
    } catch (const ZvError &e) {
      std::cerr << e << '\n' << std::flush;
      ::exit(1);
    } catch (const ZtString &e) {
      std::cerr << e << '\n' << std::flush;
      ::exit(1);
    } catch (...) {
      std::cerr << "unknown exception\n" << std::flush;
      ::exit(1);
    }
  }

  app->wait();

  app->final();

  mx->stop(true);

  ZeLog::stop();

  delete mx;

  ZmTrap::sigintFn({});
}
