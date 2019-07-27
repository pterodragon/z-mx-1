package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

import org.immutables.value.Value;

@Value.Immutable @MxTuple
public abstract class MxMDOBSideData {
  @Value.Default
  public long nv() { return 0L; }
  @Value.Default
  public long qty() { return 0L; }

  public String toString_(int pxNDP, int qtyNDP) {
    return new String()
      + "{nv=" + new MxValNDP(nv(), pxNDP)
      + ", qty=" + new MxValNDP(qty(), qtyNDP)
      + "}";
  }
}
