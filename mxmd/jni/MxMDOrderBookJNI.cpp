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

#include <MxMD.hpp>

#include <MxMDOrderBookJNI.hpp>

namespace MxMDOrderBookJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDOrderBook *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDOrderBook *)(void *)ptr;
  }
}

void MxMDOrderBookJNI::ctor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOrderBook *)(void *)(uintptr_t)ptr)->ref();
}

void MxMDOrderBookJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOrderBook *)(void *)(uintptr_t)ptr)->deref();
}

jobject MxMDOrderBookJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib

  return 0;
}

jobject MxMDOrderBookJNI::venue(JNIEnv *env, jobject obj)
{
  // () -> MxMDVenue

  return 0;
}

jobject MxMDOrderBookJNI::security(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecurity

  return 0;
}

jobject MxMDOrderBookJNI::security(JNIEnv *env, jobject obj, jint)
{
  // (int) -> MxMDSecurity

  return 0;
}

jobject MxMDOrderBookJNI::out(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook

  return 0;
}

jstring MxMDOrderBookJNI::venueID(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jstring MxMDOrderBookJNI::segment(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jstring MxMDOrderBookJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jobject MxMDOrderBookJNI::key(JNIEnv *env, jobject obj)
{
  // () -> MxSecKey

  return 0;
}

jint MxMDOrderBookJNI::legs(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jobject MxMDOrderBookJNI::side(JNIEnv *env, jobject obj)
{
  // () -> MxSide

  return 0;
}

jint MxMDOrderBookJNI::ratio(JNIEnv *env, jobject obj, jint)
{
  // (int) -> int

  return 0;
}

jint MxMDOrderBookJNI::pxNDP(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jint MxMDOrderBookJNI::qtyNDP(JNIEnv *env, jobject obj)
{
  // () -> int

  return 0;
}

jobject MxMDOrderBookJNI::tickSizeTbl(JNIEnv *env, jobject obj)
{
  // () -> MxMDTickSizeTbl

  return 0;
}

jobject MxMDOrderBookJNI::lotSizes(JNIEnv *env, jobject obj)
{
  // () -> MxMDLotSizes

  return 0;
}

jobject MxMDOrderBookJNI::l1Data(JNIEnv *env, jobject obj)
{
  // () -> MxMDL1Data

  return 0;
}

jobject MxMDOrderBookJNI::bids(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide

  return 0;
}

jobject MxMDOrderBookJNI::asks(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide

  return 0;
}

void MxMDOrderBookJNI::subscribe(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDSecHandler) -> void

}

void MxMDOrderBookJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDOrderBookJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecHandler

  return 0;
}

int MxMDOrderBookJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDOrderBookJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDOrderBookJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDOrderBookJNI::md },
    { "venue",
      "()Lcom/shardmx/mxmd/MxMDVenue;",
      (void *)&MxMDOrderBookJNI::venue },
    { "security",
      "()Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject)>(
	  MxMDOrderBookJNI::security) },
    { "security",
      "(I)Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jint)>(
	  MxMDOrderBookJNI::security) },
    { "out",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDOrderBookJNI::out },
    { "venueID",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::venueID },
    { "segment",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::segment },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::id },
    { "key",
      "()Lcom/shardmx/mxbase/MxSecKey;",
      (void *)&MxMDOrderBookJNI::key },
    { "legs",
      "()I",
      (void *)&MxMDOrderBookJNI::legs },
    { "side",
      "()Lcom/shardmx/mxbase/MxSide;",
      (void *)&MxMDOrderBookJNI::side },
    { "ratio",
      "(I)I",
      (void *)&MxMDOrderBookJNI::ratio },
    { "pxNDP",
      "()I",
      (void *)&MxMDOrderBookJNI::pxNDP },
    { "qtyNDP",
      "()I",
      (void *)&MxMDOrderBookJNI::qtyNDP },
    { "tickSizeTbl",
      "()Lcom/shardmx/mxmd/MxMDTickSizeTbl;",
      (void *)&MxMDOrderBookJNI::tickSizeTbl },
    { "lotSizes",
      "()Lcom/shardmx/mxmd/MxMDLotSizes;",
      (void *)&MxMDOrderBookJNI::lotSizes },
    { "l1Data",
      "()Lcom/shardmx/mxmd/MxMDL1Data;",
      (void *)&MxMDOrderBookJNI::l1Data },
    { "bids",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDOrderBookJNI::bids },
    { "asks",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDOrderBookJNI::asks },
    { "subscribe",
      "(Lcom/shardmx/mxmd/MxMDSecHandler;)V",
      (void *)&MxMDOrderBookJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDOrderBookJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDSecHandler;",
      (void *)&MxMDOrderBookJNI::handler },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrderBook");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDOrderBookJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
