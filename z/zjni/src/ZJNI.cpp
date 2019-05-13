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

#include <ZmSpecific.hpp>
#include <ZmThread.hpp>

#include <ZJNI.hpp>

#include <jni.h>

static JavaVM *jvm = 0;

class TLS : public ZmObject {
public:
  inline TLS() {
    ZmThreadName name = ZmThreadContext::self()->name();
    JavaVMAttachArgs args{
      JNI_VERSION_1_4, const_cast<char *>(name.data()), 0 };
    jvm->AttachCurrentThread((void **)&m_env, &args);
  }
  inline TLS(JNIEnv *env) : m_env(env) { }

  inline static void attach() {
    ZmSpecific<TLS>::instance();
  }
  inline static void detach() {
    ZmSpecific<TLS>::instance()->m_env = nullptr;
    jvm->DetachCurrentThread();
  }

  inline static void env(JNIEnv *env) {
    ZmSpecific<TLS>::instance(new TLS(env));
  }
  inline static JNIEnv *env() {
    return ZmSpecific<TLS>::instance()->m_env;
  }

private:
  JNIEnv	*m_env = nullptr;
};

static jclass nullptrex_class = 0;
static jmethodID nullptrex_ctor = 0;

static jclass instant_class = 0;
static jmethodID instant_ofEpochSecond = 0;
static jmethodID instant_getEpochSecond = 0;
static jmethodID instant_getNano = 0;

JavaVM *ZJNI::vm() { return jvm; }
JNIEnv *ZJNI::env() { return TLS::env(); }
void ZJNI::env(JNIEnv *ptr) { TLS::env(ptr); }

void ZJNI::attach()
{
  TLS::attach();
}

void ZJNI::detach()
{
  TLS::detach();
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

  TLS::env(env);

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

ZtDate ZJNI::j2t(JNIEnv *env, jobject obj, bool dlr)
{
  if (ZuUnlikely(!obj)) return ZtDate();
  ZtDate d{
      (time_t)env->CallLongMethod(obj, instant_getEpochSecond),
      (int)env->CallIntMethod(obj, instant_getNano)};
  if (dlr) env->DeleteLocalRef(obj);
  return d;
}

// ZtDate -> Java Instant
inline jobject ZJNI::t2j(JNIEnv *env, const ZtDate &t) {
  return env->CallStaticObjectMethod(instant_class,
      instant_ofEpochSecond, (jlong)t.time(), (jint)t.nsec());
}
