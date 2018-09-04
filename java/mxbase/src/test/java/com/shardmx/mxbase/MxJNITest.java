package com.shardmx.mxbase;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.util.*;

public class MxJNITest extends TestCase {
  public MxJNITest(String testName) { super(testName); }

  public static Test suite() { return new TestSuite(MxJNITest.class); }

  private native int init();

  public void testApp() {
    System.loadLibrary("MxJNITest");
    assertTrue(init() >= 0);
    MxJNITestObject o = new MxJNITestObject();
    o.helloWorld("Hello World");
    assertTrue(o.ptr != 0L);
  }
}
