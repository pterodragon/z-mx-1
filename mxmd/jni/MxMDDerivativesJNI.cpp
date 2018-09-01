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

#include <new>

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMD.hpp>

#include <MxFutKeyJNI.hpp>
#include <MxOptKeyJNI.hpp>

#include <MxMDSecurityJNI.hpp>

#include <MxMDDerivativesJNI.hpp>

namespace MxMDDerivativesJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDDerivatives *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDDerivatives> *ZuMayAlias(ptr) = (ZmRef<MxMDDerivatives> *)&ptr_;
    return ptr->ptr();
  }

  // query callbacks
  ZJNI::JavaMethod allSecuritiesFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;)J" }
  };
}

void MxMDDerivativesJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void
}

void MxMDDerivativesJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDDerivatives> *ZuMayAlias(ptr) = (ZmRef<MxMDDerivatives> *)&ptr_;
  ptr->~ZmRef<MxMDDerivatives>();
}

jobject MxMDDerivativesJNI::future(JNIEnv *env, jobject obj, jobject key)
{
  // (MxFutKey) -> MxMDSecurity
  MxMDDerivatives *der = ptr_(env, obj);
  if (ZuUnlikely(!der)) return 0;
  return MxMDSecurityJNI::ctor(env, der->future(MxFutKeyJNI::j2c(env, key)));
}

jlong MxMDDerivativesJNI::allFutures(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllSecuritiesFn) -> long
  MxMDDerivatives *der = ptr_(env, obj);
  if (ZuUnlikely(!der || !fn)) return 0;
  return der->allFutures(
      [fn = ZJNI::globalRef(env, fn)](MxMDSecurity *sec) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allSecuritiesFn[0].mid,
	  MxMDSecurityJNI::ctor(env, sec));
    return 0;
  });
}

jobject MxMDDerivativesJNI::option(JNIEnv *env, jobject obj, jobject key)
{
  // (MxOptKey) -> MxMDSecurity
  MxMDDerivatives *der = ptr_(env, obj);
  if (ZuUnlikely(!der)) return 0;
  return MxMDSecurityJNI::ctor(env, der->option(MxOptKeyJNI::j2c(env, key)));
}

jlong MxMDDerivativesJNI::allOptions(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllSecuritiesFn) -> long
  MxMDDerivatives *der = ptr_(env, obj);
  if (ZuUnlikely(!der || !fn)) return 0;
  return der->allOptions(
      [fn = ZJNI::globalRef(env, fn)](MxMDSecurity *sec) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allSecuritiesFn[0].mid,
	  MxMDSecurityJNI::ctor(env, sec));
    return 0;
  });
}

jobject MxMDDerivativesJNI::ctor(JNIEnv *env, ZmRef<MxMDDerivatives> der)
{
  uintptr_t ptr_;
  ZmRef<MxMDDerivatives> *ZuMayAlias(ptr) = (ZmRef<MxMDDerivatives> *)&ptr_;
  new (ptr) ZmRef<MxMDDerivatives>(ZuMv(der));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr_);
}

int MxMDDerivativesJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "future",
      "(Lcom/shardmx/mxbase/MxFutKey;)Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)&MxMDDerivativesJNI::future },
    { "allFutures",
      "(Lcom/shardmx/mxmd/MxMDAllSecuritiesFn;)J",
      (void *)&MxMDDerivativesJNI::allFutures },
    { "option",
      "(Lcom/shardmx/mxbase/MxOptKey;)Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)&MxMDDerivativesJNI::option },
    { "allOptions",
      "(Lcom/shardmx/mxmd/MxMDAllSecuritiesFn;)J",
      (void *)&MxMDDerivativesJNI::allOptions },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDDerivatives");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllSecuritiesFn",
	allSecuritiesFn, 1) < 0) return -1;

  return 0;
}

void MxMDDerivativesJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
