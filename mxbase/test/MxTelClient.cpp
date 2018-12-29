#include <MxTelemetry.hpp>

#include <ZmTrap.hpp>

#include <ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Client {
  void process(ZmRef<MxTelemetry::Msg> msg) {
    using namespace MxTelemetry;

    std::cout << Type::name(msg->hdr().type) << '\n' << std::flush;
    switch ((int)msg->hdr().type) {
      case Type::Heap:
	{
	  const auto &data = msg->as<Heap>().data;
	  std::cout << "  id: " << data.id << '\n' << std::flush;
	}
	break;
      case Type::Thread:
	{
	  const auto &data = msg->as<Thread>().data;
	  std::cout << "  name: " << data.name << '\n';
	}
    break;
      case Type::Multiplexer:
	{
	  const auto &data = msg->as<Multiplexer>().data;
	  std::cout << "  id: " << data.id << '\n'
	    << "  isolation: " << data.isolation << '\n'
	    << "  stackSize: " << data.stackSize << '\n'
	    << "  rxBufSize: " << data.rxBufSize << '\n'
	    << "  txBufSize: " << data.txBufSize << '\n'
	    << "  rxThread: " << data.rxThread << '\n'
	    << "  txThread: " << data.txThread << '\n'
	    << "  partition: " << data.partition << '\n'
	    << "  state: " << data.state << '\n'
	    << "  priority: " << data.priority << '\n'
	    << "  nThreads: " << data.nThreads << '\n';
	}
    break;
      case Type::Socket:
	{
	  const auto &data = msg->as<Socket>().data;
	  std::cout << "  mxID: " << data.mxID << '\n'
	    << "  socket fd: " << data.socket << '\n'
	    << "  rxBufSize: " << data.rxBufSize << '\n'
	    << "  rxBufLen: " << data.rxBufLen << '\n'
	    << "  txBufSize: " << data.txBufSize << '\n'
	    << "  txBufLen: " << data.txBufLen << '\n'
	    << "  flags: ";
        ZvFlags<ZiCxnFlags::Flags>::instance()->print("flags", std::cout, data.flags);
        std::cout << "\n  mreqAddr: " << data.mreqAddr << '\n'
	    << "  mreqIf: " << data.mreqIf << '\n'
	    << "  mif: " << data.mif << '\n'
	    << "  ttl: " << data.ttl << '\n'
	    << "  localIP: " << data.localIP << '\n'
	    << "  remoteIP: " << data.remoteIP << '\n'
	    << "  localPort: " << data.localPort << '\n'
	    << "  remotePort: " << data.remotePort << '\n'
	    << "  type: " << ZiCxnType::name(data.type) << '\n';
	}
    break;
      case Type::Queue:
	{
	  const auto &queue = msg->as<Queue>();
	  std::cout << "  id: " << queue.id << '\n'
	    << "  seqNo: " << queue.seqNo << '\n'
	    << "  count: " << queue.count << '\n'
	    << "  inCount: " << queue.inCount << '\n'
	    << "  inBytes: " << queue.inBytes << '\n'
	    << "  outCount: " << queue.outCount << '\n'
	    << "  outBytes: " << queue.outBytes << '\n'
	    << "  size: " << queue.size << '\n'
	    << "  type: " << QueueType::name(queue.type) << '\n';
	}
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
