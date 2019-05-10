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

#include <MxInstrIDSrcJNI.hpp>
#include <MxPutCallJNI.hpp>

#include <MxMDInstrRefDataJNI.hpp>

namespace MxMDInstrRefDataJNI {
  jclass	class_;

  // MxMDInstrRefDataTuple named constructor
  ZJNI::JavaMethod ctorMethod[] = {
    { "of", "("
      "Z"				// tradeable
      "Lcom/shardmx/mxbase/MxInstrIDSrc;"	// idSrc
      "Lcom/shardmx/mxbase/MxInstrIDSrc;"	// altIDSrc
      "Lcom/shardmx/mxbase/MxPutCall;"	// putCall
      "Ljava/lang/String;"		// symbol
      "Ljava/lang/String;"		// altSymbol
      "Ljava/lang/String;"		// underVenue
      "Ljava/lang/String;"		// underSegment
      "Ljava/lang/String;"		// underlying
      "I"				// mat
      "I"				// pxNDP
      "I"				// qtyNDP
      "J"				// strike
      "J"				// outstandingShares
      "J"				// adv
      ")Lcom/shardmx/mxmd/MxMDInstrRefDataTuple;"
    }
  };
}

jobject MxMDInstrRefDataJNI::ctor(JNIEnv *env, const MxMDInstrRefData &data)
{
  return env->CallStaticObjectMethod(class_, ctorMethod[0].mid,
      (jboolean)data.tradeable,
      MxInstrIDSrcJNI::ctor(env, data.idSrc),
      MxInstrIDSrcJNI::ctor(env, data.altIDSrc),
      MxPutCallJNI::ctor(env, data.putCall),
      ZJNI::s2j(env, data.symbol),
      ZJNI::s2j(env, data.altSymbol),
      ZJNI::s2j(env, data.underVenue),
      ZJNI::s2j(env, data.underSegment),
      ZJNI::s2j(env, data.underlying),
      (jint)data.mat,
      (jint)data.pxNDP,
      (jint)data.qtyNDP,
      (jlong)data.strike,
      (jlong)data.outstandingShares,
      (jlong)data.adv);
}

int MxMDInstrRefDataJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef(env, "com/shardmx/mxmd/MxMDInstrRefDataTuple");
  if (!class_) return -1;
  if (ZJNI::bindStatic(env, class_, ctorMethod, 1) < 0) return -1;
  return 0;
}

void MxMDInstrRefDataJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
}
