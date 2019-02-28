#include <MxTelemetry.hpp>

#include <ZmTrap.hpp>

#include <ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Client {
public:
  void init(MxMultiplex *mx, ZtString dir, ZvCf *cf) {
    m_dir = ZuMv(dir);
    MxTelemetry::Client::init(mx, cf);
  }

  void open() {
    open_(m_heap, "heap");
    write_(m_heap, "time,id,size,alignment,partition,sharded,cacheSize,cpuset,cacheAllocs,heapAllocs,frees\n");
    open_(m_hashTbl, "hashTbl");
    write_(m_hashTbl, "time,id,linear,bits,slots,cBits,cSlots,count,resized,loadFactor,effLoadFactor,nodeSize\n");
    open_(m_thread, "thread");
    write_(m_thread,"time,name,id,tid,cpuUsage,cpuset,priority,stackSize,partition,main,detached\n");
    open_(m_multiplexer, "multiplexer");
    write_(m_multiplexer, "time,id,state,nThreads,priority,partition,isolation,rxThread,txThread,stackSize,rxBufSize,txBufSize\n");
    open_(m_socket, "socket");
    write_(m_socket, "time,mxID,type,remoteIP,remotePort,localIP,localPort,fd,flags,mreqAddr,mreqIf,mif,ttl,rxBufSize,rxBufLen,txBufSize,txBufLen\n");
    open_(m_queue, "queue");
    write_(m_queue, "time,id,type,full,size,count,seqNo,inCount,inBytes,outCount,outBytes\n");
    open_(m_engine, "engine");
    write_(m_engine, "time,id,state,nLinks,up,down,disabled,transient,reconn,failed,mxID,rxThread,txThread\n");
    open_(m_link, "link");
    write_(m_link, "time,id,state,reconnects,rxSeqNo,txSeqNo\n");
    open_(m_dbenv, "dbenv");
    write_(m_dbenv, "time,self,master,prev,next,state,active,recovering,replicating,nDBs,nHosts,nPeers,nCxns,heartbeatFreq,heartbeatTimeout,reconnectFreq,electionTimeout,writeThread\n");
    open_(m_dbhost, "dbhost");
    write_(m_dbhost, "time,id,priority,state,voted,ip,port\n");
    open_(m_db, "db");
    write_(m_db, "time,id,recSize,compress,cacheMode,cacheSize,path,fileSize,fileRecs,filesMax,preAlloc,minRN,allocRN,fileRN,cacheLoads,cacheMisses,fileLoads,fileMisses\n");
  }

