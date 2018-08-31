package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

public class MxMDDerivatives {
  private MxMDDerivatives(long ptr) { this.ptr = ptr; }

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
