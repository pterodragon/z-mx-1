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

#include <MxMDTickSizeTblJNI.hpp>

void MxMDTickSizeTblJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDTickSizeTblJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDTickSizeTblJNI::venue(JNIEnv *env, jobject obj)
{
  // () -> MxMDVenue

  return 0;
}

jstring MxMDTickSizeTblJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jint MxMDTickSizeTblJNI::pxNDP(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jlong MxMDTickSizeTblJNI::tickSize(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> long

  return 0;
}

jlong MxMDTickSizeTblJNI::allTickSizes(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllTickSizesFn) -> long

  return 0;
}

int MxMDTickSizeTblJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDTickSizeTblJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDTickSizeTblJNI::dtor_ },
    { "venue",
      "()Lcom/shardmx/mxmd/MxMDVenue;",
      (void *)&MxMDTickSizeTblJNI::venue },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDTickSizeTblJNI::id },
    { "pxNDP",
      "()I",
      (void *)&MxMDTickSizeTblJNI::pxNDP },
    { "tickSize",
      "(J)J",
      (void *)&MxMDTickSizeTblJNI::tickSize },
    { "allTickSizes",
      "(Lcom/shardmx/mxmd/MxMDAllTickSizesFn;)J",
      (void *)&MxMDTickSizeTblJNI::allTickSizes },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDTickSizeTbl",
        methods, sizeof(methods) / sizeof(methods[0]));
}
