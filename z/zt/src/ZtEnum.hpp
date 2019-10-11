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

// enum wrapper

#ifndef ZtEnum_HPP
#define ZtEnum_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <zlib/ZtLib.hpp>
#endif

#include <zlib/ZuBox.hpp>
#include <zlib/ZuPair.hpp>
#include <zlib/ZuAssert.hpp>
#include <zlib/ZuObject.hpp>

#include <zlib/ZmObject.hpp>
#include <zlib/ZmRef.hpp>
#include <zlib/ZmLHash.hpp>
#include <zlib/ZmNoLock.hpp>

typedef ZuBox_1(int8_t) ZtEnum;

// ZtEnum class declaration macros
//   Note: must use in this order: Values; Names; Map; Flags;...
#define ZtEnumValues(...) \
  enum _ { Invalid = -1, __VA_ARGS__, N }; \
  ZuAssert(N <= 1024); \
  enum { Bits = \
    N <= 2 ? 1 : N <= 4 ? 2 : N <= 8 ? 3 : N <= 16 ? 4 : N <= 32 ? 5 : \
    N <= 64 ? 6 : N <= 128 ? 7 : N <= 256 ? 8 : N <= 512 ? 9 : 10 \
  }; \
  template <typename T> struct Map_ : public ZuObject { \
  protected: \
    typedef ZmLHash<ZtEnum, \
	      ZmLHashVal<ZuString, \
		ZmLHashStatic<Bits, \
		  ZmLHashLock<ZmNoLock> > > > V2S; \
    typedef ZmLHash<ZuString, \
	      ZmLHashVal<ZtEnum, \
		ZmLHashStatic<Bits, \
		  ZmLHashLock<ZmNoLock> > > > S2V; \
    void init(const char *s, int v, ...) { \
      if (ZuUnlikely(!s)) return; \
      add(s, v); \
      va_list args; \
      va_start(args, v); \
      while (s = va_arg(args, const char *)) \
	add(s, v = va_arg(args, int)); \
      va_end(args); \
    } \
    inline void add(ZuString s, ZtEnum v) \
      { m_s2v->add(s, v); m_v2s->add(v, s); } \
  public: \
    inline Map_() { m_s2v = new S2V(); m_v2s = new V2S(); } \
    inline static T *instance() { return ZmSingleton<T>::instance(); } \
    inline ZtEnum s2v(ZuString s) const { return m_s2v->findVal(s); } \
    inline ZuString v2s(ZtEnum v) const { return m_v2s->findVal(v); } \
    template <typename L> inline void all(L l) const { \
      auto i = m_s2v->readIterator(); \
      for (;;) { \
	auto kv = i.iterate(); \
	if (!kv.p1()) break; \
	l(kv.p1(), kv.p2()); \
      } \
    } \
  protected: \
    ZmRef<S2V>	m_s2v; \
    ZmRef<V2S>	m_v2s; \
  }
#define ZtEnumNames(...) \
  inline ZuPair<const char *const *const, unsigned> names() { \
    static const char *names[] = { __VA_ARGS__ }; \
    return ZuPair<const char *const *const, unsigned>( \
	names, sizeof(names) / sizeof(names[0])); \
  } \
  inline const char *name(int i) { \
    ZuPair<const char *const *const, unsigned> names_ = names(); \
    if (i >= (int)names_.p2()) return "Unknown"; \
    if (i < 0) return ""; \
    return names_.p1()[i]; \
  } \
  struct Map : public Map_<Map> { \
    Map() { for (unsigned i = 0; i < N; i++) this->add(name(i), i); } \
  }; \
  template <typename S> inline ZtEnum lookup(const S &s) { \
    return Map::instance()->s2v(s); \
  }
#define ZtEnumMap(Map, ...) \
  struct Map : public Map_<Map> { \
    Map() { this->init(__VA_ARGS__, (const char *)0); } \
  }
#define ZtEnumFlags(Map, ...) \
  struct Map : public Map_<Map> { \
    Map() { this->init(__VA_ARGS__, (const char *)0); } \
    template <typename S, typename Flags_> \
    inline unsigned print(S &s, const Flags_ &v, char delim = '|') const { \
      if (!v) return 0; \
      bool first = true; \
      unsigned n = 0; \
      for (unsigned i = 0; i < N; i++) { \
	if (v & (((Flags_)1U)<<i)) { \
	  if (ZuString s_ = this->v2s(i)) { \
	    if (!first) s << delim; \
	    s << s_; \
	    first = false; \
	    n++; \
	  } \
	} \
      } \
      return n; \
    } \
    template <typename Flags_> \
    inline Flags_ scan(ZuString s, char delim = '|') const { \
      if (!s) return 0; \
      Flags_ v = 0; \
      bool end = false; \
      unsigned len = 0, clen = 0; \
      const char *cstr = s.data(), *next; \
      char c = 0; \
      do { \
	for (next = cstr; \
	    ++len <= s.length() && (c = *next) != 0 && c != delim; \
	    clen++, next++); \
	if (len > s.length() || c == 0) end = true; \
	ZtEnum i = this->s2v(ZuString(cstr, clen)); \
	if (ZuUnlikely(!*i)) return 0; \
	v |= (((Flags_)1U)<<(unsigned)i); \
	cstr = ++next; \
	clen = 0; \
      } while (!end); \
      return v; \
    } \
    template <typename Flags_> \
    struct Print : public ZuPrintable { \
      inline Print(const Flags_ &v_, char delim_ = '|') : \
	v(v_), delim(delim_) { } \
      template <typename S> inline void print(S &s) const { \
	ZmSingleton<Map>::instance()->print(s, v, delim); \
      } \
      const Flags_	&v; \
      char		delim; \
    }; \
    template <typename Flags_> \
    inline static Print<Flags_> print(const Flags_ &v) { \
      return Print<Flags_>(v); \
    } \
  };

#endif /* ZtEnum_HPP */
