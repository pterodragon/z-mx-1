package com.shardmx.mxbase;

public class MxValue {
  public static final long NULL  = Long.MIN_VALUE;
  public static final long MIN   =  -999999999999999999L;
  public static final long MAX   =   999999999999999999L;
  public static final long RESET = -1000000000000000000L; // reset to NULL

  public Long l;
  
  public MxValue(long value) { l = new Long(value); }
  public boolean equals(MxValue v) { return l.equals(v.l); };
  public int compareTo(MxValue v) { return l.compareTo(v.l); }
  public String toString() {
    if (l.longValue() == NULL) return "";
    if (l.longValue() == RESET) return "reset";
    return l.toString();
  }
}