private:
  void open_(ZiFile &file, ZuString name) {
    ZeError e;
    ZiFile::Path path = ZiFile::append(m_dir, ZtString() << name << ".csv");
    if (file.open(path,
	  ZiFile::Create | ZiFile::WriteOnly | ZiFile::GC, 0777, &e) != Zi::OK)
      throw ZtString() << path << ": " << e;
  }

  void write_(ZiFile &file, ZuString s) { file.write(s.data(), s.length()); }

  void process(ZmRef<MxTelemetry::Msg> msg) {
    using namespace MxTelemetry;

    thread_local ZtDate::CSVFmt nowFmt;

    ZtDate now(ZtDate::Now);

    switch ((int)msg->hdr().type) {
      default:
	std::cout << '\n' << std::flush;
	break;
      case Type::Heap: {
	const auto &data = msg->as<Heap>();
	write_(m_heap, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << data.size
	  << ',' << ZuBoxed(data.alignment)
	  << ',' << ZuBoxed(data.partition)
	  << ',' << ZuBoxed(data.sharded)
	  << ',' << data.cacheSize
	  << ',' << ZmBitmap(data.cpuset)
	  << ',' << data.cacheAllocs
	  << ',' << data.heapAllocs
	  << ',' << data.frees << '\n');
      } break;
      case Type::HashTbl: {
	const auto &data = msg->as<HashTbl>();
	write_(m_hashTbl, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << ZuBoxed(data.linear)
	  << ',' << ZuBoxed(data.bits)
	  << ',' << ZuBoxed(((uint64_t)1)<<data.bits)
	  << ',' << ZuBoxed(data.cBits)
	  << ',' << ZuBoxed(((uint64_t)1)<<data.cBits)
	  << ',' << data.count
	  << ',' << data.resized
	  << ',' << ((double)data.loadFactor / 16.0)
	  << ',' << ((double)data.effLoadFactor / 16.0)
	  << ',' << data.nodeSize << '\n');
      } break;
      case Type::Thread: {
	const auto &data = msg->as<Thread>();
	write_(m_thread, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.name
	  << ',' << data.id
	  << ',' << data.tid
	  << ',' << ZuBoxed(data.cpuUsage * 100.0).fmt(ZuFmt::FP<2>())
	  << ',' << ZmBitmap(data.cpuset)
	  << ',' << ZuBoxed(data.priority)
	  << ',' << data.stackSize
	  << ',' << ZuBoxed(data.partition)
	  << ',' << ZuBoxed(data.main)
	  << ',' << ZuBoxed(data.detached) << '\n');
      } break;
      case Type::Multiplexer: {
	const auto &data = msg->as<Multiplexer>();
	write_(m_multiplexer, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << ZmScheduler::stateName(data.state)
	  << ',' << ZuBoxed(data.nThreads)
	  << ',' << ZuBoxed(data.priority)
	  << ',' << ZuBoxed(data.partition)
	  << ',' << ZmBitmap(data.isolation)
	  << ',' << data.rxThread
	  << ',' << data.txThread
	  << ',' << data.stackSize
	  << ',' << data.rxBufSize
	  << ',' << data.txBufSize << '\n');
      } break;
      case Type::Socket: {
	const auto &data = msg->as<Socket>();
	write_(m_socket, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.mxID
	  << ',' << ZiCxnType::name(data.type)
	  << ',' << data.remoteIP
	  << ',' << ZuBoxed(data.remotePort)
	  << ',' << data.localIP
	  << ',' << ZuBoxed(data.localPort)
	  << ',' << data.socket
	  << ',' << ZiCxnFlags::Flags::print(data.flags)
	  << ',' << data.mreqAddr
	  << ',' << data.mreqIf
	  << ',' << data.mif
	  << ',' << data.ttl
	  << ',' << data.rxBufSize
	  << ',' << data.rxBufLen
	  << ',' << data.txBufSize
	  << ',' << data.txBufLen << '\n');
      } break;
      case Type::Queue: {
	const auto &data = msg->as<Queue>();
	write_(m_queue, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << QueueType::name(data.type)
	  << ',' << data.full
	  << ',' << data.size
	  << ',' << data.count
	  << ',' << data.seqNo
	  << ',' << data.inCount
	  << ',' << data.inBytes
	  << ',' << data.outCount
	  << ',' << data.outBytes << '\n');
      } break;
      case Type::Engine: {
	const auto &data = msg->as<Engine>();
	write_(m_engine, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << MxEngineState::name(data.state)
	  << ',' << data.nLinks
	  << ',' << data.up
	  << ',' << data.down
	  << ',' << data.disabled
	  << ',' << data.transient
	  << ',' << data.reconn
	  << ',' << data.failed
	  << ',' << data.mxID
	  << ',' << data.rxThread
	  << ',' << data.txThread << '\n');
      } break;
      case Type::Link: {
	const auto &data = msg->as<Link>();
	write_(m_link, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << MxLinkState::name(data.state)
	  << ',' << data.reconnects
	  << ',' << data.rxSeqNo
	  << ',' << data.txSeqNo << '\n');
      } break;
      case Type::DBEnv: {
	const auto &data = msg->as<DBEnv>();
	write_(m_dbenv, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.self
	  << ',' << data.master
	  << ',' << data.prev
	  << ',' << data.next
	  << ',' << ZdbHost::stateName(data.state)
	  << ',' << ZuBoxed(data.active)
	  << ',' << ZuBoxed(data.recovering)
	  << ',' << ZuBoxed(data.replicating)
	  << ',' << ZuBoxed(data.nDBs)
	  << ',' << ZuBoxed(data.nHosts)
	  << ',' << ZuBoxed(data.nPeers)
	  << ',' << ZuBoxed(data.nCxns)
	  << ',' << data.heartbeatFreq
	  << ',' << data.heartbeatTimeout
	  << ',' << data.reconnectFreq
	  << ',' << data.electionTimeout
	  << ',' << data.writeThread << '\n');
      } break;
      case Type::DBHost: {
	const auto &data = msg->as<DBHost>();
	write_(m_dbhost, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << data.priority
	  << ',' << ZdbHost::stateName(data.state)
	  << ',' << ZuBoxed(data.voted)
	  << ',' << data.ip
	  << ',' << data.port << '\n');
      } break;
      case Type::DB: {
	const auto &data = msg->as<DB>();
	write_(m_db, ZuStringN<512>() << now.csv(nowFmt)
	  << ',' << data.id
	  << ',' << data.recSize
	  << ',' << data.compress
	  << ',' << ZdbCacheMode::name(data.cacheMode)
	  << ',' << data.cacheSize
	  << ',' << data.path
	  << ',' << data.fileSize
	  << ',' << data.fileRecs
	  << ',' << data.filesMax
	  << ',' << data.preAlloc
	  << ',' << data.minRN
	  << ',' << data.allocRN
	  << ',' << data.fileRN
	  << ',' << data.cacheLoads
	  << ',' << data.cacheMisses
	  << ',' << data.fileLoads
	  << ',' << data.fileMisses << '\n');
      } break;
    }
  }

