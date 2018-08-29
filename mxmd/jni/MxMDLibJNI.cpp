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
#include <MxMDCore.hpp>

#include <MxSecKeyJNI.hpp>

#include <MxMDJNI.hpp>

#include <MxMDFeedJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDSecurityJNI.hpp>
#include <MxMDOrderBookJNI.hpp>

#include <MxMDTickSizeJNI.hpp>
#include <MxMDSegmentJNI.hpp>

#include <MxMDLibHandlerJNI.hpp>

#include <MxMDLibJNI.hpp>

namespace MxMDLibJNI {
  ZmLock	lock; // only used to serialize initialization
    jobject	  obj_;
    bool	  running = false;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDLib *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) {
      ZJNI::throwNPE(env, "MxMDLib.init() not called");
      return nullptr;
    }
    return (MxMDLib *)(void *)ptr;
  }

  // query callbacks
  ZJNI::JavaMethod securityFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;)V" }
  };
  ZJNI::JavaMethod allSecuritiesFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;)J" }
  };
  ZJNI::JavaMethod orderBookFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;)V" }
  };
  ZJNI::JavaMethod allOrderBooksFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;)J" }
  };
  ZJNI::JavaMethod allFeedsFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDFeed;)J" }
  };
  ZJNI::JavaMethod allVenuesFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDVenue;)J" }
  };
}

void MxMDLibJNI::ctor_(JNIEnv *env, jobject, jlong) { }

void MxMDLibJNI::dtor_(JNIEnv *env, jobject, jlong)
{
  // called from Java close()
  MxMDJNI::final(env);
}

jobject MxMDLibJNI::instance(JNIEnv *env, jclass c)
{
  // () -> MxMDLib
  // ZmReadGuard<ZmLock> guard(lock);

  return obj_;
}

class MxMDLib_JNI {
public:
  ZuInline static MxMDLib *init(ZuString cf, ZmFn<ZmScheduler *> schedInitFn) {
    return MxMDLib::init(cf, ZuMv(schedInitFn));
  }
};

jobject MxMDLibJNI::init(JNIEnv *env, jclass c, jstring cf)
{
  // (String) -> MxMDLib
  ZmGuard<ZmLock> guard(lock);

  if (ZuUnlikely(obj_)) return obj_;

  ZJNI::env(env);

  MxMDLib *md = MxMDLib_JNI::init(ZJNI::j2s_ZtString(env, cf),
      [](ZmScheduler *mx) {
    mx->threadInit([]() {
      ZmThreadName name;
      ZmThreadContext::self()->name(name);
      ZJNI::attach(name.data());
    });
    mx->threadFinal([]() { ZJNI::detach(); });
  });
  if (ZuUnlikely(!md)) {
    ZJNI::throwNPE(env, "MxMDLib.init() failed");
    return 0;
  }

  {
    jobject obj = env->NewObject(c, ctorMethod[0].mid,
	(jlong)(uintptr_t)(void *)md);
    if (!obj) return 0;
    obj_ = env->NewGlobalRef(obj);
    env->DeleteLocalRef(obj);
  }

  return obj_;
}

void MxMDLibJNI::start(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  {
    ZmGuard<ZmLock> guard(lock);
    if (running) return;
    running = true;
    md->start();
  }
}

void MxMDLibJNI::stop(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  {
    ZmGuard<ZmLock> guard(lock);
    if (!running) return;
    running = false;
    md->allSecurities([](MxMDSecurity *sec) { sec->unsubscribe(); });
    md->stop();
    md->unsubscribe();
  }
}

void MxMDLibJNI::record(JNIEnv *env, jobject obj, jstring path)
{
  // (String) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->record(ZJNI::j2s_ZtString(env, path));
}

void MxMDLibJNI::stopRecording(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->stopRecording();
}

void MxMDLibJNI::stopStreaming(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->stopStreaming();
}

void MxMDLibJNI::replay(JNIEnv *env, jobject obj,
    jstring path, jobject begin, jboolean filter)
{
  // (String, Instant, boolean) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->replay(ZJNI::j2s_ZtString(env, path), ZJNI::j2t(env, begin), filter);
}

void MxMDLibJNI::stopReplaying(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->stopReplaying();
}

void MxMDLibJNI::startTimer(JNIEnv *env, jobject obj, jobject begin)
{
  // (Instant) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->startTimer(ZJNI::j2t(env, begin));
}

void MxMDLibJNI::stopTimer(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->stopTimer();
}

void MxMDLibJNI::subscribe(JNIEnv *env, jobject obj, jobject handler_)
{
  // (MxMDLibHandler) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->subscribe(MxMDLibHandlerJNI::j2c(env, handler_));
  md->appData((uintptr_t)(void *)(env->NewGlobalRef(handler_)));
}

void MxMDLibJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return;
  md->unsubscribe();
  env->DeleteGlobalRef((jobject)(void *)(md->appData()));
  md->appData(0);
}

jobject MxMDLibJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDLibHandler
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md)) return 0;
  return (jobject)(void *)(md->appData());
}

void MxMDLibJNI::security(JNIEnv *env, jobject obj, jobject key_, jobject fn)
{
  // (MxSecKey, MxMDSecurityFn) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return;
  md->secInvoke(MxSecKeyJNI::j2c(env, key_),
      [fn = ZJNI::globalRef(env, fn)](MxMDSecurity *sec) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, securityFn[0].mid,
	  MxMDSecurityJNI::ctor(env, sec));
  });
}

jlong MxMDLibJNI::allSecurities(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllSecuritiesFn) -> long
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return 0;
  return md->allSecurities(
      [fn = ZJNI::globalRef(env, fn)](MxMDSecurity *sec) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allSecuritiesFn[0].mid,
	  MxMDSecurityJNI::ctor(env, sec));
    return 0;
  });
}

void MxMDLibJNI::orderBook(JNIEnv *env, jobject obj, jobject key_, jobject fn)
{
  // (MxSecKey, MxMDOrderBookFn) -> void
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return;
  md->obInvoke(MxSecKeyJNI::j2c(env, key_),
      [fn = ZJNI::globalRef(env, fn)](MxMDOrderBook *ob) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, orderBookFn[0].mid,
	  MxMDOrderBookJNI::ctor(env, ob));
  });
}

jlong MxMDLibJNI::allOrderBooks(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllOrderBooksFn) -> long
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return 0;
  return md->allOrderBooks(
      [fn = ZJNI::globalRef(env, fn)](MxMDOrderBook *ob) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allOrderBooksFn[0].mid,
	  MxMDOrderBookJNI::ctor(env, ob));
    return 0;
  });
}

jobject MxMDLibJNI::feed(JNIEnv *env, jobject obj, jstring id)
{
  // (String) -> MxMDFeed
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !id)) return 0;
  if (ZmRef<MxMDFeed> feed = md->feed(ZJNI::j2s_ZuStringN<8>(env, id)))
    return MxMDFeedJNI::ctor(env, feed);
  return 0;
}

jlong MxMDLibJNI::allFeeds(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllFeedsFn) -> long
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return 0;
  return md->allFeeds(
      [fn = ZJNI::globalRef(env, fn)](MxMDFeed *feed) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allFeedsFn[0].mid,
	  MxMDFeedJNI::ctor(env, feed));
    return 0;
  });
}

jobject MxMDLibJNI::venue(JNIEnv *env, jobject obj, jstring id)
{
  // (String) -> MxMDVenue
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !id)) return 0;
  if (ZmRef<MxMDVenue> venue = md->venue(ZJNI::j2s_ZuStringN<8>(env, id)))
    return MxMDVenueJNI::ctor(env, venue);
  return 0;
}

jlong MxMDLibJNI::allVenues(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllVenuesFn) -> long
  MxMDLib *md = ptr_(env, obj);
  if (ZuUnlikely(!md || !fn)) return 0;
  return md->allVenues(
      [fn = ZJNI::globalRef(env, fn)](MxMDVenue *venue) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allVenuesFn[0].mid,
	  MxMDVenueJNI::ctor(env, venue));
    return 0;
  });
}

jobject MxMDLibJNI::instance_() { return obj_; }

int MxMDLibJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDLibJNI::ctor_ },
    { "dtor_",
      "(J)V",
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

  jclass c = env->FindClass("com/shardmx/mxmd/MxMDLib");

  if (ZJNI::bind(env, c,
	methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, c, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, c, ptrField, 1) < 0) return -1;

  env->DeleteLocalRef((jobject)c);

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDSecurityFn",
	securityFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllSecuritiesFn",
	allSecuritiesFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDOrderBookFn",
	orderBookFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllOrderBooksFn",
	allOrderBooksFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllFeedsFn",
	allFeedsFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllVenuesFn",
	allVenuesFn, 1) < 0) return -1;

  return 0;
}

void MxMDLibJNI::final(JNIEnv *env)
{
  ZmGuard<ZmLock> guard(lock);

  env->DeleteGlobalRef(obj_); obj_ = 0;

  if (MxMDLib *md = MxMDLib::instance()) {
    if (running) {
      running = false;
      md->allSecurities([](MxMDSecurity *sec) { sec->unsubscribe(); });
      md->stop();
      md->unsubscribe();
    }
    md->final();
  }
}
