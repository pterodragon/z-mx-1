//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

#include <mxmd/MxMD.hpp>

#include <mxmd/MxMDLibJNI.hpp>

#include <mxmd/MxMDFeedJNI.hpp>

namespace MxMDFeedJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  ZuInline MxMDFeed *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDFeed> *ZuMayAlias(ptr) = (ZmRef<MxMDFeed> *)&ptr_;
    return ptr->ptr();
  }
}

void MxMDFeedJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDFeed> *ZuMayAlias(ptr) = (ZmRef<MxMDFeed> *)&ptr_;
  if (ptr) ptr->~ZmRef<MxMDFeed>();
}

jobject MxMDFeedJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib
  return MxMDLibJNI::instance_();
}

jstring MxMDFeedJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDFeed *feed = ptr_(env, obj);
  if (ZuUnlikely(!feed)) return 0;
  return ZJNI::s2j(env, feed->id());
}

jint MxMDFeedJNI::level(JNIEnv *env, jobject obj)
{
  // () -> int
  MxMDFeed *feed = ptr_(env, obj);
  if (ZuUnlikely(!feed)) return 0;
  return feed->level();
}

jobject MxMDFeedJNI::ctor(JNIEnv *env, ZmRef<MxMDFeed> feed)
{
  uintptr_t ptr;
  new (&ptr) ZmRef<MxMDFeed>(ZuMv(feed));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr);
}

int MxMDFeedJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "dtor_",
      "(J)V",
      (void *)&MxMDFeedJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDFeedJNI::md },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDFeedJNI::id },
    { "level",
      "()I",
      (void *)&MxMDFeedJNI::level },
  };
#pragma GCC diagnostic pop

  {
    jclass c = env->FindClass("com/shardmx/mxmd/MxMDFeed");
    if (!c) return -1;
    class_ = (jclass)env->NewGlobalRef((jobject)c);
    env->DeleteLocalRef((jobject)c);
    if (!class_) return -1;
  }

  if (ZJNI::bind(env, class_,
	methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDFeedJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
