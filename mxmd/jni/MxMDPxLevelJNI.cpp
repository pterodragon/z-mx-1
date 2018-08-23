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

#include <iostream>

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMDPxLevelJNI.hpp>

void MxMDPxLevelJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDPxLevelJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDPxLevelJNI::obSide(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide

  return 0;
}

jobject MxMDPxLevelJNI::side(JNIEnv *env, jobject obj)
{
  // () -> MxSide

  return 0;
}

jint MxMDPxLevelJNI::pxNDP(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jint MxMDPxLevelJNI::qtyNDP(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jlong MxMDPxLevelJNI::price(JNIEnv *env, jobject obj)
{
  // () -> long

  return 0;
}

jobject MxMDPxLevelJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDPxLvlData

  return 0;
}

jlong MxMDPxLevelJNI::allOrders(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllOrdersFn) -> long

  return 0;
}

int MxMDPxLevelJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDPxLevelJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDPxLevelJNI::dtor_ },
    { "obSide",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDPxLevelJNI::obSide },
    { "side",
      "()Lcom/shardmx/mxbase/MxSide;",
      (void *)&MxMDPxLevelJNI::side },
    { "pxNDP",
      "()I",
      (void *)&MxMDPxLevelJNI::pxNDP },
    { "qtyNDP",
      "()I",
      (void *)&MxMDPxLevelJNI::qtyNDP },
    { "price",
      "()J",
      (void *)&MxMDPxLevelJNI::price },
    { "data",
      "()Lcom/shardmx/mxmd/MxMDPxLvlData;",
      (void *)&MxMDPxLevelJNI::data },
    { "allOrders",
      "(Lcom/shardmx/mxmd/MxMDAllOrdersFn;)J",
      (void *)&MxMDPxLevelJNI::allOrders },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDPxLevel",
        methods, sizeof(methods) / sizeof(methods[0]));
}
