package com.shardmx.mxmd;

import java.time.Instant;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDSegment {
  @Value.Default @Nullable
  public String id() { return null; }
  @Value.Default
  public MxTradingSession session() { return MxTradingSession.NULL; }
  @Value.Default @Nullable
  public Instant stamp() { return null; }
}
