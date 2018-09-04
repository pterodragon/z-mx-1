package com.shardmx.mxmd;

public interface MxMDAllTickSizesFn {
  long fn(MxMDTickSize ts);	// non-zero aborts iteration
}
