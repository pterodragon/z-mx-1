//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

#include <ZuLib.hpp>

#include <limits.h>

#include <ZmTrap.hpp>
#include <ZmRandom.hpp>

#include <ZeLog.hpp>

#include <ZvCf.hpp>

#include <Zdb.hpp>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

enum Side { Buy, Sell };

struct Order {
  Side		m_side;
  char		m_symbol[32];
  int		m_price;
  int		m_quantity;
  // char	m_pad[1500];
  template <typename S> inline void print(S &s) const {
    s << "Side: " << (m_side == Buy ? "Buy" : "Sell") <<
      " Symbol: " << m_symbol <<
      " Price: " << ZuBoxed(m_price) <<
      " Qty: " << ZuBoxed(m_quantity);
  }
};
template <> struct ZuPrint<Order> : public ZuPrintFn { };

typedef Zdb<Order> OrderDB;

static void deleted(const char *prefix, ZdbRN rn, ZdbRN prev)
{
  std::cout << prefix << " RN: " << ZuBoxed(rn) <<
    " prev: " << ZuBoxed(prev) << "  DELETED\n" << std::flush;
}

static void dump(const char *prefix, ZdbRN rn, ZdbRN prev, Order *o)
{
  std::cout << prefix << " RN: " << ZuBoxed(rn) <<
    " prev: " << ZuBoxed(prev) << "  " << *o << '\n' << std::flush;
}

ZmRef<OrderDB> orders;
ZmSemaphore done;
ZmScheduler *appMx = 0;
ZiMultiplex *dbMx = 0;
unsigned del = 0;
unsigned append = 0;
unsigned nThreads = 1;
ZdbRN maxRN;

void sigint()
{
  std::cerr << "SIGINT\n" << std::flush;
  done.post();
}

inline ZmRef<ZvCf> inlineCf(ZuString s)
{
  ZmRef<ZvCf> cf = new ZvCf();
  cf->fromString(s, false);
  return cf;
}

void push() {
  if (orders->allocRN() >= maxRN) return;
  ZmRef<ZdbPOD<Order> > pod;
  if (append) {
    ZdbRN rn = append == 1 ? 0 : ZmRand::randInt(append - 1);
    pod = orders->get_(rn);
    pod = orders->update(pod, orders->allocRN());
  } else {
    pod = orders->push(orders->allocRN());
  }
  if (!pod) return;
  Order *order = pod->ptr();
  order->m_side = Buy;
  strncpy(order->m_symbol, "IBM", 32);
  order->m_price = 100;
  order->m_quantity = 100;
  if (!append)
    orders->put(ZdbPOD<Order>::pod(order));
  else {
    orders->putUpdate(pod);
    if (orders->allocRN() >= maxRN) return;
    if (pod = orders->get(pod->rn()))
      orders->update(pod, orders->allocRN());
  }
  appMx->add(ZmFn<>::Ptr<&push>::fn());
}

void active() {
  puts("ACTIVE");
  if (del) {
    for (unsigned i = 0; i < del; i++)
      if (ZmRef<ZdbPOD<Order> > pod = orders->get_(i))
	orders->del(pod, orders->allocRN());
  }
  if (append) {
    for (unsigned i = 0; i < append; i++) {
      if (orders->allocRN() >= maxRN) return;
      ZmRef<ZdbPOD<Order> > pod = orders->push(orders->allocRN());
      if (!pod) return;
      Order *order = pod->ptr();
      order->m_side = Buy;
      strcpy(order->m_symbol, "IBM");
      order->m_price = 100;
      order->m_quantity = 100;
      orders->put(pod);
    }
  }
  for (unsigned i = 0; i < nThreads; i++) appMx->add(ZmFn<>::Ptr<&push>::fn());
}

void inactive() { puts("INACTIVE"); }

