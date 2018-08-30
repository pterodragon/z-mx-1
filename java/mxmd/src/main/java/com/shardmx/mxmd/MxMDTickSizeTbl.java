package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDTickSizeTbl implements AutoCloseable {
  public MxMDTickSizeTbl(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

  private native void ctor_(long ptr);
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
