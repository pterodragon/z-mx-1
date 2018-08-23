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

#include <MxMDOrderJNI.hpp>

void MxMDOrderJNI::ctor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

void MxMDOrderJNI::dtor_(JNIEnv *env, jobject obj)
{
  // () -> void

}

jobject MxMDOrderJNI::orderBook(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderBook

  return 0;
}

jobject MxMDOrderJNI::obSide(JNIEnv *env, jobject obj)
{
  // () -> MxMDOBSide

  return 0;
}

jobject MxMDOrderJNI::pxLevel(JNIEnv *env, jobject obj)
{
  // () -> MxMDPxLevel

  return 0;
}

jstring MxMDOrderJNI::id(JNIEnv *env, jobject obj)
{
  // () -> String

  return 0;
}

jobject MxMDOrderJNI::data(JNIEnv *env, jobject obj)
{
  // () -> MxMDOrderData

  return 0;
}

int MxMDOrderJNI::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "ctor_",
      "()V",
      (void *)&MxMDOrderJNI::ctor_ },
    { "dtor_",
      "()V",
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

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDOrder",
        methods, sizeof(methods) / sizeof(methods[0]));
}
