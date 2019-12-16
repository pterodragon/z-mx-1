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

// flatbuffers integration

#ifndef Zfb_HPP
#define Zfb_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZfbLib_HPP
#include <zlib/ZfbLib.hpp>
#endif

#include <flatbuffers/flatbuffers.h>

#include <stdlib.h>
#include <string.h>

#include <zlib/ZuDecimal.hpp>

#include <zlib/ZmBitmap.hpp>

#include <zlib/ZtDate.hpp>

#include <zlib/ZiIOBuf.hpp>

#include <zlib/types_fbs.h>

namespace Zfb {

using namespace flatbuffers;

using Builder = FlatBufferBuilder;

using IOBuf = ZiIOBuf;

// IOBuilder customizes FlatBufferBuilder with an allocator that
// builds directly into a detachable IOBuf for transmission
class IOBuilder : public Allocator, public Builder {
public:
  enum { Align = 8 };

  IOBuilder() : Builder(
      IOBuf::Size & ~(Align - 1), this, false, Align) { }

  ZmRef<IOBuf> buf() {
    if (ZuUnlikely(!m_buf)) return nullptr;
    auto buf = ZuMv(m_buf);
    size_t size, skip;
    ReleaseRaw(size, skip);
    buf->skip = skip;
    buf->length = size - skip;
    return buf;
  }

protected:
  uint8_t *allocate(size_t size) {
    if (ZuLikely(!m_buf)) m_buf = new IOBuf();
    return m_buf->alloc(size);
  }

  void deallocate(uint8_t *ptr, size_t size) {
    if (m_buf) m_buf->free(ptr);
  }

private:
  ZmRef<IOBuf>	m_buf;
};

namespace Save {
  // compile-time-recursive vector push
  template <typename Vector>
  inline void push_(Vector &) { }
  template <typename Vector, typename Arg0, typename ...Args>
  inline void push_(Vector &v, Arg0 &&arg0, Args &&... args) {
    v.push_back(ZuFwd<Arg0>(arg0));
    push_(v, ZuFwd<Args>(args)...);
  }

  // compile-time-recursive vector push, with a lambda element transformation
  template <typename Vector, typename L>
  inline void lpush_(Vector &, L &) { }
  template <typename Vector, typename L, typename Arg0, typename ...Args>
  inline void lpush_(Vector &v, L &l, Arg0 &&arg0, Args &&... args) {
    v.push_back(l(ZuFwd<Arg0>(arg0)));
    lpush_(v, l, ZuFwd<Args>(args)...);
  }

  // inline creation of a vector of primitive scalars
  template <typename T, typename B, typename ...Args>
  inline auto pvector(B &b, Args &&... args) {
    thread_local std::vector<T> v;
    v.clear();
    v.reserve(sizeof...(args));
    push_(v, ZuFwd<Args>(args)...);
    return b.CreateVector(v);
  }

  // inline creation of a vector of lambda-transformed non-primitive types
  template <typename T, typename B, typename L, typename ...Args>
  inline auto lvector(B &b, L l, Args &&... args) {
    thread_local std::vector<Offset<T> > v;
    v.clear();
    v.reserve(sizeof...(args));
    lpush_(v, l, ZuFwd<Args>(args)...);
    return b.CreateVector(v);
  }
  // inline creation of a vector of non-primitive types
  template <typename T, typename B, typename ...Args>
  inline auto vector(B &b, Args &&... args) {
    thread_local std::vector<Offset<T> > v;
    v.clear();
    v.reserve(sizeof...(args));
    push_(v, ZuFwd<Args>(args)...);
    return b.CreateVector(v);
  }
  // iterated creation of a vector of non-primitive types
  template <typename T, typename B, typename L>
  inline auto vectorIter(B &b, unsigned n, L l) {
    thread_local std::vector<Offset<T> > v;
    v.clear();
    v.reserve(n);
    for (unsigned i = 0; i < n; i++) v.push_back(l(b, i));
    return b.CreateVector(v);
  }

  // inline creation of a vector of keyed items
  template <typename T, typename B, typename ...Args>
  inline auto keyVec(B &b, Args &&... args) {
    thread_local std::vector<Offset<T> > v;
    v.reserve(sizeof...(args));
    push_(v, ZuFwd<Args>(args)...);
    return b.CreateVectorOfSortedTables(&v);
  }
  // iterated creation of a vector of keyed items
  template <typename T, typename B, typename L>
  inline auto keyVecIter(B &b, unsigned n, L l) {
    thread_local std::vector<Offset<T> > v;
    v.clear();
    v.reserve(n);
    for (unsigned i = 0; i < n; i++) v.push_back(l(b, i));
    return b.CreateVectorOfSortedTables(&v);
  }

  // inline creation of a string (shorthand alias for CreateString)
  template <typename B>
  ZuInline auto str(B &b, ZuString s) {
    return b.CreateString(s.data(), s.length());
  }

  // inline creation of a vector of strings
  template <typename B, typename ...Args>
  inline auto strVec(B &b, Args &&... args) {
    return lvector<String>(b, [&b](const auto &s) {
      return str(b, s);
    }, ZuFwd<Args>(args)...);
  }
  // iterated creation of a vector of strings
  template <typename B, typename L>
  inline auto strVecIter(B &b, unsigned n, L l) {
    return vectorIter<String>(b, n, [l = ZuMv(l)](B &b, unsigned i) mutable {
      return str(b, l(i));
    });
  }

