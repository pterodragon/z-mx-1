package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDDerivatives implements AutoCloseable {
  private MxMDDerivatives(long ptr) { this.ptr = ptr; }
  public void finalize() { close(); }
  public void close() { dtor_(this.ptr); this.ptr = 0; }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native MxMDSecurity future(MxFutKey key);
  public native long allFutures(MxMDAllSecuritiesFn fn); 

  public native MxMDSecurity option(MxOptKey key);
  public native long allOptions(MxMDAllSecuritiesFn fn); 
 
  // data members

  private long ptr;
}
