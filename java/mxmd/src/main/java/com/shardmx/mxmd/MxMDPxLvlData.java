package com.shardmx.mxmd;

import java.time.Instant;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDPxLvlData {
  @Value.Default @Nullable
  public Instant transactTime() { return null; }
  @Value.Default
  public long qty() { return 0L; }
  @Value.Default
  public int nOrders() { return 0; }
  @Value.Default
  public long flags() { return 0L; }

  public String toString_(int qtyNDP) {
    String s = new String() + "{transactTime=";
    if (transactTime() != null) s += transactTime();
    s += ", qty=" + new MxValNDP(qty(), qtyNDP)
      + ", nOrders=" + nOrders()
      + "}";
    return s;
  }
}
