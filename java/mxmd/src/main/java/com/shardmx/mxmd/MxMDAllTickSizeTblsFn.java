package com.shardmx.mxmd;

public interface MxMDAllTickSizeTblsFn {
  long fn(MxMDTickSizeTbl tbl);	// non-zero aborts iteration
}
