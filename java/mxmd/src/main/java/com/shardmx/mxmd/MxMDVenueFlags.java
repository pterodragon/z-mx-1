package com.shardmx.mxmd;

public enum MxMDVenueFlags {
  UNIFORM_RANKS,
  DARK,
  SYNTHETIC,
  N;

  private static final MxMDVenueFlags values[] = values();
  public static MxMDVenueFlags value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }
}
