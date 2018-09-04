//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// MxMD Test JNI

#include <iostream>

#include <jni.h>

#include <ZmRandom.hpp>

#include <ZJNI.hpp>

#include <MxMD.hpp>

#include <MxMDJNITest.hpp>

static const char *tickers[] = {
  "ETHBTC", // base-quote
  "LTCBTC", // base-quote
  0
};

struct Feed : public MxMDFeed {
  Feed(MxMDLib *md, MxID id) : MxMDFeed(md, id, 3) { }
  void start() {
    try {
      MxMDLib *md = MxMDLib::instance();

      if (!md) throw ZtString("MxMDLib::instance() failed");

      ZmRef<MxMDVenue> venue = new MxMDVenue(md, this,
	"JNITest",
	MxMDOrderIDScope::OBSide,
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
		"JNITest",
		MxID(),
		*ticker};	// ID (same as symbol in this case)

	// minimal reference data (just the native symbol and RIC)

	refData.idSrc = MxSecIDSrc::EXCH;
	refData.symbol = *ticker;
	refData.pxNDP = 2;
	refData.qtyNDP = 2;

	// add the security

	MxMDSecHandle sec = md->security(secKey, 0); // default to shard 0

	thread_local ZmSemaphore sem;
	ZtString error;
	sec.invokeMv([sem = &sem, &error,
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

      md->raise(ZeEVENT(Info, "MxMDJNITest feed started"));
    } catch (const ZtString &s) {
      ZeLOG(Error, s);
    } catch (...) {
      ZeLOG(Error, "Unknown Exception");
    }
  }
};

void MxMDJNITest::init(JNIEnv *env, jobject obj)
{
  // (MxMDLib) -> void

  try {
    MxMDLib *md = MxMDLib::instance();
    if (!md) throw ZtString("MxMDLib::instance() failed");

    ZmRef<MxMDFeed> feed = new Feed(md, "JNITest");
    md->addFeed(feed);

    md->raise(ZeEVENT(Info, "MxMDJNITest initialized"));
  } catch (const ZtString &s) {
    ZeLOG(Error, s);
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
  }
}

static void publish_()
{
  try {
    MxMDLib *md = MxMDLib::instance();
    if (!md) throw ZtString("MxMDLib::instance() failed");

    for (const char **ticker = tickers; *ticker; ticker++) {
      md->secInvoke(MxSecKey{"JNITest", MxID(), *ticker},
	  [ticker](MxMDSecurity *sec) {
	if (!sec)
	  throw ZtString() << "security \"" << *ticker << "\" not found";

	// get order book

	ZmRef<MxMDOrderBook> ob = sec->orderBook("JNITest", MxID());
	if (!ob)
	  throw ZtString() << "order book for \"" << *ticker << "\" not found";

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
  } catch (const ZtString &s) {
    ZeLOG(Error, s);
  } catch (...) {
    ZeLOG(Error, "Unknown Exception");
  }
}

void MxMDJNITest::publish(JNIEnv *env, jobject obj)
{
  ZmThread thread{0, 0, []() { publish_(); }};
  thread.join();
}

int MxMDJNITest::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "init", "()V", (void *)&MxMDJNITest::init },
    { "publish", "()V", (void *)&MxMDJNITest::publish }
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDJNITest",
        methods, sizeof(methods) / sizeof(methods[0]));
}

extern "C" {
  MxMDExtern jint JNI_OnLoad(JavaVM *, void *);
}

jint JNI_OnLoad(JavaVM *jvm, void *)
{
  jint v = ZJNI::load(jvm);

  if (v < 0) return -1;

  JNIEnv *env = ZJNI::env();

  if (MxMDJNITest::bind(env) < 0) return -1;

  return v;
}
