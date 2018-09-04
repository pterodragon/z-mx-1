package com.shardmx.mxmd;

public interface MxMDAllFeedsFn {
  long fn(MxMDFeed feed);	// non-zero aborts iteration
}
