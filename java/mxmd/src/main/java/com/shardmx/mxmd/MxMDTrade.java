package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDTrade implements AutoCloseable {
  public MxMDTrade(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDSecurity security();
  public native MxMDOrderBook orderBook();

  public native MxMDTradeData data();

  // data members

  private long ptr;
}
