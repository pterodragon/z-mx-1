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

#include <MxMDFeedJNI.hpp>

void MxMDFeedJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDFeedJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDFeedJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib

  return 0;
}

jstring MxMDFeedJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jint MxMDFeedJNI::level(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

int MxMDFeedJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDFeedJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDFeedJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDFeedJNI::md },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDFeedJNI::id },
    { "level",
      "()I",
      (void *)&MxMDFeedJNI::level },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDFeed",
        methods, sizeof(methods) / sizeof(methods[0]));
}
