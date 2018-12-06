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

#include <jni.h>

#include <ZJNI.hpp>

#include <MxBaseJNI.hpp>

#include <MxMDOrderIDScopeJNI.hpp>

#include <MxMDLibJNI.hpp>
#include <MxMDFeedJNI.hpp>
#include <MxMDVenueJNI.hpp>
#include <MxMDTickSizeTblJNI.hpp>

#include <MxMDInstrumentJNI.hpp>
#include <MxMDDerivativesJNI.hpp>

#include <MxMDOrderBookJNI.hpp>
#include <MxMDOBSideJNI.hpp>
#include <MxMDOrderJNI.hpp>
#include <MxMDPxLevelJNI.hpp>

#include <MxMDTradeJNI.hpp>

#include <MxMDLibHandlerJNI.hpp>
#include <MxMDInstrHandlerJNI.hpp>

#include <MxMDInstrHandleJNI.hpp>
#include <MxMDOBHandleJNI.hpp>

#include <MxMDTickSizeJNI.hpp>
#include <MxMDSegmentJNI.hpp>
#include <MxMDInstrRefDataJNI.hpp>
#include <MxMDLotSizesJNI.hpp>
#include <MxMDL1DataJNI.hpp>
#include <MxMDOBSideDataJNI.hpp>
#include <MxMDPxLvlDataJNI.hpp>
#include <MxMDOrderDataJNI.hpp>
#include <MxMDTradeDataJNI.hpp>

#include <MxMDJNI.hpp>

extern "C" {
  MxMDExtern jint JNI_OnLoad(JavaVM *, void *);
}

jint JNI_OnLoad(JavaVM *jvm, void *)
{
  // std::cout << "JNI_OnLoad()\n" << std::flush;

  jint v = ZJNI::load(jvm);

  JNIEnv *env = ZJNI::env();

  if (!env || MxMDJNI::bind(env) < 0) return -1;

  return v;
}

int MxMDJNI::bind(JNIEnv *env)
{
  if (MxBaseJNI::bind(env) < 0) return -1;

  if (MxMDOrderIDScopeJNI::bind(env) < 0) return -1;

  if (MxMDLibJNI::bind(env) < 0) return -1;
  if (MxMDFeedJNI::bind(env) < 0) return -1;
  if (MxMDVenueJNI::bind(env) < 0) return -1;
  if (MxMDTickSizeTblJNI::bind(env) < 0) return -1;

  if (MxMDInstrumentJNI::bind(env) < 0) return -1;
  if (MxMDDerivativesJNI::bind(env) < 0) return -1;

  if (MxMDOrderBookJNI::bind(env) < 0) return -1;
  if (MxMDOBSideJNI::bind(env) < 0) return -1;
  if (MxMDPxLevelJNI::bind(env) < 0) return -1;
  if (MxMDOrderJNI::bind(env) < 0) return -1;

  if (MxMDTradeJNI::bind(env) < 0) return -1;

  if (MxMDLibHandlerJNI::bind(env) < 0) return -1;
  if (MxMDInstrHandlerJNI::bind(env) < 0) return -1;

  if (MxMDInstrHandleJNI::bind(env) < 0) return -1;
  if (MxMDOBHandleJNI::bind(env) < 0) return -1;

  if (MxMDTickSizeJNI::bind(env) < 0) return -1;
  if (MxMDSegmentJNI::bind(env) < 0) return -1;
  if (MxMDInstrRefDataJNI::bind(env) < 0) return -1;
  if (MxMDLotSizesJNI::bind(env) < 0) return -1;
  if (MxMDL1DataJNI::bind(env) < 0) return -1;
  if (MxMDOBSideDataJNI::bind(env) < 0) return -1;
  if (MxMDPxLvlDataJNI::bind(env) < 0) return -1;
  if (MxMDOrderDataJNI::bind(env) < 0) return -1;
  if (MxMDTradeDataJNI::bind(env) < 0) return -1;

  return 0;
}

void MxMDJNI::final(JNIEnv *env)
{
  MxMDLibJNI::final(env);
  MxMDFeedJNI::final(env);
  MxMDVenueJNI::final(env);
  MxMDTickSizeTblJNI::final(env);

  MxMDInstrumentJNI::final(env);
  MxMDDerivativesJNI::final(env);

  MxMDOrderBookJNI::final(env);
  MxMDOBSideJNI::final(env);
  MxMDPxLevelJNI::final(env);
  MxMDOrderJNI::final(env);

  MxMDTradeJNI::final(env);

  MxMDLibHandlerJNI::final(env);
  MxMDInstrHandlerJNI::final(env);

  MxMDInstrHandleJNI::final(env);
  MxMDOBHandleJNI::final(env);

  MxMDTickSizeJNI::final(env);
  MxMDSegmentJNI::final(env);
  MxMDInstrRefDataJNI::final(env);
  MxMDLotSizesJNI::final(env);
  MxMDL1DataJNI::final(env);
  MxMDOBSideDataJNI::final(env);
  MxMDPxLvlDataJNI::final(env);
  MxMDOrderDataJNI::final(env);
  MxMDTradeDataJNI::final(env);

  MxMDOrderIDScopeJNI::final(env);

  MxBaseJNI::final(env);

  ZJNI::final(env);
}
