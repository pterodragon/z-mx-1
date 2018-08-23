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

#include <MxMDTradeJNI.hpp>

void MxMDTradeJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDTradeJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDTradeJNI::security(JNIEnv *env, jobject obj)
{
  // () -> MxMDSecurity

  return 0;
}

jobject MxMDTradeJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook

  return 0;
}

jobject MxMDTradeJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDTradeData

  return 0;
}

int MxMDTradeJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDTradeJNI::ctor_ },
    { "dtor_",
      "()V",
      (void *)&MxMDTradeJNI::dtor_ },
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

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDTrade",
        methods, sizeof(methods) / sizeof(methods[0]));
}
