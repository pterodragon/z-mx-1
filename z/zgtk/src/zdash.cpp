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

#ifdef _WIN32
#include <io.h>		// for _isatty
#ifndef isatty
#define isatty _isatty
#endif
#ifndef fileno
#define fileno _fileno
#endif
#else
#include <termios.h>
#include <unistd.h>	// for isatty
#include <fcntl.h>
#endif

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
#include <zlib/ZiRing.hpp>

#include <zlib/ZvCf.hpp>
#include <zlib/ZvCSV.hpp>
#include <zlib/ZvRingCf.hpp>
#include <zlib/ZvMultiplex.hpp>
#include <zlib/ZvCmdClient.hpp>
#include <zlib/ZvCmdHost.hpp>
#include <zlib/ZvUserDB.hpp>

#include <zlib/ZtlsBase32.hpp>
#include <zlib/ZtlsBase64.hpp>

#include <zlib/Zrl.hpp>

#include <zlib/ZCmd.hpp>

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
    "usage: zdash USER@[HOST:]PORT\n"
    "\tUSER\t- user\n"
    "\tHOST\t- target host (default localhost)\n"
    "\tPORT\t- target port\n\n"
    "Environment Variables:\n"
    "\tZCMD_KEY_ID\tAPI key ID\n"
    "\tZCMD_KEY_SECRET\tAPI key secret\n"
    "\tZCMD_PLUGIN\tzcmd plugin module\n";
  std::cerr << usage << std::flush;
  ZeLog::stop();
  ZmPlatform::exit(1);
}

