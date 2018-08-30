package com.shardmx.mxmd;

public interface MxMDAllSegmentsFn {
  long fn(MxMDSegment segment);	// non-zero aborts iteration
}
