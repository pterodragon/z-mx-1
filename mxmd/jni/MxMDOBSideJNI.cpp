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

#include <MxMD.hpp>

#include <MxMDOrderBookJNI.hpp>
#include <MxMDPxLevelJNI.hpp>

#include <MxMDOBSideDataJNI.hpp>

#include <MxMDOBSideJNI.hpp>

namespace MxMDOBSideJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDOBSide *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDOBSide *)(void *)ptr;
  }

  jclass	sideClass;

  // MxSide named constructor
  ZJNI::JavaMethod sideMethod[] = {
    { "value", "(I)Lcom/shardmx/mxbase/MxSide;" }
  };

  // query callbacks
  ZJNI::JavaMethod allPxLevelsFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDPxLevel;)J" }
  };
}

void MxMDOBSideJNI::ctor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOBSide *)(void *)(uintptr_t)ptr)->ref();
}

void MxMDOBSideJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOBSide *)(void *)(uintptr_t)ptr)->deref();
}

jobject MxMDOBSideJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs)) return 0;
  return MxMDOrderBookJNI::ctor(env, obs->orderBook());
}

jobject MxMDOBSideJNI::side(JNIEnv *env, jobject obj)
{
  // () -> MxSide
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs)) return 0;
  return env->CallStaticObjectMethod(sideClass, sideMethod[0].mid,
      (jint)obs->side());
}

jobject MxMDOBSideJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSideData
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs)) return 0;
  return MxMDOBSideDataJNI::ctor(env, obs->data());
}

jlong MxMDOBSideJNI::vwap(JNIEnv *env, jobject obj)
{
  // () -> long
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs)) return ZuCmp<jlong>::null();
  return obs->vwap();
}

jobject MxMDOBSideJNI::mktLevel(JNIEnv *env, jobject obj)
{
  // () -> MxMDPxLevel
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs)) return 0;
  return MxMDPxLevelJNI::ctor(env, obs->mktLevel());
}

jlong MxMDOBSideJNI::allPxLevels(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllPxLevelsFn) -> long
  MxMDOBSide *obs = ptr_(env, obj);
  if (ZuUnlikely(!obs || !fn)) return 0;
  return obs->allPxLevels(
      [fn = ZJNI::globalRef(env, fn)](MxMDPxLevel *pxLevel) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allPxLevelsFn[0].mid,
	  MxMDPxLevelJNI::ctor(env, pxLevel));
    return 0;
  });
}

int MxMDOBSideJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDOBSideJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDOBSideJNI::dtor_ },
    { "orderBook",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDOBSideJNI::orderBook },
    { "side",
      "()Lcom/shardmx/mxbase/MxSide;",
      (void *)&MxMDOBSideJNI::side },
    { "data",
      "()Lcom/shardmx/mxmd/MxMDOBSideData;",
      (void *)&MxMDOBSideJNI::data },
    { "vwap",
      "()J",
      (void *)&MxMDOBSideJNI::vwap },
    { "mktLevel",
      "()Lcom/shardmx/mxmd/MxMDPxLevel;",
      (void *)&MxMDOBSideJNI::mktLevel },
    { "allPxLevels",
      "(Lcom/shardmx/mxmd/MxMDAllPxLevelsFn;)J",
      (void *)&MxMDOBSideJNI::allPxLevels },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOBSide");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  sideClass = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxSide");
  if (!sideClass) return -1;
  if (ZJNI::bindStatic(env, sideClass, sideMethod, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllPxLevelsFn",
	allPxLevelsFn, 1) < 0) return -1;

  return 0;
}

void MxMDOBSideJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
  if (sideClass) env->DeleteGlobalRef(sideClass);
}
