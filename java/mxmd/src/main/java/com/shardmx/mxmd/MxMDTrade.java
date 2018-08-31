package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDTrade {
  public MxMDTrade(long ptr) { this.ptr = ptr; }

  // methods

  public native MxMDSecurity security();
  public native MxMDOrderBook orderBook();

  public native MxMDTradeData data();

  // data members

  private long ptr;
}
