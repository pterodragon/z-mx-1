package com.shardmx.mxbase;

public enum MxInstrIDSrc {
  NULL	(null),
  CUSIP	("1"),
  SEDOL	("2"),
  QUIK	("3"),
  ISIN	("4"),
  RIC	("5"),
  EXCH	("8"),
  CTA	("9"),
  BSYM	("A"),
  BBGID	("S"),
  FX	("X"),
  CRYPTO("C"),
  N	(null);

  private static final MxInstrIDSrc values[] = values();
  public static MxInstrIDSrc value(int i) {
    if (Integer.compareUnsigned(i, N.ordinal()) >= 0) return null;
    return values[i];
  }

  private final String fix;
  MxInstrIDSrc(String fix) { this.fix = fix; }
  public String fix() { return this.fix; }
}
