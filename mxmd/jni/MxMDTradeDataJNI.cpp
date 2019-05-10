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

#include <MxMDTradeDataJNI.hpp>

namespace MxMDTradeDataJNI {
  jclass	class_;

  // MxMDTradeDataTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of", "("
      "Ljava/lang/String;"		// tradeID
      "Ljava/time/Instant;"		// transactTime
      "J"				// price
      "J"				// qty
      ")Lcom/shardmx/mxmd/MxMDTradeDataTuple;" }
  };
}

jobject MxMDTradeDataJNI::ctor(JNIEnv *env, const MxMDTradeData &data)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      ZJNI::s2j(env, data.tradeID),
      ZJNI::t2j(env, data.transactTime),
      (jlong)data.price, (jlong)data.qty);
}

int MxMDTradeDataJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDTradeDataTuple");
  if (!class_) return -1;
  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;

  return 0;
}

void MxMDTradeDataJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
