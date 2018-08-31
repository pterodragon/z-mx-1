package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOBSide {
  public MxMDOBSide(long ptr) { this.ptr = ptr; }

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