static ZtString getpass_(const ZtString &prompt, unsigned passLen)
{
  ZtString passwd;
  passwd.size(passLen + 4); // allow for \r\n and null terminator
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  DWORD omode, nmode;
  GetConsoleMode(h, &omode);
  nmode = (omode | ENABLE_LINE_INPUT) & ~ENABLE_ECHO_INPUT;
  SetConsoleMode(h, nmode);
  DWORD n = 0;
  WriteConsole(h, prompt.data(), prompt.length(), &n, nullptr);
  n = 0;
  ReadConsole(h, passwd.data(), passwd.size() - 1, &n, nullptr);
  if (n < passwd.size()) passwd.length(n);
  WriteConsole(h, "\r\n", 2, &n, nullptr);
  mode |= ENABLE_ECHO_INPUT;
  SetConsoleMode(h, omode);
#else
  int fd = ::open("/dev/tty", O_RDWR, 0);
  if (fd < 0) return passwd;
  struct termios oflags, nflags;
  memset(&oflags, 0, sizeof(termios));
  tcgetattr(fd, &oflags);
  memcpy(&nflags, &oflags, sizeof(termios));
  nflags.c_lflag = (nflags.c_lflag & ~ECHO) | ECHONL;
  if (tcsetattr(fd, TCSANOW, &nflags)) return passwd;
  ::write(fd, prompt.data(), prompt.length());
  FILE *in = fdopen(fd, "r");
  if (!fgets(passwd, passwd.size() - 1, in)) return passwd;
  passwd.calcLength();
  tcsetattr(fd, TCSANOW, &oflags);
  fclose(in);
  ::close(fd);
#endif
  passwd.chomp();
  return passwd;
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

    template <typename Node>
    ZuInline Node *treeNode() const { return static_cast<Node *>(treeNode_); }
    template <typename Node>
    ZuInline void treeNode(Node *node) { treeNode_ = node; }

    ZuInline TelKey telKey() const { return Item__<Data_>::telKey(data); }
    ZuInline int rag() const { return Item__<Data_>::rag(data); }

    bool record(ZuString name, ZeError *e = nullptr) {
      dataFrame = new Zdf::DataFrame{Data::fields(), name, true};
      if (!dataFrame->open(e)) return false;
      dfWriter = dataFrame->writer();
      return true;
    }

    void			*link_;
    Load			data;

    void			*treeNode_ = nullptr;
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

namespace Tree {
  template <typename Impl, typename Item>
  class Node {
    Impl *impl() { return static_cast<Impl *>(this); }
    const Impl *impl() const { return static_cast<Impl *>(this); }

  public:
    Item	*item = nullptr;

    Node() = default;
    Node(Item *item_) : item{item_} { init_(); }
    void init(Item *item_) { item = item_; init_(); }
    void init_() { item->treeNode(impl()); }

    ZuInline typename Item::TelKey telKey() const { return item->telKey(); }
    ZuInline int rag() const { return item->rag(); }
  };

  template <unsigned Depth, typename Item>
  struct Leaf :
      public Node<Leaf<Depth, Item>, Item>,
      public ZGtk::TreeNode::Leaf<Leaf<Depth, Item>, Depth> {
    Leaf() = default;
    Leaf(Item *item) : Node<Leaf, Item>{item} { }
    ZuInline int cmp(const Leaf &v) const {
      return this->item->telKey().cmp(v.telKey());
    }
  };

  template <unsigned Depth, typename Item, typename Child>
  struct Parent :
      public Node<Parent<Depth, Item, Child>, Item>,
      public ZGtk::TreeNode::Parent<Parent<Depth, Item, Child>, Depth, Child> {
    Parent() = default;
    Parent(Item *item) : Node<Parent, Item>{item} { }
    ZuInline int cmp(const Parent &v) const {
      return this->item->telKey().cmp(v.telKey());
    }
    using Base = ZGtk::TreeNode::Parent<Parent, Depth, Child>;
    using Base::add;
    using Base::del;
  };

  template <unsigned Depth, typename Item, typename Tuple>
  struct Branch :
      public Node<Branch<Depth, Item, Tuple>, Item>,
      public ZGtk::TreeNode::Branch<Branch<Depth, Item, Tuple>, Depth, Tuple> {
    Branch() = default; 
    Branch(Item *item) : Node<Branch, Item>{item} { }
    ZuInline int cmp(const Branch &v) const {
      return this->item->telKey().cmp(v.telKey());
    }
    using Base = ZGtk::TreeNode::Branch<Branch, Depth, Tuple>;
    using Base::add;
    using Base::del;
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
  struct DBHosts {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"hosts"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using DBHostParent = Parent<3, DBHosts, DBHost>;
  // DBs
  struct DBs {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"dbs"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using DBParent = Parent<3, DBs, DB>;

  // heaps
  struct Heaps {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"heaps"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using HeapParent = Parent<2, Heaps, Heap>;
  // hashTbls
  struct HashTbls {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"hashTbls"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using HashTblParent = Parent<2, HashTbls, HashTbl>;
  // threads
  struct Threads {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"threads"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using ThreadParent = Parent<2, Threads, Thread>;
  // multiplexers
  struct Mxs {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"multiplexers"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using MxParent = Parent<2, Mxs, Mx>;
  // queues
  struct Queues {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"queues"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };
  using QueueParent = Parent<2, Queues, Queue>;
  // engines
  struct Engines {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() { return TelKey{"engines"}; }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
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

  struct Root : public ZGtk::TreeNode::Parent<Root, 0, App> {
    using TelKey = ZuTuple<const char *>;
    ZuInline static TelKey telKey() {
      return static_cast<const char *>(nullptr);
    }
    ZuInline static int rag() { return ZvTelemetry::RAG::Off; }
  };

  // map telemetry items to corresponding tree nodes
  inline Heap *node(TelItem<ZvTelemetry::Heap> *item) {
    return item->template treeNode<Heap>();
  }
  inline HashTbl *node(TelItem<ZvTelemetry::HashTbl> *item) {
    return item->template treeNode<HashTbl>();
  }
  inline Thread *node(TelItem<ZvTelemetry::Thread> *item) {
    return item->template treeNode<Thread>();
  }
  inline Mx *node(TelItem<ZvTelemetry::Mx> *item) {
    return item->template treeNode<Mx>();
  }
  inline Socket *node(TelItem<ZvTelemetry::Socket> *item) {
    return item->template treeNode<Socket>();
  }
  inline Queue *node(TelItem<ZvTelemetry::Queue> *item) {
    return item->template treeNode<Queue>();
  }
  inline Engine *node(TelItem<ZvTelemetry::Engine> *item) {
    return item->template treeNode<Engine>();
  }
  inline Link *node(TelItem<ZvTelemetry::Link> *item) {
    return item->template treeNode<Link>();
  }
  inline DB *node(TelItem<ZvTelemetry::DB> *item) {
    return item->template treeNode<DB>();
  }
  inline DBHost *node(TelItem<ZvTelemetry::DBHost> *item) {
    return item->template treeNode<DBHost>();
  }
  inline DBEnv *node(TelItem<ZvTelemetry::DBEnv> *item) {
    return item->template treeNode<DBEnv>();
  }
  inline App *node(TelItem<ZvTelemetry::App> *item) {
    return item->template treeNode<App>();
  }

  enum { Depth = 5 };

  // (*) - branch
  using Iter = ZuUnion<
    Root *,
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

  class Model : public ZGtk::TreeHierarchy<Model, Iter, Depth> {
  public:
    enum { RAGCol = 0, KeyCol, NCols };

    // root()
    Root *root() { return &m_root; }

    // parent() - child->parent type map
    template <typename T>
    static typename ZuSame<T, Root, Root>::T *parent(T *) {
      return nullptr;
    }
    template <typename T>
    static typename ZuSame<T, App, Root>::T *parent(T *ptr) {
      return ptr->template parent<Root>();
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, HeapParent>::Same ||
	ZuConversion<T, HashTblParent>::Same ||
	ZuConversion<T, ThreadParent>::Same ||
	ZuConversion<T, MxParent>::Same ||
	ZuConversion<T, QueueParent>::Same ||
	ZuConversion<T, EngineParent>::Same ||
	ZuConversion<T, DBEnv>::Same, App>::T *parent(T *ptr) {
      return ptr->template parent<App>();
    }
    template <typename T>
    static typename ZuSame<T, Heap, HeapParent>::T *parent(T *ptr) {
      return ptr->template parent<HeapParent>();
    }
    template <typename T>
    static typename ZuSame<T, HashTbl, HashTblParent>::T *parent(T *ptr) {
      return ptr->template parent<HashTblParent>();
    }
    template <typename T>
    static typename ZuSame<T, Thread, ThreadParent>::T *parent(T *ptr) {
      return ptr->template parent<ThreadParent>();
    }
    template <typename T>
    static typename ZuSame<T, Mx, MxParent>::T *parent(T *ptr) {
      return ptr->template parent<MxParent>();
    }
    template <typename T>
    static typename ZuSame<T, Queue, QueueParent>::T *parent(T *ptr) {
      return ptr->template parent<QueueParent>();
    }
    template <typename T>
    static typename ZuSame<T, Engine, EngineParent>::T *parent(T *ptr) {
      return ptr->template parent<EngineParent>();
    }
    template <typename T>
    static typename ZuSame<T, Socket, Mx>::T *parent(T *ptr) {
      return ptr->template parent<Mx>();
    }
    template <typename T>
    static typename ZuSame<T, Link, Engine>::T *parent(T *ptr) {
      return ptr->template parent<Engine>();
    }
    template <typename T>
    static typename ZuIfT<
	ZuConversion<T, DBHostParent>::Same ||
	ZuConversion<T, DBParent>::Same, DBEnv>::T *parent(T *ptr) {
      return ptr->template parent<DBEnv>();
    }
    template <typename T>
    static typename ZuSame<T, DBHost, DBHostParent>::T *parent(T *ptr) {
      return ptr->template parent<DBHostParent>();
    }
    template <typename T>
    static typename ZuSame<T, DB, DBParent>::T *parent(T *ptr) {
      return ptr->template parent<DBParent>();
    }

    gint get_n_columns() { return NCols; }
    GType get_column_type(gint i) {
      switch (i) {
	case RAGCol: return G_TYPE_INT;
	case KeyCol: return G_TYPE_STRING;
	default: return G_TYPE_NONE;
      }
    }
    template <typename T>
    void value(const T *ptr, gint i, ZGtk::Value *v) {
      switch (i) {
	case RAGCol: 
	  v->init(G_TYPE_INT);
	  v->set_int(ptr->rag());
	  break;
	case KeyCol:
	  m_value.length(0);
	  v->init(G_TYPE_STRING);
	  m_value << ptr->telKey().print("|");
	  v->set_static_string(m_value);
	  break;
	default:
	  v->init(G_TYPE_NONE);
	  break;
      }
    }

  private:
    Root	m_root;	// root of tree
    ZtString	m_value;// re-used string buffer
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
    auto viewCol(const char *id) {
      auto col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title(col, id);

      auto cell = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start(col, cell, true);

      gtk_tree_view_column_set_cell_data_func(col, cell, [](
	    GtkTreeViewColumn *col, GtkCellRenderer *cell,
	    GtkTreeModel *model, GtkTreeIter *iter, gpointer this_) {
	reinterpret_cast<View *>(this_)->render<RagCol, TextCol>(
	    col, cell, model, iter);
      }, reinterpret_cast<gpointer>(this), nullptr);

      return col;
    }

    template <unsigned RagCol, unsigned TextCol>
    void render(
	GtkTreeViewColumn *col, GtkCellRenderer *cell,
	GtkTreeModel *model, GtkTreeIter *iter) {
      gtk_tree_model_get_value(model, iter, TextCol, &m_values[0]);

      gint rag;
      {
	ZGtk::Value rag_;
	gtk_tree_model_get_value(model, iter, RagCol, &rag_);
	rag = rag_.get_int();
      }
      switch (rag) {
	case ZvTelemetry::RAG::Red: {
	  m_values[1].set_static_boxed(&m_rag_red_bg);
	  m_values[2].set_static_boxed(&m_rag_red_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	} break;
	case ZvTelemetry::RAG::Amber: {
	  m_values[1].set_static_boxed(&m_rag_amber_bg);
	  m_values[2].set_static_boxed(&m_rag_amber_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	} break;
	case ZvTelemetry::RAG::Green: {
	  m_values[1].set_static_boxed(&m_rag_green_bg);
	  m_values[2].set_static_boxed(&m_rag_green_fg);
	  g_object_setv(G_OBJECT(cell), 3, m_props, m_values);
	} break;
	default:
	  g_object_setv(G_OBJECT(cell), 1, m_props, m_values);
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

      gtk_tree_view_append_column(
	  m_treeView, viewCol<Model::RAGCol, Model::KeyCol>("key"));

      static const gchar *props[] = {
	"text", "background-rgba", "foreground-rgba"
      };
      m_props = props;
      m_values[0].init(G_TYPE_STRING);
      m_values[1].init(GDK_TYPE_RGBA);
      m_values[2].init(GDK_TYPE_RGBA);
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
    const gchar	**m_props;
    ZGtk::Value	m_values[3];
  };
}

class ZDash :
    public ZmPolymorph,
    public ZvCmdClient<ZDash>,
    public ZCmdHost,
    public ZGtk::App {
public:
  using Ztls::Engine<ZDash>::run;
  using Ztls::Engine<ZDash>::invoke;

  using Client = ZvCmdClient<ZDash>;

  struct Link : public ZvCmdCliLink<ZDash, Link> {
    using Base = ZvCmdCliLink<ZDash, Link>;
    Link(ZDash *app) : Base(app) { }
    void loggedIn() {
      this->app()->loggedIn(this);
    }
    void disconnected() {
      this->app()->disconnected(this);
      Base::disconnected();
    }
    void connectFailed(bool transient) {
      this->app()->connectFailed(this, transient);
    }
    int processApp(ZuArray<const uint8_t> data) {
      return this->app()->processApp(this, data);
    }
    int processTel(ZuArray<const uint8_t> data) {
      return this->app()->processTel(this, data);
    }
 
    Telemetry::Containers	telemetry;
  };

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
    bool push(Link *link, ZuArray<const uint8_t> msg) {
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
      return false;
    }
    template <typename L>
    bool shift(L l) {
      if (const void *ptr = ZiRing::shift()) {
	Link *link =
	  reinterpret_cast<Link *>(*static_cast<const uintptr_t *>(ptr));
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

  using AppNode = Tree::App;
  using DBEnvNode = Tree::DBEnv;

  void init(ZiMultiplex *mx, ZvCf *cf) {
    if (ZmRef<ZvCf> telRingCf = cf->subset("telRing", false))
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

    m_gladePath = cf->get("gtkGlade", true);
    m_stylePath = cf->get("gtkStyle");

    m_refreshRate =
      cf->getInt64("gtkRefresh", 1, 60000, false, 1) * (int64_t)1000000;
    unsigned tid = cf->getInt("gtkThread", 1, mx->params().nThreads(), true);

    Client::init(mx, cf);
    ZvCmdHost::init();
    initCmds();

    attach(mx, tid);
    mx->run(tid, [this]() { gtkInit(); });
  }

  void final() {
    detach(ZmFn<>{this, [](ZDash *this_) {
      this_->gtkFinal();
      this_->post();
    }});

    wait();

    exiting();
    disconnect();

    m_telRing->close();

    ZvCmdHost::final();
    Client::final();
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

    m_model = Tree::Model::ctor();
    m_view.init(view_, m_styleContext);
    m_view.bind(GTK_TREE_MODEL(m_model));

    g_signal_connect(G_OBJECT(m_mainWindow), "destroy",
	ZGtk::callback([](GObject *o, gpointer this_) {
	  reinterpret_cast<ZDash *>(this_)->post();
	}), reinterpret_cast<gpointer>(this));

    gtk_widget_show_all(GTK_WIDGET(m_mainWindow));

    gtk_window_present(m_mainWindow);

    m_telRing->attach();

    gtkNextRefresh();
  }

  void gtkRefresh() {
    switch (m_telRing->readStatus()) {
      case Zi::OK:
	gtkNextRefresh();
      case Zi::EndOfFile: // fallthrough
	return;
    }

    // FIXME - freeze, save sort col, unset sort col

    ZmTime deadline = ZmTimeNow() + ZmTime{ZmTime::Nano, m_refreshRate>>1};
    unsigned i = 0;
    while (m_telRing->shift([](Link *link, const ZuArray<const uint8_t> &msg) {
      link->app()->processTel2(link, msg);
    }))
      if (!(++i & 0xf) && ZmTimeNow() >= deadline) break;

    // FIXME - restore sort col, thaw

    gtkNextRefresh();
  }

  void gtkNextRefresh() {
    m_lastRefresh.now();
    gtkRun(ZmFn<>{this, [](ZDash *this_) { this_->gtkRefresh(); }},
	m_lastRefresh + ZmTime{ZmTime::Nano, m_refreshRate}, &m_refreshTimer);
  }

  void gtkFinal() {
    m_telRing->detach();

    ZGtk::App::sched()->del(&m_refreshTimer);

    m_view.final();

    if (m_model) g_object_unref(G_OBJECT(m_model));
    if (m_mainWindow) g_object_unref(G_OBJECT(m_mainWindow));
    if (m_styleContext) g_object_unref(G_OBJECT(m_styleContext));
  }

  void post() { m_done.post(); }
  void wait() { m_done.wait(); }
  
  void target(ZuString s) {
    m_prompt.null();
    m_prompt << s << "] ";
  }

  template <typename ...Args>
  void login(Args &&... args) {
    m_link = new Link(this);
    m_link->login(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  void access(Args &&... args) {
    m_link = new Link(this);
    m_link->access(ZuFwd<Args>(args)...);
  }

  void disconnect() { m_link->disconnect(); }

  int code() const { return m_code; }

  void exiting() { m_exiting = true; }

  void plugin(ZmRef<ZCmdPlugin> plugin) {
    if (ZuUnlikely(m_plugin)) m_plugin->final();
    m_plugin = ZuMv(plugin);
  }
  void sendApp(Zfb::IOBuilder &fbb) { return m_link->sendApp(fbb); }

private:
  template <typename ...Args>
  ZuInline void gtkRun(Args &&... args) {
    ZGtk::App::run(ZuFwd<Args>(args)...);
  }
  template <typename ...Args>
  ZuInline void gtkInvoke(Args &&... args) {
    ZGtk::App::invoke(ZuFwd<Args>(args)...);
  }

  void loggedIn(Link *link) {
    if (auto plugin = ::getenv("ZCMD_PLUGIN")) {
      ZmRef<ZvCf> args = new ZvCf();
      args->set("1", plugin);
      args->set("#", "2");
      ZtString out;
      loadModCmd(stdout, args, out);
      fwrite(out.data(), 1, out.length(), stdout);
    }
    std::cout <<
      "For a list of valid commands: help\n"
      "For help on a particular command: COMMAND --help\n" << std::flush;
    prompt();
  }

  int processApp(Link *, ZuArray<const uint8_t> msg) {
    if (ZuUnlikely(!m_plugin)) return -1;
    return m_plugin->processApp(msg);
  }

  int processTel(Link *link, ZuArray<const uint8_t> msg) {
    m_telRing->push(link, msg);
    return 0;
  }
  void processTel2(Link *link, const ZuArray<const uint8_t> &msg_) {
    if (ZuUnlikely(!msg_)) {
      disconnected2(link);
      return;
    }
    using namespace ZvTelemetry;
    {
      flatbuffers::Verifier verifier(msg_.data(), msg_.length());
      if (!fbs::VerifyTelemetryBuffer(verifier)) return;
    }
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
  processTel3(Link *link, const FBS *fbs) {
    ZuConstant<ZuTypeIndex<FBS, Telemetry::FBSTypeList>::I> i;
    using T = typename ZuType<i, Telemetry::TypeList>::T;
    auto &container = link->telemetry.p<i>();
    using Item = TelItem<T>;
    Item *item;
    if (item = container.lookup(fbs)) {
      item->data.loadDelta(fbs);
      m_model->updated(Tree::node(item));
    } else {
      item = new Item{link, fbs};
      container.add(item);
      addNode(link, item);
    }
  }
  AppItem *appItem(Link *link) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::App, ZvTelemetry::TypeList>::I> i;
    auto &container = link->telemetry.p<i>();
    auto item = container.lookup(
	static_cast<const ZvTelemetry::fbs::App *>(nullptr));
    if (!item) {
      item = new TelItem<ZvTelemetry::App>{link};
      item->initTelKey(link->server(), link->port());
      container.add(item);
      auto node = new Tree::App{item};
      m_model->add(node, m_model->root());
    }
    return item;
  }
  DBEnvItem *dbEnvItem(Link *link) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::DBEnv, ZvTelemetry::TypeList>::I> i;
    auto &container = link->telemetry.p<i>();
    auto item = container.lookup(
	static_cast<const ZvTelemetry::fbs::DBEnv *>(nullptr));
    if (!item) {
      item = new TelItem<ZvTelemetry::DBEnv>{link};
      container.add(item);
      auto appNode = Tree::node(appItem(link));
      auto &dbEnv = appNode->dbEnv();
      dbEnv.init(item);
      m_model->add(&dbEnv, appNode);
    }
    return item;
  }
  template <typename Item, typename ParentFn>
  void addNode_(AppItem *appItem, Item *item, ParentFn parentFn) {
    auto appNode = Tree::node(appItem);
    auto &parent = parentFn(appNode);
    if (parent.row() < 0) m_model->add(&parent, appNode);
    using Node = typename ZuDecay<decltype(*Tree::node(item))>::T;
    m_model->add(new Node{item}, &parent);
  }
  void addNode(Link *, AppItem *) { } // unused
  void addNode(Link *link, TelItem<ZvTelemetry::Heap> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::HeapParent & { return _->heaps(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::HashTbl> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::HashTblParent & { return _->hashTbls(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Thread> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::ThreadParent & { return _->threads(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Mx> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::MxParent & { return _->mxs(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Socket> *item) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::Mx, ZvTelemetry::TypeList>::I> i;
    auto &mxContainer = link->telemetry.p<i>();
    auto mxItem = mxContainer.find(ZvTelemetry::Mx::Key{item->data.mxID});
    if (!mxItem) {
      auto mxItem = new TelItem<ZvTelemetry::Mx>{link};
      mxItem->data.id = item->data.mxID;
      mxContainer.add(mxItem);
      addNode(link, mxItem);
    }
    m_model->add(new Tree::Socket{item}, Tree::node(mxItem));
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Queue> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::QueueParent & { return _->queues(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Engine> *item) {
    addNode_(appItem(link), item,
	[](AppNode *_) -> Tree::EngineParent & { return _->engines(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::Link> *item) {
    ZuConstant<ZuTypeIndex<ZvTelemetry::Engine, ZvTelemetry::TypeList>::I> i;
    auto &engContainer = link->telemetry.p<i>();
    auto engItem =
      engContainer.find(ZvTelemetry::Engine::Key{item->data.engineID});
    if (!engItem) {
      auto engItem = new TelItem<ZvTelemetry::Mx>{link};
      engItem->data.id = item->data.engineID;
      engContainer.add(engItem);
      addNode(link, engItem);
    }
    m_model->add(new Tree::Link{item}, Tree::node(engItem));
  }
  void addNode(Link *link, TelItem<ZvTelemetry::DBEnv> *item) {
    auto appNode = Tree::node(appItem(link));
    auto &dbEnv = appNode->dbEnv();
    dbEnv.init(item);
    m_model->add(&dbEnv, appNode);
  }
  template <typename Item, typename ParentFn>
  void addNode_(DBEnvItem *dbEnvItem, Item *item, ParentFn parentFn) {
    auto dbEnvNode = Tree::node(dbEnvItem);
    auto &parent = parentFn(dbEnvNode);
    if (parent.row() < 0) m_model->add(&parent, dbEnvNode);
    using Node = typename ZuDecay<decltype(*Tree::node(item))>::T;
    m_model->add(new Node{item}, &parent);
  }
  void addNode(Link *link, TelItem<ZvTelemetry::DBHost> *item) {
    addNode_(dbEnvItem(link), item,
	[](DBEnvNode *_) -> Tree::DBHostParent & { return _->hosts(); });
  }
  void addNode(Link *link, TelItem<ZvTelemetry::DB> *item) {
    addNode_(dbEnvItem(link), item,
	[](DBEnvNode *_) -> Tree::DBParent & { return _->dbs(); });
  }
  template <typename FBS>
  typename ZuIs<ZvTelemetry::fbs::Alert, FBS, bool>::T
  processTel3(Link *link, const FBS *fbs) {
    ZuConstant<ZuTypeIndex<FBS, Telemetry::FBSTypeList>::I> i;
    using T = typename ZuType<i, Telemetry::TypeList>::T;
    auto &container = link->telemetry.p<i>();
    alert(new (container.data.push()) ZvTelemetry::load<T>{fbs});
    return true;
  }

  void alert(const ZvTelemetry::Alert *) {
    // FIXME - update alerts
  }

  void disconnected(Link *link) {
    m_telRing->push(link, ZuArray<const uint8_t>{});
  }
  void disconnected2(Link *link) {
    if (m_exiting) return;
    Zrl::stop();
    std::cerr << "server disconnected\n" << std::flush;
    ZmPlatform::exit(1);
  }

  void connectFailed(Link *, bool transient) {
    Zrl::stop();
    std::cerr << "connect failed\n" << std::flush;
    ZmPlatform::exit(1);
  }

  void prompt() {
    mx()->add(ZmFn<>{this, [](ZDash *app) { app->prompt_(); }});
  }
  void prompt_() {
    ZtString cmd;
next:
    try {
      cmd = Zrl::readline_(m_prompt);
    } catch (const Zrl::EndOfFile &) {
      post();
      return;
    }
    if (!cmd) goto next;
    send(ZuMv(cmd));
  }

  void send(ZtString cmd) {
    FILE *file = stdout;
    ZtString cmd_ = ZuMv(cmd);
    {
      ZtRegex::Captures c;
      const auto &append = ZtStaticRegex("\\s*>>\\s*");
      const auto &output = ZtStaticRegex("\\s*>\\s*");
      unsigned pos = 0, n = 0;
      if (n = append.m(cmd, c, pos)) {
	if (!(file = fopen(c[2], "a"))) {
	  logError(ZtString{c[2]}, ": ", ZeLastError);
	  return;
	}
	cmd = c[0];
      } else if (n = output.m(cmd, c, pos)) {
	if (!(file = fopen(c[2], "w"))) {
	  logError(ZtString{c[2]}, ": ", ZeLastError);
	  return;
	}
	cmd = c[0];
      } else {
	cmd = ZuMv(cmd_);
      }
    }
    ZtArray<ZtString> args;
    ZvCf::parseCLI(cmd, args);
    if (args[0] == "help") {
      if (args.length() == 1) {
	ZtString out;
	out << "Local ";
	processCmd(file, args, out);
	out << "\nRemote ";
	fwrite(out.data(), 1, out.length(), file);
      } else if (args.length() == 2 && hasCmd(args[1])) {
	ZtString out;
	int code = processCmd(file, args, out);
	if (code || out) executed(code, file, out);
	return;
      }
    } else if (hasCmd(args[0])) {
      ZtString out;
      int code = processCmd(file, args, out);
      if (code || out) executed(code, file, out);
      return;
    }
    auto seqNo = m_seqNo++;
    m_fbb.Clear();
    m_fbb.Finish(ZvCmd::fbs::CreateRequest(m_fbb, seqNo,
	  Zfb::Save::strVecIter(m_fbb, args.length(),
	    [&args](unsigned i) { return args[i]; })));
    m_link->sendCmd(m_fbb, seqNo, [this, file](const ZvCmd::fbs::ReqAck *ack) {
      using namespace Zfb::Load;
      executed(ack->code(), file, str(ack->out()));
    });
  }

  int executed(int code, FILE *file, ZuString out) {
    m_code = code;
    if (out) fwrite(out.data(), 1, out.length(), file);
    if (file != stdout) fclose(file);
    prompt();
    return code;
  }

private:
  // built-in commands

  int filterAck(
      FILE *file, const ZvUserDB::fbs::ReqAck *ack,
      int ackType1, int ackType2,
      const char *op, ZtString &out) {
    using namespace ZvUserDB;
    if (ack->rejCode()) {
      out << '[' << ZuBox<unsigned>(ack->rejCode()) << "] "
	<< Zfb::Load::str(ack->rejText()) << '\n';
      return 1;
    }
    auto ackType = ack->data_type();
    if ((int)ackType != ackType1 &&
	ackType2 >= fbs::ReqAckData_MIN && (int)ackType != ackType2) {
      logError("mismatched ack from server: ",
	  fbs::EnumNameReqAckData(ackType));
      out << op << " failed\n";
      return 1;
    }
    return 0;
  }

  void initCmds() {
    addCmd("passwd", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->passwdCmd(static_cast<FILE *>(file), args, out);
	}},  "change passwd", "usage: passwd");

    addCmd("users", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->usersCmd(static_cast<FILE *>(file), args, out);
	}},  "list users", "usage: users");
    addCmd("useradd",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add user",
	"usage: useradd ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("resetpass", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->resetPassCmd(static_cast<FILE *>(file), args, out);
	}},  "reset password", "usage: resetpass USERID");
    addCmd("usermod",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify user",
	"usage: usermod ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("userdel", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete user", "usage: userdel ID");

    addCmd("roles", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->rolesCmd(static_cast<FILE *>(file), args, out);
	}},  "list roles", "usage: roles");
    addCmd("roleadd", "i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add role",
	"usage: roleadd NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("rolemod", "i immutable immutable { type scalar }",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify role",
	"usage: rolemod NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("roledel", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete role",
	"usage: roledel NAME");

    addCmd("perms", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permsCmd(static_cast<FILE *>(file), args, out);
	}},  "list permissions", "usage: perms");
    addCmd("permadd", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add permission", "usage: permadd NAME");
    addCmd("permmod", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify permission", "usage: permmod ID NAME");
    addCmd("permdel", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete permission", "usage: permdel ID");

    addCmd("keys", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keysCmd(static_cast<FILE *>(file), args, out);
	}},  "list keys", "usage: keys [USERID]");
    addCmd("keyadd", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add key", "usage: keyadd [USERID]");
    addCmd("keydel", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete key", "usage: keydel ID");
    addCmd("keyclr", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyClrCmd(static_cast<FILE *>(file), args, out);
	}},  "clear all keys", "usage: keyclr [USERID]");

    addCmd("loadmod", "",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->loadModCmd(static_cast<FILE *>(file), args, out);
	}},  "load application-specific module", "usage: loadmod MODULE");

    addCmd("telcap",
	"i interval interval { type scalar } "
	"u unsubscribe unsubscribe { type flag }",
	ZvCmdFn{this, [](ZDash *app, void *file, ZvCf *args, ZtString &out) {
	  return app->telcapCmd(static_cast<FILE *>(file), args, out);
	}},  "telemetry capture",
	"usage: telcap [OPTIONS...] PATH [TYPE[:FILTER]]...\n\n"
	"  PATH\tdirectory for capture CSV files\n"
	"  TYPE\t[Heap|HashTbl|Thread|Mx|Queue|Engine|DbEnv|App|Alert]\n"
	"  FILTER\tfilter specification in type-specific format\n\n"
	"Options:\n"
	"  -i, --interval=N\tset scan interval in milliseconds "
	  "(100 <= N <= 1M)\n"
	"  -u, --unsubscribe\tunsubscribe (i.e. end capture)\n");
  }

  int passwdCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 1) throw ZvCmdUsage{};
    ZtString oldpw = getpass_("Current password: ", 100);
    ZtString newpw = getpass_("New password: ", 100);
    ZtString checkpw = getpass_("Retype new password: ", 100);
    if (checkpw != newpw) {
      out << "passwords do not match\npassword unchanged!\n";
      return 1;
    }
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_ChPass,
	    fbs::CreateUserChPass(m_fbb,
	      str(m_fbb, oldpw),
	      str(m_fbb, newpw)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_ChPass, -1, "password change", out))
	return executed(code, file, out);
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "password change rejected\n";
	return executed(1, file, out);
      }
      out << "password changed\n";
      return executed(0, file, out);
    });
    return 0;
  }

  static void printUser(ZtString &out, const ZvUserDB::fbs::User *user_) {
    using namespace ZvUserDB;
    using namespace Zfb::Load;
    auto hmac_ = bytes(user_->hmac());
    ZtString hmac;
    hmac.length(Ztls::Base64::enclen(hmac_.length()));
    Ztls::Base64::encode(
	hmac.data(), hmac.length(), hmac_.data(), hmac_.length());
    auto secret_ = bytes(user_->secret());
    ZtString secret;
    secret.length(Ztls::Base32::enclen(secret_.length()));
    Ztls::Base32::encode(
	secret.data(), secret.length(), secret_.data(), secret_.length());
    out << user_->id() << ' ' << str(user_->name()) << " roles=[";
    all(user_->roles(), [&out](unsigned i, auto role_) {
      if (i) out << ',';
      out << str(role_);
    });
    out << "] hmac=" << hmac << " secret=" << secret << " flags=";
    bool pipe = false;
    if (user_->flags() & User::Enabled) {
      out << "Enabled";
      pipe = true;
    }
    if (user_->flags() & User::Immutable) {
      if (pipe) out << '|';
      out << "Immutable";
      pipe = true;
    }
    if (user_->flags() & User::ChPass) {
      if (pipe) out << '|';
      out << "ChPass";
      pipe = true;
    }
  }

  int usersCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      fbs::UserIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_id(args->getInt64("1", 0, LLONG_MAX, true));
      auto userID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserGet, userID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserGet, -1,
	    "user get", out))
	return executed(code, file, out);
      auto userList = static_cast<const fbs::UserList *>(ack->data());
      using namespace Zfb::Load;
      all(userList->list(), [&out](unsigned, auto user_) {
	printUser(out, user_); out << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int userAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      uint8_t flags = 0;
      if (args->get("enabled")) flags |= User::Enabled;
      if (args->get("immutable")) flags |= User::Immutable;
      const auto &comma = ZtStaticRegex(",");
      ZtRegex::Captures roles;
      comma.split(args->get("3"), roles);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserAdd,
	    fbs::CreateUser(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true),
	      str(m_fbb, args->get("2")), 0, 0,
	      strVecIter(m_fbb, roles.length(),
		[&roles](unsigned i) { return roles[i]; }),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserAdd, -1,
	    "user add", out))
	return executed(code, file, out);
      auto userPass = static_cast<const fbs::UserPass *>(ack->data());
      if (!userPass->ok()) {
	out << "user add rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userPass->user()); out << '\n';
      using namespace Zfb::Load;
      out << "passwd=" << str(userPass->passwd()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int resetPassCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_ResetPass,
	    fbs::CreateUserID(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_ResetPass, -1,
	    "reset password", out))
	return executed(code, file, out);
      auto userPass = static_cast<const fbs::UserPass *>(ack->data());
      if (!userPass->ok()) {
	out << "reset password rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userPass->user()); out << '\n';
      using namespace Zfb::Load;
      out << "passwd=" << str(userPass->passwd()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int userModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      uint8_t flags = 0;
      if (args->get("enabled")) flags |= User::Enabled;
      if (args->get("immutable")) flags |= User::Immutable;
      const auto &comma = ZtStaticRegex(",");
      ZtRegex::Captures roles;
      comma.split(args->get("3"), roles);
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserMod,
	    fbs::CreateUser(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true),
	      str(m_fbb, args->get("2")), 0, 0,
	      strVecIter(m_fbb, roles.length(),
		[&roles](unsigned i) { return roles[i]; }),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserMod, -1,
	    "user modify", out))
	return executed(code, file, out);
      auto userUpdAck = static_cast<const fbs::UserUpdAck *>(ack->data());
      if (!userUpdAck->ok()) {
	out << "user modify rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userUpdAck->user()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int userDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_UserDel,
	    fbs::CreateUserID(m_fbb,
	      args->getInt64("1", 0, LLONG_MAX, true)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_UserDel, -1,
	    "user delete", out))
	return executed(code, file, out);
      auto userUpdAck = static_cast<const fbs::UserUpdAck *>(ack->data());
      if (!userUpdAck->ok()) {
	out << "user delete rejected\n";
	return executed(1, file, out);
      }
      printUser(out, userUpdAck->user()); out << '\n';
      out << "user deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  static void printRole(ZtString &out, const ZvUserDB::fbs::Role *role_) {
    using namespace ZvUserDB;
    using namespace Zfb::Load;
    Bitmap perms, apiperms;
    all(role_->perms(), [&perms](unsigned i, uint64_t w) {
      if (ZuLikely(i < Bitmap::Words)) perms.data[i] = w;
    });
    all(role_->apiperms(), [&apiperms](unsigned i, uint64_t w) {
      if (ZuLikely(i < Bitmap::Words)) apiperms.data[i] = w;
    });
    out << str(role_->name())
      << " perms=[" << perms
      << "] apiperms=[" << apiperms
      << "] flags=";
    if (role_->flags() & Role::Immutable) out << "Immutable";
  }

  int rolesCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      Zfb::Offset<Zfb::String> name_;
      if (argc == 2) name_ = str(m_fbb, args->get("1"));
      fbs::RoleIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_name(name_);
      auto roleID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleGet, roleID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleGet, -1,
	    "role get", out))
	return executed(code, file, out);
      auto roleList = static_cast<const fbs::RoleList *>(ack->data());
      using namespace Zfb::Load;
      all(roleList->list(), [&out](unsigned, auto role_) {
	printRole(out, role_); out << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int roleAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      Bitmap perms{args->get("2")};
      Bitmap apiperms{args->get("3")};
      uint8_t flags = 0;
      if (args->get("immutable")) flags |= Role::Immutable;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleAdd,
	    fbs::CreateRole(m_fbb,
	      str(m_fbb, args->get("1")),
	      m_fbb.CreateVector(perms.data, Bitmap::Words),
	      m_fbb.CreateVector(apiperms.data, Bitmap::Words),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleAdd, -1,
	    "role add", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role add rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int roleModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 4) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      Bitmap perms{args->get("2")};
      Bitmap apiperms{args->get("3")};
      uint8_t flags = 0;
      if (args->get("immutable")) flags |= Role::Immutable;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleMod,
	    fbs::CreateRole(m_fbb,
	      str(m_fbb, args->get("1")),
	      m_fbb.CreateVector(perms.data, Bitmap::Words),
	      m_fbb.CreateVector(apiperms.data, Bitmap::Words),
	      flags).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleMod, -1,
	    "role modify", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role modify rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int roleDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_RoleDel,
	    fbs::CreateRoleID(m_fbb, str(m_fbb, args->get("1"))).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_RoleMod, -1,
	    "role delete", out))
	return executed(code, file, out);
      auto roleUpdAck = static_cast<const fbs::RoleUpdAck *>(ack->data());
      if (!roleUpdAck->ok()) {
	out << "role delete rejected\n";
	return executed(1, file, out);
      }
      printRole(out, roleUpdAck->role()); out << '\n';
      out << "role deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int permsCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      fbs::PermIDBuilder fbb_(m_fbb);
      if (argc == 2) fbb_.add_id(args->getInt("1", 0, Bitmap::Bits, true));
      auto permID = fbb_.Finish();
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermGet, permID.Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermGet, -1,
	    "perm get", out))
	return executed(code, file, out);
      auto permList = static_cast<const fbs::PermList *>(ack->data());
      using namespace Zfb::Load;
      all(permList->list(), [&out](unsigned, auto perm_) {
	out << ZuBoxed(perm_->id()).fmt(ZuFmt::Right<3, ' '>()) << ' '
	  << str(perm_->name()) << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }
  int permAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto name = args->get("1");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermAdd,
	    fbs::CreatePermAdd(m_fbb, str(m_fbb, name)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermAdd, -1,
	    "permission add", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission add rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "added " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int permModCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 3) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto permID = args->getInt("1", 0, Bitmap::Bits, true);
      auto permName = args->get("2");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermMod,
	    fbs::CreatePerm(m_fbb, permID, str(m_fbb, permName)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermMod, -1,
	    "permission modify", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission modify rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "modified " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }
  int permDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto permID = args->getInt("1", 0, Bitmap::Bits, true);
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_PermDel,
	    fbs::CreatePermID(m_fbb, permID).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_PermDel, -1,
	    "permission delete", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto permUpdAck = static_cast<const fbs::PermUpdAck *>(ack->data());
      if (!permUpdAck->ok()) {
	out << "permission delete rejected\n";
	return executed(1, file, out);
      }
      auto perm = permUpdAck->perm();
      out << "deleted " << perm->id() << ' ' << str(perm->name()) << '\n';
      return executed(0, file, out);
    });
    return 0;
  }

  int keysCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyGet,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyGet,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyGet, fbs::ReqAckData_KeyGet,
	    "key get", out))
	return executed(code, file, out);
      auto keyIDList = static_cast<const fbs::KeyIDList *>(ack->data());
      using namespace Zfb::Load;
      all(keyIDList->list(), [&out](unsigned, auto key_) {
	out << str(key_) << '\n';
      });
      return executed(0, file, out);
    });
    return 0;
  }

  int keyAddCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyAdd,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyAdd,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyAdd, fbs::ReqAckData_KeyAdd,
	    "key add", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto keyUpdAck = static_cast<const fbs::KeyUpdAck *>(ack->data());
      if (!keyUpdAck->ok()) {
	out << "key add rejected\n";
	return executed(1, file, out);
      }
      auto secret_ = bytes(keyUpdAck->key()->secret());
      ZtString secret;
      secret.length(Ztls::Base64::enclen(secret_.length()));
      Ztls::Base64::encode(
	  secret.data(), secret.length(), secret_.data(), secret_.length());
      out << "id: " << str(keyUpdAck->key()->id())
	<< "\nsecret: " << secret << '\n';
      return executed(0, file, out);
    });
    return 0;
  }

  int keyDelCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      auto keyID = args->get("1");
      m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	    fbs::ReqData_KeyDel,
	    fbs::CreateKeyID(m_fbb, str(m_fbb, keyID)).Union()));
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyDel, fbs::ReqAckData_KeyDel,
	    "key delete", out))
	return executed(code, file, out);
      using namespace Zfb::Load;
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "key delete rejected\n";
	return executed(1, file, out);
      }
      out << "key deleted\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int keyClrCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc < 1 || argc > 2) throw ZvCmdUsage{};
    auto seqNo = m_seqNo++;
    using namespace ZvUserDB;
    {
      using namespace Zfb::Save;
      m_fbb.Clear();
      if (argc == 1)
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_OwnKeyClr,
	      fbs::CreateUserID(m_fbb, m_link->userID()).Union()));
      else {
	auto userID = ZuBox<uint64_t>(args->get("1"));
	m_fbb.Finish(fbs::CreateRequest(m_fbb, seqNo,
	      fbs::ReqData_KeyClr,
	      fbs::CreateUserID(m_fbb, userID).Union()));
      }
    }
    m_link->sendUserDB(m_fbb, seqNo, [this, file](const fbs::ReqAck *ack) {
      ZtString out;
      if (int code = filterAck(
	    file, ack, fbs::ReqAckData_OwnKeyClr, fbs::ReqAckData_KeyClr,
	    "key clear", out))
	return executed(code, file, out);
      auto userAck = static_cast<const fbs::UserAck *>(ack->data());
      if (!userAck->ok()) {
	out << "key clear rejected\n";
	return executed(1, file, out);
      }
      out << "keys cleared\n";
      return executed(0, file, out);
    });
    return 0;
  }

  int loadModCmd(FILE *, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    if (argc != 2) throw ZvCmdUsage{};
    ZiModule module;
    ZiModule::Path name = args->get("1", true);
    ZtString e;
    if (module.load(name, false, &e) < 0) {
      out << "failed to load \"" << name << "\": " << ZuMv(e) << '\n';
      return 1;
    }
    ZCmdInitFn initFn = (ZCmdInitFn)module.resolve("ZCmd_plugin", &e);
    if (!initFn) {
      module.unload();
      out << "failed to resolve \"ZCmd_plugin\" in \"" <<
	name << "\": " << ZuMv(e) << '\n';
      return 1;
    }
    (*initFn)(static_cast<ZCmdHost *>(this));
    out << "module \"" << name << "\" loaded\n";
    return 0;
  }

  int telcapCmd(FILE *file, ZvCf *args, ZtString &out) {
    ZuBox<int> argc = args->get("#");
    using namespace ZvTelemetry;
    unsigned interval = args->getInt("interval", 100, 1000000, false, 0);
    bool subscribe = !args->getInt("unsubscribe", 0, 1, false, 0);
    if (!subscribe) {
      if (argc > 1) throw ZvCmdUsage{};
    } else {
      if (argc < 2) throw ZvCmdUsage{};
    }
    ZtArray<ZmAtomic<unsigned>> ok;
    ZtArray<ZuString> filters;
    ZtArray<int> types;
    auto reqNames = fbs::EnumNamesReqType();
    if (argc <= 1 + subscribe) {
      ok.length(ReqType::N);
      filters.length(ok.length());
      types.length(ok.length());
      for (unsigned i = 0; i < ReqType::N; i++) {
	filters[i] = "*";
	types[i] = ReqType::MIN + i;
      }
    } else {
      ok.length(argc - (1 + subscribe));
      filters.length(ok.length());
      types.length(ok.length());
      const auto &colon = ZtStaticRegex(":");
      for (unsigned i = 2; i < (unsigned)argc; i++) {
	auto j = i - 2;
	auto arg = args->get(ZuStringN<24>{} << i);
	ZuString type_;
	ZtRegex::Captures c;
	if (colon.m(arg, c)) {
	  type_ = c[0];
	  filters[j] = c[2];
	} else {
	  type_ = arg;
	  filters[j] = "*";
	}
	types[j] = -1;
	for (unsigned k = ReqType::MIN; k <= ReqType::MAX; k++)
	  if (type_ == reqNames[k]) { types[j] = k; break; }
	if (types[j] < 0) throw ZvCmdUsage{};
      }
    }
    thread_local ZmSemaphore sem;
    for (unsigned i = 0, n = ok.length(); i < n; i++) {
      using namespace Zfb::Save;
      auto seqNo = m_seqNo++;
      m_fbb.Clear();
      m_fbb.Finish(fbs::CreateRequest(m_fbb,
	    seqNo, str(m_fbb, filters[i]), interval,
	    static_cast<fbs::ReqType>(types[i]), subscribe));
      m_link->sendTelReq(m_fbb, seqNo,
	  [ok = &ok[i], sem = &sem](const fbs::ReqAck *ack) {
	    ok->store_(ack->ok());
	    sem->post();
	  });
    }
    for (unsigned i = 0, n = ok.length(); i < n; i++) sem.wait();
    bool allOK = true;
    for (unsigned i = 0, n = ok.length(); i < n; i++)
      if (!ok[i].load_()) {
	out << "telemetry request "
	  << reqNames[types[i]] << ':' << filters[i] << " rejected\n";
	allOK = false;
      }
    if (!allOK) return 1;
    if (subscribe) {
      if (!interval)
	out << "telemetry queried\n";
      else
	out << "telemetry subscribed\n";
    } else
      out << "telemetry unsubscribed\n";
    return 0;
  }

