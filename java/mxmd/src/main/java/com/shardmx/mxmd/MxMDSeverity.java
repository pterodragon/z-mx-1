package com.shardmx.mxmd;

public enum MxMDSeverity {
  Debug, Info, Warning, Error, Fatal, N;

  private static final MxMDSeverity values[] = values();
  public static MxMDSeverity value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }
}
