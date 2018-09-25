package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOBSide implements AutoCloseable {
  private MxMDOBSide(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
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

  // data members

  private long ptr;
}
