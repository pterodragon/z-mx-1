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
	  const Heap &heap = msg->as<Heap>();
	  std::cout << "  id: " << heap.id << '\n';
	}
	break;
      case Type::Thread:
	{
	  const Thread &thread = msg->as<Thread>();
	  std::cout << "  name: " << thread.name << '\n';
	}
    break;
      case Type::Multiplexer:
	{
	  const Multiplexer &multiplexer = msg->as<Multiplexer>();
	  std::cout << "  id: " << multiplexer.id << '\n'
	    << "  isolation: " << multiplexer.isolation << '\n'
	    << "  stackSize: " << multiplexer.stackSize << '\n'
	    << "  rxBufSize: " << multiplexer.rxBufSize << '\n'
	    << "  txBufSize: " << multiplexer.txBufSize << '\n'
	    << "  rxThread: " << multiplexer.rxThread << '\n'
	    << "  txThread: " << multiplexer.txThread << '\n'
	    << "  partition: " << multiplexer.partition << '\n'
	    << "  state: " << multiplexer.state << '\n'
	    << "  priority: " << multiplexer.priority << '\n'
	    << "  nThreads: " << multiplexer.nThreads << '\n';
	}
    break;
      case Type::Socket:
	{
	  const Socket &socket = msg->as<Socket>();
	  std::cout << "  mxID: " << socket.data.mxID << '\n'
	    << "  socket fd: " << socket.data.socket << '\n'
	    << "  rxBufSize: " << socket.data.rxBufSize << '\n'
	    << "  rxBufLen: " << socket.data.rxBufLen << '\n'
	    << "  txBufSize: " << socket.data.txBufSize << '\n'
	    << "  txBufLen: " << socket.data.txBufLen << '\n'
	    << "  flags: ";
        ZvFlags<ZiCxnFlags::Flags>::instance()->print("flags", std::cout, socket.data.flags);
        std::cout << "\n  mreqAddr: " << socket.data.mreqAddr << '\n'
	    << "  mreqIf: " << socket.data.mreqIf << '\n'
	    << "  mif: " << socket.data.mif << '\n'
	    << "  ttl: " << socket.data.ttl << '\n'
	    << "  localIP: " << socket.data.localIP << '\n'
	    << "  remoteIP: " << socket.data.remoteIP << '\n'
	    << "  localPort: " << socket.data.localPort << '\n'
	    << "  remotePort: " << socket.data.remotePort << '\n'
	    << "  type: " << ZiCxnType::name(socket.data.type) << '\n';
	}
    break;
      case Type::Queue:
	{
	  const Queue &queue = msg->as<Queue>();
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
    std::cout.flush();
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
