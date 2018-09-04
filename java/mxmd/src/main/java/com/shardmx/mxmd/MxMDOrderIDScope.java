package com.shardmx.mxmd;

public enum MxMDOrderIDScope {
  VENUE,
  ORDERBOOK,
  OBSIDE,
  N;

  private static final MxMDOrderIDScope values[] = values();
  public static MxMDOrderIDScope value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }
}

