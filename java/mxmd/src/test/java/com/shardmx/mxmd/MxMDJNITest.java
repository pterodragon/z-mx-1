package com.shardmx.mxmd;

import java.time.Instant;

import java.math.BigDecimal;

import java.util.*;
import java.util.concurrent.*;

import com.shardmx.mxbase.*;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

public class MxMDJNITest extends TestCase {
  public MxMDJNITest(String testName) { super(testName); }

  public static Test suite() { return new TestSuite(MxMDJNITest.class); }

  public void testApp() {
    System.loadLibrary("MxMDJNI");

    MxMDLib md = MxMDLib.init("subscriber.cf");
    assertTrue(md != null);

    Semaphore sem = new Semaphore(0);

    {
      MxDecimal v = new MxDecimal("100000.000001");
      System.out.println(new String() + v);
      MxDecimal w = v.multiply(v);
      System.out.println(new String() + w);
      assertTrue(w.equals(new MxDecimal("10000000000.200000000001")));
    }

    md.subscribe(new MxMDLibHandlerTuple.Builder().
	exception((MxMDLib md_, MxMDException e) -> {
	  if (e.message().equals("MxMDJNITest end")) {
	    sem.release();
	    sem.release(); // twice to ensure exit
	  }
	  System.out.println(e.toString());
	}).
        timer((Instant stamp1) -> {
          System.out.println("timer fn");
          return stamp1;
        }).
	connected((MxMDFeed feed) -> {
          System.out.println(new String() + "connected " + feed.id());
	}).
	refDataLoaded((MxMDVenue venue) -> {
          System.out.println(new String() + "ref data loaded " + venue.id());
	  sem.release();
	}).
	build());

    md.start();

    try {
      sem.acquire(); // wait for ref data load to complete
    } catch (InterruptedException e) { }

    MxMDInstrHandler instrHandler = new MxMDInstrHandlerTuple.Builder().
      l1((MxMDOrderBook ob, MxMDL1Data l1) -> {
	if (l1.last() != MxValue.NULL) {
	  System.out.println(new String()
	      + "l1 tick " + ob.id() + " -> "
	      + new MxValNDP(l1.last(), l1.pxNDP()));
	}
      }).
      addOrder((MxMDOrder order, Instant stamp) -> {
	System.out.println(new String() + "add order " + order);
      }).
      deletedOrder((MxMDOrder order, Instant stamp) -> {
	System.out.println(new String() + "del order " + order);
      }).
      build();

    MxMDInstrumentFn fn = (MxMDInstrument instr) -> {
      instr.subscribe(instrHandler);
    };

    md.allInstruments((MxMDInstrument instrument) -> {
	System.out.println(new String() + instrument);
	return 0;
    });
    md.instrument(MxInstrKeyTuple.of("6502", "XTKS", null), fn);
    {
      MxMDInstrHandle instr = 
	md.instrument(MxInstrKeyTuple.of("2914", "XTKS", null));
      if (instr != null) instr.invoke(fn);
    }

    try {
      sem.acquire(); // wait for "log MxMDJNITest end" zcmd
    } catch (InterruptedException e) { }

    md.stop();

    md.close();

    System.gc();
  }
}
