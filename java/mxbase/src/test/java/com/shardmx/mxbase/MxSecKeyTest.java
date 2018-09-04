package com.shardmx.mxbase;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.util.*;

public class MxSecKeyTest extends TestCase {
  public MxSecKeyTest(String testName) { super(testName); }

  public static Test suite() { return new TestSuite(MxSecKeyTest.class); }

  public void testApp() {
    SortedMap<MxSecKey, String> sm = new TreeMap<MxSecKey, String>();
    MxSecKey key1 = new MxSecKeyTuple.Builder()
      .venue("XTKS").id("6502").build();
    MxSecKey key2 = MxSecKeyTuple.of("XTKS", "", "2914");
    sm.put(key1, "Toshiba");
    sm.put(key2, "Japan Tobacco");
    assertTrue(sm.get(key1) == "Toshiba");
  }
}
