package com.shardmx.mxbase;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.DecimalFormat;

public class MxDecimal {
  public MxDecimal(long h, long l) { this.h = h; this.l = l; }

  public MxDecimal(String s) { scan(s); }

  // methods

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

  public native boolean isNull();
  public native boolean isReset();

  public native MxDecimal add(MxDecimal v);
  public native MxDecimal subtract(MxDecimal v);
  public native MxDecimal multiply(MxDecimal v);
  public native MxDecimal divide(MxDecimal v);

  public native String toString();
  private native void scan(String s);

  public native boolean equals(MxDecimal v);
  public native int compareTo(MxDecimal v);

  // data members

  private long h;	// high 64 bits
  private long l;	// low 64 bits
}
