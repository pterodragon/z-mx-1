package com.shardmx.mxbase;

public enum MxTradingSession {
  NULL			(null),
  PRE_TRADING		("1"),
  OPENING		("2"),
  CONTINUOUS		("3"),
  CLOSING		("4"),
  POST_TRADING		("5"),
  INTRADAY_AUCTION	("6"),
  QUIESCENT		("7"),
  N			(null);

  private static final MxTradingSession values[] = values();
  public static MxTradingSession value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxTradingSession(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
