package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDTimerFn {
  Instant fn(Instant stamp);	// returns time of next timer event
}
