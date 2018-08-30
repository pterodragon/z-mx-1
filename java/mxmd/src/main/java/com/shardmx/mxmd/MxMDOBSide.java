package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOBSide implements AutoCloseable {
  public MxMDOBSide(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

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
