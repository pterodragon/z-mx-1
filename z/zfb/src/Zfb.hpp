//  -*- mode:c++; indent-tabs-mode:t; tab-width:8; c-basic-offset:2; -*-
//  vi: noet ts=8 sw=2 cino=l1,g0,N-s,j1,U1,i4

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

#include <zlib/ZePlatform.hpp>

#include <zlib/ZiPlatform.hpp>
#include <zlib/ZiIOBuf.hpp>

#include <zlib/types_fbs.h>

namespace Zfb {

using namespace flatbuffers;

using Builder = FlatBufferBuilder;

using IOBuf = ZiIOBuf<>;

// IOBuilder customizes FlatBufferBuilder with an allocator that
// builds directly into a detachable IOBuf for transmission/persistence
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
    Clear();
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

  uint8_t *reallocate_downward(
      uint8_t *old_p, size_t old_size, size_t new_size,
      size_t in_use_back, size_t in_use_front) {
    return m_buf->realloc(old_size, new_size, in_use_front, in_use_back);
  }

private:
  ZmRef<IOBuf>	m_buf;
};

namespace Save {
  // compile-time-recursive vector push
  template <typename T, typename I>
  inline void push_(T *, I) { }
  template <typename T, typename I, typename Arg0, typename ...Args>
  inline void push_(T *buf, I i, Arg0 &&arg0, Args &&... args) {
    buf[i] = ZuFwd<Arg0>(arg0);
    push_(buf, ZuConstant<i + 1>{}, ZuFwd<Args>(args)...);
  }

  // compile-time-recursive vector push, with a lambda map function
  template <typename T, typename I, typename L>
  inline void lpush_(T *, I, L &) { }
  template <typename T, typename I, typename L, typename Arg0, typename ...Args>
  inline void lpush_(T *buf, I i, L &l, Arg0 &&arg0, Args &&... args) {
    buf[i] = l(ZuFwd<Arg0>(arg0));
    lpush_(buf, ZuConstant<i + 1>{}, l, ZuFwd<Args>(args)...);
  }

  // inline creation of a vector of primitive scalars
  template <typename T, typename B, typename ...Args>
  inline Offset<Vector<T>> pvector(B &b, Args &&... args) {
    auto n = ZuConstant<sizeof...(args)>{};
    T *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    push_(buf, ZuConstant<0>{}, ZuFwd<Args>(args)...);
    return b.CreateVector(buf, n);
  }

  // inline creation of a vector of lambda-transformed non-primitive values
  template <typename T, typename B, typename L, typename ...Args>
  inline Offset<Vector<Offset<T>>> lvector(B &b, L l, Args &&... args) {
    auto n = ZuConstant<sizeof...(args)>{};
    Offset<T> *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    lpush_(buf, ZuConstant<0>{}, l, ZuFwd<Args>(args)...);
    return b.CreateVector(buf, n);
  }
  // inline creation of a vector of non-primitive values
  template <typename T, typename B, typename ...Args>
  inline Offset<Vector<Offset<T>>> vector(B &b, Args &&... args) {
    auto n = ZuConstant<sizeof...(args)>{};
    Offset<T> *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    push_(buf, ZuConstant<0>{}, ZuFwd<Args>(args)...);
    return b.CreateVector(buf, n);
  }
  // iterated creation of a vector of non-primitive values
  template <typename T, typename B, typename L>
  inline Offset<Vector<Offset<T>>> vectorIter(B &b, unsigned n, L l) {
    Offset<T> *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    for (unsigned i = 0; i < n; i++) buf[i] = l(b, i);
    return b.CreateVector(buf, n);
  }

  // inline creation of a vector of keyed items
  template <typename T, typename B, typename ...Args>
  inline Offset<Vector<Offset<T>>> keyVec(B &b, Args &&... args) {
    auto n = ZuConstant<sizeof...(args)>{};
    Offset<T> *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    push_(buf, ZuConstant<0>{}, ZuFwd<Args>(args)...);
    return b.CreateVectorOfSortedTables(buf, n);
  }
  // iterated creation of a vector of lambda-transformed keyed items
  template <typename T, typename B, typename L>
  inline Offset<Vector<Offset<T>>> keyVecIter(B &b, unsigned n, L l) {
    Offset<T> *buf = ZuAlloca(buf, n);
    if (ZuUnlikely(!buf)) return {};
    for (unsigned i = 0; i < n; i++) buf[i] = l(b, i);
    return b.CreateVectorOfSortedTables(buf, n);
  }

