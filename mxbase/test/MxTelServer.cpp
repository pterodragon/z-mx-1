#include <MxTelemetry.hpp>

#include <ZmTrap.hpp>

#include <ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Server {
  void run(MxTelemetry::Server::Cxn *cxn) {
    using namespace MxTelemetry;

    ZmHeapMgr::all(ZmFn<ZmHeapCache *>{cxn,
	[](Cxn *cxn, ZmHeapCache *h) { cxn->transmit(heap(*h)); }});

    ZmHashMgr::all(ZmFn<ZmAnyHash *>{
	cxn, [](Cxn *cxn, ZmAnyHash *h) {
	  if (!h->telCount()) cxn->transmit(hashTbl(*h));
	}});

    ZmSpecific<ZmThreadContext>::all(ZmFn<ZmThreadContext *>{cxn,
	[](Cxn *cxn, ZmThreadContext *tc) {
	  cxn->transmit(thread(*tc)); }});
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
