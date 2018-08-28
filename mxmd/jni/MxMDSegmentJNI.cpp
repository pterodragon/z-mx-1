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

#include <MxMDSegmentJNI.hpp>

namespace MxMDSegmentJNI {
  jclass	class_;

  ZJNI::JavaMethod ctorMethod[] = { { "<init>", "(JJJ)V" } };

  jclass	tsClass;

  ZJNI::JavaMethod tsMethod = {
    { "value", "(I)Lcom/shardmx/mxbase/MxTradingSession;" }
  };
}

jobject MxMDSegmentJNI::ctor(JNIEnv *env, MxMDSegment segment)
{
  return env->NewObject(class_, ctorMethod[0].mid,
      ZJNI::s2j(segment.id),
      env->CallStaticObjectMethod(tsClass, tsMethod[1].mid,
	(jint)segment.session),
      ZJNI::t2j(segment.stamp));
}

int MxMDSegmentJNI::bind(JNIEnv *env)
{
  class_ = ZJNI::globalClassRef("com/shardmx/mxmd/MxMDSegment");
  if (!class_) return -1;
  if (ZJNI::bind(env, class_, ctorMethod, 1) < 0) return -1;

  tsClass = ZJNI::globalClassRef("com/shardmx/mxbase/MxTradingSession");
  if (!tsClass) return -1;
  return ZJNI::bind(env, tsClass, tradingSessionMethod, 1);
}

void MxMDSegmentJNI::final(JNIEnv *env)
{
  if (class_) env->DeleteGlobalRef(class_);
  if (tsClass) env->DeleteGlobalRef(tsClass);
}
