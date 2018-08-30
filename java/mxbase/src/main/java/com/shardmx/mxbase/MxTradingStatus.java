package com.shardmx.mxbase;

public enum MxTradingStatus {
  NULL		(null),
  OPEN		("17"),
  CLOSED	("18"),
  PREOPEN	("21"),
  AUCTION	("5"),
  HALTED	("2"),
  RESUMED	("3"),
  NOT_TRADED	("19"),
  UNKNOWN	("20"),
  N		(null);

  private static final MxTradingStatus values[] = values();
  public static MxTradingStatus value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxTradingStatus(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
