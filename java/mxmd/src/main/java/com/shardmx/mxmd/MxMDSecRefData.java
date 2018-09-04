package com.shardmx.mxmd;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDSecRefData {
  @Value.Default
  public boolean tradeable() { return true; }
  public abstract MxSecIDSrc idSrc();
  @Value.Default @Nullable
  public MxSecIDSrc altIDSrc() { return MxSecIDSrc.NULL; }
  @Value.Default @Nullable
  public MxPutCall putCall() { return MxPutCall.NULL; }
  public abstract String symbol();
  @Value.Default @Nullable
  public String altSymbol() { return null; }
  @Value.Default @Nullable
  public String underVenue() { return null; }
  @Value.Default @Nullable
  public String underSegment() { return null; }
  @Value.Default @Nullable
  public String underlying() { return null; }
  @Value.Default
  public int mat() { return 0; }
  public abstract int pxNDP();
  public abstract int qtyNDP();
  @Value.Default
  public long strike() { return 0L; }
  @Value.Default
  public long outstandingShares() { return 0L; }
  @Value.Default
  public long adv() { return 0L; }
}
