package com.shardmx.mxbase;

import java.math.BigDecimal;
import java.text.DecimalFormat;

public class MxDecimal {
  public MxDecimal(long h, long l) { this.h = h; this.l = l; }

  public MxDecimal(String s) { scan(s); }

  // methods

  // FIXME - need isNull(), isReset(), etc.
  // FIXME - add(), sub(), div(), mul()

  public static final MxDecimal valueOf(BigDecimal bd) {
    BigInteger bi = bd.setScale(18).unscaledValue();
    long zero = 0;
    BigInteger mask = BigInteger.valueOf(~zero);
    return new MxDecimal(
      bi.shiftRight(64).longValue(),
      bi.and(mask).longValue());
  }

  public BigDecimal bdValue() {
    BigInteger bi = BigInteger.valueOf(this.h);
    bi = bi.shiftLeft(64);
    bi = bi.or(BigInteger.valueOf(this.l));
    return new BigDecimal(bi, 18);
  }

  public native String toString();
  private native void scan(String s);

  public native boolean equals(MxDecimal v);
  public native int compareTo(MxDecimal v);

  // data members

  private long h;	// high 64 bits
  private long l;	// low 64 bits
}
