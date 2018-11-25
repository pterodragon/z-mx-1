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
#include <MxMDCore.hpp>

#include <MxInstrKeyJNI.hpp>

#include <MxMDJNI.hpp>

#include <MxMDFeedJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDInstrumentJNI.hpp>
#include <MxMDOrderBookJNI.hpp>

#include <MxMDLibHandlerJNI.hpp>

#include <MxMDInstrHandleJNI.hpp>
#include <MxMDOBHandleJNI.hpp>

#include <MxMDTickSizeJNI.hpp>
#include <MxMDSegmentJNI.hpp>

#include <MxMDLibJNI.hpp>

namespace MxMDLibJNI {
  ZmLock	lock; // only used to serialize initialization
    jobject	  obj_;
    MxMDLib	  *md_;
    bool	  running = false;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "()V" } };

  // query callbacks
  ZJNI::JavaMethod instrumentFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDInstrument;)V" }
  };
  ZJNI::JavaMethod allInstrumentsFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDInstrument;)J" }
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

class MxMDLib_JNI {
public:
  ZuInline static MxMDLib *init(ZuString cf, ZmFn<ZmScheduler *> schedInitFn) {
    return MxMDLib::init(cf, ZuMv(schedInitFn));
  }
  ZuInline static ZmRef<MxMDInstrument> instrument_(
      const MxMDLib *md, const MxInstrKey &key) {
    return md->m_allInstruments.findKey(key);
  }
  ZuInline static ZmRef<MxMDOrderBook> orderBook_(
      const MxMDLib *md, const MxInstrKey &key) {
    return md->m_allOrderBooks.findKey(key);
  }
};

jobject MxMDLibJNI::instance(JNIEnv *env, jclass c)
{
  // () -> MxMDLib
  // ZmReadGuard<ZmLock> guard(lock);
  return obj_;
}

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
    jobject obj = env->NewObject(c, ctorMethod[0].mid);
    if (!obj) return 0;
    obj_ = env->NewGlobalRef(obj);
    env->DeleteLocalRef(obj);
  }

  md_ = md;

  return obj_;
}

void MxMDLibJNI::close(JNIEnv *env, jobject)
{
  MxMDJNI::final(env);
}

void MxMDLibJNI::start(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = md_;
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
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return;
  {
    ZmGuard<ZmLock> guard(lock);
    if (!running) return;
    running = false;
    md->allInstruments([](MxMDInstrument *instr) { instr->unsubscribe(); });
    md->stop();
    md->unsubscribe();
  }
}

jboolean MxMDLibJNI::record(JNIEnv *env, jobject obj, jstring path)
{
  // (String) -> boolean
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return false;
  return md->record(ZJNI::j2s_ZtString(env, path));
}

jstring MxMDLibJNI::stopRecording(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return 0;
  return ZJNI::s2j(env, md->stopRecording());
}

jboolean MxMDLibJNI::replay(JNIEnv *env, jobject obj,
    jstring path, jobject begin, jboolean filter)
{
  // (String, Instant, boolean) -> boolean
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return false;
  return md->replay(
      ZJNI::j2s_ZtString(env, path), ZJNI::j2t(env, begin), filter);
}

jstring MxMDLibJNI::stopReplaying(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return 0;
  return ZJNI::s2j(env, md->stopReplaying());
}

void MxMDLibJNI::startTimer(JNIEnv *env, jobject obj, jobject begin)
{
  // (Instant) -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return;
  md->startTimer(ZJNI::j2t(env, begin));
}

void MxMDLibJNI::stopTimer(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return;
  md->stopTimer();
}

void MxMDLibJNI::subscribe(JNIEnv *env, jobject obj, jobject handler_)
{
  // (MxMDLibHandler) -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return;
  md->subscribe(MxMDLibHandlerJNI::j2c(env, handler_));
  md->appData((uintptr_t)(void *)(env->NewGlobalRef(handler_)));
}

void MxMDLibJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return;
  md->unsubscribe();
  env->DeleteGlobalRef((jobject)(void *)(md->appData()));
  md->appData(0);
}

jobject MxMDLibJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDLibHandler
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return 0;
  return (jobject)(void *)(md->appData());
}

jobject MxMDLibJNI::instrument(JNIEnv *env, jobject obj, jobject key_)
{
  // (MxInstrKey) -> MxMDInstrHandle
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return 0;
  if (MxMDInstrHandle instr = md->instrument(MxInstrKeyJNI::j2c(env, key_)))
    return MxMDInstrHandleJNI::ctor(env, ZuMv(instr));
  return 0;
}

void MxMDLibJNI::instrument(JNIEnv *env, jobject obj, jobject key_, jobject fn)
{
  // (MxInstrKey, MxMDInstrumentFn) -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md || !fn)) return;
  md->instrInvoke(MxInstrKeyJNI::j2c(env, key_),
      [fn = ZJNI::globalRef(env, fn)](MxMDInstrument *instr) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, instrumentFn[0].mid,
	  MxMDInstrumentJNI::ctor(env, instr));
  });
}

