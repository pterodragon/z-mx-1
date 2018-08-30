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

#include <iostream>

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMD.hpp>

#include <MxSecKeyJNI.hpp>

#include <MxMDLibJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>
#include <MxMDSecurityJNI.hpp>
#include <MxMDOBSideJNI.hpp>

#include <MxMDLotSizesJNI.hpp>
#include <MxMDL1DataJNI.hpp>

#include <MxMDSecHandlerJNI.hpp>

#include <MxMDOrderBookJNI.hpp>

namespace MxMDOrderBookJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDOrderBook *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDOrderBook *)(void *)ptr;
  }

  jclass	sideClass;

  // MxSide named constructor
  ZJNI::JavaMethod sideMethod[] = {
    { "value", "(I)Lcom/shardmx/mxbase/MxSide;" }
  };
}

void MxMDOrderBookJNI::ctor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOrderBook *)(void *)(uintptr_t)ptr)->ref();
}

void MxMDOrderBookJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDOrderBook *)(void *)(uintptr_t)ptr)->deref();
  env->SetLongField(obj, ptrField[0].fid, (jlong)0);
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

jobject MxMDOrderBookJNI::security(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecurity
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDSecurityJNI::ctor(env, ob->security());
}

jobject MxMDOrderBookJNI::security(JNIEnv *env, jobject obj, jint leg)
{
  // (int) -> MxMDSecurity
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxMDSecurityJNI::ctor(env, ob->security(leg));
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
  // () -> MxSecKey
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return MxSecKeyJNI::ctor(env, ob->key());
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
  return env->CallStaticObjectMethod(sideClass, sideMethod[0].mid,
      (jint)ob->side(leg));
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
  // (MxMDSecHandler) -> void
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return;
  ob->subscribe(MxMDSecHandlerJNI::j2c(env, handler_));
  ob->appData((uintptr_t)(void *)(env->NewGlobalRef(handler_)));

}

void MxMDOrderBookJNI::unsubscribe(JNIEnv *env, jobject obj)
{
  // () -> void
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return;
  ob->unsubscribe();
  env->DeleteGlobalRef((jobject)(void *)(ob->appData()));
  ob->appData(0);
}

jobject MxMDOrderBookJNI::handler(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecHandler
  MxMDOrderBook *ob = ptr_(env, obj);
  if (ZuUnlikely(!ob)) return 0;
  return (jobject)(void *)(ob->appData());
}

jobject MxMDOrderBookJNI::ctor(JNIEnv *env, void *ptr)
{
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)(uintptr_t)ptr);
}

int MxMDOrderBookJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDOrderBookJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDOrderBookJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDOrderBookJNI::md },
    { "venue",
      "()Lcom/shardmx/mxmd/MxMDVenue;",
      (void *)&MxMDOrderBookJNI::venue },
    { "security",
      "()Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject)>(
	  MxMDOrderBookJNI::security) },
    { "security",
      "(I)Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)static_cast<jobject (*)(JNIEnv *, jobject, jint)>(
	  MxMDOrderBookJNI::security) },
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
      "()Lcom/shardmx/mxbase/MxSecKey;",
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
      "(Lcom/shardmx/mxmd/MxMDSecHandler;)V",
      (void *)&MxMDOrderBookJNI::subscribe },
    { "unsubscribe",
      "()V",
      (void *)&MxMDOrderBookJNI::unsubscribe },
    { "handler",
      "()Lcom/shardmx/mxmd/MxMDSecHandler;",
      (void *)&MxMDOrderBookJNI::handler },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrderBook");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  sideClass = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxSide");
  if (!sideClass) return -1;
  if (ZJNI::bindStatic(env, sideClass, sideMethod, 1) < 0) return -1;

  return 0;
}

void MxMDOrderBookJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
  if (sideClass) env->DeleteGlobalRef(sideClass);
}
