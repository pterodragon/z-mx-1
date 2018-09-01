package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDPxLevel implements AutoCloseable {
  public MxMDPxLevel(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

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
