package com.shardmx.mxbase;

public enum MxPutCall {
  NULL	(null),
  PUT	("0"),
  CALL	("1"),
  N	(null);

  private static final MxPutCall values[] = values();
  public static MxPutCall value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxPutCall(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
