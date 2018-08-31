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

#include <MxMDVenueJNI.hpp>

#include <MxMDTickSizeJNI.hpp>

#include <MxMDTickSizeTblJNI.hpp>

namespace MxMDTickSizeTblJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDTickSizeTbl *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDTickSizeTbl *)(void *)ptr;
  }

  // query callbacks
  ZJNI::JavaMethod allTickSizesFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSize;)J" }
  };
}

void MxMDTickSizeTblJNI::ctor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDTickSizeTbl *)(void *)(uintptr_t)ptr)->ref();
}

void MxMDTickSizeTblJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDTickSizeTbl *)(void *)(uintptr_t)ptr)->deref();
  env->SetLongField(obj, ptrField[0].fid, (jlong)0);
}

jobject MxMDTickSizeTblJNI::venue(JNIEnv *env, jobject obj)
{
  // () -> MxMDVenue
  MxMDTickSizeTbl *tbl = ptr_(env, obj);
  if (ZuUnlikely(!tbl)) return 0;
  return MxMDVenueJNI::ctor(env, tbl->venue());
}

jstring MxMDTickSizeTblJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDTickSizeTbl *tbl = ptr_(env, obj);
  if (ZuUnlikely(!tbl)) return 0;
  return ZJNI::s2j(env, tbl->id());
}

jint MxMDTickSizeTblJNI::pxNDP(JNIEnv *env, jobject obj)
{
  // () -> int
  MxMDTickSizeTbl *tbl = ptr_(env, obj);
  if (ZuUnlikely(!tbl)) return 0;
  return tbl->pxNDP();
}

jlong MxMDTickSizeTblJNI::tickSize(JNIEnv *env, jobject obj, jlong price)
{
  // (long) -> long
  MxMDTickSizeTbl *tbl = ptr_(env, obj);
  if (ZuUnlikely(!tbl)) return 0;
  return tbl->tickSize(price);
}

jlong MxMDTickSizeTblJNI::allTickSizes(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllTickSizesFn) -> long
  MxMDTickSizeTbl *tbl = ptr_(env, obj);
  if (ZuUnlikely(!tbl || !fn)) return 0;
  return tbl->allTickSizes(
      [fn = ZJNI::globalRef(env, fn)](const MxMDTickSize &ts) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allTickSizesFn[0].mid,
	  MxMDTickSizeJNI::ctor(env, ts));
    return 0;
  });
}

jobject MxMDTickSizeTblJNI::ctor(JNIEnv *env, void *ptr)
{
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)(uintptr_t)ptr);
}

int MxMDTickSizeTblJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDTickSizeTblJNI::ctor_ },
    { "dtor_",
      "(J)V",
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

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDTickSizeTbl");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllTickSizesFn",
	allTickSizesFn, 1) < 0) return -1;

  return 0;
}

void MxMDTickSizeTblJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
