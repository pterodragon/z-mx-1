package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDPxLevel {
  public MxMDPxLevel(long ptr) { this.ptr = ptr; }

  // methods

  public native MxMDOBSide obSide();
  public native MxSide side();
  public native int pxNDP();
  public native int qtyNDP();
  public native long price();

  public native MxMDPxLvlData data();

  public native long allOrders(MxMDAllOrdersFn fn);

  // data members

  private long ptr;
}
