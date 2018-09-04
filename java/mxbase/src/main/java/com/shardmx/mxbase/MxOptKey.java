package com.shardmx.mxbase;

import org.immutables.value.Value;

@Value.Immutable @MxTuple
public abstract class MxOptKey implements Comparable<MxOptKey> {
  public abstract int mat();
  public abstract MxPutCall putCall();
  public abstract long strike();

  public int compareTo(MxOptKey k) {
    int i;
    if ((i = Integer.valueOf(mat()).compareTo(k.mat())) != 0) return i;
    if ((i = Integer.valueOf(putCall().ordinal()).
	  compareTo(k.putCall().ordinal())) != 0) return i;
    return Long.valueOf(strike()).compareTo(k.strike());
  }
}
