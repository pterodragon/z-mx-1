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

#include <MxSideJNI.hpp>
#include <MxInstrKeyJNI.hpp>

#include <MxMD.hpp>

#include <MxMDLibJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>
#include <MxMDInstrumentJNI.hpp>
#include <MxMDOBSideJNI.hpp>

#include <MxMDLotSizesJNI.hpp>
#include <MxMDL1DataJNI.hpp>

#include <MxMDInstrHandlerJNI.hpp>

#include <MxMDOrderBookJNI.hpp>

namespace MxMDOrderBookJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  ZuInline MxMDOrderBook *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDOrderBook> *ZuMayAlias(ptr) = (ZmRef<MxMDOrderBook> *)&ptr_;
    return ptr->ptr();
  }

  // generic unsubscribe
  using MxMDLibJNI::unsubscribe_;
}

void MxMDOrderBookJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void
}

void MxMDOrderBookJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDOrderBook> *ZuMayAlias(ptr) = (ZmRef<MxMDOrderBook> *)&ptr_;
  if (ptr) {
    if (MxMDOrderBook *ob = *ptr) unsubscribe_(env, ob);
    ptr->~ZmRef<MxMDOrderBook>();
  }
}

jobject MxMDOrderBookJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib
  return MxMDLibJNI::instance_();
}

jobject MxMDOrderBookJNI::venue(JNIEnv *env, jobject obj)
{
  // () -> MxMDVenue
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDVenueJNI::ctor(env, ob->venue());
}

jobject MxMDOrderBookJNI::instrument(JNIEnv *env, jobject obj)
{
  // () -> MxMDInstrument
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDInstrumentJNI::ctor(env, ob->instrument());
}

jobject MxMDOrderBookJNI::instrument(JNIEnv *env, jobject obj, jint leg)
{
  // (int) -> MxMDInstrument
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDInstrumentJNI::ctor(env, ob->instrument(leg));
}

jobject MxMDOrderBookJNI::out(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDOrderBookJNI::ctor(env, ob->out());
}

jstring MxMDOrderBookJNI::venueID(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ZJNI::s2j(env, ob->venueID());
}

jstring MxMDOrderBookJNI::segment(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ZJNI::s2j(env, ob->segment());
}

jstring MxMDOrderBookJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ZJNI::s2j(env, ob->id());
}

jobject MxMDOrderBookJNI::key(JNIEnv *env, jobject obj)
{
  // () -> MxInstrKey
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxInstrKeyJNI::ctor(env, ob->key());
}

jint MxMDOrderBookJNI::legs(JNIEnv *env, jobject obj)
{
  // () -> int
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ob->legs();
}

jobject MxMDOrderBookJNI::side(JNIEnv *env, jobject obj, jint leg)
{
  // (int) -> MxSide
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxSideJNI::ctor(env, ob->side(leg));
}

jint MxMDOrderBookJNI::ratio(JNIEnv *env, jobject obj, jint leg)
{
  // (int) -> int
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ob->ratio(leg);
}

jint MxMDOrderBookJNI::pxNDP(JNIEnv *env, jobject obj)
{
  // () -> int
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ob->pxNDP();
}

jint MxMDOrderBookJNI::qtyNDP(JNIEnv *env, jobject obj)
{
  // () -> int
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ob->qtyNDP();
}

jobject MxMDOrderBookJNI::tickSizeTbl(JNIEnv *env, jobject obj)
{
  // () -> MxMDTickSizeTbl
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDTickSizeTblJNI::ctor(env, ob->tickSizeTbl());
}

jobject MxMDOrderBookJNI::lotSizes(JNIEnv *env, jobject obj)
{
  // () -> MxMDLotSizes
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDLotSizesJNI::ctor(env, ob->lotSizes());
}

jobject MxMDOrderBookJNI::l1Data(JNIEnv *env, jobject obj)
{
  // () -> MxMDL1Data
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDL1DataJNI::ctor(env, ob->l1Data());
}

jobject MxMDOrderBookJNI::bids(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDOBSideJNI::ctor(env, ob->bids());
}

jobject MxMDOrderBookJNI::asks(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDOBSideJNI::ctor(env, ob->asks());
}

void MxMDOrderBookJNI::subscribe(JNIEnv *env, jobject obj, jobject handler_)
{
  // (MxMDInstrHandler) -> void
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return;
  ob->subscribe(MxMDInstrHandlerJNI::j2c(env, handler_));
  ob->libData(env->NewGlobalRef(handler_));

}

void MxMDOrderBookJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void
  if (MxMDOrderBook *ob = ptr_(env, obj)) unsubscribe_(env, ob);
}

jobject MxMDOrderBookJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDInstrHandler
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return ob->libData<jobject>();
}

jobject MxMDOrderBookJNI::ctor(JNIEnv *env, ZmRef<MxMDOrderBook> ob)
{
  uintptr_t ptr;
  new (&ptr) ZmRef<MxMDOrderBook>(ZuMv(ob));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr);
}

int MxMDOrderBookJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDOrderBookJNI::md },
    { "venue",
      "()Lcom/shardmx/mxmd/MxMDVenue;",
      (void *)&MxMDOrderBookJNI::venue },
    { "instrument",
      "()Lcom/shardmx/mxmd/MxMDInstrument;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject)>(
	  MxMDOrderBookJNI::instrument) },
    { "instrument",
      "(I)Lcom/shardmx/mxmd/MxMDInstrument;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jint)>(
	  MxMDOrderBookJNI::instrument) },
    { "out",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDOrderBookJNI::out },
    { "venueID",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::venueID },
    { "segment",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::segment },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderBookJNI::id },
    { "key",
      "()Lcom/shardmx/mxbase/MxInstrKey;",
      (void *)&MxMDOrderBookJNI::key },
    { "legs",
      "()I",
      (void *)&MxMDOrderBookJNI::legs },
    { "side",
      "(I)Lcom/shardmx/mxbase/MxSide;",
      (void *)&MxMDOrderBookJNI::side },
    { "ratio",
      "(I)I",
      (void *)&MxMDOrderBookJNI::ratio },
    { "pxNDP",
      "()I",
      (void *)&MxMDOrderBookJNI::pxNDP },
    { "qtyNDP",
      "()I",
      (void *)&MxMDOrderBookJNI::qtyNDP },
    { "tickSizeTbl",
      "()Lcom/shardmx/mxmd/MxMDTickSizeTbl;",
      (void *)&MxMDOrderBookJNI::tickSizeTbl },
    { "lotSizes",
      "()Lcom/shardmx/mxmd/MxMDLotSizes;",
      (void *)&MxMDOrderBookJNI::lotSizes },
    { "l1Data",
      "()Lcom/shardmx/mxmd/MxMDL1Data;",
      (void *)&MxMDOrderBookJNI::l1Data },
    { "bids",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDOrderBookJNI::bids },
    { "asks",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDOrderBookJNI::asks },
    { "subscribe",
      "(Lcom/shardmx/mxmd/MxMDInstrHandler;)V",
      (void *)&MxMDOrderBookJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDOrderBookJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDInstrHandler;",
      (void *)&MxMDOrderBookJNI::handler },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrderBook");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDOrderBookJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
