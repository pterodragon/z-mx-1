#include <ZuLib.hpp>

#include <stdio.h>
#include <signal.h>

#include <MxMD.hpp>

#include <ZmRandom.hpp>
#include <ZmThread.hpp>

#include <ZeLog.hpp>

// the tickers to use

static const char *tickers[] = {
  "6502",
  "2914",
  0
};

ZmSemaphore sem;	// used to signal exit

extern "C" { void sigint(int); };
void sigint(int sig) { sem.post(); }	// CTRL-C signal handler

int subscribe();	// subscribe to events

int main(int argc, char **argv)
{
  // configure and start logging
  ZeLog::init("mdsample_subscriber");	// program name
  ZeLog::add(ZeLog::fileSink("&2"));	// log errors to stderr
  ZeLog::start();			// start logger thread

  signal(SIGINT, &sigint);		// handle CTRL-C

  try {
    MxMDLib *md = MxMDLib::init("sub.cf");	// initialize market data library

    if (!md) return 1;

    if (subscribe() < 0) {		// subscribe to library events
      md->final();
      return 1;
    }

    md->start();			// start all feeds

    md->startTimer();

    sem.wait();

    md->stop();				// stop all feeds

    md->final();			// clean up

  } catch (...) { std::cout << "exception in main" << std::endl; }

  return 0;
}

// subscriber

void l1(MxMDOrderBook *ob, const MxMDL1Data &l1Data)
{
  // if last traded price changed, print it
  if (!!*l1Data.last)
    printf("tick %s -> %f\n",
	   ob->id().data(),
	   (double)MxValNDP{l1Data.last, ob->pxNDP()}.fp());
}

void l2(MxMDOrderBook *ob, MxDateTime stamp)
{
  printf("L2 updated %s\n", ob->id().data());
}

void addOrder(MxMDOrder *order, MxDateTime stamp)
{
  MxMDOrderBook *ob = order->orderBook();
  printf("add order %s %s %s px=%f qty=%f\n",
      ob->id().data(),
      order->id().data(),
      MxSide::name(order->data().side),
      (double)MxValNDP{order->data().price, ob->pxNDP()}.fp(),
      (double)MxValNDP{order->data().qty, ob->pxNDP()}.fp());
}

void canceledOrder(MxMDOrder *order, MxDateTime stamp)
{
  MxMDOrderBook *ob = order->orderBook();
  printf("canceled order %s %s %s px=%f qty=%f\n",
      ob->id().data(),
      order->id().data(),
      MxSide::name(order->data().side),
      (double)MxValNDP{order->data().price, ob->pxNDP()}.fp(),
      (double)MxValNDP{order->data().qty, ob->pxNDP()}.fp());
}

void addPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  MxMDOrderBook *ob = pxLevel->obSide()->orderBook();
  printf("add px level %s %s px=%f qty=%f\n",
      ob->id().data(),
      MxSide::name(pxLevel->side()),
      (double)MxValNDP{pxLevel->price(), pxLevel->pxNDP()}.fp(),
      (double)MxValNDP{pxLevel->data().qty, pxLevel->qtyNDP()}.fp());
}

void updatedPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  MxMDOrderBook *ob = pxLevel->obSide()->orderBook();
  printf("updated px level %s %s px=%f qty=%f\n",
      ob->id().data(),
      MxSide::name(pxLevel->side()),
      (double)MxValNDP{pxLevel->price(), pxLevel->pxNDP()}.fp(),
      (double)MxValNDP{pxLevel->data().qty, pxLevel->qtyNDP()}.fp());
}

void deletedPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  MxMDOrderBook *ob = pxLevel->obSide()->orderBook();
  printf("deleted px level %s %s px=%f qty=%f\n",
      ob->id().data(),
      MxSide::name(pxLevel->side()),
      (double)MxValNDP{pxLevel->price(), pxLevel->pxNDP()}.fp(),
      (double)MxValNDP{pxLevel->data().qty, pxLevel->qtyNDP()}.fp());
}

void exception(MxMDLib *, ZmRef<ZeEvent> e) { ZeLog::log(ZuMv(e)); }

void timer(MxDateTime now, MxDateTime &next)
{
  thread_local ZtDateFmt::ISO fmt;
  std::cout << "TIMER " << now.iso(fmt) << '\n'; fflush(stdout);
  next = now + ZmTime(1);
}

void loaded(MxMDVenue *venue)
{
  MxMDLib *md = venue->md();

  ZmRef<MxMDSecHandler> secHandler = new MxMDSecHandler();
  secHandler->
    l1Fn(MxMDLevel1Fn::Ptr<&l1>::fn()).
    addMktLevelFn(MxMDPxLevelFn::Ptr<&addPxLevel>::fn()).
    updatedMktLevelFn(MxMDPxLevelFn::Ptr<&updatedPxLevel>::fn()).
    deletedMktLevelFn(MxMDPxLevelFn::Ptr<&deletedPxLevel>::fn()).
    addPxLevelFn(MxMDPxLevelFn::Ptr<&addPxLevel>::fn()).
    updatedPxLevelFn(MxMDPxLevelFn::Ptr<&updatedPxLevel>::fn()).
    deletedPxLevelFn(MxMDPxLevelFn::Ptr<&deletedPxLevel>::fn()).
    l2Fn(MxMDOrderBookFn::Ptr<&l2>::fn()).
    addOrderFn(MxMDOrderFn::Ptr<&addOrder>::fn()).
    canceledOrderFn(MxMDOrderFn::Ptr<&canceledOrder>::fn());

  // iterate through all tickers subscribing to market data updates

  for (const char **ticker = tickers; *ticker; ticker++) {

    // look up security

    md->secInvoke(MxSecKey{"XTKS", MxID(), *ticker},
	[secHandler, ticker](MxMDSecurity *sec) {
      if (!sec) {
	ZeLOG(Error, ZtString() <<
	    "security \"" << *ticker << "\" not found");
	return;
      }
      sec->subscribe(secHandler); // subscribe to L1/L2 data
    });
  }
}

int subscribe()
{
  try {
    MxMDLib *md = MxMDLib::instance();
    if (!md) throw ZtString("MxMDLib::instance() failed");

    md->subscribe(&((new MxMDLibHandler())->
	  exceptionFn(MxMDExceptionFn::Ptr<&exception>::fn()).
	  timerFn(MxMDTimerFn::Ptr<&timer>::fn()).
	  refDataLoadedFn(MxMDVenueFn::Ptr<&loaded>::fn())));
  } catch (const ZtString &s) {
    ZeLOG(Error, ZtString() << "error: " << s);
    return -1;
  } catch (...) {
    ZeLOG(Error, "unknown exception");
    return -1;
  }

  return 0;
}
