package com.shardmx.mxbase;

public class MxJNITestObject implements AutoCloseable {
  private MxJNITestObject(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  public native boolean helloWorld(String text);
  private void helloWorld_(String text) { System.out.println(text); }

  public long ptr;
}
