package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.util.*;

public class MxMDJNITest extends TestCase {
  public MxMDJNITest(String testName) { super(testName); }

  public static Test suite() { return new TestSuite(MxMDJNITest.class); }

  public native void init();
  public native void publish();

  public void testApp() {
    System.loadLibrary("MxMDJNI");

    MxMDLib md = MxMDLib.init(null);
    assertTrue(md != null);

    md.subscribe(new MxMDLibHandlerTuple.Builder().
	exception((MxMDLib md_, String s) -> {
	  System.out.format("exception %s\n", s);
	}).
	build());

    System.loadLibrary("MxMDJNITest");

    init();

    md.start();

    MxMDInstrHandler instrHandler = new MxMDInstrHandlerTuple.Builder().
      l1((MxMDOrderBook ob, MxMDL1Data l1) -> {
	System.out.format("tick %s -> %f\n",
	    ob.id(), ((double)l1.last()) / 100.0);
      }).
      build();

    MxMDInstrumentFn fn = (MxMDInstrument instr) -> {
      if (instr != null) instr.subscribe(instrHandler);
    };

    md.instrument(MxInstrKeyTuple.of("JNITest", null, "ETHBTC"), fn);

    {
      MxMDInstrHandle instr = 
	md.instrument(MxInstrKeyTuple.of("JNITest", null, "LTCBTC"));
      if (instr != null) instr.invoke(fn);
    }

    publish();

    md.stop();

    md.close();
  }
}
