package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDTrade implements AutoCloseable {
  private MxMDTrade(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDInstrument instrument();
  public native MxMDOrderBook orderBook();

  public native MxMDTradeData data();

  public String toString() {
    MxMDTradeData data_ = data();
    MxMDOrderBook ob = orderBook();
    return new String()
      + "{tradeID=" + data_.tradeID()
      + ", transactTime=" + data_.transactTime()
      + ", price=" + new MxValNDP(data_.price(), ob.pxNDP())
      + ", qty=" + new MxValNDP(data_.qty(), ob.qtyNDP())
      + "}";
  }

  // data members

  private long ptr;
}
