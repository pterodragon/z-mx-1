#include <MxEngine.hpp>

// MxEngine connectivity framework unit smoke test

class Mgr : public MxEngineMgr {
public:
  virtual ~Mgr() { }

  // Engine Management
  void addEngine(MxEngine *) { }
  void delEngine(MxEngine *) { }
  void engineState(MxEngine *, MxEnum) { }

  // Link Management
  void updateLink(MxAnyLink *) { }
  void delLink(MxAnyLink *) { }
  void linkState(MxAnyLink *, MxEnum, ZuString txt) { }

  // Pool Management
  void updateTxPool(MxAnyTxPool *) { }
  void delTxPool(MxAnyTxPool *) { }

  // Queue Management
  void addQueue(MxID id, bool tx, MxQueue *) { }
  void delQueue(MxID id, bool tx) { }

  // Exception handling
  void exception(ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

  // Traffic Logging
  void log(MxMsgID, MxTraffic) { }
};

class App : public MxEngineApp {
public:
  inline App() { }
  virtual ~App() { }

  inline void final() { }

  // Rx
  ZuInline void process_(MxAnyLink *, MxQMsg *) { }
  ProcessFn processFn() {
    return [](MxEngineApp *self, MxAnyLink *link, MxQMsg *msg) {
      static_cast<App *>(self)->process_(link, msg);
    };
  }

  // Tx
  void sent(MxAnyLink *, MxQMsg *) { }
  void aborted(MxAnyLink *, MxQMsg *) { }
  void archive(MxAnyLink *, MxQMsg *) { }
  ZmRef<MxQMsg> retrieve(MxAnyLink *, MxSeqNo) { return 0; }
};

class Engine : public MxEngine {
public:
  // inline Engine() { }

  void init(Mgr *mgr, App *app, Mx *mx, ZvCf *cf);

  void up() { std::cerr << "up\n"; }
  void down() { std::cerr << "down\n"; }

  ZmTime reconnectInterval() { return ZmTime(m_reconnectInterval); }
  ZmTime reRequestInterval() { return ZmTime(m_reRequestInterval); }

  void connected() { m_connected.post(); }
  void waitConnected() { m_connected.wait(); }
  void disconnected() { m_disconnected.post(); }
  void waitDisconnected() { m_disconnected.wait(); }

private:
  ZuBox<double>	m_reconnectInterval;
  ZuBox<double>	m_reRequestInterval;
  
  ZmSemaphore	m_connected;
  ZmSemaphore	m_disconnected;
};

class Link : public MxLink<Link> {
public:
  Link(MxID id) : MxLink<Link>(id) { }

  ZuInline Engine *engine() {
    return static_cast<Engine *>(MxAnyLink::engine()); // actually MxAnyTx
  }

  ZmTime reconnectInterval(unsigned) { return engine()->reconnectInterval(); }
  ZmTime reRequestInterval() { return engine()->reRequestInterval(); }

  void update(ZvCf *cf) { }
  void reset(MxSeqNo rxSeqNo, MxSeqNo txSeqNo) { }

#define linkINFO(code) \
    engine()->appException(ZeEVENT(Info, \
      ([=, id = id()](const ZeEvent &, ZmStream &out) { out << code; })))
  void connect() {
    linkINFO("connect(): " << id);
    connected();
    engine()->connected();
  }
  void disconnect() {
    linkINFO("disconnect(): " << id);
    disconnected();
    engine()->disconnected();
  }

  // Rx
  void request(const MxQueue::Gap &prev, const MxQueue::Gap &now) { }
  void reRequest(const MxQueue::Gap &now) { }

  // Tx
  bool send_(MxQMsg *msg, bool more) {
    // if a msg deadline has passed, could call aborted_() and return false
    this->sent_(msg);
    return true;
  }
  bool resend_(MxQMsg *msg, bool more) { return true; }
  void abort_(MxQMsg *msg) { }

  // void loaded_(MxQMsg *msg) { }
  // void unloaded_(MxQMsg *msg) { }

  bool sendGap_(const MxQueue::Gap &gap, bool more) { return true; }
  bool resendGap_(const MxQueue::Gap &gap, bool more) { return true; }
};

void Engine::init(Mgr *mgr, App *app, Mx *mx, ZvCf *cf)
{
  MxEngine::init(mgr, app, mx, cf);
  m_reconnectInterval = cf->getDbl("reconnectInterval", 0, 3600, false, 1);
  m_reRequestInterval = cf->getDbl("reRequestInterval", 0, 3600, false, 1);
  if (ZmRef<ZvCf> linksCf = cf->subset("links", false)) {
    ZvCf::Iterator i(linksCf);
    ZuString id;
    while (ZmRef<ZvCf> linkCf = i.subset(id))
      MxEngine::updateLink<Link>(id, linkCf);
  }
}

int main()
{
  ZeLog::init("MxEngineTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  ZmRef<ZvCf> cf = new ZvCf();
  cf->fromString(
      "id Engine\n"
      "mx {\n"
	"nThreads 4\n"		// thread IDs are 1-based
	"rxThread 1\n"		// I/O Rx
	"txThread 2\n"		// I/O Tx
	"isolation 1-3\n"	// leave thread 4 for general purpose
      "}\n"
      "rxThread 3\n"		// App Rx
      "txThread 2\n"		// App Tx (same as I/O Tx)
      "links { link1 { } }\n",
      false);

  App* app = new App();
  Mgr* mgr = new Mgr();
  ZmRef<Engine> engine = new Engine();

  ZmRef<MxMultiplex> mx = new MxMultiplex("mx", cf->subset("mx", true));

  engine->init(mgr, app, mx, cf);

  mx->start();
  engine->start();

  engine->waitConnected();

  engine->stop();

  engine->waitDisconnected();

  mx->stop(true);

  engine = 0;
  app->final();
  delete mgr;
  delete app;
}
