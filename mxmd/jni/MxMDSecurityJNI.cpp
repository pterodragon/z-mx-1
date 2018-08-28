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

#include <MxMDSecurityJNI.hpp>

namespace MxMDSecurityJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDSecurity *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDSecurity *)(void *)ptr;
  }
}

void MxMDSecurityJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void

}

void MxMDSecurityJNI::dtor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void

}

jobject MxMDSecurityJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib

  return 0;
}

jstring MxMDSecurityJNI::primaryVenue(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jstring MxMDSecurityJNI::primarySegment(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jstring MxMDSecurityJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jobject MxMDSecurityJNI::key(JNIEnv *env, jobject obj)
{
  // () -> MxSecKey

  return 0;
}

jobject MxMDSecurityJNI::refData(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecRefData

  return 0;
}

jobject MxMDSecurityJNI::underlying(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecurity

  return 0;
}

jobject MxMDSecurityJNI::derivatives(JNIEnv *env, jobject obj)
{
  // () -> MxMDDerivatives

  return 0;
}

void MxMDSecurityJNI::subscribe(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDSecHandler) -> void

}

void MxMDSecurityJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDSecurityJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecHandler

  return 0;
}

jobject MxMDSecurityJNI::orderBook(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> MxMDOrderBook

  return 0;
}

jobject MxMDSecurityJNI::orderBook(JNIEnv *env, jobject obj, jstring, jstring)
{
  // (String, String) -> MxMDOrderBook

  return 0;
}

jlong MxMDSecurityJNI::allOrderBooks(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllOrderBooksFn) -> long

  return 0;
}

int MxMDSecurityJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDSecurityJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDSecurityJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDSecurityJNI::md },
    { "primaryVenue",
      "()Ljava/lang/String;",
      (void *)&MxMDSecurityJNI::primaryVenue },
    { "primarySegment",
      "()Ljava/lang/String;",
      (void *)&MxMDSecurityJNI::primarySegment },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDSecurityJNI::id },
    { "key",
      "()Lcom/shardmx/mxbase/MxSecKey;",
      (void *)&MxMDSecurityJNI::key },
    { "refData",
      "()Lcom/shardmx/mxmd/MxMDSecRefData;",
      (void *)&MxMDSecurityJNI::refData },
    { "underlying",
      "()Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)&MxMDSecurityJNI::underlying },
    { "derivatives",
      "()Lcom/shardmx/mxmd/MxMDDerivatives;",
      (void *)&MxMDSecurityJNI::derivatives },
    { "subscribe",
      "(Lcom/shardmx/mxmd/MxMDSecHandler;)V",
      (void *)&MxMDSecurityJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDSecurityJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDSecHandler;",
      (void *)&MxMDSecurityJNI::handler },
    { "orderBook",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jstring)>(
	  MxMDSecurityJNI::orderBook) },
    { "orderBook",
      "(Ljava/lang/String;Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jstring, jstring)>(
	  MxMDSecurityJNI::orderBook) },
    { "allOrderBooks",
      "(Lcom/shardmx/mxmd/MxMDAllOrderBooksFn;)J",
      (void *)&MxMDSecurityJNI::allOrderBooks },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDSecurity");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDSecurityJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
