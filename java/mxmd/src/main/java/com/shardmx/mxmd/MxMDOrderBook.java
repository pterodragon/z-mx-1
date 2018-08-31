package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOrderBook {
  public MxMDOrderBook(long ptr) { this.ptr = ptr; }

  // methods

  public native MxMDLib md();

  public native MxMDVenue venue();

  public native MxMDSecurity security();
  public native MxMDSecurity security(int leg);

  public native MxMDOrderBook out();

  public native String venueID();
  public native String segment();
  public native String id();
  public native MxSecKey key();

  public native int legs();
  public native MxSide side(int leg);
  public native int ratio(int leg);

  public native int pxNDP();
  public native int qtyNDP();

  public native MxMDTickSizeTbl tickSizeTbl();

  public native MxMDLotSizes lotSizes();
  public native MxMDL1Data l1Data();

  public native MxMDOBSide bids();
  public native MxMDOBSide asks();

  public native void subscribe(MxMDSecHandler handler);
  public native void unsubscribe();
  public native MxMDSecHandler handler();

  // data members

  private long ptr;
}

