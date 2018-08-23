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

#include <MxMDLibJNI.hpp>

#include <MxMD.hpp>

namespace MxMDLibJNI {
  ZmLock	lock;
    jfieldID	  ptr_fid = 0;
    jmethodID	  ctor_mid = 0;
    jobject	  global_obj = 0;

  void init_(JNIEnv *env, jclass c) {
    if (!ptr_fid) {
      ZJNI::env(env);
      ptr_fid = env->GetFieldID(c, "ptr", "J");
      ctor_mid = env->GetMethodID(c, "<init>", "(J)V");
    }
  }

  MxMDLib *ptr_(JNIEnv *env, jobject obj) {
    return (MxMDLib *)(void *)(uintptr_t)env->GetLongField(obj, ptr_fid);
  }
}

void MxMDLibJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDLibJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDLibJNI::instance(JNIEnv *env, jclass c)
{
  // () -> MxMDLib
  using namespace MxMDLibJNI;

  ZmGuard<ZmLock> guard(lock);

  if (ZuLikely(global_obj)) return global_obj;

  init_(env, c);

  if (MxMDLib *md = MxMDLib::instance()) {
    jobject obj = env->NewObject(c, ctor_mid, (jlong)(uintptr_t)(void *)md);
    global_obj = env->NewGlobalRef(obj);
    env->DeleteLocalRef(obj);
    return global_obj;
  }

  return 0;
}

jobject MxMDLibJNI::init(JNIEnv *env, jclass c, jstring cf)
{
  // (String) -> MxMDLib
  using namespace MxMDLibJNI;

  ZmGuard<ZmLock> guard(lock);

  init_(env, c);

  MxMDLib *md = MxMDLib::init(ZJNI::j2t(env, cf));
  if (ZuUnlikely(!md)) return 0; // FIXME

  jobject obj = env->NewObject(c, ctor_mid, (jlong)(uintptr_t)(void *)md);
  global_obj = env->NewGlobalRef(obj);
  env->DeleteLocalRef(obj);
  return global_obj;
}

void MxMDLibJNI::start(JNIEnv *env, jobject obj)
{
  // () -> void
  using namespace MxMDLibJNI;
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return; // FIXME
  md->start();
}

void MxMDLibJNI::stop(JNIEnv *env, jobject obj)
{
  // () -> void
  using namespace MxMDLibJNI;
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return; // FIXME
  md->stop();
}

void MxMDLibJNI::record(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> void

}

void MxMDLibJNI::stopRecording(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDLibJNI::stopStreaming(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDLibJNI::replay(JNIEnv *env, jobject obj, jstring, jobject, jboolean)
{
  // (String, Instant, boolean) -> void

}

void MxMDLibJNI::stopReplaying(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDLibJNI::startTimer(JNIEnv *env, jobject obj, jobject)
{
  // (Instant) -> void

}

void MxMDLibJNI::stopTimer(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDLibJNI::subscribe(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDLibHandler) -> void

}

void MxMDLibJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDLibJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDLibHandler

  return 0;
}

void MxMDLibJNI::security(JNIEnv *env, jobject obj, jobject, jobject)
{
  // (MxSecKey, MxMDSecurityFn) -> void

}

jlong MxMDLibJNI::allSecurities(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllSecuritiesFn) -> long

  return 0;
}

void MxMDLibJNI::orderBook(JNIEnv *env, jobject obj, jobject, jobject)
{
  // (MxSecKey, MxMDOrderBookFn) -> void

}

jlong MxMDLibJNI::allOrderBooks(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllOrderBooksFn) -> long

  return 0;
}

jobject MxMDLibJNI::feed(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> MxMDFeed

  return 0;
}

jlong MxMDLibJNI::allFeeds(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllFeedsFn) -> long

  return 0;
}

jobject MxMDLibJNI::venue(JNIEnv *env, jobject obj, jstring)
{
  // (String) -> MxMDVenue

  return 0;
}

jlong MxMDLibJNI::allVenues(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDAllVenuesFn) -> long

  return 0;
}

int MxMDLibJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDLibJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDLibJNI::dtor_ },
    { "instance",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDLibJNI::instance },
    { "init",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDLibJNI::init },
    { "start",
      "()V",
      (void *)&MxMDLibJNI::start },
    { "stop",
      "()V",
      (void *)&MxMDLibJNI::stop },
    { "record",
      "(Ljava/lang/String;)V",
      (void *)&MxMDLibJNI::record },
    { "stopRecording",
      "()V",
      (void *)&MxMDLibJNI::stopRecording },
    { "stopStreaming",
      "()V",
      (void *)&MxMDLibJNI::stopStreaming },
    { "replay",
      "(Ljava/lang/String;Ljava/time/Instant;Z)V",
      (void *)&MxMDLibJNI::replay },
    { "stopReplaying",
      "()V",
      (void *)&MxMDLibJNI::stopReplaying },
    { "startTimer",
      "(Ljava/time/Instant;)V",
      (void *)&MxMDLibJNI::startTimer },
    { "stopTimer",
      "()V",
      (void *)&MxMDLibJNI::stopTimer },
    { "subscribe",
      "(Lcom/shardmx/mxmd/MxMDLibHandler;)V",
      (void *)&MxMDLibJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDLibJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDLibHandler;",
      (void *)&MxMDLibJNI::handler },
    { "security",
      "(Lcom/shardmx/mxbase/MxSecKey;Lcom/shardmx/mxmd/MxMDSecurityFn;)V",
      (void *)&MxMDLibJNI::security },
    { "allSecurities",
      "(Lcom/shardmx/mxmd/MxMDAllSecuritiesFn;)J",
      (void *)&MxMDLibJNI::allSecurities },
    { "orderBook",
      "(Lcom/shardmx/mxbase/MxSecKey;Lcom/shardmx/mxmd/MxMDOrderBookFn;)V",
      (void *)&MxMDLibJNI::orderBook },
    { "allOrderBooks",
      "(Lcom/shardmx/mxmd/MxMDAllOrderBooksFn;)J",
      (void *)&MxMDLibJNI::allOrderBooks },
    { "feed",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDFeed;",
      (void *)&MxMDLibJNI::feed },
    { "allFeeds",
      "(Lcom/shardmx/mxmd/MxMDAllFeedsFn;)J",
      (void *)&MxMDLibJNI::allFeeds },
    { "venue",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDVenue;",
      (void *)&MxMDLibJNI::venue },
    { "allVenues",
      "(Lcom/shardmx/mxmd/MxMDAllVenuesFn;)J",
      (void *)&MxMDLibJNI::allVenues },
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDLib",
        methods, sizeof(methods) / sizeof(methods[0]));
}
