package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDInstrument implements AutoCloseable {
  private MxMDInstrument(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();

  public native String primaryVenue();
  public native String primarySegment();
  public native String id();
  public native MxInstrKey key();
  public native void keys(MxMDInstrKeyFn fn);

  public native MxMDInstrRefData refData();

  public native MxMDInstrument underlying();
  public native MxMDDerivatives derivatives();

  public native void subscribe(MxMDInstrHandler handler);
  public native void unsubscribe();
  public native MxMDInstrHandler handler();

  public native MxMDOrderBook orderBook(String venueID);
  public native MxMDOrderBook orderBook(String venueID, String segmentID);
  public native long allOrderBooks(MxMDAllOrderBooksFn fn);

  @Override
  public String toString() {
    if (ptr == 0L) { return ""; }
    return new String()
      + "{key=" + key()
      + ", refData=" + refData()
      + "}";
  }
 
  // data members

  private long ptr;
}
