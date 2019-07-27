package com.shardmx.mxmd;

import org.immutables.value.Value;
import javax.annotation.Nullable;

import com.shardmx.mxbase.*;

@Value.Immutable @MxTuple
public abstract class MxMDInstrRefData {
  @Value.Default
  public boolean tradeable() { return true; }
  public abstract MxInstrIDSrc idSrc();
  @Value.Default @Nullable
  public MxInstrIDSrc altIDSrc() { return MxInstrIDSrc.NULL; }
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
  public long strike() { return MxValue.NULL; }
  @Value.Default
  public long outstandingShares() { return MxValue.NULL; }
  @Value.Default
  public long adv() { return MxValue.NULL; }

  @Override
  public String toString() {
    String s = new String()
      + "{tradeable=" + tradeable()
      + ", idSrc=" + idSrc()
      + ", symbol=" + symbol();
    if (altIDSrc() != null) { s += ", altIDSrc=" + altIDSrc(); }
    if (altSymbol() != null) { s += ", altSymbol=" + altSymbol(); }
    if (underlying() != null) {
      if (underVenue() != null) { s += underVenue(); } s += "|";
      if (underSegment() != null) { s += underSegment(); } s += "|";
      s += underlying();
    }
    if (putCall() != null) { s += ", putCall=" + putCall(); }
    if (mat() != 0) { s += ", mat=" + mat(); }
    if (putCall() != null && strike() != MxValue.NULL) {
      s += ", strike=" + new MxValNDP(strike(), pxNDP());
    }
    if (outstandingShares() != MxValue.NULL) {
      s += ", outstandingShares=" + new MxValNDP(outstandingShares(), qtyNDP());
    }
    if (adv() != MxValue.NULL) {
      s += ", adv=" + new MxValNDP(adv(), pxNDP());
    }
    return s;
  }
}
