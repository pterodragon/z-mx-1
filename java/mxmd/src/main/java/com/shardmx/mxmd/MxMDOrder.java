package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOrder implements AutoCloseable {
  public MxMDOrder(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDOrderBook orderBook();
  public native MxMDOBSide obSide();
  public native MxMDPxLevel pxLevel();

  public native String id();
  public native MxMDOrderData data();

  // data members

  private long ptr;
}
