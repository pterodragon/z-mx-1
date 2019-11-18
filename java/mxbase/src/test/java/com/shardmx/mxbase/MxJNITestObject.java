package com.shardmx.mxbase;

public class MxJNITestObject implements AutoCloseable {
  public MxJNITestObject() { ptr = 0L; }
  public void close() { dtor_(); }

  private native void dtor_();

  public native boolean helloWorld(String text);
  private void helloWorld_(String text) { System.out.println(text); }

  public long ptr;
}
