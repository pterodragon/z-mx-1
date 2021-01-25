#include <mxbase/MxTelemetry.hpp>

#include <zlib/ZmTrap.hpp>

#include <zlib/ZeLog.hpp>

static ZmSemaphore sem;

class App : public MxTelemetry::Server {
public:
  App() : m_time(ZmTime::Now) { }

private:
  void run(MxTelemetry::Server::Cxn *cxn) {
    using namespace MxTelemetry;

    ZmHeapMgr::all(ZmFn<ZmHeapCache *>{cxn,
	[](Cxn *cxn, ZmHeapCache *h) { cxn->transmit(heap(h)); }});

    ZmHashMgr::all(ZmFn<ZmAnyHash *>{
	cxn, [](Cxn *cxn, ZmAnyHash *h) {
	  cxn->transmit(hashTbl(h));
	}});

    ZmSpecific<ZmThreadContext>::all([cxn](ZmThreadContext *tc) {
      cxn->transmit(thread(tc));
    });
  }

  ZmTime	m_time;
};

int main()
{
  ZmTrap::sigintFn([]() { sem.post(); });
  ZmTrap::trap();

  ZeLog::init("MxTelServer");
  ZeLog::level(0);
  ZeLog::sink(ZeLog::fileSink("&2"));
  ZeLog::start();

  ZmRef<ZvCf> cf = new ZvCf();
  cf->fromString(
      "telemetry {\n"
      "  ip 127.0.0.1\n"
      "  port 19300\n"
      "  freq 1000000\n"
      "}\n",
      false);

  ZmRef<MxMultiplex> mx = new MxMultiplex("mx", cf->subset("mx"));

  App app;

  app.init(mx, cf->subset("telemetry", true));

  mx->start();

  ZmThread{0, []() { while (sem.trywait() < 0); sem.post(); },
    ZmThreadParams().name("busy")};

  app.start();

  sem.wait();

  app.stop();

  mx->stop(true);

  app.final();
}
