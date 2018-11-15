#include <MxTelemetry.hpp>

#include <ZmTrap.hpp>

#include <ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Server {
  void run(MxTelemetry::Server::Cxn *cxn) {
    using namespace MxTelemetry;

    ZmHeapMgr::stats(ZmHeapMgr::StatsFn{
	[](Cxn *cxn, const ZmHeapInfo &info, const ZmHeapStats &stats) {
	  cxn->transmit(heap(
		info.id, info.config.cacheSize, info.config.cpuset.uint64(),
		stats.cacheAllocs, stats.heapAllocs, stats.frees,
		stats.allocated, stats.maxAllocated,
		info.size, info.partition, info.config.alignment));
	}, cxn});
  }
};

int main()
{
  ZmTrap::sigintFn([]() { sem.post(); });
  ZmTrap::trap();

  ZeLog::init("MxTelServer");
  ZeLog::level(0);
  ZeLog::add(ZeLog::fileSink("&2"));
  ZeLog::start();

  ZmRef<ZvCf> cf = new ZvCf();
  cf->fromString(
      "telemetry {\n"
      "  ip 127.0.0.1\n"
      "  port 19300\n"
      "  freq 1000000\n"
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
