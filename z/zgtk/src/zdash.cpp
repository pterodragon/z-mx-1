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

#include <zlib/Zdf.hpp>

#include <zlib/ZGtkApp.hpp>
#include <zlib/ZGtkCallback.hpp>
#include <zlib/ZGtkTreeModel.hpp>
#include <zlib/ZGtkValue.hpp>

// FIXME - css
//
// @define-color rag_red_bg #ff1515;

// FIXME
// each node needs to include:
// index row (in left-hand tree)
// watch window and row (in watchlist)
// array of time series

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
	ZmRBTreeUnique<true,
	  ZmRBTreeLock<ZmNoLock> > > >;

  template <unsigned Depth, typename Data>
  struct Child : public Data, public ZGtk::TreeNode::Child<Depth> {
    Child() = default;
    template <typename ...Args>
    Child(Args &&... args) : Data{ZuFwd<Args>(args)...} { }
  };

  template <
    unsigned Depth, typename Data, typename Child, typename Tree>
  struct Parent :
      public Data,
      public ZGtk::TreeNode::Parent<Depth, Child> {
    Parent() = default;
    template <typename ...Args>
    Parent(Args &&... args) : Data{ZuFwd<Args>(args)...} { }

    Tree	tree;
  };

  template <unsigned Depth, typename Data, typename Tuple>
  struct Branch :
      public Data,
      public ZGtk::TreeNode::Branch<Depth, Tuple> {
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
  struct AppData : public ZvTelemetry::App_load { Link *link = nullptr; };
  using App = Branch<0, AppData, AppTuple>;

  using Root = ZGtk::TreeNode::Parent<0, App, ZGtk::TreeNode::Root>;

  enum { MaxDepth = 3 };

  // (*) - branch
  using Iter = ZuUnion<
    App *,		// [app]

    // app children
    HeapParent *,	// app->heaps (*)
    HashTblParent *,	// app->hashTbls (*)
    ThreadParent *,	// app->threads (*)
    MxParent *,		// app->mxTbl (*)
    EngineParent *,	// app->engines (*)
    DBEnv *,		// app->dbEnv (*)

    // app grandchildren
    Heap *,		// app->heaps->[heap]
    HashTbl *,		// app->hashTbls->[hashTbl]
    Thread *,		// app->threads->[thread]
    Mx *,		// app->mxTbl->[mx]
    Engine *,		// app->engines->[engine]

    // app great-grandchildren
    Socket *,		// app->mxTbl->mx->[socket]
    Link *,		// app->engines->engine->[link]

    // DBEnv children
    DBHostParent *,	// app->dbEnv->hosts (*)
    DBParent *,		// app->dbEnv->dbs (*)

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

    void final() {
      g_object_unref(G_OBJECT(m_treeView));
    }

    void bind(GtkTreeModel *model) {
      gtk_tree_view_set_model(m_treeView, GTK_TREE_MODEL(model));

      g_object_unref(G_OBJECT(model));
    }

  private:
    GtkTreeView	*m_treeView = nullptr;
    GdkRGBA	m_rag_red_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_red_bg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_amber_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_amber_bg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_green_fg = { 0.0, 0.0, 0.0, 0.0 };
    GdkRGBA	m_rag_green_bg = { 0.0, 0.0, 0.0, 0.0 };
  };
}

