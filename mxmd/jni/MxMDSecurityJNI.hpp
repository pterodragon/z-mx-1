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

#include <jni.h>

namespace MxMDSecurityJNI {
  // () -> MxMDLib
  jobject md(JNIEnv *, jobject);

  // () -> String
  jstring primaryVenue(JNIEnv *, jobject);

  // () -> String
  jstring primarySegment(JNIEnv *, jobject);

  // () -> String
  jstring id(JNIEnv *, jobject);

  // () -> MxSecKey
  jobject key(JNIEnv *, jobject);

  // () -> MxMDSecRefData
  jobject refData(JNIEnv *, jobject);

  // () -> MxMDSecurity
  jobject underlying(JNIEnv *, jobject);

  // () -> MxMDDerivatives
  jobject derivatives(JNIEnv *, jobject);

  // (MxMDSecHandler) -> void
  void subscribe(JNIEnv *, jobject, jobject);
  // () -> void
  void unsubscribe(JNIEnv *, jobject);
  // () -> MxMDSecHandler
  jobject handler(JNIEnv *, jobject);

  // (String) -> MxMDOrderBook
  jobject orderBook(JNIEnv *, jobject, jstring);

  // (String, String) -> MxMDOrderBook
  jobject orderBook(JNIEnv *, jobject, jstring, jstring);

  // (MxMDAllOrderBooksFn) -> long
  jlong allOrderBooks(JNIEnv *, jobject, jobject);

  jobject ctor(JNIEnv *, void *ptr);
  int bind(JNIEnv *);
  void final(JNIEnv *);
}

#endif /* MxMDSecurityJNI_HPP */
