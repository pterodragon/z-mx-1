package com.shardmx.mxmd;

// bit mask is (1<<(MxMDVenueFlags.FLAG.ordinal()))
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
