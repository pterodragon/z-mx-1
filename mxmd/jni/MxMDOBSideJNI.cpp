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

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMDOBSideJNI.hpp>

void MxMDOBSideJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void

}

void MxMDOBSideJNI::dtor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void

}

jobject MxMDOBSideJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook

  return 0;
}

jobject MxMDOBSideJNI::side(JNIEnv *env, jobject obj)
{
  // () -> MxSide

  return 0;
}

jobject MxMDOBSideJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSideData

  return 0;
}

jlong MxMDOBSideJNI::vwap(JNIEnv *env, jobject obj)
{
  // () -> long

  return 0;
}

jobject MxMDOBSideJNI::mktLevel(JNIEnv *env, jobject obj)
{
  // () -> MxMDPxLevel

  return 0;
}

jlong MxMDOBSideJNI::allPxLevels(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllPxLevelsFn) -> long

  return 0;
}

int MxMDOBSideJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDOBSideJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDOBSideJNI::dtor_ },
    { "orderBook",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDOBSideJNI::orderBook },
    { "side",
      "()Lcom/shardmx/mxbase/MxSide;",
      (void *)&MxMDOBSideJNI::side },
    { "data",
      "()Lcom/shardmx/mxmd/MxMDOBSideData;",
      (void *)&MxMDOBSideJNI::data },
    { "vwap",
      "()J",
      (void *)&MxMDOBSideJNI::vwap },
    { "mktLevel",
      "()Lcom/shardmx/mxmd/MxMDPxLevel;",
      (void *)&MxMDOBSideJNI::mktLevel },
    { "allPxLevels",
      "(Lcom/shardmx/mxmd/MxMDAllPxLevelsFn;)J",
      (void *)&MxMDOBSideJNI::allPxLevels },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDOBSide",
        methods, sizeof(methods) / sizeof(methods[0]));
}