  // inline creation of a vector of bytes from raw data
  template <typename B>
  ZuInline auto bytes(B &b, const void *data, unsigned len) {
    return b.CreateVector(static_cast<const uint8_t *>(data), len);
  }
  // inline creation of a vector of bytes from raw data as a ZuArray<uint8_t>
  template <typename B>
  ZuInline auto bytes(B &b, ZuArray<uint8_t> a) {
    return b.CreateVector(a.data(), a.length());
  }

  // decimal
  ZuInline auto decimal(const ZuDecimal &v) {
    return Decimal{(uint64_t)(v.value>>64), (uint64_t)v.value};
  }

  // bitmap
  template <typename B>
  ZuInline auto bitmap(B &b, const ZmBitmap &m) {
    thread_local std::vector<uint64_t> v;
    v.clear();
    int n = m.last();
    if (n > 0) {
      n = (n>>6) + 1;
      v.reserve(n);
      for (unsigned i = 0; i < n; i++)
	v.push_back(hwloc_bitmap_to_ith_ulong(m, i));
    }
    return b.CreateVector(v);
  }

  // date/time
  template <typename B>
  ZuInline auto dateTime(B &b, const ZtDate &v) {
    return CreateDateTime(b, v.julian(), v.sec(), v.nsec());
  }
}

namespace Load {
  // shorthand iteration over fastbuffer [T] vectors
  template <typename T, typename L>
  ZuInline void all(T *v, L l) {
    for (unsigned i = 0, n = v->size(); i < n; i++) l(i, v->Get(i));
  }

  // inline zero-copy conversion of a FB string to a ZuString
  ZuInline ZuString str(const flatbuffers::String *s) {
    if (!s) return ZuString();
    return ZuString{reinterpret_cast<const char *>(s->Data()), s->size()};
  }

  // inline zero-copy conversion of a [uint8] to a ZuArray<uint8_t>
  ZuInline ZuArray<const uint8_t> bytes(const Vector<uint8_t> *v) {
    if (!v) return ZuArray<const uint8_t>{};
    return ZuArray<const uint8_t>(v->data(), v->size());
  }

  // decimal
  ZuInline ZuDecimal decimal(const Decimal *v) {
    return ZuDecimal{ZuDecimal::Unscaled, (((int128_t)(v->h()))<<64) | v->l()};
  }

  // bitmap
  ZuInline ZmBitmap bitmap(const Vector<uint64_t> *v) {
    if (!v) return ZmBitmap{};
    ZmBitmap m;
    if (unsigned n = v->size()) {
      --n;
      hwloc_bitmap_from_ith_ulong(m, n, (*v)[n]);
      while (n) {
	--n;
	hwloc_bitmap_set_ith_ulong(m, n, (*v)[n]);
      }
    }
    return m;
  }

  // date/time
  ZuInline ZtDate dateTime(const DateTime *v) {
    return ZtDate{ZtDate::Julian, v->julian(), v->sec(), v->nsec()};
  }
}

} // Zfb

#define ZfbEnum_Value(Enum, Value) Value = fbs::Enum##_##Value,
#define ZfbEnumValues(Enum, ...) \
  enum _ { Invalid = -1, \
    ZuPP_Eval(ZuPP_MapArg(ZfbEnum_Value, Enum, __VA_ARGS__)) \
    N = fbs::Enum##_MAX + 1 }; \
  ZuAssert(N <= 1024); \
  enum { Bits = \
    N <= 2 ? 1 : N <= 4 ? 2 : N <= 8 ? 3 : N <= 16 ? 4 : N <= 32 ? 5 : \
    N <= 64 ? 6 : N <= 128 ? 7 : N <= 256 ? 8 : N <= 512 ? 9 : 10 \
  }; \
  template <typename T> struct Map_ : public ZuObject { \
  private: \
    typedef ZmLHash<ZuString, \
	      ZmLHashVal<ZtEnum, \
		ZmLHashStatic<Bits, \
		  ZmLHashLock<ZmNoLock> > > > S2V; \
  protected: \
    void init(const char *s, int v, ...) { \
      if (ZuUnlikely(!s)) return; \
      add(s, v); \
      va_list args; \
      va_start(args, v); \
      while (s = va_arg(args, const char *)) \
	add(s, v = va_arg(args, int)); \
      va_end(args); \
    } \
    void add(ZuString s, ZtEnum v) { m_s2v->add(s, v); } \
  public: \
    Map_() { m_s2v = new S2V(); } \
    static T *instance() { return ZmSingleton<T>::instance(); } \
    static ZuString v2s(int v) { \
      return fbs::EnumName##Enum(static_cast<fbs::Enum>(v)); \
    } \
    ZtEnum s2v(ZuString s) const { return m_s2v->findVal(s); } \
    template <typename L> void all(L l) const { \
      auto i = m_s2v->readIterator(); \
      for (;;) { \
	auto kv = i.iterate(); \
	if (!kv.template p<0>()) break; \
	l(kv.template p<0>(), kv.template p<1>()); \
      } \
    } \
  private: \
    ZmRef<S2V>	m_s2v; \
  }; \
  const char *name(int i) { \
    return fbs::EnumName##Enum(static_cast<fbs::Enum>(i)); \
  } \
  struct Map : public Map_<Map> { \
    Map() { for (unsigned i = 0; i < N; i++) this->add(name(i), i); } \
  }; \
  template <typename S> ZtEnum lookup(const S &s) { \
    return Map::instance()->s2v(s); \
  }

#endif /* Zfb_HPP */
