package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDFeed implements AutoCloseable {
  private MxMDFeed(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();
  public native String id();
  public native int level();
 
  // data members

  private long ptr;
}
