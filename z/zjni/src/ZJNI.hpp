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

#include <ZtDate.hpp>
#include <ZtString.hpp>

namespace ZJNI {
  // called from JNI_OnLoad(JavaVM *, void *)
  ZJNIExtern jint load(JavaVM *);
  ZJNIExtern void final(JNIEnv *);

  ZJNIExtern JavaVM *vm();
  ZJNIExtern JNIEnv *env();		// retrieves the env from TLS
  ZJNIExtern void env(JNIEnv *);	// stores the env in TLS

  // bind C++ methods to Java (wrapper of RegisterNatives)
  ZJNIExtern int bind(JNIEnv *, const char *cname,
      const JNINativeMethod *, unsigned n);
  ZJNIExtern int bind(JNIEnv *, jclass,
      const JNINativeMethod *, unsigned n);

  // bind Java methods to C++ (reverse of RegisterNatives)
  struct JavaMethod {
    const char	*name;
    const char	*signature;
    jmethodID	mid = 0;
  };
  ZJNIExtern int bind(JNIEnv *, jclass,
      JavaMethod *, unsigned n);
  ZJNIExtern int bind(JNIEnv *, const char *cname,
      JavaMethod *, unsigned n);
  ZJNIExtern int bindStatic(JNIEnv *, jclass,
      JavaMethod *, unsigned n);
  ZJNIExtern int bindStatic(JNIEnv *, const char *cname,
      JavaMethod *, unsigned n);
 
  // bind Java fields to C++
  struct JavaField {
    const char	*name;
    const char	*type;
    jfieldID	fid = 0;
  };
  ZJNIExtern int bind(JNIEnv *, const char *cname,
      JavaField *, unsigned n);
  ZJNIExtern int bind(JNIEnv *, jclass,
      JavaField *, unsigned n);
  ZJNIExtern int bindStatic(JNIEnv *, const char *cname,
      JavaField *, unsigned n);
  ZJNIExtern int bindStatic(JNIEnv *, jclass,
      JavaField *, unsigned n);

  ZJNIExtern int attach(const char *name);	// attach thread - -ve on error
  ZJNIExtern void detach();			// detach thread

  ZJNIExtern void throwNPE(JNIEnv *, ZuString);	// throw NullPointerException

  // RAI for local/global references to Java (jobject/jstring/jclass/...)
  template <typename T> class LocalRef {
    LocalRef(const LocalRef &r) = delete;
    LocalRef &operator =(const LocalRef &r) = delete;
  public:
    ZuInline LocalRef(JNIEnv *env, T obj) : m_env(env), m_obj(obj) { }
    ZuInline ~LocalRef() { if (m_obj) m_env->DeleteLocalRef((jobject)m_obj); }
    ZuInline LocalRef(LocalRef &&r) : m_env(r.m_env), m_obj(r.m_obj) {
      r.m_obj = 0;
    }
    ZuInline LocalRef &operator =(LocalRef &&r) {
      if (m_obj) m_env->DeleteLocalRef((jobject)m_obj);
      m_env = r.m_env;
      m_obj = r.m_obj;
      r.m_obj = 0;
      return *this;
    }
    ZuInline void null() {
      if (m_obj) {
	m_env->DeleteLocalRef((jobject)m_obj);
	m_obj = 0;
      }
    }
    ZuInline operator T() const { return m_obj; }
    ZuInline bool operator !() const { return !m_obj; }
  private:
    JNIEnv	*m_env;
    T		m_obj;
  };
  template <typename T>
  ZuInline LocalRef<T> localRef(JNIEnv *env, T obj) {
    return LocalRef<T>(env, obj);
  }
  template <typename T> class GlobalRef {
    GlobalRef(const GlobalRef &r) = delete;
    GlobalRef &operator =(const GlobalRef &r) = delete;
  public:
    ZuInline GlobalRef() : m_obj(0) { }
    ZuInline GlobalRef(JNIEnv *env, T obj) :
      m_obj(env->NewGlobalRef(obj)) { }
    ZuInline ~GlobalRef() {
      if (m_obj) ZJNI::env()->DeleteGlobalRef((jobject)m_obj);
    }
    ZuInline GlobalRef(GlobalRef &&r) : m_obj(r.m_obj) {
      r.m_obj = 0;
    }
    ZuInline GlobalRef &operator =(GlobalRef &&r) {
      if (m_obj) ZJNI::env()->DeleteGlobalRef((jobject)m_obj);
      m_obj = r.m_obj;
      r.m_obj = 0;
      return *this;
    }
    ZuInline void null() {
      if (m_obj) {
	ZJNI::env()->DeleteGlobalRef((jobject)m_obj);
	m_obj = 0;
      }
    }
    ZuInline operator T() const { return m_obj; }
    ZuInline bool operator !() const { return !m_obj; }
  private:
    T		m_obj;
  };
  template <typename T>
  ZuInline GlobalRef<T> globalRef(JNIEnv *env, T obj) {
    return GlobalRef<T>(env, obj);
  }
  template <typename T>
  ZuInline GlobalRef<T> globalRef(JNIEnv *env, const LocalRef<T> &obj) {
    return GlobalRef<T>(env, (T)obj);
  }
  // return global ref to Java class
  inline jclass globalClassRef(JNIEnv *env, const char *cname) {
    jclass c = env->FindClass(cname);
    if (!c) return 0;
    jclass g = (jclass)env->NewGlobalRef((jobject)c);
    env->DeleteLocalRef((jobject)c);
    return g;
  }

  // C++ any string -> Java String
  inline jstring s2j(JNIEnv *env, ZuString s) {
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

  // Java String -> C++ any string
  template <typename Alloc, typename Buf, typename SetLen>
  inline auto j2s(JNIEnv *env, jstring s_,
      Alloc alloc, Buf buf, SetLen setLen) {
    if (ZuUnlikely(!s_)) return alloc(0);
    unsigned n = env->GetStringLength(s_);
    if (ZuUnlikely(!n)) return alloc(0);
    ZuArray<const jchar> s(env->GetStringCritical(s_, 0), n);
    auto o = alloc(ZuUTF<char, jchar>::len(s));
    setLen(o, ZuUTF<char, jchar>::cvt(buf(o), s));
    env->ReleaseStringCritical(s_, s.data()); // should be nop
    return o;
  }
  // Java String -> ZuStringN<N>
  template <unsigned N>
  ZuInline ZuStringN<N> j2s_ZuStringN(JNIEnv *env, jstring s) {
    return j2s(env, s,
	[](unsigned) { return ZuStringN<N>(); },
	[](ZuStringN<N> &s) { return ZuArray<char>(s.data(), N - 1); },
	[](ZuStringN<N> &s, unsigned n) { s.length(n); });
  }
  // Java String -> ZtString
  ZuInline ZtString j2s_ZtString(JNIEnv *env, jstring s) {
    return j2s(env, s,
	[](unsigned n) { return ZtString(n + 1); },
	[](ZtString &s) { return ZuArray<char>(s.data(), s.size() - 1); },
	[](ZtString &s, unsigned n) { s.length(n); });
  }

  // Java Instant -> ZtDate
  ZJNIExtern ZtDate j2t(JNIEnv *env, jobject obj);
  // ZtDate -> Java Instant
  ZJNIExtern jobject t2j(JNIEnv *env, const ZtDate &t);
};

#endif /* ZJNI_HPP */
