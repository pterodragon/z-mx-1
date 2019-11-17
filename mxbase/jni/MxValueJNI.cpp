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

#include <zlib/ZJNI.hpp>

#include <mxbase/MxValueJNI.hpp>

namespace MxValueJNI {
  jclass	class_; // MxValue

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(JJ)V" } };
  ZJNI::JavaField fields[] = {
    { "h", "J" },
    { "l", "J" }
  };
}

MxValue MxValueJNI::j2c(JNIEnv *env, jobject obj, bool dlr)
{
  if (ZuUnlikely(!obj)) return MxEnum();
  uint64_t h = env->GetLongField(obj, fields[0].fid);
  uint64_t l = env->GetLongField(obj, fields[1].fid);
  if (dlr) env->DeleteLocalRef(obj);
  return MxValue{MxValue::Unscaled, (((uint128_t)h)<<64) | l};
}

jobject MxValueJNI::ctor(JNIEnv *env, const MxValue &v)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      (jlong)(int64_t)(v.value>>64), (jlong)(int64_t)v.value);
}

jstring MxValueJNI::toString(JNIEnv *env, jobject obj)
{
  if (!obj) return ZJNI::s2j(env, ZuString());
  return ZJNI::s2j(env, ZuStringN<44>() << j2c(env, obj, true));
}

void MxValueJNI::scan(JNIEnv *env, jobject obj, jstring s_)
{
  ZuStringN<44> s;
  ZJNI::j2s_ZuStringN(s, env, s_, true);
  MxValue v{s};
  env->SetLongField(obj, fields[0].fid, (int64_t)(v.value));
  env->SetLongField(obj, fields[1].fid, (int64_t)(v.value>>64));
}

jboolean MxValueJNI::equals(JNIEnv *env, jobject obj, jobject v_)
{
  return j2c(env, obj, true) == j2c(env, v_, true);
}

jint MxValueJNI::compareTo(JNIEnv *env, jobject obj, jobject v_)
{
  return j2c(env, obj, true).cmp(j2c(env, v_, true));
}

int MxValueJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "toString",
      "()Ljava/lang/String;",
      (void *)&MxValueJNI::toString },
    { "scan",
      "(Ljava/lang/String;)V",
      (void *)&MxValueJNI::scan },
    { "equals",
      "(Lcom/shardmx/mxbase/MxValue;)Z",
      (void *)&MxValueJNI::equals },
    { "compareTo",
      "(Lcom/shardmx/mxbase/MxValue;)I",
      (void *)&MxValueJNI::compareTo }
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxValue");
  if (!class_) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, fields,
	sizeof(fields) / sizeof(fields[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, methods,
	sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  return 0;
}

void MxValueJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
