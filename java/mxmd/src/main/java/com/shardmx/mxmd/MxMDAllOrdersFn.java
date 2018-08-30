package com.shardmx.mxmd;

public interface MxMDAllOrdersFn {
  long fn(MxMDOrder order);	// non-zero aborts iteration
}
