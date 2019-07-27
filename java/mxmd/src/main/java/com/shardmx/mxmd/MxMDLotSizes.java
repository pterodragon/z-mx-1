package com.shardmx.mxmd;

import org.immutables.value.Value;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDLotSizes {
  @Value.Default
  public long oddLotSize() { return 0L; }
  @Value.Default
  public long lotSize() { return 0L; }
  @Value.Default
  public long blockLotSize() { return 0L; }

  public String toString_(int ndp) {
    return new String() + '['
      + new MxValNDP(oddLotSize(), ndp)  + ", "
      + new MxValNDP(lotSize(), ndp) + ", "
      + new MxValNDP(blockLotSize(), ndp) + ']';
  }
}