void usage()
{
  static const char *help =
    "usage: ZdbTest [OPTIONS] nThreads maxRN\n\n"
    "Options:\n"
    "  -D, --del=N\t\t- delete first N sequences\n"
    "  -a, --append=N\t\t- append to N sequences\n"
    "  -s, --dbs:0:fileSize=N\t- file size\n"
    "  -f, --dbs:0:path=PATH\t\t- file path\n"
    "  -p, --dbs:0:preAlloc=N\t- number of records to pre-allocate\n"
    "  -h, --hostID=N\t\t- host ID\n"
    "  -H, --hashOut=FILE\t\t- hash table CSV output file\n"
    "  -d\t\t\t\t- enable debug logging\n"
    "  --nAccepts=N\t\t\t- number of concurrent accepts to listen for\n"
    "  --heartbeatFreq=N\t\t- heartbeat frequency in seconds\n"
    "  --heartbeatTimeout=N\t\t- heartbeat timeout in seconds\n"
    "  --reconnectFreq=N\t\t- reconnect frequency in seconds\n"
    "  --electionTimeout=N\t\t- election timeout in seconds\n"
    "  --dbs:0:cache:bits=N\t\t- bits for cache\n"
    "  --dbs:0:cache:loadFactor=N\t- load factor for cache\n"
    "  --dbs:0:fileHash:bits=N\t- bits for file hash table\n"
    "  --dbs:0:fileHash:loadFactor=N - load factor for file hash table\n"
    "  --dbs:0:fileHash:cBits=N\t- concurrency bits for file hash table\n"
    "  --dbs:0:indexHash:bits=N\t- bits for index hash table\n"
    "  --dbs:0:indexHash:loadFactor=N - load factor for index hash table\n"
    "  --dbs:0:indexHash:cBits=N\t- concurrency bits for index hash table\n"
    "  --dbs:0:lockHash:bits=N\t- bits for lock hash table\n"
    "  --dbs:0:lockHash:loadFactor=N\t- load factor for lock hash table\n"
    "  --hosts:0:priority=N\t\t- host 0 priority\n"
    "  --hosts:0:IP=N\t\t- host 0 IP\n"
    "  --hosts:0:port=N\t\t- host 0 port\n"
    "  --hosts:0:up=CMD\t\t- host 0 up command\n"
    "  --hosts:0:down=CMD\t\t- host 0 down command\n"
    "  --hosts:1:priority=N\t\t- host 1 priority\n"
    "  --hosts:1:IP=N\t\t- host 1 IP\n"
    "  --hosts:1:port=N\t\t- host 1 port\n"
    "  --hosts:1:up=CMD\t\t- host 1 up command\n"
    "  --hosts:1:down=CMD\t\t- host 1 down command\n"
    "  --hosts:2:priority=N\t\t- host 2 priority\n"
    "  --hosts:2:IP=N\t\t- host 2 IP\n"
    "  --hosts:2:port=N\t\t- host 2 port\n"
    "  --hosts:2:up=CMD\t\t- host 2 up command\n"
    "  --hosts:2:down=CMD\t\t- host 2 down command\n"
    "  --cxnHash:bits=N\t\t- bits for cxn hash table\n"
    "  --cxnHash:loadFactor=N\t- load factor for cxn hash table\n"
    "  --cxnHash:cBits=N\t\t- concurrency bits for cxn hash table\n";

  std::cerr << help << std::flush;
  ZmPlatform::exit(1);
}

