package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOBSide implements AutoCloseable {
  private MxMDOBSide(long ptr) { this.ptr = ptr; }
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
  public native MxSide side();

  public native MxMDOBSideData data();
  public native long vwap();

  public native MxMDPxLevel mktLevel();

  public native long allPxLevels(MxMDAllPxLevelsFn fn);
  public native long allPxLevels(
      long minPrice, long maxPrice, MxMDAllPxLevelsFn fn);

  @Override
  public String toString() {
    if (ptr == 0L) { return ""; }
    MxMDOrderBook ob = orderBook();
    return new String()
      + "{side=" + side()
      + ", data=" + data().toString_(ob.pxNDP(), ob.qtyNDP())
      + ", vwap=" + new MxValNDP(vwap(), ob.pxNDP())
      + "}";
  }

  // data members

  private long ptr;
}