jlong MxMDLibJNI::allInstruments(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllInstrumentsFn) -> long
  MxMDLib *md = md_;
  if (ZuUnlikely(!md || !fn)) return 0;
  return md->allInstruments(
      [fn = ZJNI::globalRef(env, fn)](MxMDInstrument *instr) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allInstrumentsFn[0].mid,
	  MxMDInstrumentJNI::ctor(env, instr));
    return 0;
  });
}

jobject MxMDLibJNI::orderBook(JNIEnv *env, jobject obj, jobject key_)
{
  // (MxInstrKey) -> MxMDOBHandle
  MxMDLib *md = md_;
  if (ZuUnlikely(!md)) return 0;
  if (MxMDOBHandle ob = md->orderBook(MxInstrKeyJNI::j2c(env, key_)))
    return MxMDOBHandleJNI::ctor(env, ZuMv(ob));
  return 0;
}

void MxMDLibJNI::orderBook(JNIEnv *env, jobject obj, jobject key_, jobject fn)
{
  // (MxInstrKey, MxMDOrderBookFn) -> void
  MxMDLib *md = md_;
  if (ZuUnlikely(!md || !fn)) return;
  md->obInvoke(MxInstrKeyJNI::j2c(env, key_),
      [fn = ZJNI::globalRef(env, fn)](MxMDOrderBook *ob) {
    if (JNIEnv *env = ZJNI::env())
      env->CallVoidMethod(fn, orderBookFn[0].mid,
	  MxMDOrderBookJNI::ctor(env, ob));
  });
}

jlong MxMDLibJNI::allOrderBooks(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllOrderBooksFn) -> long
  MxMDLib *md = md_;
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
  MxMDLib *md = md_;
  if (ZuUnlikely(!md || !id)) return 0;
  if (ZmRef<MxMDFeed> feed = md->feed(ZJNI::j2s_ZuStringN<8>(env, id)))
    return MxMDFeedJNI::ctor(env, ZuMv(feed));
  return 0;
}

jlong MxMDLibJNI::allFeeds(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllFeedsFn) -> long
  MxMDLib *md = md_;
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
  MxMDLib *md = md_;
  if (ZuUnlikely(!md || !id)) return 0;
  if (ZmRef<MxMDVenue> venue = md->venue(ZJNI::j2s_ZuStringN<8>(env, id)))
    return MxMDVenueJNI::ctor(env, ZuMv(venue));
  return 0;
}

jlong MxMDLibJNI::allVenues(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllVenuesFn) -> long
  MxMDLib *md = md_;
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
    { "instance",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDLibJNI::instance },
    { "init",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDLibJNI::init },
    { "close",
      "()V",
      (void *)&MxMDLibJNI::close },
    { "start",
      "()V",
      (void *)&MxMDLibJNI::start },
    { "stop",
      "()V",
      (void *)&MxMDLibJNI::stop },
    { "record",
      "(Ljava/lang/String;)Z",
      (void *)&MxMDLibJNI::record },
    { "stopRecording",
      "()Ljava/lang/String;",
      (void *)&MxMDLibJNI::stopRecording },
    { "replay",
      "(Ljava/lang/String;Ljava/time/Instant;Z)Z",
      (void *)&MxMDLibJNI::replay },
    { "stopReplaying",
      "()Ljava/lang/String;",
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
    { "instrument",
      "(Lcom/shardmx/mxbase/MxInstrKey;)Lcom/shardmx/mxmd/MxMDInstrHandle;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jobject)>(
	  MxMDLibJNI::instrument) },
    { "instrument",
      "(Lcom/shardmx/mxbase/MxInstrKey;Lcom/shardmx/mxmd/MxMDInstrumentFn;)V",
      (void *)static_cast<void (*)(JNIEnv *, jobject, jobject, jobject)>(
	  MxMDLibJNI::instrument) },
    { "allInstruments",
      "(Lcom/shardmx/mxmd/MxMDAllInstrumentsFn;)J",
      (void *)&MxMDLibJNI::allInstruments },
    { "orderBook",
      "(Lcom/shardmx/mxbase/MxInstrKey;)Lcom/shardmx/mxmd/MxMDOBHandle;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jobject)>(
	  MxMDLibJNI::orderBook) },
    { "orderBook",
      "(Lcom/shardmx/mxbase/MxInstrKey;Lcom/shardmx/mxmd/MxMDOrderBookFn;)V",
      (void *)static_cast<void (*)(JNIEnv *, jobject, jobject, jobject)>(
	  MxMDLibJNI::orderBook) },
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

  env->DeleteLocalRef((jobject)c);

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDInstrumentFn",
	instrumentFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllInstrumentsFn",
	allInstrumentsFn, 1) < 0) return -1;
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

  if (!obj_) return;

  env->DeleteGlobalRef(obj_); obj_ = 0;

  if (MxMDLib *md = md_) {
    md_ = 0;
    if (running) {
      running = false;
      md->allInstruments([](MxMDInstrument *instr) { instr->unsubscribe(); });
      md->stop();
      md->unsubscribe();
    }
    md->final();
  }
}
