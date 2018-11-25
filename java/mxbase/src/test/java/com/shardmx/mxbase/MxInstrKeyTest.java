package com.shardmx.mxbase;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.util.*;

public class MxInstrKeyTest extends TestCase {
  public MxInstrKeyTest(String testName) { super(testName); }

  public static Test suite() { return new TestSuite(MxInstrKeyTest.class); }

  public void testApp() {
    SortedMap<MxInstrKey, String> sm = new TreeMap<MxInstrKey, String>();
    MxInstrKey key1 = new MxInstrKeyTuple.Builder()
      .venue("XTKS").id("6502").build();
    MxInstrKey key2 = MxInstrKeyTuple.of("XTKS", "", "2914");
    sm.put(key1, "Toshiba");
    sm.put(key2, "Japan Tobacco");
    assertTrue(sm.get(key1) == "Toshiba");
  }
}