private:
  ZmSemaphore		m_done;

  ZtString		m_prompt;

  ZmRef<Link>		m_link;
  ZvSeqNo		m_seqNo = 0;

  Zfb::IOBuilder	m_fbb;

  int			m_code = 0;
  bool			m_exiting = false;

  ZmRef<ZCmdPlugin>	m_plugin;

  ZvRingParams		m_telRingParams;
  ZuPtr<TelRing>	m_telRing;

  // FIXME - need Zdf in-memory manager (later, file manager)

  ZtString		m_gladePath;
  ZtString		m_stylePath;
  GtkStyleContext	*m_styleContext = nullptr;
  GtkWindow		*m_mainWindow = nullptr;

  int64_t		m_refreshRate;	// in nanoseconds
  ZmTime		m_lastRefresh;
  ZmScheduler::Timer	m_refreshTimer;

  Tree::View		m_view;
  Tree::Model		*m_model;
};

int main(int argc, char **argv)
{
  if (argc < 2) usage();

  ZeLog::init("zcmd");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::lambdaSink(
	[](ZeEvent *e) { std::cerr << e->message() << '\n' << std::flush; }));
  ZeLog::start();

  auto keyID = ::getenv("ZCMD_KEY_ID");
  auto secret = ::getenv("ZCMD_KEY_SECRET");
  ZtString user, passwd, server;
  ZuBox<unsigned> totp;
  ZuBox<unsigned> port;

  try {
    {
      const auto &remote = ZtStaticRegex("^([^@]+)@([^:]+):(\\d+)$");
      ZtRegex::Captures c;
      if (remote.m(argv[1], c) == 4) {
	user = c[2];
	server = c[3];
	port = c[4];
      }
    }
    if (!user) {
      const auto &local = ZtStaticRegex("^([^@]+)@(\\d+)$");
      ZtRegex::Captures c;
      if (local.m(argv[1], c) == 3) {
	user = c[2];
	server = "localhost";
	port = c[3];
      }
    }
    if (!user) {
      const auto &remote = ZtStaticRegex("^([^:]+):(\\d+)$");
      ZtRegex::Captures c;
      if (remote.m(argv[1], c) == 3) {
	server = c[2];
	port = c[3];
      }
    }
    if (!server) {
      const auto &local = ZtStaticRegex("^(\\d+)$");
      ZtRegex::Captures c;
      if (local.m(argv[1], c) == 2) {
	server = "localhost";
	port = c[2];
      }
    }
  } catch (const ZtRegexError &) {
    usage();
  }
  if (!server || !*port || !port) usage();
  if (user)
    keyID = secret = nullptr;
  else if (!keyID) {
    std::cerr << "set ZCMD_KEY_ID and ZCMD_KEY_SECRET "
      "to use without username\n" << std::flush;
    ::exit(1);
  }
  if (keyID) {
    if (!secret) {
      std::cerr << "set ZCMD_KEY_SECRET "
	"to use with ZCMD_KEY_ID\n" << std::flush;
      ::exit(1);
    }
  } else {
    passwd = getpass_("password: ", 100);
    totp = getpass_("totp: ", 6);
  }

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

  ZmTrap::sigintFn(ZmFn<>{app, [](ZDash *app) { app->post(); }});
  ZmTrap::trap();

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

  app->target(argv[1]);

  if (keyID)
    app->access(server, port, keyID, secret);
  else
    app->login(server, port, user, passwd, totp);

  app->wait();

  Zrl::stop();
  std::cout << std::flush;

  app->final();

  mx->stop(true);

  ZeLog::stop();

  delete mx;

  ZmTrap::sigintFn(ZmFn<>{});

  return app->code();
}
