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
	  std::cout << "  id: " << heap.data.id << '\n' << std::flush;
	}
	break;
      case Type::Thread:
	{
	  const Thread &thread = msg->as<Thread>();
	  std::cout << "  name: " << thread.data.name << '\n' << std::flush;
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
