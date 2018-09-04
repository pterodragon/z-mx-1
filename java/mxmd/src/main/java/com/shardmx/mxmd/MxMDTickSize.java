package com.shardmx.mxmd;

import org.immutables.value.Value;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDTickSize implements Comparable<MxMDTickSize> {
  public abstract long minPrice();
  public abstract long maxPrice();
  public abstract long tickSize();

  public int compareTo(MxMDTickSize k) {
    return Long.valueOf(minPrice()).compareTo(k.minPrice());
  }
}
