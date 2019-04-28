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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// dynamic bitmap optimized for an incrementing sliding window of values

#ifndef ZtBitWindow_HPP
#define ZtBitWindow_HPP

#ifdef _MSC_VER
#pragma once
#endif

#ifndef ZtLib_HPP
#include <ZtLib.hpp>
#endif

#include <stdlib.h>

#include <ZuInt.hpp>

template <unsigned> struct ZtBitWindow_ { enum { OK = 0 }; };
template <> struct ZtBitWindow_<1> { enum { OK = 1, Shift = 0 }; };
template <> struct ZtBitWindow_<2> { enum { OK = 1, Shift = 1 }; };
template <> struct ZtBitWindow_<3> { enum { OK = 1 }; };
template <> struct ZtBitWindow_<4> { enum { OK = 1, Shift = 2 }; };
template <> struct ZtBitWindow_<8> { enum { OK = 1, Shift = 3 }; };
template <> struct ZtBitWindow_<16> { enum { OK = 1, Shift = 4 }; };
template <> struct ZtBitWindow_<32> { enum { OK = 1, Shift = 5 }; };

template <unsigned Bits_ = 1, bool = ZtBitWindow_<Bits_>::OK>
class ZtBitWindow;

template <unsigned Bits_> class ZtBitWindow<Bits_, true> {
  ZtBitWindow(const ZtBitWindow &) = delete;
  ZtBitWindow &operator =(const ZtBitWindow &) = delete;

public:
  enum { Bits = Bits_ };

private:
  enum { Shift = ZtBitWindow_<Bits>::Shift };
  enum { Mask = (1<<Bits) - 1 };
  enum { IndexShift = (6 - Shift) };
  enum { IndexMask = (1<<IndexShift) - 1 };

public:
  inline ZtBitWindow() : m_data(nullptr), m_size(0), m_head(0) { }
  inline ~ZtBitWindow() { if (m_data) ::free(m_data); }

  inline ZtBitWindow(ZtBitWindow &&q) :
      m_data(q.m_data), m_size(q.m_size), m_head(q.m_head) {
    q.m_data = nullptr;
    q.m_size = 0;
    q.m_head = 0;
  }
  inline ZtBitWindow &operator =(ZtBitWindow &&q) {
    if (m_data) ::free(m_data);
    m_data = q.m_data;
    m_size = q.m_size;
    m_head = q.m_head;
    q.m_data = nullptr;
    q.m_size = 0;
    q.m_head = 0;
    return *this;
  }

private:
  inline uint64_t ensure(uint64_t i) {
    if (ZuUnlikely(i < m_head)) {
      uint64_t j = ((m_head - i) + IndexMask)>>IndexShift;
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      memset(data, 0, j<<3);
      memcpy(data + j, m_data, m_size<<3);
      ::free(m_data);
      m_data = data;
      m_size += j;
      m_head -= (j<<IndexShift);
      i -= m_head;
    } else {
      i -= m_head;
      if (ZuUnlikely(i >= (m_size<<IndexShift))) {
	uint64_t j = (((i + 1) - (m_size<<IndexShift)) + IndexMask)>>IndexShift;
	if (j < (m_size>>3)) j = m_size>>3; // grow by at least 12.5%
	uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
	memcpy(data, m_data, m_size<<3);
	memset(data + m_size, 0, j<<3);
	::free(m_data);
	m_data = data;
	m_size += j;
      }
    }
    return i;
  }

public:
  inline void set(uint64_t i) {
    i = ensure(i);
    m_data[i>>IndexShift] |= (((uint64_t)Mask)<<((i & IndexMask)<<Shift));
  }
  inline void set(uint64_t i, uint64_t v) {
    i = ensure(i);
    m_data[i>>IndexShift] |= (v<<((i & IndexMask)<<Shift));
  }
  inline void clr(uint64_t i) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= (m_size<<IndexShift)) return;
    m_data[i>>IndexShift] &= ~(((uint64_t)Mask)<<((i & IndexMask)<<Shift));
    if (i < (1U<<IndexShift)) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[j]; j++);
      if (j) {
	if (j < m_size) memmove(m_data, m_data + j, ((m_size - j)<<3));
	m_head += (j<<IndexShift);
      }
    }
  }

  inline unsigned val(unsigned i) const {
    if (i < m_head) return 0;
    i -= m_head;
    if (i >= (m_size<<IndexShift)) return 0;
    return (m_data[i>>IndexShift]>>((i & IndexMask)<<Shift)) & Mask;
  }

  inline unsigned head() const { return m_head; }
  inline unsigned tail() const { return m_head + (m_size<<IndexShift); }
  inline unsigned size() const { return m_size<<IndexShift; }

  // l(unsigned i, unsigned v) -> uintptr_t
  template <typename L>
  inline uintptr_t all(L l) {
    for (unsigned i = 0, n = m_size; i < n; i++) {
      uint64_t w = m_data[i];
      uint64_t m = Mask;
      for (unsigned j = 0; j < (1U<<IndexShift); j++) {
	if (uint64_t v = (w & m))
	  if (uintptr_t v = l(m_head + (i<<IndexShift) + j, v>>(j<<Shift)))
	    return v;
	m <<= (1U<<Shift);
      }
    }
    return 0;
  }

