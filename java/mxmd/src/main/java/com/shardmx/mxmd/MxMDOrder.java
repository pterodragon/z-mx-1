package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOrder implements AutoCloseable {
  private MxMDOrder(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  // methods

  public native MxMDOrderBook orderBook();
  public native MxMDOBSide obSide();
  public native MxMDPxLevel pxLevel();

  public native String id();
  public native MxMDOrderData data();

  @Override
  public String toString() {
    if (ptr == 0L) { return ""; }
    MxMDOrderBook ob = orderBook();
    return new String()
      + "{id=" + id()
      + ", data=" + data().toString_(ob.pxNDP(), ob.qtyNDP())
      + "}";
  }
 
  // data members

  private long ptr;
}
