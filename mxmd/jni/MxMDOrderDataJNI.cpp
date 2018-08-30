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

#include <MxMDOrderDataJNI.hpp>

namespace MxMDOrderDataJNI {
  jclass	class_;

  // MxMDOrderDataTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of",
      "(Ljava/time/Instant;Lcom/shardmx/mxbase/MxSide;IJJJ)"
      "Lcom/shardmx/mxmd/MxMDOrderDataTuple;" }
  };

  jclass	sideClass;

  // MxSide named constructor
  ZJNI::JavaMethod sideMethod[] = {
    { "value", "(I)Lcom/shardmx/mxbase/MxSide;" }
  };
}

jobject MxMDOrderDataJNI::ctor(JNIEnv *env, const MxMDOrderData &data)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      ZJNI::t2j(env, data.transactTime),
      env->CallStaticObjectMethod(sideClass, sideMethod[0].mid,
	(jint)data.side),
      (jint)data.rank, (jlong)data.price, (jlong)data.qty, (jlong)data.flags);
}

int MxMDOrderDataJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDOrderDataTuple");
  if (!class_) return -1;
  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;

  sideClass = ZJNI::globalClassRef(env, "com/shardmx/mxbase/MxSide");
  if (!sideClass) return -1;
  if (ZJNI::bindStatic(env, sideClass, sideMethod, 1) < 0) return -1;

  return 0;
}

void MxMDOrderDataJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
  if (sideClass) env->DeleteGlobalRef(sideClass);
}
