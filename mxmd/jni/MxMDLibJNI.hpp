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
  void ctor_(JNIEnv *env, jobject obj);
  // () -> void
  void dtor_(JNIEnv *env, jobject obj);
  // () -> MxMDLib
  jobject instance(JNIEnv *env, jobject obj);

  // (String) -> MxMDLib
  jobject init(JNIEnv *env, jobject obj, jstring);

  // () -> void
  void start(JNIEnv *env, jobject obj);
  // () -> void
  void stop(JNIEnv *env, jobject obj);

  // (String) -> void
  void record(JNIEnv *env, jobject obj, jstring);
  // () -> void
  void stopRecording(JNIEnv *env, jobject obj);
  // () -> void
  void stopStreaming(JNIEnv *env, jobject obj);

  // (String, String, String) -> void
  void replay(JNIEnv *env, jobject obj, jstring, jstring, jstring);
  // () -> void
  void stopReplaying(JNIEnv *env, jobject obj);

  // (Instant) -> void
  void startTimer(JNIEnv *env, jobject obj, jobject);
  // () -> void
  void stopTimer(JNIEnv *env, jobject obj);

  // (MxMDLibHandler) -> void
  void subscribe(JNIEnv *env, jobject obj, jobject);
  // () -> void
  void unsubscribe(JNIEnv *env, jobject obj);
  // () -> MxMDLibHandler
  jobject handler(JNIEnv *env, jobject obj);

  // (MxSecKey, MxSecKey) -> void
  void security(JNIEnv *env, jobject obj, jobject, jobject);
  // (MxMDAllSecuritiesFn) -> long
  jlong allSecurities(JNIEnv *env, jobject obj, jobject);

  // (MxSecKey, MxSecKey) -> void
  void orderBook(JNIEnv *env, jobject obj, jobject, jobject);
  // (MxMDAllOrderBooksFn) -> long
  jlong allOrderBooks(JNIEnv *env, jobject obj, jobject);

  // (String) -> MxMDFeed
  jobject feed(JNIEnv *env, jobject obj, jstring);
  // (MxMDAllFeedsFn) -> long
  jlong allFeeds(JNIEnv *env, jobject obj, jobject);

  // (String) -> MxMDVenue
  jobject venue(JNIEnv *env, jobject obj, jstring);
  // (MxMDAllVenuesFn) -> long
  jlong allVenues(JNIEnv *env, jobject obj, jobject);

  int bind(JNIEnv *env);
}

#endif /* MxMDLibJNI_HPP */
