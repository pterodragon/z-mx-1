package com.shardmx.mxmd;

public class MxMDOBHandle implements AutoCloseable {
  private MxMDOBHandle(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  // methods

  public native void invoke(MxMDOrderBookFn fn);

  // data members

  private long ptr;
}
