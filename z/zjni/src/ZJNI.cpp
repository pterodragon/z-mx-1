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

// Z JNI library

#include <ZJNI.hpp>

#include <jni.h>

static JavaVM *jvm = 0;
static thread_local JNIEnv *jenv = 0;

static jclass nullptrex_class = 0;
static jmethodID nullptrex_ctor = 0;

static jclass instant_class = 0;
static jmethodID instant_ofEpochSecond = 0;
static jmethodID instant_getEpochSecond = 0;
static jmethodID instant_getNano = 0;

JavaVM *ZJNI::vm() { return jvm; }
JNIEnv *ZJNI::env() { return jenv; }
void ZJNI::env(JNIEnv *env) { jenv = env; }

int ZJNI::attach(const char *name)
{
  JavaVMAttachArgs args{ JNI_VERSION_1_4, const_cast<char *>(name), 0 };
  JNIEnv *env = 0;
  if (jvm->AttachCurrentThread((void **)&env, &args) < 0 || !env) return -1;
  jenv = env;
  return 0;
}

void ZJNI::detach()
{
  jvm->DetachCurrentThread();
}

void ZJNI::throwNPE(JNIEnv *env, ZuString s)
{
  env->Throw((jthrowable)env->NewObject(
	nullptrex_class, nullptrex_ctor, ZJNI::s2j(env, s)));
}

int ZJNI::bind(JNIEnv *env, const char *cname,
    const JNINativeMethod *methods, unsigned n)
{
  jclass c = env->FindClass(cname);
  if (!c) return -1;
  return bind(env, c, methods, n);
}

int ZJNI::bind(JNIEnv *env, jclass c,
    const JNINativeMethod *methods, unsigned n)
{
  return env->RegisterNatives(c, methods, n);
}

int ZJNI::bind(JNIEnv *env, const char *cname, JavaMethod *methods, unsigned n)
{
  jclass c = env->FindClass(cname);
  if (!c) return -1;
  return bind(env, c, methods, n);
}

int ZJNI::bind(JNIEnv *env, jclass c, JavaMethod *methods, unsigned n)
{
  for (unsigned i = 0; i < n; i++)
    if (!(methods[i].mid = env->GetMethodID(
	    c, methods[i].name, methods[i].signature))) return -1;
  return 0;
}

int ZJNI::bindStatic(
    JNIEnv *env, const char *cname, JavaMethod *methods, unsigned n)
{
  jclass c = env->FindClass(cname);
  if (!c) return -1;
  return bindStatic(env, c, methods, n);
}

int ZJNI::bindStatic(JNIEnv *env, jclass c, JavaMethod *methods, unsigned n)
{
  for (unsigned i = 0; i < n; i++)
    if (!(methods[i].mid = env->GetStaticMethodID(
	    c, methods[i].name, methods[i].signature))) return -1;
  return 0;
}

int ZJNI::bind(JNIEnv *env, const char *cname, JavaField *fields, unsigned n)
{
  jclass c = env->FindClass(cname);
  if (!c) return -1;
  return bind(env, c, fields, n);
}

int ZJNI::bind(JNIEnv *env, jclass c, JavaField *fields, unsigned n)
{
  for (unsigned i = 0; i < n; i++)
    if (!(fields[i].fid = env->GetFieldID(
	    c, fields[i].name, fields[i].type))) return -1;
  return 0;
}

int ZJNI::bindStatic(
    JNIEnv *env, const char *cname, JavaField *fields, unsigned n)
{
  jclass c = env->FindClass(cname);
  if (!c) return -1;
  return bindStatic(env, c, fields, n);
}

int ZJNI::bindStatic(JNIEnv *env, jclass c, JavaField *fields, unsigned n)
{
  for (unsigned i = 0; i < n; i++)
    if (!(fields[i].fid = env->GetStaticFieldID(
	    c, fields[i].name, fields[i].type))) return -1;
  return 0;
}

jint ZJNI::load(JavaVM *jvm_)
{
  jvm = jvm_;

  JNIEnv *env = 0;

  if (jvm_->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK || !env)
    return -1;

  jenv = env;

  {
    jclass c = env->FindClass("java/lang/NullPointerException");
    nullptrex_class = (jclass)env->NewGlobalRef(c);
    env->DeleteLocalRef(c);
    nullptrex_ctor = env->GetMethodID(nullptrex_class,
	"<init>", "(Ljava/lang/String;)V");
  }

  {
    jclass c = env->FindClass("java/time/Instant");
    instant_class = (jclass)env->NewGlobalRef(c);
    env->DeleteLocalRef(c);
    instant_ofEpochSecond =
      env->GetStaticMethodID(instant_class,
	  "ofEpochSecond", "(JJ)Ljava/time/Instant;");
    instant_getEpochSecond =
      env->GetMethodID(instant_class, "getEpochSecond", "()J");
    instant_getNano =
      env->GetMethodID(instant_class, "getNano", "()I");
  }

  return JNI_VERSION_1_4;
}

void ZJNI::final(JNIEnv *env)
{
  if (nullptrex_class) env->DeleteGlobalRef(nullptrex_class);
  if (instant_class) env->DeleteGlobalRef(instant_class);
}

// Java Instant -> ZtDate
ZJNIExtern ZtDate ZJNI::j2t(JNIEnv *env, jobject obj)
{
  return ZtDate(
      (time_t)env->CallLongMethod(obj, instant_getEpochSecond),
      (int)env->CallIntMethod(obj, instant_getNano));
}

// ZtDate -> Java Instant
ZJNIExtern jobject ZJNI::t2j(JNIEnv *env, const ZtDate &t)
{
  return env->CallStaticObjectMethod(instant_class,
      instant_ofEpochSecond, (jlong)t.time(), (jint)t.nsec());
}
