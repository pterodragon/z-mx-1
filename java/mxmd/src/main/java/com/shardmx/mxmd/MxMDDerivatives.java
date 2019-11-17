package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDDerivatives implements AutoCloseable {
  private MxMDDerivatives(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() {
    if (this.ptr != 0L) {
      dtor_(this.ptr);
      this.ptr = 0L;
    }
  }
  private native void dtor_(long ptr);

  // methods

  public native MxMDInstrument future(MxFutKey key);
  public native long allFutures(MxMDAllInstrumentsFn fn); 

  public native MxMDInstrument option(MxOptKey key);
  public native long allOptions(MxMDAllInstrumentsFn fn); 
 
  // data members

  private long ptr;
}