class ZDash :
    public ZmPolymorph,
    public ZvCmdClient<ZDash>,
    public ZCmdHost,
    public ZGtk::App {
public:
  using Base = ZvCmdClient<ZDash>;

  // FIXME - use ZGtk::TreeArray<> to define all watchlists
  // FIXME - figure out how to save/load tree view columns, sort, etc.
  // - need View class that with a create column(col) with data func that
  //   calls gtk_tree_model_get_value(model, iter, col, value)
  // - load() loads sequcence of visible columns and calls
  //   append_column for each (also persists column ordering/visibility)
  //   - load() uses all columns in order as default
  //   - may want to prevent loading an outdated column schema
  // - save() persists column order (sequence of column IDs)

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

  void init(ZiMultiplex *mx, ZvCf *cf) {
    m_gladePath = cf->get("gtkGlade", true);
    m_stylePath = cf->get("gtkStyle");
    unsigned tid = cf->getInt("gtkThread", 1, mx->nThreads(), true);

    Base::init(mx, cf);
    ZvCmdHost::init();
    initCmds();

    attach(mx, tid);
    mx->run(tid, [this]() { gtkInit(); });
  }

  void final() {
    // m_link = nullptr;

    detach([this]() {
      ZvCmdHost::final();
      Base::final();
    });
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
      gtkPost();
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
      gtk_style_context_add_provider(m_styleContext, provider, G_MAXUINT);
      g_object_unref(G_OBJECT(provider));
    }

    m_view.init(view_, m_styleContext);
    m_view.bind(m_model = MainTree::Model::ctor());

    g_signal_connect(G_OBJECT(m_mainWindow), "destroy",
	ZGtk::callback([](GObject *o, gpointer this_) {
	  reinterpret_cast<ZDash *>(this_)->gtkFinal();
	}), reinterpret_cast<gpointer>(this));

    gtk_widget_show_all(GTK_WIDGET(m_mainWindow));

    gtk_window_present(m_mainWindow);
  }

  void gtkFinal() {
    m_view.final();
    if (m_mainWindow) g_object_unref(G_OBJECT(m_mainWindow));
    if (m_styleContext) g_object_unref(G_OBJECT(m_styleContext));
    post();
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

  void exiting() { m_exiting = true; }

  void plugin(ZmRef<ZCmdPlugin> plugin) {
    if (ZuUnlikely(m_plugin)) m_plugin->final();
    m_plugin = ZuMv(plugin);
  }
  void sendApp(Zfb::IOBuilder &fbb) { return m_link->sendApp(fbb); }

private:
  void loggedIn() {
    if (auto plugin = ::getenv("ZCMD_PLUGIN")) {
      ZmRef<ZvCf> args = new ZvCf();
      args->set("1", plugin);
      args->set("#", "2");
      ZtString out;
      loadModCmd(stdout, args, out);
      fwrite(out.data(), 1, out.length(), stdout);
    }
    if (m_solo) {
      send(ZuMv(m_soloMsg));
    } else {
      if (m_interactive)
	std::cout <<
	  "For a list of valid commands: help\n"
	  "For help on a particular command: COMMAND --help\n" << std::flush;
      prompt();
    }
  }

  int processApp(Link *, ZuArray<const uint8_t> data) {
    if (ZuUnlikely(!m_plugin)) return -1;
    return m_plugin->processApp(data);
  }

  enum {
    ReqTypeN = ZvTelemetry::ReqType::N - ZvTelemetry::ReqType::MIN,
    TelDataN = ZvTelemetry::TelData::N - ZvTelemetry::TelData::MIN
  };

  int processTel(Link *link, const ZvTelemetry::fbs::Telemetry *data) {
    // FIXME
    using namespace ZvTelemetry;
    int i = data->data_type();
    if (ZuUnlikely(i < TelData::MIN)) return 0;
    i -= TelData::MIN;
    if (ZuUnlikely(i >= TelDataN)) return 0;
    // FIXME - find link in tree, create skeleton app if not already present
    ZuSwitch::dispatch<TelDataN>([this](auto i) {
      // FIXME - duspatch i to FBS type
      // data->data() is const void *fbs_;
      // auto fbs = static_cast<const FBS *>(fbs_);
      // FBS is e.g. fbs::Socket
      // then e.g. new Socket_load{fbs}, or node->loadDelta(fbs)
      // need to update time series
      process;
    });
    m_telcap[i](data->data());
    return 0;
  }

  void disconnected() {
    if (m_exiting) return;
    Zrl::stop();
    std::cerr << "server disconnected\n" << std::flush;
    ZmPlatform::exit(1);
  }
  void connectFailed() {
    Zrl::stop();
    std::cerr << "connect failed\n" << std::flush;
    ZmPlatform::exit(1);
  }

  void prompt() {
    mx()->add(ZmFn<>{this, [](ZCmd *app) { app->prompt_(); }});
  }
  void prompt_() {
    ZtString cmd;
next:
    try {
      cmd = Zrl::readline_(m_prompt);
    } catch (const Zrl::EndOfFile &) {
      post(); // FIXME
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
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->passwdCmd(static_cast<FILE *>(file), args, out);
	}},  "change passwd", "usage: passwd");

    addCmd("users", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->usersCmd(static_cast<FILE *>(file), args, out);
	}},  "list users", "usage: users");
    addCmd("useradd",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add user",
	"usage: useradd ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("resetpass", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->resetPassCmd(static_cast<FILE *>(file), args, out);
	}},  "reset password", "usage: resetpass USERID");
    addCmd("usermod",
	"e enabled enabled { type flag } "
	"i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify user",
	"usage: usermod ID NAME ROLE[,ROLE,...] [OPTIONS...]\n\n"
	"Options:\n"
	"  -e, --enabled\t\tset Enabled flag\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("userdel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->userDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete user", "usage: userdel ID");

    addCmd("roles", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->rolesCmd(static_cast<FILE *>(file), args, out);
	}},  "list roles", "usage: roles");
    addCmd("roleadd", "i immutable immutable { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add role",
	"usage: roleadd NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("rolemod", "i immutable immutable { type scalar }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify role",
	"usage: rolemod NAME PERMS APIPERMS [OPTIONS...]\n\n"
	"Options:\n"
	"  -i, --immutable\tset Immutable flag\n");
    addCmd("roledel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->roleDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete role",
	"usage: roledel NAME");

    addCmd("perms", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permsCmd(static_cast<FILE *>(file), args, out);
	}},  "list permissions", "usage: perms");
    addCmd("permadd", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add permission", "usage: permadd NAME");
    addCmd("permmod", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permModCmd(static_cast<FILE *>(file), args, out);
	}},  "modify permission", "usage: permmod ID NAME");
    addCmd("permdel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->permDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete permission", "usage: permdel ID");

    addCmd("keys", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keysCmd(static_cast<FILE *>(file), args, out);
	}},  "list keys", "usage: keys [USERID]");
    addCmd("keyadd", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyAddCmd(static_cast<FILE *>(file), args, out);
	}},  "add key", "usage: keyadd [USERID]");
    addCmd("keydel", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyDelCmd(static_cast<FILE *>(file), args, out);
	}},  "delete key", "usage: keydel ID");
    addCmd("keyclr", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->keyClrCmd(static_cast<FILE *>(file), args, out);
	}},  "clear all keys", "usage: keyclr [USERID]");

    addCmd("loadmod", "",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
	  return app->loadModCmd(static_cast<FILE *>(file), args, out);
	}},  "load application-specific module", "usage: loadmod MODULE");

    addCmd("telcap",
	"i interval interval { type scalar } "
	"u unsubscribe unsubscribe { type flag }",
	ZvCmdFn{this, [](ZCmd *app, void *file, ZvCf *args, ZtString &out) {
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
      for (unsigned i = 0; i < TelDataN; i++) m_telcap[i] = TelCap{};
      if (argc > 1) throw ZvCmdUsage{};
    } else {
      if (argc < 2) throw ZvCmdUsage{};
    }

    // FIXME from here - add telemetry on-demand into m_model
    // using TreeHierarchy::add(...)

    ZtArray<ZmAtomic<unsigned>> ok;
    ZtArray<ZuString> filters;
    ZtArray<int> types;
    auto reqNames = fbs::EnumNamesReqType();
    if (argc <= 1 + subscribe) {
      ok.length(ReqTypeN);
      filters.length(ok.length());
      types.length(ok.length());
      for (unsigned i = 0; i < ReqTypeN; i++) {
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
    if (subscribe) {
      auto dir = args->get("1");
      ZiFile::age(dir, 10);
      {
	ZeError e;
	if (ZiFile::mkdir(dir, &e) != Zi::OK) {
	  out << dir << ": " << e << '\n';
	  return 1;
	}
      }
      for (unsigned i = 0, n = ok.length(); i < n; i++) {
	try {
	  switch (types[i]) {
	    case ReqType::Heap:
	      m_telcap[TelData::Heap - TelData::MIN] =
		TelCap::keyedFn<Heap_load, fbs::Heap>(
		    ZiFile::append(dir, "heap.csv"));
	      break;
	    case ReqType::HashTbl:
	      m_telcap[TelData::HashTbl - TelData::MIN] =
		TelCap::keyedFn<HashTbl_load, fbs::HashTbl>(
		    ZiFile::append(dir, "hash.csv"));
	      break;
	    case ReqType::Thread:
	      m_telcap[TelData::Thread - TelData::MIN] =
		TelCap::keyedFn<Thread_load, fbs::Thread>(
		    ZiFile::append(dir, "thread.csv"));
	      break;
	    case ReqType::Mx:
	      m_telcap[TelData::Mx - TelData::MIN] =
		TelCap::keyedFn<Mx_load, fbs::Mx>(
		    ZiFile::append(dir, "mx.csv"));
	      m_telcap[TelData::Socket - TelData::MIN] =
		TelCap::keyedFn<Socket_load, fbs::Socket>(
		    ZiFile::append(dir, "socket.csv"));
	      break;
	    case ReqType::Queue:
	      m_telcap[TelData::Queue - TelData::MIN] =
		TelCap::keyedFn<Queue_load, fbs::Queue>(
		    ZiFile::append(dir, "queue.csv"));
	      break;
	    case ReqType::Engine:
	      m_telcap[TelData::Engine - TelData::MIN] =
		TelCap::keyedFn<Engine_load, fbs::Engine>(
		    ZiFile::append(dir, "engine.csv"));
	      m_telcap[TelData::Link - TelData::MIN] =
		TelCap::keyedFn<Link_load, fbs::Link>(
		    ZiFile::append(dir, "link.csv"));
	      break;
	    case ReqType::DBEnv:
	      m_telcap[TelData::DBEnv - TelData::MIN] =
		TelCap::singletonFn<DBEnv_load, fbs::DBEnv>(
		    ZiFile::append(dir, "dbenv.csv"));
	      m_telcap[TelData::DBHost - TelData::MIN] =
		TelCap::keyedFn<DBHost_load, fbs::DBHost>(
		    ZiFile::append(dir, "dbhost.csv"));
	      m_telcap[TelData::DB - TelData::MIN] =
		TelCap::keyedFn<DB_load, fbs::DB>(
		    ZiFile::append(dir, "db.csv"));
	      break;
	    case ReqType::App:
	      m_telcap[TelData::App - TelData::MIN] =
		TelCap::singletonFn<App_load, fbs::App>(
		    ZiFile::append(dir, "app.csv"));
	      break;
	    case ReqType::Alert:
	      m_telcap[TelData::Alert - TelData::MIN] =
		TelCap::alertFn<Alert_load, fbs::Alert>(
		    ZiFile::append(dir, "alert.csv"));
	      break;
	  }
	} catch (const ZvError &e) {
	  out << e << '\n';
	  return 1;
	}
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

  ZtString		m_gladePath;
  ZtString		m_stylePath;
  GtkStyleContext	*m_styleContext = nullptr;
  GtkWindow		*m_mainWindow = nullptr;
  MainTree::Model	*m_model;
  MainTree::View	m_view;
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

  ZmRef<ZDash> client = new ZDash();

  ZmTrap::sigintFn(ZmFn<>{client, [](ZDash *client) { client->post(); }});
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
      client->init(mx, cf);
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

  client->target(argv[1]);

  if (keyID)
    client->access(server, port, keyID, secret);
  else
    client->login(server, port, user, passwd, totp);

  client->wait();

  Zrl::stop();
  std::cout << std::flush;

  client->exiting();
  client->disconnect();

  mx->stop(true);

  ZeLog::stop();

  client->final();

  delete mx;

  ZmTrap::sigintFn(ZmFn<>{});

  return client->code();
}
