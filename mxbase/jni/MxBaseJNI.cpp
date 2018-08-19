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

// MxBase JNI library

#include <MxBaseJNI.hpp>

#include <jni.h>

extern "C" {
  extern jint JNI_OnLoad(JavaVM *, void *);
}

static JavaVM *jvm = 0;
static thread_local JNIEnv *jenv = 0;

JavaVM *MxBaseJNI::vm() { return jvm; }
JNIEnv *MxBaseJNI::env() { return jenv; }
void MxBaseJNI::env(JNIEnv *env) { jenv = env; }

int MxBaseJNI::attach(const char *name)
{
  JavaVMAttachArgs args{ JNI_VERSION_1_4, const_cast<char *>(name), 0 };
  JNIEnv *env = 0;
  if (jvm->AttachCurrentThread((void **)&env, &args) < 0 || !env) return -1;
  jenv = env;
  return 0;
}

void MxBaseJNI::detach()
{
  jvm->DetachCurrentThread();
}

int MxBaseJNI::bind(
    JNIEnv *env, const char *cname, JNINativeMethod *methods, unsigned n)
{
  jclass c = env->FindClass(cname);

  if (!c) return -1;

  if (env->RegisterNatives(c, methods, n) < 0) return -1;

  return 0;
}

jint MxBaseJNI::onload(JavaVM *jvm_)
{
  jvm = jvm_;

  JNIEnv *env = 0;

  if (jvm_->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK || !env)
    return -1;

  jenv = env;

  return JNI_VERSION_1_4;
}
