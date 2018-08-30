package com.shardmx.mxmd;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDSecHandler {
  @Value.Default @Nullable
  public MxMDSecEventFn		updatedSecurity() { return null; }
  @Value.Default @Nullable
  public MxMDOBEventFn		updatedOrderBook() { return null; }
  @Value.Default @Nullable
  public MxMDLevel1Fn		l1() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		addMktLevel() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		updatedMktLevel() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		deletedMktLevel() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		addPxLevel() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		updatedPxLevel() { return null; }
  @Value.Default @Nullable
  public MxMDPxLevelFn		deletedPxLevel() { return null; }
  @Value.Default @Nullable
  public MxMDOBEventFn		l2() { return null; }
  @Value.Default @Nullable
  public MxMDOrderFn		addOrder() { return null; }
  @Value.Default @Nullable
  public MxMDOrderFn		modifiedOrder() { return null; }
  @Value.Default @Nullable
  public MxMDOrderFn		canceledOrder() { return null; }
  @Value.Default @Nullable
  public MxMDTradeFn		addTrade() { return null; }
  @Value.Default @Nullable
  public MxMDTradeFn		correctedTrade() { return null; }
  @Value.Default @Nullable
  public MxMDTradeFn		canceledTrade() { return null; }
}
