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

int initFeed();		// initialize synthetic feed
int startFeed(MxMDLib *md, MxMDFeed *feed);

bool startFailed = 0;

struct Feed : public MxMDFeed {
  inline Feed(MxMDLib *md, MxID id) : MxMDFeed(md, id) { }
  void start() { if (startFeed(md(), this)) startFailed = 1; }
};

void publish();		// publisher thread - generates random ticks

int subscribe();	// subscribe to events

int main(int argc, char **argv)
{
  // configure and start logging
  ZeLog::init("mdsample");		// program name
  ZeLog::add(ZeLog::fileSink("&2"));	// log errors to stderr
  ZeLog::start();			// start logger thread

  signal(SIGINT, &sigint);		// handle CTRL-C

  try {

    MxMDLib *md = MxMDLib::init(0);	// initialize market data library
    if (!md) return 1;

    if (subscribe() < 0) {		// subscribe to library events
      md->final();
      return 1;
    }

    if (initFeed() < 0) {		// initialize synthetic feed
      md->final();
      return 1;
    }

    md->record("foo");

    md->start();			// start all feeds

    if (startFailed) { md->stop(); return 1; }

    md->startTimer();

    {
      ZmThread thread(0, 0, []() { publish(); });
					// generate ticks in other thread
      thread.join();
    }

    md->stop();				// stop all feeds

    md->final();			// clean up

  } catch (...) { }

  return 0;
}

// subscriber

void l1(MxMDOrderBook *orderBook, const MxMDL1Data &l1Data)
{
  // if last traded price changed, print it
  if (!!*l1Data.last)
    printf("tick %s -> %f\n",
	   orderBook->id().data(),
	   (double)l1Data.last);
}

void l2(MxMDOrderBook *orderBook, MxDateTime stamp)
{
  printf("L2 updated %s\n", orderBook->id().data());
}

void addOrder(MxMDOrder *order, MxDateTime stamp)
{
  printf("add order %s %s %s px=%f qty=%f\n",
      order->orderBook()->id().data(),
      order->id().data(),
      MxSide::name(order->data().side),
      (double)order->data().price,
      (double)order->data().qty);
}

void canceledOrder(MxMDOrder *order, MxDateTime stamp)
{
  printf("canceled order %s %s %s px=%f qty=%f\n",
      order->orderBook()->id().data(),
      order->id().data(),
      MxSide::name(order->data().side),
      (double)order->data().price,
      (double)order->data().qty);
}

void addPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  printf("add px level %s %s px=%f qty=%f\n",
      pxLevel->obSide()->orderBook()->id().data(),
      MxSide::name(pxLevel->side()),
      (double)pxLevel->price(),
      (double)pxLevel->data().qty);
}

void updatedPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  printf("updated px level %s %s px=%f qty=%f\n",
      pxLevel->obSide()->orderBook()->id().data(),
      MxSide::name(pxLevel->side()),
      (double)pxLevel->price(),
      (double)pxLevel->data().qty);
}

void deletedPxLevel(MxMDPxLevel *pxLevel, MxDateTime stamp)
{
  printf("deleted px level %s %s px=%f qty=%f\n",
      pxLevel->obSide()->orderBook()->id().data(),
      MxSide::name(pxLevel->side()),
      (double)pxLevel->price(),
      (double)pxLevel->data().qty);
}

void exception(MxMDLib *, ZmRef<ZeEvent> e) { std::cerr << *e << '\n'; }

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
    l2Fn(MxMDLevel2Fn::Ptr<&l2>::fn()).
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

// synthetic feed (market data publisher)

int initFeed()
{
  try {
    MxMDLib *md = MxMDLib::instance();

    // add a synthetic feed to generate market data events
   
    ZmRef<MxMDFeed> feed = new Feed(md,
      "XTKS");		// Tokyo Stock Exchange
    md->addFeed(feed);

  } catch (const ZtString &s) {
    ZeLOG(Error, s);
    return -1;
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
    return -1;
  }

  return 0;
}

