package com.shardmx.mxbase;

import org.immutables.value.Value;
import javax.annotation.Nullable;

@Value.Immutable @MxTuple
public abstract class MxUniKey implements Comparable<MxUniKey> {
  public abstract String id();
  @Value.Default @Nullable
  public String venue() { return null; }
  @Value.Default @Nullable
  public String segment() { return null; }
  @Value.Default @Nullable
  public MxInstrIDSrc src() { return MxInstrIDSrc.NULL; }
  @Value.Default
  public int mat() { return 0; }
  @Value.Default @Nullable
  public MxPutCall putCall() { return MxPutCall.NULL; }
  @Value.Default
  public long strike() { return 0L; }

  public int compareTo(MxUniKey k) {
    int i;
    if ((i = id().compareTo(k.id())) != 0) return i;
    if (src() == null) {
      if (k.src() != null) return -1;
      if (venue() == null) {
	if (k.venue() != null) return -1;
	return 0;
      }
      if ((i = venue().compareTo(k.venue())) != 0) return i;
      if (segment() == null) {
	if (k.segment() != null) return -1;
	return 0;
      }
      if ((i = segment().compareTo(k.segment())) != 0) return i;
    }
    if ((i = Integer.valueOf(src().ordinal()).
	  compareTo(k.src().ordinal())) != 0) return i;
    if ((i = Integer.valueOf(mat()).compareTo(k.mat())) != 0) return i;
    if ((i = Integer.valueOf(putCall().ordinal()).
	  compareTo(k.putCall().ordinal())) != 0) return i;
    return Long.valueOf(strike()).compareTo(k.strike());
  }
}
