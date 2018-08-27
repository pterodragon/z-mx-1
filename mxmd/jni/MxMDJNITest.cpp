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

// MxMD Test JNI

#include <iostream>

#include <jni.h>

#include <ZJNI.hpp>

#include <MxMD.hpp>

#include <MxMDJNITest.hpp>

void MxMDJNITest::init(JNIEnv *env, jobject obj, jobject)
{
  // (MxMDLib) -> void

  std::cout << "MxMDJNITest::init()\n" << std::flush;

  if (MxMDLib *md = MxMDLib::instance())
    md->raise(ZeEVENT(Info, "Hello World"));
}

int MxMDJNITest::bind(JNIEnv *env)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
  static JNINativeMethod methods[] = {
    { "init",
      "(Lcom/shardmx/mxmd/MxMDLib;)V",
      (void *)&MxMDJNITest::init }
  };
#pragma GCC diagnostic pop

  return ZJNI::bind(env, "com/shardmx/mxmd/MxMDJNITest",
        methods, sizeof(methods) / sizeof(methods[0]));
}

extern "C" {
  MxMDExtern jint JNI_OnLoad(JavaVM *, void *);
}

jint JNI_OnLoad(JavaVM *jvm, void *)
{
  jint v = ZJNI::load(jvm);

  if (v < 0) return -1;

  JNIEnv *env = ZJNI::env();

  if (MxMDJNITest::bind(env) < 0) return -1;

  return v;
}
