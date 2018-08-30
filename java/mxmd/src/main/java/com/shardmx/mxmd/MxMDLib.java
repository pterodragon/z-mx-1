package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

import java.time.Instant;

public class MxMDLib implements AutoCloseable {
  public MxMDLib(long ptr) { ctor_(this.ptr = ptr); }
  public void close() { dtor_(this.ptr); }

  private native void ctor_(long ptr);
  private native void dtor_(long ptr);

  // methods

  public native static MxMDLib instance();

  public native static MxMDLib init(String cf);

  public native void start();
  public native void stop();

  public native void record(String path);
  public native void stopRecording();
  public native void stopStreaming();

  public void replay(String path) { replay(path, null, true); }
  public void replay(String path, Instant begin) { replay(path, begin, true); }
  public native void replay(String path, Instant begin, boolean filter);
  public native void stopReplaying();

  public void startTimer() { startTimer(null); }
  public native void startTimer(Instant begin);
  public native void stopTimer();

  public native void subscribe(MxMDLibHandler handler);
  public native void unsubscribe();
  public native MxMDLibHandler handler();

  public native void security(MxSecKey key, MxMDSecurityFn fn);
  public native long allSecurities(MxMDAllSecuritiesFn fn);

  public native void orderBook(MxSecKey key, MxMDOrderBookFn fn);
  public native long allOrderBooks(MxMDAllOrderBooksFn fn);

  public native MxMDFeed feed(String id);
  public native long allFeeds(MxMDAllFeedsFn fn);

  public native MxMDVenue venue(String id);
  public native long allVenues(MxMDAllVenuesFn fn);

  // data members

  private long ptr;
}
