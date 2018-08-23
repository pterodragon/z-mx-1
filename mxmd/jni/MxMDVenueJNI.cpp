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

#include <MxMDVenueJNI.hpp>

void MxMDVenueJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDVenueJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDVenueJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib

  return 0;
}

jobject MxMDVenueJNI::feed(JNIEnv *env, jobject obj)
{
  // () -> MxMDFeed

  return 0;
}

jstring MxMDVenueJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jobject MxMDVenueJNI::orderIDScope(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderIDScope

  return 0;
}

jlong MxMDVenueJNI::flags(JNIEnv *env, jobject obj)
{
  // () -> long

  return 0;
}

jobject MxMDVenueJNI::tickSizeTbl(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> MxMDTickSizeTbl

  return 0;
}

jlong MxMDVenueJNI::allTickSizeTbls(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllTickSizeTblsFn) -> long

  return 0;
}

void MxMDVenueJNI::allSegments(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllSegmentsFn) -> void

}

jobject MxMDVenueJNI::tradingSession(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> MxMDSegment

  return 0;
}

int MxMDVenueJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDVenueJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDVenueJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDVenueJNI::md },
    { "feed",
      "()Lcom/shardmx/mxmd/MxMDFeed;",
      (void *)&MxMDVenueJNI::feed },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDVenueJNI::id },
    { "orderIDScope",
      "()Lcom/shardmx/mxmd/MxMDOrderIDScope;",
      (void *)&MxMDVenueJNI::orderIDScope },
    { "flags",
      "()J",
      (void *)&MxMDVenueJNI::flags },
    { "tickSizeTbl",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDTickSizeTbl;",
      (void *)&MxMDVenueJNI::tickSizeTbl },
    { "allTickSizeTbls",
      "(Lcom/shardmx/mxmd/MxMDAllTickSizeTblsFn;)J",
      (void *)&MxMDVenueJNI::allTickSizeTbls },
    { "allSegments",
      "(Lcom/shardmx/mxmd/MxMDAllSegmentsFn;)V",
      (void *)&MxMDVenueJNI::allSegments },
    { "tradingSession",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDSegment;",
      (void *)&MxMDVenueJNI::tradingSession },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDVenue",
        methods, sizeof(methods) / sizeof(methods[0]));
}
