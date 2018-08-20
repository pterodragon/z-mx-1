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

// Z JNI library

#ifndef ZJNI_HPP
#define ZJNI_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZJNILib_HPP
#include <ZJNILib.hpp>
#endif

#ifndef _WIN32
#include <alloca.h>
#endif

#include <jni.h>

#include <ZuArray.hpp>
#include <ZuString.hpp>
#include <ZuUTF.hpp>
#include <ZuStringN.hpp>

class ZJNIAPI ZJNI {
public:
  // called from JNI_OnLoad(JavaVM *, void *)
  static jint onload(JavaVM *);

  static JavaVM *vm();
  static JNIEnv *env();		// retrieves the env from TLS
  static void env(JNIEnv *);	// stores the env in TLS

  // bind C++ native methods to Java class - returns -ve on error
  static int bind(
      JNIEnv *, const char *cname, JNINativeMethod *methods, unsigned n);

  static int attach(const char *name);	// attach thread - returns -ve on error
  static void detach();			// detach thread

  // C++ string -> Java string
  inline static jstring c2j(JNIEnv *env, ZuString s) {
    if (ZuUnlikely(!s)) return env->NewStringUTF("");
    unsigned n = ZuUTF<jchar, char>::len(s);
    if (ZuUnlikely(!n)) return env->NewStringUTF("");
    jchar *buf;
#ifdef _MSC_VER
    __try {
      buf = (jchar *)_alloca(n * sizeof(jchar));
    } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
      _resetstkoflw();
      buf = 0;
    }
#else
    buf = (jchar *)alloca(n * sizeof(jchar));
#endif
    if (!buf) return 0;
    n = ZuUTF<jchar, char>::cvt(ZuArray<jchar>(buf, n), s);
    return env->NewString(buf, n);
  }

  // Java string -> C++ string
  template <typename Alloc, typename Buf, typename SetLen>
  inline static auto j2c(JNIEnv *env, jstring s_,
      Alloc &&alloc, Buf &&buf, SetLen &&setLen) {
    if (ZuUnlikely(!s_)) return alloc(0);
    unsigned n = env->GetStringLength(s_);
    if (ZuUnlikely(!n)) return alloc(0);
    ZuArray<const jchar> s(env->GetStringCritical(s_, 0), n);
    auto o = alloc(ZuUTF<char, jchar>::len(s));
    setLen(o, ZuUTF<char, jchar>::cvt(buf(o), s));
    env->ReleaseStringCritical(s_, s.data()); // should be nop
    return o;
  }
  // usual fixed-width case - Java -> ZuStringN<N>
  template <unsigned N>
  ZuInline static ZuStringN<N> j2n(JNIEnv *env, jstring s) {
    return j2c(env, s,
	[](unsigned) { return ZuStringN<N>(); },
	[](ZuStringN<N> &s) { return ZuArray<char>(s.data(), N - 1); },
	[](ZuStringN<N> &s, unsigned n) { s.length(n); });
  }
};

#endif /* ZJNI_HPP */
