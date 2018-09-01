package com.shardmx.mxmd;

public class MxMDSecHandle implements AutoCloseable {
  private MxMDSecHandle(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native void invoke(MxMDSecurityFn fn);

  // data members

  private long ptr;
}
