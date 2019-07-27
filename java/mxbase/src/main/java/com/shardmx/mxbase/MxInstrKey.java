package com.shardmx.mxbase;

import org.immutables.value.Value;
import javax.annotation.Nullable;

@Value.Immutable @MxTuple
public abstract class MxInstrKey implements Comparable<MxInstrKey> {
  public abstract String id();
  public abstract String venue();
  @Value.Default @Nullable
  public String segment() { return null; }

  public int compareTo(MxInstrKey k) {
    int i;
    if ((i = id().compareTo(k.id())) != 0) return i;
    if ((i = venue().compareTo(k.venue())) != 0) return i;
    if (segment() == null) {
      if (k.segment() != null) return -1;
      return 0;
    }
    if (k.segment() == null) return 1;
    return segment().compareTo(k.segment());
  }

  @Override
  public String toString() {
    return new String() + venue() + '|' + segment() + '|' + id();
  }
}
