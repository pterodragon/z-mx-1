package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOrder {
  public MxMDOrder(long ptr) { this.ptr = ptr; }

  // methods

  public native MxMDOrderBook orderBook();
  public native MxMDOBSide obSide();
  public native MxMDPxLevel pxLevel();

  public native String id();
  public native MxMDOrderData data();

  // data members

  private long ptr;
}
