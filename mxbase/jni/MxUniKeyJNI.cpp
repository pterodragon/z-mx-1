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

#include <MxInstrIDSrcJNI.hpp>
#include <MxPutCallJNI.hpp>

#include <MxUniKeyJNI.hpp>

namespace MxUniKeyJNI {
  jclass	class_; // MxUniKeyTuple

  // MxUniKeyTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
      "Lcom/shardmx/mxbase/MxInstrIDSrc;ILcom/shardmx/mxbase/MxPutCall;J)"
      "Lcom/shardmx/mxbase/MxUniKeyTuple;" }
  };

  // MxUniKey accessors
  ZJNI::JavaMethod methods[] {
    { "id", "()Ljava/lang/String;" },
    { "venue", "()Ljava/lang/String;" },
    { "segment", "()Ljava/lang/String;" },
    { "src", "()Lcom/shardmx/mxbase/MxInstrIDSrc;" },
    { "mat", "()I" },
    { "putCall", "()Lcom/shardmx/mxbase/MxPutCall;" },
    { "strike", "()J" },
  };
}

MxUniKey MxUniKeyJNI::j2c(JNIEnv *env, jobject obj)
{
  jstring id = (jstring)(env->CallObjectMethod(obj, methods[0].mid));
  jstring venue = (jstring)(env->CallObjectMethod(obj, methods[1].mid));
  jstring segment = (jstring)(env->CallObjectMethod(obj, methods[2].mid));
  MxEnum src;
  if (jobject srcObj = env->CallObjectMethod(obj, methods[3].mid)) {
    src = MxInstrIDSrcJNI::j2c(env, srcObj);
    env->DeleteLocalRef(srcObj);
  }
  unsigned mat = env->CallIntMethod(obj, methods[4].mid);
  MxEnum putCall;
  if (jobject pcObj = env->CallObjectMethod(obj, methods[5].mid)) {
    putCall = MxPutCallJNI::j2c(env, pcObj);
    env->DeleteLocalRef(pcObj);
  }
  MxValue strike = env->CallLongMethod(obj, methods[6].mid);
  return MxUniKey{
    ZJNI::j2s_ZuStringN<MxIDStrSize>(env, id, true),
    ZJNI::j2s_ZuStringN<8>(env, venue, true),
    ZJNI::j2s_ZuStringN<8>(env, segment, true),
    src, mat, putCall, strike};
}

jobject MxUniKeyJNI::ctor(JNIEnv *env, const MxUniKey &key)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      ZJNI::s2j(env, key.id),
      ZJNI::s2j(env, key.venue),
      ZJNI::s2j(env, key.segment),
      MxInstrIDSrcJNI::ctor(env, key.src),
      (jint)key.mat,
      MxPutCallJNI::ctor(env, key.putCall),
      (jlong)key.strike);
}

int MxUniKeyJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxUniKeyTuple");
  if (!class_) return -1;

  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;

  {
    jclass c = env->FindClass("com/shardmx/mxbase/MxUniKey");
    if (!c) return -1;
    if (ZJNI::bind(env, c, methods, sizeof(methods) / sizeof(methods[0])) < 0)
      return -1;
    env->DeleteLocalRef((jobject)c);
  }

  return 0;
}

void MxUniKeyJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
