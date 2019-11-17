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

#include <zlib/ZJNI.hpp>

#include <mxmd/MxMD.hpp>

#include <mxmd/MxMDInstrumentJNI.hpp>

#include <mxmd/MxMDInstrHandleJNI.hpp>

namespace MxMDInstrHandleJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  // invoke fn
  ZJNI::JavaMethod instrumentFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDInstrument;)V" }
  };
}

void MxMDInstrHandleJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  MxMDInstrHandle *ZuMayAlias(ptr) = (MxMDInstrHandle *)&ptr_;
  if (ptr) ptr->~MxMDInstrHandle();
}

void MxMDInstrHandleJNI::invoke(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDInstrumentFn) -> void
  if (!fn) return;
  uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
  MxMDInstrHandle *ZuMayAlias(ptr) = (MxMDInstrHandle *)&ptr_;
  ptr->invoke(
      [fn = ZJNI::globalRef(env, fn)](MxMDShard *, MxMDInstrument *instr) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, instrumentFn[0].mid,
	  MxMDInstrumentJNI::ctor(env, instr));
  });
}

jobject MxMDInstrHandleJNI::ctor(JNIEnv *env, MxMDInstrHandle handle)
{
  uintptr_t ptr_;
  MxMDInstrHandle *ZuMayAlias(ptr) = (MxMDInstrHandle *)&ptr_;
  new (ptr) MxMDInstrHandle(ZuMv(handle));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr_);
}

int MxMDInstrHandleJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "dtor_",
      "(J)V",
      (void *)&MxMDInstrHandleJNI::dtor_ },
    { "invoke",
      "(Lcom/shardmx/mxmd/MxMDInstrumentFn;)V",
      (void *)&MxMDInstrHandleJNI::invoke }
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDInstrHandle");
  if (!class_) return -1;

  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDInstrumentFn",
	instrumentFn, 1) < 0) return -1;

  return 0;
}

void MxMDInstrHandleJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
