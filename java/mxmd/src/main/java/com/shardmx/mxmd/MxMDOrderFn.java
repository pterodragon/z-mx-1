package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDOrderFn {
  void fn(MxMDOrder order, Instant stamp);
}
