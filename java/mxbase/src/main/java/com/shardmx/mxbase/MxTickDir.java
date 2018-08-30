package com.shardmx.mxbase;

public enum MxTickDir {
  NULL		(null),
  UP		("0"),
  LEVELUP	("1"),
  DOWN		("2"),
  LEVELDOWN	("3"),
  N		(null);

  private static final MxTickDir values[] = values();
  public static MxTickDir value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxTickDir(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
