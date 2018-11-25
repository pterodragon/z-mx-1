package com.shardmx.mxmd;

import com.shardmx.mxbase.*;

import java.time.Instant;

public class MxMDLib {
  private MxMDLib() { }

  // methods

  public native static MxMDLib instance();

  public native static MxMDLib init(String cf);

  public native void start();
  public native void stop();

  public native void close(); // calls C++ final()

  public native boolean record(String path);
  public native String stopRecording();

  public boolean replay(String path) {
    return replay(path, null, true);
  }
  public boolean replay(String path, Instant begin) {
    return replay(path, begin, true);
  }
  public native boolean replay(String path, Instant begin, boolean filter);
  public native String stopReplaying();

  public void startTimer() { startTimer(null); }
  public native void startTimer(Instant begin);
  public native void stopTimer();

  public native void subscribe(MxMDLibHandler handler);
  public native void unsubscribe();
  public native MxMDLibHandler handler();

  public native MxMDInstrHandle instrument(MxInstrKey key);
  public native void instrument(MxInstrKey key, MxMDInstrumentFn fn);
  public native long allInstruments(MxMDAllInstrumentsFn fn);

  public native MxMDOBHandle orderBook(MxInstrKey key);
  public native void orderBook(MxInstrKey key, MxMDOrderBookFn fn);
  public native long allOrderBooks(MxMDAllOrderBooksFn fn);

  public native MxMDFeed feed(String id);
  public native long allFeeds(MxMDAllFeedsFn fn);

  public native MxMDVenue venue(String id);
  public native long allVenues(MxMDAllVenuesFn fn);
}