private:
  uint64_t	*m_data;
  uint64_t	m_size;
  uint64_t	m_head;
};

// optimized for 3 bits per value, 21 values per 64bit word
template <> class ZtBitWindow<3> {
  ZtBitWindow(const ZtBitWindow &) = delete;
  ZtBitWindow &operator =(const ZtBitWindow &) = delete;

public:
  enum { Bits = 3 };

  inline ZtBitWindow() : m_data(nullptr), m_size(0), m_head(0) { }
  inline ~ZtBitWindow() { if (m_data) ::free(m_data); }

  inline ZtBitWindow(ZtBitWindow &&q) :
      m_data(q.m_data), m_size(q.m_size), m_head(q.m_head) {
    q.m_data = nullptr;
    q.m_size = 0;
    q.m_head = 0;
  }
  inline ZtBitWindow &operator =(ZtBitWindow &&q) {
    if (m_data) ::free(m_data);
    m_data = q.m_data;
    m_size = q.m_size;
    m_head = q.m_head;
    q.m_data = nullptr;
    q.m_size = 0;
    q.m_head = 0;
    return *this;
  }

private:
  inline uint64_t ensure(uint64_t i) {
    if (ZuUnlikely(i < m_head)) {
      uint64_t j = ((m_head - i) + 20) / 21;
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      memset(data, 0, j<<3);
      memcpy(data + j, m_data, m_size<<3);
      ::free(m_data);
      m_data = data;
      m_size += j;
      m_head -= j * 21;
      i -= m_head;
    } else {
      i -= m_head;
      uint64_t size = m_size * 21;
      if (ZuUnlikely(i >= size)) {
	uint64_t j = (((i + 1) - size) + 20) / 21;
	if (j < (m_size>>3)) j = m_size>>3; // grow by at least 12.5%
	uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
	memcpy(data, m_data, m_size<<3);
	memset(data + m_size, 0, j<<3);
	::free(m_data);
	m_data = data;
	m_size += j;
      }
    }
    return i;
  }

public:
  inline void set(uint64_t i) {
    i = ensure(i);
    m_data[i / 21] |= (((uint64_t)7)<<((i % 21) * 3));
  }
  inline void set(uint64_t i, uint64_t v) {
    i = ensure(i);
    m_data[i / 21] |= (v<<((i % 21) * 3));
  }
  inline void clr(uint64_t i) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= m_size * 21) return;
    m_data[i / 21] &= ~(((uint64_t)7)<<((i % 21) * 3));
    if (i < 21) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[j]; j++);
      if (j) {
	if (j < m_size) memmove(m_data, m_data + j, ((m_size - j)<<3));
	m_head += j * 21;
      }
    }
  }

  inline unsigned val(unsigned i) const {
    if (i < m_head) return 0;
    i -= m_head;
    if (i >= m_size * 21) return 0;
    return (m_data[i / 21]>>((i % 21) * 3)) & 7;
  }

  inline unsigned head() const { return m_head; }
  inline unsigned tail() const { return m_head + m_size * 21; }
  inline unsigned size() const { return m_size * 21; }

  // l(unsigned i, unsigned v) -> uintptr_t
  template <typename L>
  inline uintptr_t all(L l) {
    for (unsigned i = 0, k = 0, n = m_size; i < n; i++) {
      uint64_t w = m_data[i];
      uint64_t m = 7;
      for (unsigned j = 0, z = 0; j < 21; j++, k++, z += 3) {
	if (uint64_t v = (w & m))
	  if (uintptr_t r = l(m_head + k, v>>z))
	    return r;
	m <<= 3;
      }
    }
    return 0;
  }

private:
  uint64_t	*m_data;
  uint64_t	m_size;
  uint64_t	m_head;
};

#endif /* ZtBitWindow_HPP */
