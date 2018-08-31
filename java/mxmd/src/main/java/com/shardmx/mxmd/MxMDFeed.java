package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDFeed implements AutoCloseable {
  public MxMDFeed(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();
  public native String id();
  public native int level();
 
  // data members

  private long ptr;
}