  // inline creation of a string (shorthand alias for CreateString)
  template <typename B>
  inline auto str(B &b, ZuString s) {
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
  inline auto bytes(B &b, const void *data, unsigned len) {
    return b.CreateVector(static_cast<const uint8_t *>(data), len);
  }
  // inline creation of a vector of bytes from raw data
  template <typename B>
  inline auto bytes(B &b, ZuArray<const uint8_t> a) {
    return b.CreateVector(a.data(), a.length());
  }

  // decimal
  inline auto decimal(const ZuDecimal &v) {
    return Decimal{(uint64_t)(v.value>>64), (uint64_t)v.value};
  }

  // bitmap
  template <typename B>
  inline auto bitmap(B &b, const ZmBitmap &m) {
    thread_local std::vector<uint64_t> v;
    v.clear();
    int n = m.last();
    if (n > 0) {
      n = (n>>6) + 1;
      v.reserve(n);
      for (unsigned i = 0; i < (unsigned)n; i++)
	v.push_back(hwloc_bitmap_to_ith_ulong(m, i));
    }
    return b.CreateVector(v);
  }

  // date/time
  template <typename B>
  inline auto dateTime(B &b, const ZtDate &v) {
    return CreateDateTime(b, v.julian(), v.sec(), v.nsec());
  }

  // save file
  ZfbExtern int save(
      const ZiPlatform::Path &path, Builder &fbb, unsigned mode, ZeError *e);
}

namespace Load {
  // shorthand iteration over fastbuffer [T] vectors
  template <typename T, typename L>
  inline void all(T *v, L l) {
    for (unsigned i = 0, n = v->size(); i < n; i++) l(i, v->Get(i));
  }

  // inline zero-copy conversion of a FB string to a ZuString
  inline ZuString str(const flatbuffers::String *s) {
    if (!s) return ZuString{};
    return ZuString{reinterpret_cast<const char *>(s->Data()), s->size()};
  }

  // inline zero-copy conversion of a [uint8] to a ZuArray<const uint8_t>
  inline ZuArray<const uint8_t> bytes(const Vector<uint8_t> *v) {
    if (!v) return ZuArray<const uint8_t>{};
    return ZuArray<const uint8_t>{v->data(), v->size()};
  }

  // decimal
  inline ZuDecimal decimal(const Decimal *v) {
    return ZuDecimal{ZuDecimal::Unscaled,
      (static_cast<int128_t>(v->h())<<64) | v->l()};
  }

  // bitmap
  inline ZmBitmap bitmap(const Vector<uint64_t> *v) {
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
  inline ZtDate dateTime(const DateTime *v) {
    return ZtDate{ZtDate::Julian, v->julian(), v->sec(), v->nsec()};
  }

  // load file
  using LoadFn = ZmFn<const uint8_t *, unsigned>;
  ZfbExtern int load(
      const ZiPlatform::Path &path, LoadFn, unsigned maxSize, ZeError *e);
}

} // Zfb

#define ZfbEnum_Value(Enum, Value) Value = fbs::Enum##_##Value,
#define ZfbEnumValues(Enum, ...) \
  enum _ { Invalid = -1, \
    ZuPP_Eval(ZuPP_MapArg(ZfbEnum_Value, Enum, \
	  __VA_OPT__(__VA_ARGS__ ,) MIN, MAX)) \
    N = fbs::Enum##_MAX + 1 }; \
  ZuAssert(N <= 1024); \
  enum { Bits = \
    N <= 2 ? 1 : N <= 4 ? 2 : N <= 8 ? 3 : N <= 16 ? 4 : N <= 32 ? 5 : \
    N <= 64 ? 6 : N <= 128 ? 7 : N <= 256 ? 8 : N <= 512 ? 9 : 10 \
  }; \
  template <typename T> struct Map_ : public ZuObject { \
  private: \
    using S2V = \
      ZmLHash<ZuString, \
	ZmLHashVal<ZtEnum, \
	  ZmLHashStatic<Bits, \
	    ZmLHashLock<ZmNoLock> > > >; \
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
      while (auto kv = i.iterate()) { l(kv->key(), kv->val()); } \
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
#define ZfbEnum_Type(T) fbs::T
#define ZfbEnum_Assert(T) \
  ZuAssert(static_cast<int>(T) == static_cast<int>(TypeIndex<fbs::T>::I));
#define ZfbEnumUnion(Enum, First_, ...) \
  ZfbEnumValues(Enum, First_, __VA_ARGS__) \
  enum { First = First_ }; \
  using TypeList = ZuTypeList< \
    ZuPP_Eval(ZuPP_MapComma(ZfbEnum_Type, First_, __VA_ARGS__))>; \
  template <unsigned I> \
  using Type = typename ZuType<I - First, TypeList>::T; \
  template <typename T> \
  struct TypeIndex { enum { I = ZuTypeIndex<T, TypeList>::I + First }; }; \
  ZuPP_Eval(ZuPP_Map(ZfbEnum_Assert, First_, __VA_ARGS__)) \
  ZuAssert( \
    static_cast<int>(First) == static_cast<int>(TypeIndex<Type<First>>::I)); \
  ZuAssert(static_cast<int>(MAX) == static_cast<int>(TypeIndex<Type<MAX>>::I));

#endif /* Zfb_HPP */
