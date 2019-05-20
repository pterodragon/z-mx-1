package com.shardmx.mxbase;

import java.math.BigDecimal;

public class MxValNDP {
  public BigDecimal bd;
  public MxValNDP(long value, int ndp) {
    bd = (new BigDecimal(value)).scaleByPowerOfTen(-ndp);
  }
  public boolean equals(Object v) {
    if (this == v) return true;
    if (v == null) return false;
    if (v instanceof BigDecimal) {
      return bd.compareTo((BigDecimal)v) == 0;
    }
    if (v instanceof MxValNDP) {
      return bd.compareTo(((MxValNDP)v).bd) == 0;
    }
    return false;
  }
  public int compareTo(Object v) {
    if (this == v) return 0;
    if (v == null) throw new NullPointerException();
    if (v instanceof BigDecimal)
      return bd.compareTo((BigDecimal)v);
    if (v instanceof MxValNDP)
      return bd.compareTo(((MxValNDP)v).bd);
    throw new RuntimeException("invalid object " + v.getClass());
  }
  public String toString() {
    return bd.toString();
  }
}
