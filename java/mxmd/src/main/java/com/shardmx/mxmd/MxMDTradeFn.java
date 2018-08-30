package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDTradeFn {
  void fn(MxMDTrade trade, Instant stamp);
}
