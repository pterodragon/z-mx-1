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

#include <MxSecKeyJNI.hpp>

namespace MxSecKeyJNI {
  jclass	class_; // MxSecKeyTuple

  // MxSecKeyTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)"
      "Lcom/shardmx/mxbase/MxSecKeyTuple;" }
  };

  // MxSecKey accessors
  ZJNI::JavaMethod methods[] {
    { "id", "()Ljava/lang/String;" },
    { "venue", "()Ljava/lang/String;" },
    { "segment", "()Ljava/lang/String;" },
  };
}

MxSecKey MxSecKeyJNI::j2c(JNIEnv *env, jobject obj)
{
  jstring id = (jstring)(env->CallObjectMethod(obj, methods[0].mid));
  jstring venue = (jstring)(env->CallObjectMethod(obj, methods[1].mid));
  jstring segment = (jstring)(env->CallObjectMethod(obj, methods[2].mid));

  MxSecKey key{
    ZJNI::j2s_ZuStringN<MxIDStrSize>(env, id),
    ZJNI::j2s_ZuStringN<8>(env, venue),
    ZJNI::j2s_ZuStringN<8>(env, segment)};

  env->DeleteLocalRef(id);
  env->DeleteLocalRef(venue);
  env->DeleteLocalRef(segment);

  return key;
}

jobject MxSecKeyJNI::ctor(JNIEnv *env, const MxSecKey &key)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      ZJNI::s2j(env, key.id),
      ZJNI::s2j(env, key.venue),
      ZJNI::s2j(env, key.segment));
}

int MxSecKeyJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxSecKeyTuple");
  if (!class_) return -1;

  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;

  {
    jclass c = env->FindClass("com/shardmx/mxbase/MxSecKey");
    if (!c) return -1;
    if (ZJNI::bind(env, c, methods, sizeof(methods) / sizeof(methods[0])) < 0)
      return -1;
    env->DeleteLocalRef((jobject)c);
  }

  return 0;
}

void MxSecKeyJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
