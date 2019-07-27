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

#include <MxTickDirJNI.hpp>
#include <MxTradingStatusJNI.hpp>

#include <MxMDL1DataJNI.hpp>

namespace MxMDL1DataJNI {
  jclass	class_;

  // MxMDL1DataTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of", "("
      "Ljava/time/Instant;"		// stamp
      "J"				// base
      "J"				// open
      "J"				// close
      "J"				// last
      "J"				// lastQty
      "J"				// bid
      "J"				// bidQty
      "J"				// ask
      "J"				// askQty
      "J"				// high
      "J"				// low
      "J"				// accVol
      "J"				// accVolQty
      "J"				// match
      "J"				// matchQty
      "J"				// surplusQty
      "J"				// flags
      "I"				// pxNDP
      "I"				// qtyNDP
      "Lcom/shardmx/mxbase/MxTickDir;"	// tickDir
      "Lcom/shardmx/mxbase/MxTradingStatus;"// status
      ")Lcom/shardmx/mxmd/MxMDL1DataTuple;" }
  };
}

jobject MxMDL1DataJNI::ctor(JNIEnv *env, const MxMDL1Data &l1Data)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      ZJNI::t2j(env, l1Data.stamp),
      (jlong)l1Data.base,
      (jlong)l1Data.open,
      (jlong)l1Data.close,
      (jlong)l1Data.last,
      (jlong)l1Data.lastQty,
      (jlong)l1Data.bid,
      (jlong)l1Data.bidQty,
      (jlong)l1Data.ask,
      (jlong)l1Data.askQty,
      (jlong)l1Data.high,
      (jlong)l1Data.low,
      (jlong)l1Data.accVol,
      (jlong)l1Data.accVolQty,
      (jlong)l1Data.match,
      (jlong)l1Data.matchQty,
      (jlong)l1Data.surplusQty,
      (jlong)l1Data.flags,
      (jint)l1Data.pxNDP,
      (jint)l1Data.qtyNDP,
      MxTickDirJNI::ctor(env, l1Data.tickDir),
      MxTradingStatusJNI::ctor(env, l1Data.status));
}

int MxMDL1DataJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDL1DataTuple");
  if (!class_) return -1;
  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;
  return 0;
}

void MxMDL1DataJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
