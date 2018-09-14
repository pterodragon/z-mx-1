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

#include <MxMDSecurityJNI.hpp>
#include <MxMDOrderBookJNI.hpp>
#include <MxMDTradeDataJNI.hpp>

#include <MxMDTradeJNI.hpp>

namespace MxMDTradeJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(J)V" } };
  ZJNI::JavaField ptrField[] = { { "ptr", "J" } };

  ZuInline MxMDTrade *ptr_(JNIEnv *env, jobject obj) {
    uintptr_t ptr_ = env->GetLongField(obj, ptrField[0].fid);
    if (ZuUnlikely(!ptr_)) return nullptr;
    ZmRef<MxMDTrade> *ZuMayAlias(ptr) = (ZmRef<MxMDTrade> *)&ptr_;
    return ptr->ptr();
  }
}

void MxMDTradeJNI::ctor_(JNIEnv *env, jobject obj, jlong)
{
  // (long) -> void
}

void MxMDTradeJNI::dtor_(JNIEnv *env, jobject obj, jlong ptr_)
{
  // (long) -> void
  ZmRef<MxMDTrade> *ZuMayAlias(ptr) = (ZmRef<MxMDTrade> *)&ptr_;
  ptr->~ZmRef<MxMDTrade>();
}

jobject MxMDTradeJNI::security(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecurity
  MxMDTrade *trade = ptr_(env, obj);
  if (ZuUnlikely(!trade)) return 0;
  return MxMDSecurityJNI::ctor(env, trade->security());
}

jobject MxMDTradeJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook
  MxMDTrade *trade = ptr_(env, obj);
  if (ZuUnlikely(!trade)) return 0;
  return MxMDOrderBookJNI::ctor(env, trade->orderBook());
}

jobject MxMDTradeJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDTradeData
  MxMDTrade *trade = ptr_(env, obj);
  if (ZuUnlikely(!trade)) return 0;
  return MxMDTradeDataJNI::ctor(env, trade->data());
}

jobject MxMDTradeJNI::ctor(JNIEnv *env, ZmRef<MxMDTrade> trade)
{
  uintptr_t ptr_;
  ZmRef<MxMDTrade> *ZuMayAlias(ptr) = (ZmRef<MxMDTrade> *)&ptr_;
  new (ptr) ZmRef<MxMDTrade>(ZuMv(trade));
  return env->NewObject(class_, ctorMethod[0].mid, (jlong)ptr_);
}

int MxMDTradeJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "security",
      "()Lcom/shardmx/mxmd/MxMDSecurity;",
      (void *)&MxMDTradeJNI::security },
    { "orderBook",
      "()Lcom/shardmx/mxmd/MxMDOrderBook;",
      (void *)&MxMDTradeJNI::orderBook },
    { "data",
      "()Lcom/shardmx/mxmd/MxMDTradeData;",
      (void *)&MxMDTradeJNI::data },
  };
#pragma GCC diagnostic pop

  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDTrade");
  if (ZJNI::bind(env, class_,
        methods, sizeof(methods) / sizeof(methods[0])) < 0) return -1;

  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;
  if (ZJNI::bind(env, class_, ptrField, 1) < 0) return -1;

  return 0;
}

void MxMDTradeJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