int main(int argc, char **argv)
{
  static ZvOpt opts[] = {
    { "del", "D", ZvOptScalar },
    { "append", "a", ZvOptScalar },
    { "dbs:0:fileSize", "s", ZvOptScalar, "4096" },
    { "dbs:0:path", "f", ZvOptScalar, "orders" },
    { "dbs:0:preAlloc", "p", ZvOptScalar, "1" },
    { "dbs:0:cache:bits", 0, ZvOptScalar, "8" },
    { "dbs:0:cache:loadFactor", 0, ZvOptScalar, "1.0" },
    { "dbs:0:fileHash:bits", 0, ZvOptScalar, "8" },
    { "dbs:0:fileHash:loadFactor", 0, ZvOptScalar, "1.0" },
    { "dbs:0:fileHash:cBits", 0, ZvOptScalar, "5" },
    { "dbs:0:indexHash:bits", 0, ZvOptScalar, "8" },
    { "dbs:0:indexHash:loadFactor", 0, ZvOptScalar, "1.0" },
    { "dbs:0:indexHash:cBits", 0, ZvOptScalar, "5" },
    { "dbs:0:lockHash:bits", 0, ZvOptScalar, "8" },
    { "dbs:0:lockHash:loadFactor", 0, ZvOptScalar, "1.0" },
    { "cxnHash:bits", 0, ZvOptScalar, "5" },
    { "cxnHash:loadFactor", 0, ZvOptScalar, "1.0" },
    { "cxnHash:cBits", 0, ZvOptScalar, "5" },
    { "hostID", "h", ZvOptScalar, "0" },
    { "hashOut", "H", ZvOptScalar },
    { "debug", "d", ZvOptFlag },
    { "hosts:0:priority", 0, ZvOptScalar, "100" },
    { "hosts:0:IP", 0, ZvOptScalar, "127.0.0.1" },
    { "hosts:0:port", 0, ZvOptScalar, "9943" },
    { "hosts:0:up", 0, ZvOptScalar },
    { "hosts:0:down", 0, ZvOptScalar },
    { "hosts:1:priority", 0, ZvOptScalar },
    { "hosts:1:IP", 0, ZvOptScalar },
    { "hosts:1:port", 0, ZvOptScalar },
    { "hosts:1:up", 0, ZvOptScalar },
    { "hosts:1:down", 0, ZvOptScalar },
    { "hosts:2:priority", 0, ZvOptScalar },
    { "hosts:2:IP", 0, ZvOptScalar },
    { "hosts:2:port", 0, ZvOptScalar },
    { "hosts:2:up", 0, ZvOptScalar },
    { "hosts:2:down", 0, ZvOptScalar },
    { "nAccepts", 0, ZvOptScalar },
    { "heartbeatFreq", 0, ZvOptScalar },
    { "heartbeatTimeout", 0, ZvOptScalar },
    { "reconnectFreq", 0, ZvOptScalar },
    { "electionTimeout", 0, ZvOptScalar },
    { 0 }
  };

  ZmRef<ZvCf> cf;
  ZuString hashOut;

  try {
    cf = inlineCf(
      "writeThread 3\n"
      "hostID 0\n"
      // "hosts { 0 { priority 100 IP 127.0.0.1 port 9943 } }\n"
      "hosts { 1 { priority 75 IP 127.0.0.1 port 9944 } }\n"
      "hosts { 2 { priority 50 IP 127.0.0.1 port 9945 } }\n"
      "dbs { 0 { fileSize 4096 path orders preAlloc 0 } }\n"
    );

    if (cf->fromArgs(opts, argc, argv) != 3) usage();

    del = cf->getInt("del", 1, INT_MAX, false, 0);
    append = cf->getInt("append", 1, INT_MAX, false, 0);
    nThreads = cf->getInt("1", 1, 1<<10, true);
    maxRN = cf->getInt("2", 0, 1<<20, true);
    hashOut = cf->get("hashOut");

  } catch (const ZvError &e) {
    std::cerr << e << '\n' << std::flush;
    usage();
  } catch (const ZeError &e) {
    std::cerr << e << '\n' << std::flush;
    usage();
  } catch (...) {
    usage();
  }

  ZeLog::init("ZdbTest");
  ZeLog::level(0);
  ZeLog::add(ZeLog::debugSink());
  ZeLog::start();

  ZmTrap::sigintFn(ZmFn<>::Ptr<&sigint>::fn());
  ZmTrap::trap();

  try {
    ZeError e;

    {
      appMx = new ZmScheduler(ZmSchedParams().nThreads(nThreads));
      dbMx = new ZiMultiplex(
	  ZiMxParams().nThreads(4).rxThread(1).txThread(2).
	    isolation(ZmBitmap().set(1).set(2).set(3))
#ifdef ZiMultiplex_DEBUG
	    .debug(cf->getInt("debug", 0, 1, false, 0))
#endif
	    );
    }

    appMx->start();
    if (dbMx->start() != Zi::OK) throw ZtString("multiplexer start failed");

    ZuRef<ZdbEnv> env = new ZdbEnv(0);

    env->init(ZdbEnvConfig(cf),
      dbMx, ZmFn<>::Ptr<&active>::fn(), ZmFn<>::Ptr<&inactive>::fn());

    orders = new OrderDB(env, 0, 0, ZdbHandler{
	  [](ZdbAny *db, ZmRef<ZdbAnyPOD> &pod) {
	    pod = new ZdbPOD<Order>(db);
	    // new (pod->ptr()) Order();
	  },
	  [](ZdbAnyPOD *pod, bool recovered) {
	    dump(recovered ? "recovered" : "replicated (add)",
		pod->rn(), pod->prevRN(), (Order *)pod->ptr());
	  },
	  [](ZdbAnyPOD *pod) {
	    dump("replicated (del)",
		pod->rn(), pod->prevRN(), (Order *)pod->ptr());
	  },
	  [](ZdbAnyPOD *pod, ZdbRange, int op) {
	    if (op == ZdbOp::Delete)
	      deleted("DC delete", pod->rn(), pod->prevRN());
	    else if (op == ZdbOp::Update)
	      dump("DC update", pod->rn(), pod->prevRN(), (Order *)pod->ptr());
	    else
	      dump("DC new", pod->rn(), pod->prevRN(), (Order *)pod->ptr());
	  }
	});

    if (!env->open()) throw ZtString() << "Zdb open failed";
    env->start();

    done.wait();

    env->checkpoint();

    env->stop();
    env->close();

    appMx->stop();
    dbMx->stop(true);

    env->final();

  } catch (const ZvError &e) {
    std::cerr << e << '\n' << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  } catch (const ZeError &e) {
    std::cerr << e << '\n' << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  } catch (const ZtString &s) {
    std::cerr << s << '\n' << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  } catch (...) {
    std::cerr << "Unknown Exception\n" << std::flush;
    ZeLog::stop();
    ZmPlatform::exit(1);
  }

  if (appMx) delete appMx;
  if (dbMx) delete dbMx;

  ZeLog::stop();

  if (hashOut) {
    FILE *f = fopen(hashOut, "w");
    if (!f)
      perror(hashOut);
    else {
      ZtString csv;
      csv << ZmHashMgr::csv();
      fwrite(csv.data(), 1, csv.length(), f);
    }
    fclose(f);
  }

  return 0;
}
