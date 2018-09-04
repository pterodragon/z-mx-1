package com.shardmx.mxmd;

import org.immutables.value.Value;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDLotSizes {
  @Value.Default
  public long oddLotSize() { return 0; }
  @Value.Default
  public long lotSize() { return 0; }
  @Value.Default
  public long blockLotSize() { return 0; }
}
