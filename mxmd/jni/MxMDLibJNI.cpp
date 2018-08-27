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

#include <MxMDJNI.hpp>

#include <MxMDFeedJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDTickSizeJNI.hpp>

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

  // MxMDLibHandler bindings
  ZJNI::JavaMethod exceptionFn[] = {
    { "exception", "()Lcom/shardmx/mxmd/MxMDExceptionFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDLib;Ljava/lang/String;)V" }
  };
  ZJNI::JavaMethod connectedFn[] = {
    { "connected", "()Lcom/shardmx/mxmd/MxMDFeedFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDFeed;)V" }
  };
  ZJNI::JavaMethod disconnectedFn[] = {
    { "disconnected", "()Lcom/shardmx/mxmd/MxMDFeedFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDFeed;)V" }
  };
  ZJNI::JavaMethod eofFn[] = {
    { "eof", "()Lcom/shardmx/mxmd/MxMDLibFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDLib;)V" }
  };
  ZJNI::JavaMethod refDataLoadedFn[] = {
    { "refDataLoaded", "()Lcom/shardmx/mxmd/MxMDVenueFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDVenue;)V" }
  };
  ZJNI::JavaMethod addTickSizeTblFn[] = {
    { "addTickSizeTbl", "()Lcom/shardmx/mxmd/MxMDTickSizeTblFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;)V" }
  };
  ZJNI::JavaMethod resetTickSizeTblFn[] = {
    { "resetTickSizeTbl", "()Lcom/shardmx/mxmd/MxMDTickSizeTblFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;)V" }
  };
  ZJNI::JavaMethod addTickSizeFn[] = {
    { "addTickSize", "()Lcom/shardmx/mxmd/MxMDTickSizeFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;"
	"Lcom/shardmx/mxmd/MxMDTickSize;)V" }
  };
  ZJNI::JavaMethod addSecurityFn[] = {
    { "addSecurity", "()Lcom/shardmx/mxmd/MxMDSecEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedSecurityFn[] = {
    { "updatedSecurity", "()Lcom/shardmx/mxmd/MxMDSecEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDSecurity;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod addOrderBookFn[] = {
    { "addOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod updatedOrderBookFn[] = {
    { "updatedOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod deletedOrderBookFn[] = {
    { "deletedOrderBook", "()Lcom/shardmx/mxmd/MxMDOBEventFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;Ljava/time/Instant;)V" }
  };
  ZJNI::JavaMethod tradingSessionFn[] = {
    { "tradingSession", "()Lcom/shardmx/mxmd/MxMDTradingSessionFn;" },
    { "fn", "(Lcom/shardmx/mxmd/MxMDVenue;Lcom/shardmx/mxmd/MxMDSegment;)V" }
  };
  ZJNI::JavaMethod timerFn[] = {
    { "timer", "()Lcom/shardmx/mxmd/MxMDTimerFn;" },
    { "fn", "(Ljava/time/Instant;)Ljava/time/Instant;" }
  };
  int bindHandlerFn(JNIEnv *env, jclass c, ZJNI::JavaMethod *fn) {
    if (ZJNI::bind(env, c, fn, 1) < 0) return -1;
    ZuStringN<80> fn_c =
      ZuString(&fn[0].signature[3], strlen(fn[0].signature) - 4);
    return ZJNI::bind(env, fn_c, &fn[1], 1);
  }
  int bindHandler(JNIEnv *env) {
    jclass c;
    if (!(c = env->FindClass("com/shardmx/mxmd/MxMDLibHandler"))) return -1;
    if (bindHandlerFn(env, c, exceptionFn) < 0) return -1;
    if (bindHandlerFn(env, c, connectedFn) < 0) return -1;
    if (bindHandlerFn(env, c, disconnectedFn) < 0) return -1;
    if (bindHandlerFn(env, c, eofFn) < 0) return -1;
    if (bindHandlerFn(env, c, refDataLoadedFn) < 0) return -1;
    if (bindHandlerFn(env, c, addTickSizeTblFn) < 0) return -1;
    if (bindHandlerFn(env, c, resetTickSizeTblFn) < 0) return -1;
    if (bindHandlerFn(env, c, addTickSizeFn) < 0) return -1;
    if (bindHandlerFn(env, c, addSecurityFn) < 0) return -1;
    if (bindHandlerFn(env, c, updatedSecurityFn) < 0) return -1;
    if (bindHandlerFn(env, c, addOrderBookFn) < 0) return -1;
    if (bindHandlerFn(env, c, updatedOrderBookFn) < 0) return -1;
    if (bindHandlerFn(env, c, deletedOrderBookFn) < 0) return -1;
    if (bindHandlerFn(env, c, tradingSessionFn) < 0) return -1;
    if (bindHandlerFn(env, c, timerFn) < 0) return -1;
    env->DeleteLocalRef((jobject)c);
    return 0;
  }
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
  ZmReadGuard<ZmLock> guard(lock);

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

  ZmRef<MxMDLibHandler> handler = new MxMDLibHandler();

#ifdef handle1
#undef handle1
#endif
#ifdef handle2
#undef handle2
#endif
#define handle1(method, arg1, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(handler_, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg1) { \
      if (JNIEnv *env = ZJNI::env()) \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); })
#define handle2(method, arg1, arg2, ...) \
  if (auto fn = ZJNI::localRef( \
	env, env->CallObjectMethod(handler_, method ## Fn[0].mid))) \
    handler->method ## Fn( \
	[fn = ZJNI::globalRef(env, fn)](arg1, arg2) { \
      if (JNIEnv *env = ZJNI::env()) \
	env->CallVoidMethod(fn, method ## Fn[1].mid, __VA_ARGS__); })

  // FIXME - change ctor args to be MD-native C++ types, i.e. migrate
  // casting/decomposition into ctor() functions and out of here -
  // ctor() will only ever be called to convert from MD to JNI anyway

  handle2(exception,
      MxMDLib *md, ZmRef<ZeEvent> e,
      obj_, ZJNI::s2j(env, ZuStringN<512>() << e->message()));
  handle1(connected,
      MxMDFeed *feed,
      MxMDFeedJNI::ctor(env, (uintptr_t)(void *)feed));
  handle1(disconnected,
      MxMDFeed *feed,
      MxMDFeedJNI::ctor(env, (uintptr_t)(void *)feed));
  handle1(eof,
      MxMDLib *md,
      obj_);
  handle1(refDataLoaded,
      MxMDVenue *venue,
      MxMDVenueJNI::ctor(env, (uintptr_t)(void *)venue));
  handle1(addTickSizeTbl,
      MxMDTickSizeTbl *tbl,
      MxMDTickSizeTblJNI::ctor(env, (uintptr_t)(void *)tbl));
  handle1(resetTickSizeTbl,
      MxMDTickSizeTbl *tbl,
      MxMDTickSizeTblJNI::ctor(env, (uintptr_t)(void *)tbl));
  handle2(addTickSize,
      MxMDTickSizeTbl *tbl, const MxMDTickSize &ts,
      MxMDTickSizeTblJNI::ctor(env, (uintptr_t)(void *)tbl),
      MxMDTickSizeJNI::ctor(env, ts.minPrice(), ts.maxPrice(), ts.tickSize()));
  handle1(addSecurity,
      MxMDSecurity *sec,
      MxMDSecurityJNI::ctor(env, (uintptr_t)(void *)sec));
  handle1(updatedSecurity,
      MxMDSecurity *sec,
      MxMDSecurityJNI::ctor(env, (uintptr_t)(void *)sec));
  handle1(addOrderBook,
      MxMDOrderBook *ob,
      MxMDOrderBookJNI::ctor(env, (uintptr_t)(void *)ob));
  handle1(updatedOrderBook,
      MxMDOrderBook *ob,
      MxMDOrderBookJNI::ctor(env, (uintptr_t)(void *)ob));
  handle1(deletedOrderBook,
      MxMDOrderBook *ob,
      MxMDOrderBookJNI::ctor(env, (uintptr_t)(void *)ob));
  handle2(tradingSession,
      MxMDVenue *venue, MxMDSegment segment,
      MxMDVenueJNI::ctor(env, (uintptr_t)(void *)venue),
      MxMDSegmentJNI::ctor(env, segment.id, segment.session, segment.stamp));

#undef handle1
#undef handle2

  if (auto fn = ZJNI::localRef(
	env, env->CallObjectMethod(handler_, timerFn[0].mid)))
    handler->timerFn(
	[fn = ZJNI::globalRef(env, fn)](MxDateTime stamp, MxDateTime &next_) {
      if (JNIEnv *env = ZJNI::env()) {
	jobject next = env->CallObjectMethod(
	    fn, timerFn[1].mid, ZJNI::t2j(env, stamp));
	if (!next)
	  next_ = MxDateTime();
	else
	  next_ = ZJNI::j2t(next);
      });

#if 0
  if (jobject fn = env->CallObjectMethod(handler_, exceptionFn[0].mid))
    handler->exceptionFn(
	[fn = ZJNI::globalRef(env, fn)](MxMDLib *md, ZmRef<ZeEvent> e) {
      if (JNIEnv *env = ZJNI::env())
	env->CallVoidMethod(fn, exceptionFn[1].mid, obj_,
	    ZJNI::s2j(env, ZuStringN<512>() << e->message())); });
#endif

  md->subscribe(handler);
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

  return bindHandler(env);
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
