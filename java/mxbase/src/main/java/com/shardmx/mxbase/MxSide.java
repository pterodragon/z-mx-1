package com.shardmx.mxbase;

public enum MxSide {
  NULL			(null),
  BUY			("1"),
  SELL			("2"),
  SELL_SHORT		("5"),
  SELL_SHORT_EXEMPT	("6"),
  CROSS			("8"),
  N			(null);

  private static final MxSide values[] = values();
  public static MxSide value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxSide(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
