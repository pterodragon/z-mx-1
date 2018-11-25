package com.shardmx.mxmd;

import java.time.Instant;

public interface MxMDInstrEventFn {
  void fn(MxMDInstrument instr, Instant stamp);
}
