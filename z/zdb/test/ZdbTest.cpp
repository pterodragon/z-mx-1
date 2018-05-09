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
  ZtString dump() {
    return ZtSprintf("Side: %s Symbol: %s Price: %d Qty: %d\n",
		     m_side == Buy ? "Buy" : "Sell", m_symbol,
		     m_price, m_quantity);
  }
};

typedef Zdb<Order> OrderDB;

static void dump(const char *prefix, ZdbRN head, ZdbRN rn, Order *o)
{
  printf("%s Head: %d RN: %d %s", prefix, (int)head, (int)rn, o->dump().data());
}

ZmRef<OrderDB> orders;
ZmSemaphore done;
ZmScheduler *appMx = 0;
ZiMultiplex *dbMx = 0;
int append;
unsigned nThreads;
ZdbRN maxRN;

void sigint()
{
  fputs("SIGINT\n", stderr);
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
    ZdbRN head = append == 1 ? 0 : ZmRand::randInt(append - 1);
    pod = orders->push(head);
  } else
    pod = orders->push();
  if (!pod) return;
  Order *order = pod->ptr();
  order->m_side = Buy;
  strncpy(order->m_symbol, "IBM", 32);
  order->m_price = 100;
  order->m_quantity = 100;
  orders->put(pod);
  if (append > 0) {
    if (orders->allocRN() >= maxRN) return;
    ZmRef<ZdbPOD<Order> > pod2 = orders->push(pod->headRN(), false);
    if (!pod2) return;
    memcpy(pod2->ptr(), pod->ptr(), sizeof(Order));
    orders->put(pod2);
  }
  appMx->add(ZmFn<>::Ptr<&push>::fn());
}

void active() {
  puts("ACTIVE");
  if (append) {
    for (int i = 0; i < append; i++) {
      if (orders->allocRN() >= maxRN) return;
      ZmRef<ZdbPOD<Order> > pod = orders->push();
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

void alloc(Zdb_ *db, ZmRef<ZdbAnyPOD> &pod)
{
  pod = new ZdbPOD<Order>(db);
  // new (pod->ptr()) Order();
}

void replicated(ZdbAnyPOD *pod, void *ptr, ZdbRange range, bool update)
{
  dump("replicated", pod->headRN(), pod->rn(), (Order *)ptr);
  memcpy((char*)pod->ptr() + range.off(), ptr, range.len());
}

void recover(ZdbAnyPOD *pod)
{
  dump("recovered", pod->headRN(), pod->rn(), (Order *)pod->ptr());
}

void copy(ZdbAnyPOD *pod, ZdbRange, bool)
{
  dump(ZtSprintf("DC TID=%d", (int)ZmPlatform::getTID()).data(),
       pod->headRN(), pod->rn(), (Order *)pod->ptr());
}

void usage()
{
  static const char *help =
    "usage: ZdbTest [OPTIONS] nThreads maxRN\n\n"
    "Options:\n"
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

  fputs(help, stderr);
  exit(1);
}

int main(int argc, char **argv)
{
  static ZvOpt opts[] = {
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
      "hosts { 0 { priority 100 IP 127.0.0.1 port 9943 } }\n"
      "hosts { 1 { priority 75 IP 127.0.0.1 port 9944 } }\n"
      "hosts { 2 { priority 50 IP 127.0.0.1 port 9945 } }\n"
      "dbs { 0 { fileSize 4096 path orders preAlloc 0 } }\n"
    );

    if (cf->fromArgs(opts, argc, argv) != 3) usage();

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
      appMx = new ZmScheduler(ZmSchedulerParams().nThreads(nThreads));
      dbMx = new ZiMultiplex(
	  ZiMultiplexParams().nThreads(4).rxThread(1).txThread(2).
	    isolation(ZmBitmap() << 1 << 2 << 3)
#ifdef ZiMultiplex_DEBUG
	    .debug(cf->getInt("debug", 0, 1, false, 0))
#endif
	    );
    }

    appMx->start();
    if (dbMx->start() != Zi::OK) throw ZtString("multiplexer start failed");

    ZmRef<ZdbEnv> env = new ZdbEnv(0);

    env->init(ZdbEnvConfig(cf),
      dbMx, ZmFn<>::Ptr<&active>::fn(), ZmFn<>::Ptr<&inactive>::fn());

    orders = new OrderDB(env, 0,
	ZdbAllocFn::Ptr<&alloc>::fn(),
	ZdbRecoverFn::Ptr<&recover>::fn(),
	ZdbReplicateFn::Ptr<&replicated>::fn(),
	ZdbCopyFn::Ptr<&copy>::fn());

    env->open();
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
    exit(1);
  } catch (const ZeError &e) {
    std::cerr << e << '\n' << std::flush;
    ZeLog::stop();
    exit(1);
  } catch (const ZtString &s) {
    std::cerr << s << '\n' << std::flush;
    ZeLog::stop();
    exit(1);
  } catch (...) {
    std::cerr << "Unknown Exception\n" << std::flush;
    ZeLog::stop();
    exit(1);
  }

  if (appMx) delete appMx;
  if (dbMx) delete dbMx;

  ZeLog::stop();

  if (hashOut) {
    ZtString csv;
    ZmHashMgr::csv(csv);
    FILE *f = fopen(hashOut, "w");
    if (!f)
      perror(hashOut);
    else
      fwrite(csv.data(), 1, csv.length(), f);
    fclose(f);
  }

  return 0;
}