private:
  ZtString	m_dir;
  ZiFile	m_heap;
  ZiFile	m_hashTbl;
  ZiFile	m_thread;
  ZiFile	m_multiplexer;
  ZiFile	m_socket;
  ZiFile	m_queue;
  ZiFile	m_engine;
  ZiFile	m_link;
  ZiFile	m_dbenv;
  ZiFile	m_dbhost;
  ZiFile	m_db;
};

static void usage()
{
  std::cerr <<
    "usage: MxTelClient DIR [PORT]\n"
    "  capture telemetry in directory DIR\n"
    "\tPORT\t- UDP port (defaults to 19300)\n"
    "Options:\n"
    << std::flush;
  ZmPlatform::exit(1);
}

int main(int argc, char **argv)
{
  ZtString dir, port;

  {
    ZmRef<ZvCf> args = new ZvCf();

    static ZvOpt opts[] = { { 0 } };

    try {
      if (args->fromArgs(opts, argc, argv) < 2) usage();
      dir = args->get("1");
      port = args->get("2");
    } catch (const ZvError &e) {
      std::cerr << e << '\n' << std::flush;
      usage();
    } catch (...) {
      usage();
    }
  }

  if (!dir) usage();

  ZiFile::age(dir, 10);
  {
    ZeError e;
    if (ZiFile::mkdir(dir, &e) != Zi::OK) {
      std::cerr << dir << ": " << e << '\n' << std::flush;
      ZmPlatform::exit(1);
    }
  }

  ZmTrap::sigintFn([]() { sem.post(); });
  ZmTrap::trap();

  ZeLog::init("MxTelClient");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  ZmRef<ZvCf> cf = new ZvCf();
  cf->fromString(
      "telemetry {\n"
      "  ip 127.0.0.1\n"
      "  port 19300\n"
      "}\n",
      false);
  if (port) cf->set("telemetry:port", port);

  ZmRef<MxMultiplex> mx = new MxMultiplex("mx", cf->subset("mx", true));

  App app;

  try {
    app.init(mx, ZuMv(dir), cf->subset("telemetry", false));
  } catch (const ZvError &e) {
    std::cerr << e << '\n' << std::flush;
    usage();
  }

  try {
    app.open();
  } catch (const ZtString &e) {
    std::cerr << e << '\n' << std::flush;
    ZmPlatform::exit(1);
  }

  mx->start();

  app.start();

  sem.wait();

  app.stop();

  mx->stop(true);

  app.final();
}
