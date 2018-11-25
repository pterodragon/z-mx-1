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

#ifndef MxMDTradeJNI_HPP
#define MxMDTradeJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef MxMDLib_HPP
#include <MxMDLib.hpp>
#endif

#include <jni.h>

#include <MxMD.hpp>

namespace MxMDTradeJNI {
  // (long) -> void
  void ctor_(JNIEnv *, jobject, jlong);
  // (long) -> void
  void dtor_(JNIEnv *, jobject, jlong);

  // () -> MxMDInstrument
  jobject instrument(JNIEnv *, jobject);

  // () -> MxMDOrderBook
  jobject orderBook(JNIEnv *, jobject);

  // () -> MxMDTradeData
  jobject data(JNIEnv *, jobject);

  jobject ctor(JNIEnv *, ZmRef<MxMDTrade>);
  int bind(JNIEnv *);
  void final(JNIEnv *);
}

#endif /* MxMDTradeJNI_HPP */
