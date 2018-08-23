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

#ifndef MxMDOBSideJNI_HPP
#define MxMDOBSideJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

namespace MxMDOBSideJNI {
  // () -> void
  void ctor_(JNIEnv *env, jobject obj);
  // () -> void
  void dtor_(JNIEnv *env, jobject obj);

  // () -> MxMDOrderBook
  jobject orderBook(JNIEnv *env, jobject obj);

  // () -> MxSide
  jobject side(JNIEnv *env, jobject obj);

  // () -> MxMDOBSideData
  jobject data(JNIEnv *env, jobject obj);

  // () -> long
  jlong vwap(JNIEnv *env, jobject obj);

  // () -> MxMDPxLevel
  jobject mktLevel(JNIEnv *env, jobject obj);
  // (MxMDAllPxLevelsFn) -> long
  jlong allPxLevels(JNIEnv *env, jobject obj, jobject);

  int bind(JNIEnv *env);
}

#endif /* MxMDOBSideJNI_HPP */
