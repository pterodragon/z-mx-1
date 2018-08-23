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

#ifndef MxMDSecurityJNI_HPP
#define MxMDSecurityJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

namespace MxMDSecurityJNI {
  // () -> void
  void ctor_(JNIEnv *env, jobject obj);
  // () -> void
  void dtor_(JNIEnv *env, jobject obj);

  // () -> MxMDLib
  jobject md(JNIEnv *env, jobject obj);

  // () -> String
  jstring primaryVenue(JNIEnv *env, jobject obj);

  // () -> String
  jstring primarySegment(JNIEnv *env, jobject obj);

  // () -> String
  jstring id(JNIEnv *env, jobject obj);

  // () -> MxSecKey
  jobject key(JNIEnv *env, jobject obj);

  // () -> MxMDSecRefData
  jobject refData(JNIEnv *env, jobject obj);

  // () -> MxMDSecurity
  jobject underlying(JNIEnv *env, jobject obj);

  // () -> MxMDDerivatives
  jobject derivatives(JNIEnv *env, jobject obj);

  // (MxMDSecHandler) -> void
  void subscribe(JNIEnv *env, jobject obj, jobject);
  // () -> void
  void unsubscribe(JNIEnv *env, jobject obj);
  // () -> MxMDSecHandler
  jobject handler(JNIEnv *env, jobject obj);

  // (String) -> MxMDOrderBook
  jobject orderBook(JNIEnv *env, jobject obj, jstring);

  // (String, String) -> MxMDOrderBook
  jobject orderBook(JNIEnv *env, jobject obj, jstring, jstring);

  // (MxMDAllOrderBooksFn) -> long
  jlong allOrderBooks(JNIEnv *env, jobject obj, jobject);

  int bind(JNIEnv *env);
}

#endif /* MxMDSecurityJNI_HPP */
