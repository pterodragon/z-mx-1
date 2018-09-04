package com.shardmx.mxbase;

import org.immutables.value.Value;

@Value.Immutable @MxTuple
public abstract class MxSecSymKey
    implements Comparable<MxSecSymKey> {
  public abstract MxSecIDSrc src();
  public abstract String id();

  public int compareTo(MxSecSymKey k) {
    int i;
    if ((i = src().compareTo(k.src())) != 0) return i;
    return id().compareTo(k.id());
  }
}
