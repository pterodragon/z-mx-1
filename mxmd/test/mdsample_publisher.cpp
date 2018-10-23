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
  inline Feed(MxMDLib *md, MxID id) : MxMDFeed(md, id, 3) { }
  void start() { if (startFeed(md(), this)) startFailed = 1; }
};

void publish();		// publisher thread - generates random ticks

int main(int argc, char **argv)
{
  // configure and start logging
  ZeLog::init("mdsample publisher");		// program name
  ZeLog::add(ZeLog::fileSink("&2"));	// log errors to stderr
  ZeLog::start();			// start logger thread

  signal(SIGINT, &sigint);		// handle CTRL-C

  try {
    MxMDLib *md = MxMDLib::init("pub.cf");

    if (!md) return 1;

    if (initFeed() < 0) {		// initialize synthetic feed
      md->final();
      return 1;
    }

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

  } catch (...) { std::cout << "exception in main" << std::endl; }

  ZeLog::stop();
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

    ZmRef<MxMDTickSizeTbl> tickSizeTbl = venue->addTickSizeTbl("1", 0);
    if (!tickSizeTbl) throw ZtString("MxMDVenue::addTickSizeTbl() failed");
    // tick size 1 from 0 to infinity
    tickSizeTbl->addTickSize(0, MxValueMax, 1);

    // configure lot sizes

    MxMDLotSizes lotSizes{100, 100, 100}; // odd, round, block

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
      refData.pxNDP = 2;
      refData.qtyNDP = 2;

      // add the security

      MxMDSecHandle sec = md->security(secKey, 0); // default to shard 0

      ZtString error;
      thread_local ZmSemaphore sem;
      sec.invokeMv([&error, sem = &sem,
	  secKey, &refData, &tickSizeTbl, &lotSizes](
	    MxMDShard *shard, ZmRef<MxMDSecurity> sec) {

	// this runs inside shard 0

	sec = shard->addSecurity(ZuMv(sec), secKey, refData, MxDateTime()); 

	if (ZuUnlikely(!sec)) {
	  error = "MxMDLib::addSecurity() failed";
	  sem->post();
	  return;
	}

	// add the order book

	ZmRef<MxMDOrderBook> orderBook = sec->addOrderBook(
	  secKey,			// primary key for order book
	  tickSizeTbl,			// tick sizes
	  lotSizes,			// lot sizes
	  MxDateTime());
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
	  l1Data.last = MxValNDP{ZmRand::randExc(90) + 10, 2}.value; // 10-100
	  ob->l1(l1Data);

	  ob->addOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, MxValNDP{100.0, 2}.value, MxValNDP{100.0, 2}.value, 0);
	  ob->modifyOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, MxValNDP{50.0, 2}.value, MxValNDP{50.0, 2}.value, 0);
	  ob->addOrder("bar",
	      l1Data.stamp, MxSide::Sell,
	      1, MxValNDP{50.0, 2}.value, MxValNDP{50.0, 2}.value, 0);
	  ob->reduceOrder("foo",
	      l1Data.stamp, MxSide::Sell, MxValNDP{50.0, 2}.value);
	  ob->cancelOrder("bar",
	      l1Data.stamp, MxSide::Sell);
	  ob->addOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, MxValNDP{100.0, 2}.value, MxValNDP{100.0, 2}.value, 0);
	  ob->reduceOrder("foo",
	      l1Data.stamp, MxSide::Sell, MxValNDP{50.0, 2}.value);
	  ob->modifyOrder("foo",
	      l1Data.stamp, MxSide::Sell,
	      1, MxValNDP{50.0, 2}.value, 0, 0);
	});
      }

    // wait three second between ticks
    } while (sem.timedwait(ZmTime(ZmTime::Now, 3)) < 0);

  } catch (const ZtString &s) {
    ZeLOG(Error, s);
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
  }
}