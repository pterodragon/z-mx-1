#include <MxTelemetry.hpp>

#include <ZmTrap.hpp>

#include <ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Client {
  void process(ZmRef<MxTelemetry::Msg> msg) {
    using namespace MxTelemetry;

    std::cout << Type::name(msg->hdr().type);
    switch ((int)msg->hdr().type) {
      case Type::Heap: {
	const auto &data = msg->as<Heap>();
	std::cout << "  id: " << data.id
	  << "  cacheSize: " << data.cacheSize
	  << "  cpuset: " << data.cpuset
	  << "  cacheAllocs: " << data.cacheAllocs
	  << "  heapAllocs: " << data.heapAllocs
	  << "  frees: " << data.frees
	  << "  allocated: " << data.allocated
	  << "  maxAllocated: " << data.maxAllocated
	  << "  size: " << data.size
	  << "  partition: " << ZuBoxed(data.partition)
	  << "  sharded: " << ZuBoxed(data.sharded)
	  << "  alignment: " << ZuBoxed(data.alignment) << '\n' << std::flush;
      } break;
      case Type::Thread: {
	const auto &data = msg->as<Thread>();
	std::cout << "  name: " << data.name
	  << "  tid: " << data.tid
	  << "  stackSize: " << data.stackSize
	  << "  cpuset: " << data.cpuset
	  << "  id: " << data.id
	  << "  priority: " << ZuBoxed(data.priority)
	  << "  partition: " << ZuBoxed(data.partition)
	  << "  main: " << ZuBoxed(data.main)
	  << "  detached: " << ZuBoxed(data.detached) << '\n' << std::flush;
      } break;
      case Type::Multiplexer: {
	const auto &data = msg->as<Multiplexer>();
	std::cout << "  id: " << data.id
	  << "  isolation: " << data.isolation
	  << "  stackSize: " << data.stackSize
	  << "  rxBufSize: " << data.rxBufSize
	  << "  txBufSize: " << data.txBufSize
	  << "  rxThread: " << data.rxThread
	  << "  txThread: " << data.txThread
	  << "  partition: " << ZuBoxed(data.partition)
	  << "  state: " << ZmScheduler::stateName(data.state)
	  << "  priority: " << ZuBoxed(data.priority)
	  << "  nThreads: " << ZuBoxed(data.nThreads) << '\n' << std::flush;
      } break;
      case Type::Socket: {
	const auto &data = msg->as<Socket>();
	std::cout << "  mxID: " << data.mxID
	  << "  socket fd: " << data.socket
	  << "  rxBufSize: " << data.rxBufSize
	  << "  rxBufLen: " << data.rxBufLen
	  << "  txBufSize: " << data.txBufSize
	  << "  txBufLen: " << data.txBufLen
	  << "  flags: " << ZiCxnFlags::Flags::print(data.flags)
	  << "  mreqAddr: " << data.mreqAddr
	  << "  mreqIf: " << data.mreqIf
	  << "  mif: " << data.mif
	  << "  ttl: " << data.ttl
	  << "  localIP: " << data.localIP
	  << "  remoteIP: " << data.remoteIP
	  << "  localPort: " << ZuBoxed(data.localPort)
	  << "  remotePort: " << ZuBoxed(data.remotePort)
	  << "  type: " << ZiCxnType::name(data.type) << '\n' << std::flush;
      } break;
      case Type::Queue: {
	const auto &data = msg->as<Queue>();
	std::cout << "  id: " << data.id
	  << "  seqNo: " << data.seqNo
	  << "  count: " << data.count
	  << "  inCount: " << data.inCount
	  << "  inBytes: " << data.inBytes
	  << "  outCount: " << data.outCount
	  << "  outBytes: " << data.outBytes
	  << "  size: " << data.size
	  << "  type: " << QueueType::name(data.type) << '\n' << std::flush;
      } break;
      case Type::HashTbl: {
	const auto &data = msg->as<HashTbl>();
	std::cout << "  id: " << data.id
	  << "  nodeSize: " << data.nodeSize
	  << "  loadFactor: " <<
	    ((double)data.loadFactor / 16.0)
	  << "  count: " << data.count
	  << "  effLoadFactor: " <<
	    ((double)data.effLoadFactor / 16.0)
	  << "  resized: " << data.resized
	  << "  bits: " << ZuBoxed(data.bits)
	  << "  cBits: " << ZuBoxed(data.cBits)
	  << "  linear: " << ZuBoxed(data.linear) << '\n' << std::flush;
      } break;
    }
  }
};

int main()
{
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

  ZmRef<MxMultiplex> mx = new MxMultiplex("mx", cf->subset("mx", true));

  App app;

  app.init(mx, cf->subset("telemetry", false));

  mx->start();

  app.start();

  sem.wait();

  app.stop();

  mx->stop(true);

  app.final();
}
