package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDDerivatives implements AutoCloseable {
  public MxMDDerivatives(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

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
