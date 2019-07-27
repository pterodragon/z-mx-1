package com.shardmx.mxmd;

import java.time.Instant;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDOrderData {
  @Value.Default @Nullable
  public Instant transactTime() { return null; }
  public abstract MxSide side();
  @Value.Default
  public int rank() { return 0; }
  @Value.Default
  public long price() { return MxValue.NULL; }
  public abstract long qty();
  @Value.Default
  public long flags() { return 0L; }

  public String toString_(int pxNDP, int qtyNDP) {
    String s = new String() + "{transactTime=";
    if (transactTime() != null) s += transactTime();
    s += ", side=" + side()
      + ", rank=" + rank()
      + ", px=" + new MxValNDP(price(), pxNDP)
      + ", qty=" + new MxValNDP(qty(), qtyNDP)
      + "}";
    return s;
  }
}
