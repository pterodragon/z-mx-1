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

#ifndef MxMDOrderBookJNI_HPP
#define MxMDOrderBookJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

namespace MxMDOrderBookJNI {
  // () -> void
  void ctor_(JNIEnv *, jobject);
  // () -> void
  void dtor_(JNIEnv *, jobject);

  // () -> MxMDLib
  jobject md(JNIEnv *, jobject);
  // () -> MxMDVenue
  jobject venue(JNIEnv *, jobject);

  // () -> MxMDSecurity
  jobject security(JNIEnv *, jobject);
  // (int) -> MxMDSecurity
  jobject security(JNIEnv *, jobject, jint);

  // () -> MxMDOrderBook
  jobject out(JNIEnv *, jobject);

  // () -> String
  jstring venueID(JNIEnv *, jobject);
  // () -> String
  jstring segment(JNIEnv *, jobject);
  // () -> String
  jstring id(JNIEnv *, jobject);
  // () -> MxSecKey
  jobject key(JNIEnv *, jobject);

  // () -> int
  jint legs(JNIEnv *, jobject);
  // () -> MxSide
  jobject side(JNIEnv *, jobject);
  // (int) -> int
  jint ratio(JNIEnv *, jobject, jint);

  // () -> int
  jint pxNDP(JNIEnv *, jobject);
  // () -> int
  jint qtyNDP(JNIEnv *, jobject);

  // () -> MxMDTickSizeTbl
  jobject tickSizeTbl(JNIEnv *, jobject);

  // () -> MxMDLotSizes
  jobject lotSizes(JNIEnv *, jobject);

  // () -> MxMDL1Data
  jobject l1Data(JNIEnv *, jobject);

  // () -> MxMDOBSide
  jobject bids(JNIEnv *, jobject);
  // () -> MxMDOBSide
  jobject asks(JNIEnv *, jobject);

  // (MxMDSecHandler) -> void
  void subscribe(JNIEnv *, jobject, jobject);
  // () -> void
  void unsubscribe(JNIEnv *, jobject);
  // () -> MxMDSecHandler
  jobject handler(JNIEnv *, jobject);

  int bind(JNIEnv *);
}

#endif /* MxMDOrderBookJNI_HPP */
