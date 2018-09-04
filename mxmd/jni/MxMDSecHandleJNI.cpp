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

#include <MxMDSecurityJNI.hpp>

#include <MxMDSecHandleJNI.hpp>

namespace MxMDSecHandleJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  // invoke fn
  ZJNI::JavaMethod securityFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;)V" }
  };
}

void MxMDSecHandleJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void
}

void MxMDSecHandleJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  MxMDSecHandle *ZuMayAlias(ptr) = (MxMDSecHandle *)&ptr_;
  ptr->~MxMDSecHandle();
}

void MxMDSecHandleJNI::invoke(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDSecurityFn) -> void
  if (!fn) return;
  uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
  MxMDSecHandle *ZuMayAlias(ptr) = (MxMDSecHandle *)&ptr_;
  ptr->invoke(
      [fn = ZJNI::globalRef(env, fn)](MxMDShard *, MxMDSecurity *sec) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, securityFn[0].mid,
	  MxMDSecurityJNI::ctor(env, sec));
  });
}

jobject MxMDSecHandleJNI::ctor(JNIEnv *env, MxMDSecHandle handle)
{
  uintptr_t ptr_;
  MxMDSecHandle *ZuMayAlias(ptr) = (MxMDSecHandle *)&ptr_;
  new (ptr) MxMDSecHandle(ZuMv(handle));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr_);
}

int MxMDSecHandleJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDSecHandleJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDSecHandleJNI::dtor_ },
    { "invoke",
      "(Lcom/shardmx/mxmd/MxMDSecurityFn;)V",
      (void *)&MxMDSecHandleJNI::invoke }
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDSecHandle");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDSecurityFn",
	securityFn, 1) < 0) return -1;

  return 0;
}

void MxMDSecHandleJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}