int startFeed(MxMDLib *md, MxMDFeed *feed)
{
  try {
    // add the venue

    ZmRef<MxMDVenue> venue = new MxMDVenue(md, feed,
      "XTKS",				// Tokyo Stock Exchange
      MxMDOrderIDScope::OBSide,		// order IDs unique within OB side
      0);
    md->addVenue(venue);

    // add a tick size table

    ZmRef<MxMDTickSizeTbl> tickSizeTbl = venue->addTickSizeTbl("1");
    if (!tickSizeTbl) throw ZtString("MxMDVenue::addTickSizeTbl() failed");
    // tick size 1 from 0 to infinity
    tickSizeTbl->addTickSize(0, MxFloat::inf(), 1);

    // configure lot sizes

    MxMDLotSizes lotSizes{1, 1, 1};	// odd, round, block

    // add securities

    for (const char **ticker = tickers; *ticker; ticker++) {
      MxMDSecRefData refData;

      // primary security key (venue, segment and ID)
 
      MxSecKey secKey{
	      "XTKS",			// Tokyo Stock Exchange
	      MxID(),			// null segment (segment not used)
	      *ticker};			// ID (same as symbol in this case)

      // minimal reference data (just the native symbol and RIC)

      refData.idSrc = MxSecIDSrc::EXCH;
      refData.symbol = *ticker;
      refData.altIDSrc = MxSecIDSrc::RIC;
      refData.altSymbol = *ticker; refData.altSymbol += ".T";

      // add the security

      MxMDSecHandle sec = md->security(secKey, 0); // default to shard 0

      thread_local ZmSemaphore sem;
      ZtString error;
      sec.invokeMv([sem = &sem, &error,
	  secKey, &refData, &tickSizeTbl, &lotSizes](
	    MxMDShard *shard, ZmRef<MxMDSecurity> sec) {

	// this runs inside shard 0

	sec = shard->addSecurity(ZuMv(sec), secKey, refData); 

	if (ZuUnlikely(!sec)) {
	  error = "MxMDLib::addSecurity() failed";
	  sem->post();
	  return;
	}

	// add the order book

	ZmRef<MxMDOrderBook> orderBook = sec->addOrderBook(
	  secKey,			// primary key for order book
	  tickSizeTbl,			// tick sizes
	  lotSizes);			// lot sizes
	if (ZuUnlikely(!orderBook)) {
	  error = "MxMDSecurity::addOrderBook() failed";
	  sem->post();
	  return;
	}
	sem->post();
      });
      sem.wait();
      if (ZuUnlikely(error)) throw error;
    }
    md->loaded(venue);
  } catch (const ZtString &s) {
    ZeLOG(Error, s);
    return -1;
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
    return -1;
  }
  return 0;
}

void publish()
{
  try {
    MxMDLib *md = MxMDLib::instance();
    if (!md) throw ZtString("MxMDLib::instance() failed");

    do {

      // iterate through all tickers generating a random last trade price

      for (const char **ticker = tickers; *ticker; ticker++) {

	// look up security

	md->secInvoke(MxSecKey{"XTKS", MxID(), *ticker},
	    [ticker](MxMDSecurity *sec) {
	  if (!sec) {
	    ZeLOG(Error, ZtString() <<
		"security \"" << *ticker << "\" not found");
	    return;
	  }

	  // get order book

	  ZmRef<MxMDOrderBook> ob = sec->orderBook("XTKS", MxID());
	  if (!ob) {
	    ZeLOG(Error, ZtString() <<
		"XTKS order book for \"" << *ticker << "\" not found");
	    return;
	  }

	  // generate random last traded price

	  MxMDL1Data l1Data;
	  l1Data.stamp = MxDateTime(MxDateTime::Now);
	  l1Data.last = MxFloat(ZmRand::randExc(90) + 10); // 10-100

	  ob->l1(l1Data);

	  ob->addOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, 100, 100, 0);
	  ob->modifyOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, 50, 50, 0);
	  ob->addOrder("bar",
	      l1Data.stamp, MxSide::Sell,
	      1, 50, 50, 0);
	  ob->reduceOrder("foo",
	      l1Data.stamp, MxSide::Sell, 50);
	  ob->cancelOrder("bar",
	      l1Data.stamp, MxSide::Sell);
	  ob->addOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, 100, 100, 0);
	  ob->reduceOrder("foo",
	      l1Data.stamp, MxSide::Sell, 50);
	  ob->modifyOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, 50, 0, 0);
	});
      }

    // wait one second between ticks
    } while (sem.timedwait(ZmTime(ZmTime::Now, 1)) < 0);

  } catch (const ZtString &s) {
    ZeLOG(Error, s);
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
  }
}
