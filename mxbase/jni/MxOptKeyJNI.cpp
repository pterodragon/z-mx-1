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

#include <MxBase.hpp>

#include <MxPutCallJNI.hpp>

#include <MxOptKeyJNI.hpp>

namespace MxOptKeyJNI {
  jclass	class_; // MxOptKeyTuple

  // MxOptKeyTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of",
      "(ILcom/shardmx/mxbase/MxPutCall;J)"
      "Lcom/shardmx/mxbase/MxOptKeyTuple;" }
  };

  // MxOptKey accessors
  ZJNI::JavaMethod methods[] {
    { "mat", "()I" },
    { "putCall", "()Lcom/shardmx/mxbase/MxPutCall;" },
    { "strike", "()J" },
  };
}

MxOptKey MxOptKeyJNI::j2c(JNIEnv *env, jobject obj)
{
  unsigned mat = env->CallIntMethod(obj, methods[0].mid);
  MxEnum putCall;
  if (jobject pcObj = env->CallObjectMethod(obj, methods[1].mid)) {
    putCall = MxPutCallJNI::j2c(env, pcObj);
    env->DeleteLocalRef(pcObj);
  }
  MxValue strike = env->CallLongMethod(obj, methods[2].mid);
  return MxOptKey{mat, putCall, strike};
}

jobject MxOptKeyJNI::ctor(JNIEnv *env, const MxOptKey &key)
{
  return env->CallStaticObjectMethod(
      class_, ctorMethod[0].mid,
      (jint)key.mat,
      MxPutCallJNI::ctor(env, key.putCall),
      (jlong)key.strike);
}

int MxOptKeyJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxOptKeyTuple");
  if (!class_) return -1;

  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;

  {
    jclass c = env->FindClass("com/shardmx/mxbase/MxOptKey");
    if (!c) return -1;
    if (ZJNI::bind(env, c, methods, sizeof(methods) / sizeof(methods[0])) < 0)
      return -1;
    env->DeleteLocalRef((jobject)c);
  }

  return 0;
}

void MxOptKeyJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
