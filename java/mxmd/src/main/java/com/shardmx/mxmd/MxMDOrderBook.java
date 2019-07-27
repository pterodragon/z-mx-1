package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDOrderBook implements AutoCloseable {
  private MxMDOrderBook(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();

  public native MxMDVenue venue();

  public native MxMDInstrument instrument();
  public native MxMDInstrument instrument(int leg);

  public native MxMDOrderBook out();

  public native String venueID();
  public native String segment();
  public native String id();
  public native MxInstrKey key();

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

  public native void subscribe(MxMDInstrHandler handler);
  public native void unsubscribe();
  public native MxMDInstrHandler handler();

  @Override
  public String toString() {
    if (ptr == 0L) { return ""; }
    return new String()
      + "{key=" + key()
      + ", pxNDP=" + pxNDP()
      + ", qtyNDP=" + qtyNDP()
      + ", tickSizeTbl=" + tickSizeTbl().id()
      + ", lotSizes=" + lotSizes().toString_(qtyNDP())
      + ", l1Data=" + l1Data()
      + "}";
  }

  // data members

  private long ptr;
}

