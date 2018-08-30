package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDOBEventFn {
  void fn(MxMDOrderBook ob, Instant stamp);
}
