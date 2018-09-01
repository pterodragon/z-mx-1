package com.shardmx.mxmd;

public class MxMDOBHandle implements AutoCloseable {
  private MxMDOBHandle(long ptr) { ctor_(this.ptr = ptr); }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native void invoke(MxMDOrderBookFn fn);

  // data members

  private long ptr;
}
