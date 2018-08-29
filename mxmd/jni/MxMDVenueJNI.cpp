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

#include <MxMDLibJNI.hpp>
#include <MxMDFeedJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDSegmentJNI.hpp>

#include <MxMDVenueJNI.hpp>

namespace MxMDVenueJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  MxMDVenue *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr)) return nullptr;
    return (MxMDVenue *)(void *)ptr;
  }

  jclass	oisClass;

  // MxOrderIDScope named constructor
  ZJNI::JavaMethod oisMethod[] = {
    { "value", "(I)Lcom/shardmx/mxmd/MxMDOrderIDScope;" }
  };

  // query callbacks
  ZJNI::JavaMethod allTickSizeTblsFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDTickSizeTbl;)J" }
  };
  ZJNI::JavaMethod allSegmentsFn[] = {
    { "fn", "(Lcom/shardmx/mxmd/MxMDSegment;)J" }
  };
}

void MxMDVenueJNI::ctor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDVenue *)(void *)(uintptr_t)ptr)->ref();
}

void MxMDVenueJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr)
{
  // (long) -> void
  if (ptr) ((MxMDVenue *)(void *)(uintptr_t)ptr)->deref();
}

jobject MxMDVenueJNI::md(JNIEnv *env, jobject obj)
{
  // () -> MxMDLib
  return MxMDLibJNI::instance_();
}

jobject MxMDVenueJNI::feed(JNIEnv *env, jobject obj)
{
  // () -> MxMDFeed

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue)) return 0;
  return MxMDFeedJNI::ctor(env, venue->feed());
}

jstring MxMDVenueJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue)) return 0;
  return ZJNI::s2j(env, venue->id());
}

jobject MxMDVenueJNI::orderIDScope(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderIDScope

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue)) return 0;
  return env->CallStaticObjectMethod(oisClass, oisMethod[0].mid,
      (jint)(int)venue->orderIDScope());
}

jlong MxMDVenueJNI::flags(JNIEnv *env, jobject obj)
{
  // () -> long

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue)) return 0;
  return (unsigned)venue->flags();
}

jboolean MxMDVenueJNI::loaded(JNIEnv *env, jobject obj)
{
  // () -> boolean

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue)) return 0;
  return venue->loaded();
}

jobject MxMDVenueJNI::tickSizeTbl(JNIEnv *env, jobject obj, jstring id)
{
  // (String) -> MxMDTickSizeTbl

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue || !id)) return 0;
  ZmRef<MxMDTickSizeTbl> tbl =
    venue->tickSizeTbl(ZJNI::j2s_ZuStringN<8>(env, id));
  if (!tbl) return 0;
  return MxMDTickSizeTblJNI::ctor(env, tbl);
}

jlong MxMDVenueJNI::allTickSizeTbls(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllTickSizeTblsFn) -> long

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue || !fn)) return 0;
  return venue->allTickSizeTbls(
      [fn = ZJNI::globalRef(env, fn)](MxMDTickSizeTbl *tbl) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allTickSizeTblsFn[0].mid,
	  MxMDTickSizeTblJNI::ctor(env, tbl));
    return 0;
  });
}

jlong MxMDVenueJNI::allSegments(JNIEnv *env, jobject obj, jobject fn)
{
  // (MxMDAllSegmentsFn) -> void

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue || !fn)) return 0;
  return venue->allSegments(
      [fn = ZJNI::globalRef(env, fn)](const MxMDSegment &seg) -> uintptr_t {
    if (JNIEnv *env = ZJNI::env())
      return env->CallLongMethod(fn, allSegmentsFn[0].mid,
	  MxMDSegmentJNI::ctor(env, seg));
    return 0;
  });
}

jobject MxMDVenueJNI::tradingSession(JNIEnv *env, jobject obj, jstring id)
{
  // (String) -> MxMDSegment

  MxMDVenue *venue = ptr_(env, obj);
  if (ZuUnlikely(!venue || !id)) return 0;
  MxMDSegment seg =
    venue->tradingSession(ZJNI::j2s_ZuStringN<8>(env, id));
  return MxMDSegmentJNI::ctor(env, seg);
}

jobject MxMDVenueJNI::ctor(JNIEnv *env, void *ptr)
{
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)(uintptr_t)ptr);
}

int MxMDVenueJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "(J)V",
      (void *)&MxMDVenueJNI::ctor_ },
    { "dtor_",
      "(J)V",
      (void *)&MxMDVenueJNI::dtor_ },
    { "md",
      "()Lcom/shardmx/mxmd/MxMDLib;",
      (void *)&MxMDVenueJNI::md },
    { "feed",
      "()Lcom/shardmx/mxmd/MxMDFeed;",
      (void *)&MxMDVenueJNI::feed },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDVenueJNI::id },
    { "orderIDScope",
      "()Lcom/shardmx/mxmd/MxMDOrderIDScope;",
      (void *)&MxMDVenueJNI::orderIDScope },
    { "flags",
      "()J",
      (void *)&MxMDVenueJNI::flags },
    { "loaded",
      "()Z",
      (void *)&MxMDVenueJNI::loaded },
    { "tickSizeTbl",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDTickSizeTbl;",
      (void *)&MxMDVenueJNI::tickSizeTbl },
    { "allTickSizeTbls",
      "(Lcom/shardmx/mxmd/MxMDAllTickSizeTblsFn;)J",
      (void *)&MxMDVenueJNI::allTickSizeTbls },
    { "allSegments",
      "(Lcom/shardmx/mxmd/MxMDAllSegmentsFn;)J",
      (void *)&MxMDVenueJNI::allSegments },
    { "tradingSession",
      "(Ljava/lang/String;)Lcom/shardmx/mxmd/MxMDSegment;",
      (void *)&MxMDVenueJNI::tradingSession },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDVenue");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  oisClass = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrderIDScope");
  if (!oisClass) return -1;
  if (ZJNI::bindStatic(env, oisClass, oisMethod, 1) < 0) return -1;

  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllTickSizeTblsFn",
	allTickSizeTblsFn, 1) < 0) return -1;
  if (ZJNI::bind(env, "com/shardmx/mxmd/MxMDAllSegmentsFn",
	allSegmentsFn, 1) < 0) return -1;

  return 0;
}

void MxMDVenueJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
  if (oisClass) env->DeleteGlobalRef(oisClass);
}
