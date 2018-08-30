package com.shardmx.mxbase;

import org.immutables.value.Value;
import javax.annotation.Nullable;

@Value.Immutable @MxTuple
public abstract class MxSecKey implements Comparable<MxSecKey> {
  public abstract String venue();
  @Value.Default @Nullable
  public String segment() { return null; }
  public abstract String id();

  public int compareTo(MxSecKey k) {
    int i;
    if ((i = venue().compareTo(k.venue())) != 0) return i;
    if (segment() == null) {
      if (k.segment() != null) return -1;
    } else {
      if (k.segment() == null) return 1;
      if ((i = segment().compareTo(k.segment())) != 0) return i;
    }
    return id().compareTo(k.id());
  }
}
