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

#include <zlib/ZJNI.hpp>

#include <mxmd/MxMD.hpp>

#include <mxmd/MxMDOrderBookJNI.hpp>
#include <mxmd/MxMDOBSideJNI.hpp>
#include <mxmd/MxMDPxLevelJNI.hpp>
#include <mxmd/MxMDOrderDataJNI.hpp>

#include <mxmd/MxMDOrderJNI.hpp>

namespace MxMDOrderJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  ZuInline MxMDOrder *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDOrder> *ZuMayAlias(ptr) = (ZmRef<MxMDOrder> *)&ptr_;
    return ptr->ptr();
  }
}

void MxMDOrderJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDOrder> *ZuMayAlias(ptr) = (ZmRef<MxMDOrder> *)&ptr_;
  if (ptr) ptr->~ZmRef<MxMDOrder>();
}

jobject MxMDOrderJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrder
  MxMDOrder *order = ptr_(env, obj);
  if (ZuUnlikely(!order)) return 0;
  return MxMDOrderBookJNI::ctor(env, order->orderBook());
}

jobject MxMDOrderJNI::obSide(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide
  MxMDOrder *order = ptr_(env, obj);
  if (ZuUnlikely(!order)) return 0;
  return MxMDOBSideJNI::ctor(env, order->obSide());
}

jobject MxMDOrderJNI::pxLevel(JNIEnv *env, jobject obj)
{
  // () -> MxMDPxLevel
  MxMDOrder *order = ptr_(env, obj);
  if (ZuUnlikely(!order)) return 0;
  return MxMDPxLevelJNI::ctor(env, order->pxLevel());
}

jstring MxMDOrderJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String
  MxMDOrder *order = ptr_(env, obj);
  if (ZuUnlikely(!order)) return 0;
  return ZJNI::s2j(env, order->id());
}

jobject MxMDOrderJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderData
  MxMDOrder *order = ptr_(env, obj);
  if (ZuUnlikely(!order)) return 0;
  return MxMDOrderDataJNI::ctor(env, order->data());
}

jobject MxMDOrderJNI::ctor(JNIEnv *env, ZmRef<MxMDOrder> order)
{
  uintptr_t ptr;
  new (&ptr) ZmRef<MxMDOrder>(ZuMv(order));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr);
}

int MxMDOrderJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "dtor_",
      "(J)V",
      (void *)&MxMDOrderJNI::dtor_ },
    { "orderBook",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDOrderJNI::orderBook },
    { "obSide",
      "()Lcom/shardmx/mxmd/MxMDOBSide;",
      (void *)&MxMDOrderJNI::obSide },
    { "pxLevel",
      "()Lcom/shardmx/mxmd/MxMDPxLevel;",
      (void *)&MxMDOrderJNI::pxLevel },
    { "id",
      "()Ljava/lang/String;",
      (void *)&MxMDOrderJNI::id },
    { "data",
      "()Lcom/shardmx/mxmd/MxMDOrderData;",
      (void *)&MxMDOrderJNI::data },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrder");
  if (!class_) return -1;

  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDOrderJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
