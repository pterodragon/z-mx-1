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

#ifndef MxMDTickSizeTblJNI_HPP
#define MxMDTickSizeTblJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

namespace MxMDTickSizeTblJNI {
  // () -> void
  void ctor_(JNIEnv *env, jobject obj);
  // () -> void
  void dtor_(JNIEnv *env, jobject obj);

  // () -> MxMDVenue
  jobject venue(JNIEnv *env, jobject obj);

  // () -> String
  jstring id(JNIEnv *env, jobject obj);

  // () -> int
  jint pxNDP(JNIEnv *env, jobject obj);

  // (long) -> long
  jlong tickSize(JNIEnv *env, jobject obj, jlong);

  // (MxMDAllTickSizesFn) -> long
  jlong allTickSizes(JNIEnv *env, jobject obj, jobject);

  int bind(JNIEnv *env);
}

#endif /* MxMDTickSizeTblJNI_HPP */
