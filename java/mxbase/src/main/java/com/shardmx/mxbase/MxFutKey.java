package com.shardmx.mxbase;

import org.immutables.value.Value;

@Value.Immutable @MxTuple
public abstract class MxFutKey implements Comparable<MxFutKey> {
  public abstract int mat();

  public int compareTo(MxFutKey k) {
    return Integer.valueOf(mat()).compareTo(k.mat());
  }
}
