package com.shardmx.mxbase;

import org.immutables.value.Value;

@Value.Style(
  allParameters = true,
  // typeAbstract = "*",
  typeImmutable = "*Tuple",
  // of = "create",
  // get = "get*",
  // init = "set*",
  builder = "new",
  defaults = @Value.Immutable(copy = false))
public @interface MxTuple { }
