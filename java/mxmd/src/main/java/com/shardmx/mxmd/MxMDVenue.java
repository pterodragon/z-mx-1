package com.shardmx.mxmd;

import java.time.Instant;

import com.shardmx.mxbase.*;

public class MxMDVenue implements AutoCloseable {
  private MxMDVenue(long ptr) { ctor_(this.ptr = ptr); }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDLib md();
  public native MxMDFeed feed();
  public native String id();
  public native MxMDOrderIDScope orderIDScope();
  public native long flags();

  public native boolean loaded();

  public native MxMDTickSizeTbl tickSizeTbl(String id);
  public native long allTickSizeTbls(MxMDAllTickSizeTblsFn fn);

  public native long allSegments(MxMDAllSegmentsFn fn);

  public MxMDSegment tradingSession() { return tradingSession(null); }
  public native MxMDSegment tradingSession(String segmentID);

  // data members

  private long ptr;
}
