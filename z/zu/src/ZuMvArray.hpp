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

// heap-allocated move-only array

#ifndef ZuMvArray_HPP
#define ZuMvArray_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuArray.hpp>
#include <zlib/ZuArrayFn.hpp>

template <typename T_> class ZuMvArray : public ZuArrayFn<T_> {
public:
  using T = T_;

  ZuMvArray(const ZuMvArray &) = delete;
  ZuMvArray &operator =(const ZuMvArray &) = delete;

  ZuMvArray() = default;

  inline ZuMvArray(unsigned n) :
      m_length{n}, m_data{!n ? (T *)nullptr : (T *)::malloc(n * sizeof(T))} {
    if (ZuUnlikely(n && !m_data)) throw std::bad_alloc();
    this->initItems(m_data, n);
  }
  inline ZuMvArray(T *data, unsigned n) :
      m_length{n}, m_data{!n ? (T *)nullptr : (T *)::malloc(n * sizeof(T))} {
    if (ZuUnlikely(n && !m_data)) throw std::bad_alloc();
    this->moveItems(m_data, data, n);
  }
  inline ~ZuMvArray() {
    if (!m_data) return;
    this->destroyItems(m_data, m_length);
    ::free(m_data);
  }

  inline ZuMvArray(ZuMvArray &&a) {
    m_length = a.m_length;
    m_data = a.m_data;
    a.m_length = 0;
    a.m_data = nullptr;
  }
  inline ZuMvArray &operator =(ZuMvArray &&a) {
    m_length = a.m_length;
    m_data = a.m_data;
    a.m_length = 0;
    a.m_data = nullptr;
    return *this;
  }

// array/ptr operators

  ZuInline T &operator [](int i) { return m_data[i]; }
  ZuInline const T &operator [](int i) const { return m_data[i]; }

// accessors

  ZuInline unsigned length() const { return m_length; } 
  ZuInline T *data() const { return m_data; }

// reset to null

  ZuInline void null() { delete [] m_data; m_length = 0;  m_data = nullptr;}

// set length

  inline void length(unsigned newLength) {
    T *oldData = m_data;
    unsigned oldLength = m_length;
    m_length = newLength;
    m_data = (T *)::malloc(newLength * sizeof(T));
    unsigned mvLength = oldLength < newLength ? oldLength : newLength;
    unsigned initLength = newLength - mvLength;
    if (mvLength) this->moveItems(m_data, oldData, mvLength);
    if (initLength) this->initItems(m_data + mvLength, initLength);
    ::free(oldData);
  }

// comparison

  ZuInline bool operator !() const { return !m_length; }
  ZuOpBool

protected:
  ZuInline bool same(const ZuMvArray &a) const { return this == &a; }
  template <typename A> ZuInline bool same(const A &a) const { return false; }

public:
  template <typename A>
  ZuInline int cmp(const A &a) const {
    if (same(a)) return 0;
    ZuArray self{data(), length()};
    return self.cmp(a);
  }

  template <typename A>
  ZuInline bool equals(const A &a) const {
    if (same(a)) return true;
    ZuArray self{data(), length()};
    return self.equals(a);
  }

  template <typename A>
  ZuInline bool operator ==(const A &a) const { return equals(a); }
  template <typename A>
  ZuInline bool operator !=(const A &a) const { return !equals(a); }
  template <typename A>
  ZuInline bool operator >(const A &a) const { return cmp(a) > 0; }
  template <typename A>
  ZuInline bool operator >=(const A &a) const { return cmp(a) >= 0; }
  template <typename A>
  ZuInline bool operator <(const A &a) const { return cmp(a) < 0; }
  template <typename A>
  ZuInline bool operator <=(const A &a) const { return cmp(a) <= 0; }

// hash

  ZuInline uint32_t hash() const {
    return ZuHash<ZuMvArray>::hash(*static_cast<const ZuMvArray *>(this));
  }

private:
  uint32_t	m_length = 0;
  T		*m_data = nullptr;
};
template <typename T_>
struct ZuTraits<ZuMvArray<T_> > :
    public ZuGenericTraits<ZuMvArray<T_> > {
  typedef ZuMvArray<T_> T;
  typedef T_ Elem;
  enum {
    IsArray = 1, IsPrimitive = 0, IsPOD = 0,
    IsString =
      ZuConversion<char, Elem>::Same ||
      ZuConversion<wchar_t, Elem>::Same,
    IsWString = ZuConversion<wchar_t, Elem>::Same,
    IsHashable = 1, IsComparable = 1
  };
#if 0
  inline static T make(const T *data, unsigned length) {
    if (!data) return T();
    return T(data, length);
  }
#endif
  inline static const Elem *data(const T &a) { return a.data(); }
  inline static unsigned length(const T &a) { return a.length(); }
};

// generic printing
template <>
struct ZuPrint<ZuMvArray<char> > : public ZuPrintString { };

#endif /* ZuMvArray_HPP */
