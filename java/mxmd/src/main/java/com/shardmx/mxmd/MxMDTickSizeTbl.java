package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDTickSizeTbl implements AutoCloseable {
  private MxMDTickSizeTbl(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  // methods

  public native MxMDVenue venue();
  public native String id();
  public native int pxNDP();

  public native long tickSize(long price);
  public native long allTickSizes(MxMDAllTickSizesFn fn);

  // data members

  private long ptr;
}
