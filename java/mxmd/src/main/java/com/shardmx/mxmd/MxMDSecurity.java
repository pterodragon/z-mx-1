package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDSecurity implements AutoCloseable {
  private MxMDSecurity(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();

  public native String primaryVenue();
  public native String primarySegment();
  public native String id();
  public native MxSecKey key();

  public native MxMDSecRefData refData();

  public native MxMDSecurity underlying();
  public native MxMDDerivatives derivatives();

  public native void subscribe(MxMDSecHandler handler);
  public native void unsubscribe();
  public native MxMDSecHandler handler();

  public native MxMDOrderBook orderBook(String venueID);
  public native MxMDOrderBook orderBook(String venueID, String segmentID);
  public native long allOrderBooks(MxMDAllOrderBooksFn fn);
 
  // data members

  private long ptr;
}
