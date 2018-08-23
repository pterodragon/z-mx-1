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

#include <MxMDDerivativesJNI.hpp>

void MxMDDerivativesJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDDerivativesJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDDerivativesJNI::future(JNIEnv *env, jobject obj, jobject)
{
  // (MxFutKey) -> MxMDSecurity

  return 0;
}

jlong MxMDDerivativesJNI::allFutures(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllSecuritiesFn) -> long

  return 0;
}

jobject MxMDDerivativesJNI::option(JNIEnv *env, jobject obj, jobject)
{
  // (MxOptKey) -> MxMDSecurity

  return 0;
}

jlong MxMDDerivativesJNI::allOptions(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllSecuritiesFn) -> long

  return 0;
}

int MxMDDerivativesJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDDerivativesJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDDerivativesJNI::dtor_ },
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

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDDerivatives",
        methods, sizeof(methods) / sizeof(methods[0]));
}
