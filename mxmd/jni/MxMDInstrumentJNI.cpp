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

#include <MxInstrKeyJNI.hpp>

#include <MxMDLibJNI.hpp>
#include <MxMDDerivativesJNI.hpp>
#include <MxMDOrderBookJNI.hpp>

#include <MxMDInstrRefDataJNI.hpp>

#include <MxMDInstrHandlerJNI.hpp>

#include <MxMDInstrumentJNI.hpp>

namespace MxMDInstrumentJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  ZuInline MxMDInstrument *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDInstrument> *ZuMayAlias(ptr) = (ZmRef<MxMDInstrument> *)&ptr_;
    return ptr->ptr();
  }

  // query callbacks
  ZJNI::JavaMethod allOrderBooksFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDOrderBook;)J" }
  };
}

void MxMDInstrumentJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void
}

void MxMDInstrumentJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDInstrument> *ZuMayAlias(ptr) = (ZmRef<MxMDInstrument> *)&ptr_;
  ptr->~ZmRef<MxMDInstrument>();
}

jobject MxMDInstrumentJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib
  return MxMDLibJNI::instance_();
}

jstring MxMDInstrumentJNI::primaryVenue(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return ZJNI::s2j(env, instr->primaryVenue());
}

jstring MxMDInstrumentJNI::primarySegment(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return ZJNI::s2j(env, instr->primarySegment());
}

jstring MxMDInstrumentJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return ZJNI::s2j(env, instr->id());
}

jobject MxMDInstrumentJNI::key(JNIEnv *env, jobject obj)
{
  // () -> MxInstrKey
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return MxInstrKeyJNI::ctor(env, instr->key());
}

jobject MxMDInstrumentJNI::refData(JNIEnv *env, jobject obj)
{
  // () -> MxMDInstrRefData
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return MxMDInstrRefDataJNI::ctor(env, instr->refData());
}

jobject MxMDInstrumentJNI::underlying(JNIEnv *env, jobject obj)
{
  // () -> MxMDInstrument
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return MxMDInstrumentJNI::ctor(env, instr->underlying());
}

jobject MxMDInstrumentJNI::derivatives(JNIEnv *env, jobject obj)
{
  // () -> MxMDDerivatives
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return MxMDDerivativesJNI::ctor(env, instr->derivatives());
}

void MxMDInstrumentJNI::subscribe(JNIEnv *env, jobject obj, jobject handler_)
{
  // (MxMDInstrHandler) -> void
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return;
  instr->subscribe(MxMDInstrHandlerJNI::j2c(env, handler_));
  instr->appData((uintptr_t)(void *)(env->NewGlobalRef(handler_)));
}

void MxMDInstrumentJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return;
  instr->unsubscribe();
  env->DeleteGlobalRef((jobject)(void *)(instr->appData()));
  instr->appData(0);
}

jobject MxMDInstrumentJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDInstrHandler
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr)) return 0;
  return (jobject)(void *)(instr->appData());
}

jobject MxMDInstrumentJNI::orderBook(JNIEnv *env, jobject obj, jstring venue)
{
  // (String) -> MxMDOrderBook
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr || !venue)) return 0;
  return MxMDOrderBookJNI::ctor(env, instr->orderBook(
	ZJNI::j2s_ZuStringN<8>(env, venue)));
}

jobject MxMDInstrumentJNI::orderBook(
    JNIEnv *env, jobject obj, jstring venue, jstring segment)
{
  // (String, String) -> MxMDOrderBook
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr || !venue)) return 0;
  return MxMDOrderBookJNI::ctor(env, instr->orderBook(
	ZJNI::j2s_ZuStringN<8>(env, venue),
	ZJNI::j2s_ZuStringN<8>(env, segment)));
}

jlong MxMDInstrumentJNI::allOrderBooks(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllOrderBooksFn) -> long
  MxMDInstrument *instr = ptr_(env, obj);
  if (ZuUnlikely(!instr || !fn)) return 0;
  return instr->allOrderBooks(
      [fn = ZJNI::globalRef(env, fn)](MxMDOrderBook *ob) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allOrderBooksFn[0].mid,
	  MxMDOrderBookJNI::ctor(env, ob));
    return 0;
  });
}

jobject MxMDInstrumentJNI::ctor(JNIEnv *env, ZmRef<MxMDInstrument> instr)
{
  uintptr_t ptr_;
  ZmRef<MxMDInstrument> *ZuMayAlias(ptr) = (ZmRef<MxMDInstrument> *)&ptr_;
  new (ptr) ZmRef<MxMDInstrument>(ZuMv(instr));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr_);
}

int MxMDInstrumentJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDInstrumentJNI::md },
    { "primaryVenue",
      "()Ljava/lang/String;",
      (void *)&MxMDInstrumentJNI::primaryVenue },
    { "primarySegment",
      "()Ljava/lang/String;",
      (void *)&MxMDInstrumentJNI::primarySegment },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDInstrumentJNI::id },
    { "key",
      "()Lcom/shardmx/mxbase/MxInstrKey;",
      (void *)&MxMDInstrumentJNI::key },
    { "refData",
      "()Lcom/shardmx/mxmd/MxMDInstrRefData;",
      (void *)&MxMDInstrumentJNI::refData },
    { "underlying",
      "()Lcom/shardmx/mxmd/MxMDInstrument;",
      (void *)&MxMDInstrumentJNI::underlying },
    { "derivatives",
      "()Lcom/shardmx/mxmd/MxMDDerivatives;",
      (void *)&MxMDInstrumentJNI::derivatives },
    { "subscribe",
      "(Lcom/shardmx/mxmd/MxMDInstrHandler;)V",
      (void *)&MxMDInstrumentJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDInstrumentJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDInstrHandler;",
      (void *)&MxMDInstrumentJNI::handler },
    { "orderBook",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jstring)>(
	  MxMDInstrumentJNI::orderBook) },
    { "orderBook",
      "(Ljava/lang/String;Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jstring, jstring)>(
	  MxMDInstrumentJNI::orderBook) },
    { "allOrderBooks",
      "(Lcom/shardmx/mxmd/MxMDAllOrderBooksFn;)J",
      (void *)&MxMDInstrumentJNI::allOrderBooks },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDInstrument");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllOrderBooksFn",
	allOrderBooksFn, 1) < 0) return -1;

  return 0;
}

void MxMDInstrumentJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}