package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDSecurity {
  private MxMDSecurity(long ptr) { this.ptr = ptr; }

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
