package com.shardmx.mxmd;

public interface MxMDAllOrderBooksFn {
  long fn(MxMDOrderBook ob);	// non-zero aborts iteration
}
