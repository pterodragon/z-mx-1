//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// MxMD JNI

#ifndef MxMDLibJNI_HPP
#define MxMDLibJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

namespace MxMDLibJNI {
  // () -> void
  void ctor_(JNIEnv *, jobject);
  // () -> void
  void dtor_(JNIEnv *, jobject);

  // () -> MxMDLib
  jobject instance(JNIEnv *, jclass);

  // (String) -> MxMDLib
  jobject init(JNIEnv *, jclass, jstring);

  // () -> void
  void start(JNIEnv *, jobject);
  // () -> void
  void stop(JNIEnv *, jobject);

  // (String) -> void
  void record(JNIEnv *, jobject, jstring);
  // () -> void
  void stopRecording(JNIEnv *, jobject);
  // () -> void
  void stopStreaming(JNIEnv *, jobject);

  // (String, Instant, boolean) -> void
  void replay(JNIEnv *, jobject, jstring, jobject, jboolean);
  // () -> void
  void stopReplaying(JNIEnv *, jobject);

  // (Instant) -> void
  void startTimer(JNIEnv *, jobject, jobject);
  // () -> void
  void stopTimer(JNIEnv *, jobject);

  // (MxMDLibHandler) -> void
  void subscribe(JNIEnv *, jobject, jobject);
  // () -> void
  void unsubscribe(JNIEnv *, jobject);
  // () -> MxMDLibHandler
  jobject handler(JNIEnv *, jobject);

  // (MxSecKey, MxMDSecurityFn) -> void
  void security(JNIEnv *, jobject, jobject, jobject);
  // (MxMDAllSecuritiesFn) -> long
  jlong allSecurities(JNIEnv *, jobject, jobject);

  // (MxSecKey, MxMDOrderBookFn) -> void
  void orderBook(JNIEnv *, jobject, jobject, jobject);
  // (MxMDAllOrderBooksFn) -> long
  jlong allOrderBooks(JNIEnv *, jobject, jobject);

  // (String) -> MxMDFeed
  jobject feed(JNIEnv *, jobject, jstring);
  // (MxMDAllFeedsFn) -> long
  jlong allFeeds(JNIEnv *, jobject, jobject);

  // (String) -> MxMDVenue
  jobject venue(JNIEnv *, jobject, jstring);
  // (MxMDAllVenuesFn) -> long
  jlong allVenues(JNIEnv *, jobject, jobject);

  int bind(JNIEnv *);
}

#endif /* MxMDLibJNI_HPP */
