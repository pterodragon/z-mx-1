package com.shardmx.mxmd;

import java.time.Instant;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

// MxMDL1Data l1;
// ...
// last = new BigDecimal(l1.last()).movePointLeft(l1.pxNDP())
// lastQty = new BigDecimal(l1.lastQty()).movePointLeft(l1.qtyNDP())

@Value.Immutable @MxTuple
public abstract class MxMDL1Data {
  @Value.Default @Nullable
  public Instant stamp() { return null; }
  @Value.Default
  public long base() { return MxValue.NULL; }
  @Value.Default
  public long open() { return MxValue.NULL; }
  @Value.Default
  public long close() { return MxValue.NULL; }
  @Value.Default
  public long last() { return MxValue.NULL; }
  @Value.Default
  public long lastQty() { return MxValue.NULL; }
  @Value.Default
  public long bid() { return MxValue.NULL; }
  @Value.Default
  public long bidQty() { return MxValue.NULL; }
  @Value.Default
  public long ask() { return MxValue.NULL; }
  @Value.Default
  public long askQty() { return MxValue.NULL; }
  @Value.Default
  public long high() { return MxValue.NULL; }
  @Value.Default
  public long low() { return MxValue.NULL; }
  @Value.Default
  public long accVol() { return MxValue.NULL; }
  @Value.Default
  public long accVolQty() { return MxValue.NULL; }
  @Value.Default
  public long match() { return MxValue.NULL; }
  @Value.Default
  public long matchQty() { return MxValue.NULL; }
  @Value.Default
  public long surplusQty() { return MxValue.NULL; }
  @Value.Default
  public long flags() { return 0L; }
  @Value.Default
  public int pxNDP() { return 0; }
  @Value.Default
  public int qtyNDP() { return 0; }
  @Value.Default @Nullable
  public MxTickDir tickDir() { return MxTickDir.NULL; };
  @Value.Default @Nullable
  public MxTradingStatus status() { return MxTradingStatus.NULL; };

  @Override
  public String toString() {
    return new String()
      + "{base=" + new MxValNDP(base(), pxNDP())
      + ", last=" + new MxValNDP(last(), pxNDP())
      + ", lastQty=" + new MxValNDP(lastQty(), qtyNDP())
      + ", bid=" + new MxValNDP(bid(), pxNDP())
      + ", bidQty=" + new MxValNDP(bidQty(), qtyNDP())
      + ", ask=" + new MxValNDP(ask(), pxNDP())
      + ", askQty=" + new MxValNDP(askQty(), qtyNDP())
      + ", high=" + new MxValNDP(high(), pxNDP())
      + ", low=" + new MxValNDP(low(), pxNDP())
      + ", status=" + status()
      + ", tickDir=" + tickDir()
      + "}";
  }
}
