package com.shardmx.mxmd;

import java.time.Instant;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDTradeData {
  public abstract String tradeID();
  @Value.Default @Nullable
  public Instant transactTime() { return null; }
  public abstract long price();
  public abstract long qty();
}
