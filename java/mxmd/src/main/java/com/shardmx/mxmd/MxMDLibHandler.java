package com.shardmx.mxmd;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDLibHandler {
  @Value.Default @Nullable
  public MxMDExceptionFn	exception() { return null; }
  @Value.Default @Nullable
  public MxMDFeedFn		connected() { return null; }
  @Value.Default @Nullable
  public MxMDFeedFn		disconnected() { return null; }
  @Value.Default @Nullable
  public MxMDLibFn		eof() { return null; }
  @Value.Default @Nullable
  public MxMDVenueFn		refDataLoaded() { return null; }
  @Value.Default @Nullable
  public MxMDTickSizeTblFn	addTickSizeTbl() { return null; }
  @Value.Default @Nullable
  public MxMDTickSizeTblFn	resetTickSizeTbl() { return null; }
  @Value.Default @Nullable
  public MxMDTickSizeFn		addTickSize() { return null; }
  @Value.Default @Nullable
  public MxMDSecEventFn		addSecurity() { return null; }
  @Value.Default @Nullable
  public MxMDSecEventFn		updatedSecurity() { return null; }
  @Value.Default @Nullable
  public MxMDOBEventFn		addOrderBook() { return null; }
  @Value.Default @Nullable
  public MxMDOBEventFn		updatedOrderBook() { return null; }
  @Value.Default @Nullable
  public MxMDOBEventFn		deletedOrderBook() { return null; }
  @Value.Default @Nullable
  public MxMDTradingSessionFn	tradingSession() { return null; }
  @Value.Default @Nullable
  public MxMDTimerFn		timer() { return null; }
}
