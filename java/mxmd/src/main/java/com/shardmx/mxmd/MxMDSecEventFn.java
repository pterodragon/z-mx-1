package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDSecEventFn {
  void fn(MxMDSecurity sec, Instant stamp);
}
