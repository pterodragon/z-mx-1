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
#include <ZfbLib.hpp>
#endif

#include <flatbuffers/flatbuffers.h>

#include <stdlib.h>
#include <string.h>

#include <ZiIOBuf.hpp>

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
      return str(b, l(b, i));
    });
  }

  // inline creation of a vector of bytes from raw data
  template <typename B>
  inline auto bytes(B &b, const void *data, unsigned len) {
    return b.CreateVector(static_cast<const uint8_t *>(data), len);
  }
  // inline creation of a vector of bytes from raw data as a ZuArray<uint8_t>
  template <typename B>
  inline auto bytes(B &b, ZuArray<uint8_t> a) {
    return b.CreateVector(a.data(), a.length());
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
    if (!v) return ZuArray<const uint8_t>();
    return ZuArray<const uint8_t>(v->data(), v->size());
  }
}

} // Zfb

#endif /* Zfb_HPP */
