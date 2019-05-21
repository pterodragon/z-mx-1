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
#include <string.h>

#include <ZuInt.hpp>

template <unsigned> struct ZtBitWindow_ { enum { OK = 0 }; };
template <> struct ZtBitWindow_<1>  { enum { OK = 1, Pow2 = 1, Shift =  0 }; };
template <> struct ZtBitWindow_<2>  { enum { OK = 1, Pow2 = 1, Shift =  1 }; };
template <> struct ZtBitWindow_<3>  { enum { OK = 1, Pow2 = 0,   Mul = 21 }; };
template <> struct ZtBitWindow_<4>  { enum { OK = 1, Pow2 = 1, Shift =  2 }; };
template <> struct ZtBitWindow_<5>  { enum { OK = 1, Pow2 = 0,   Mul = 12 }; };
template <> struct ZtBitWindow_<8>  { enum { OK = 1, Pow2 = 1, Shift =  3 }; };
template <> struct ZtBitWindow_<16> { enum { OK = 1, Pow2 = 1, Shift =  4 }; };
template <> struct ZtBitWindow_<32> { enum { OK = 1, Pow2 = 1, Shift =  5 }; };
template <> struct ZtBitWindow_<64> { enum { OK = 1, Pow2 = 1, Shift =  6 }; };

template <unsigned Bits = 1,
	 bool = ZtBitWindow_<Bits>::OK,
	 bool = ZtBitWindow_<Bits>::Pow2>
class ZtBitWindow;

template <unsigned Bits_> class ZtBitWindow<Bits_, true, true> {
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
  inline ZtBitWindow() : m_data(nullptr), m_size(0), m_head(0), m_offset(0) { }
  inline ~ZtBitWindow() { if (m_data) ::free(m_data); }

  inline ZtBitWindow(ZtBitWindow &&q) :
      m_data(q.m_data), m_size(q.m_size),
      m_head(q.m_head), m_offset(q.m_offset) {
    q.null();
  }
  inline ZtBitWindow &operator =(ZtBitWindow &&q) {
    if (m_data) ::free(m_data);
    m_data = q.m_data;
    m_size = q.m_size;
    m_head = q.m_head;
    q.null();
    return *this;
  }

  inline void null() {
    if (m_data) ::free(m_data);
    m_data = nullptr;
    m_size = 0;
    m_head = 0;
    m_offset = 0;
  }

private:
  inline uint64_t ensure(uint64_t i) {
    if (ZuUnlikely(i < m_head)) {
      uint64_t j = ((m_head - i) + IndexMask)>>IndexShift;
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      memset(data, 0, j<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data + j, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + j + tailOffset, m_data, m_offset<<3);
      if (m_data) ::free(m_data);
      m_data = data;
      m_size += j;
      m_head -= (j<<IndexShift);
      m_offset = 0;
      return (i - m_head)>>IndexShift;
    }
    i -= m_head;
    if (ZuUnlikely(i >= (m_size<<IndexShift))) {
      uint64_t j = (((i + 1) - (m_size<<IndexShift)) + IndexMask)>>IndexShift;
      if (j < (m_size>>3)) j = m_size>>3; // grow by at least 12.5%
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + tailOffset, m_data, m_offset<<3);
      memset(data + m_size, 0, j<<3);
      if (m_data) ::free(m_data);
      m_data = data;
      m_size += j;
      m_offset = 0;
      return i>>IndexShift;
    }
    return index(i>>IndexShift);
  }
  ZuInline uint64_t index(uint64_t i) const {
    i += m_offset;
    if (i >= m_size) i -= m_size;
    return i;
  }

public:
  inline void set(uint64_t i) {
    uint64_t j = ensure(i);
    m_data[j] |= (((uint64_t)Mask)<<((i & IndexMask)<<Shift));
  }
  inline void set(uint64_t i, uint64_t v) {
    uint64_t j = ensure(i);
    m_data[j] |= (v<<((i & IndexMask)<<Shift));
  }
  inline void clr(uint64_t i) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= (m_size<<IndexShift)) return;
    if (m_data[index(i>>IndexShift)] &=
	~(((uint64_t)Mask)<<((i & IndexMask)<<Shift))) return;
    if (i < (1U<<IndexShift)) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += (j<<IndexShift);
      }
    }
  }
  inline void clr(uint64_t i, uint64_t v) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= (m_size<<IndexShift)) return;
    if (m_data[index(i>>IndexShift)] &= ~(v<<((i & IndexMask)<<Shift))) return;
    if (i < (1U<<IndexShift)) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += (j<<IndexShift);
      }
    }
  }

  inline unsigned val(unsigned i) const {
    if (i < m_head) return 0;
    i -= m_head;
    if (i >= (m_size<<IndexShift)) return 0;
    return (m_data[index(i>>IndexShift)]>>((i & IndexMask)<<Shift)) & Mask;
  }

  inline unsigned head() const { return m_head; }
  inline unsigned tail() const { return m_head + (m_size<<IndexShift); }
  inline unsigned size() const { return m_size<<IndexShift; }

  // l(unsigned index, unsigned value) -> uintptr_t
  template <typename L>
  inline uintptr_t all(L l) {
    for (unsigned i = 0, n = m_size; i < n; i++) {
      uint64_t w = m_data[index(i)];
      if (!w) continue;
      uint64_t m = Mask;
      for (unsigned j = 0; j < (1U<<IndexShift); j++) {
	if (uint64_t v = (w & m))
	  if (uintptr_t r = l(m_head + (i<<IndexShift) + j, v>>(j<<Shift)))
	    return r;
	m <<= (1U<<Shift);
      }
    }
    return 0;
  }

private:
  uint64_t	*m_data;
  uint64_t	m_size;
  uint64_t	m_head;
  uint64_t	m_offset;
};

template <unsigned Bits_> class ZtBitWindow<Bits_, true, false> {
  ZtBitWindow(const ZtBitWindow &) = delete;
  ZtBitWindow &operator =(const ZtBitWindow &) = delete;

public:
  enum { Bits = Bits_ };

private:
  enum { Mask = (1<<Bits) - 1 };
  enum { IndexMul = ZtBitWindow_<Bits>::Mul };

public:
  inline ZtBitWindow() : m_data(nullptr), m_size(0), m_head(0), m_offset(0) { }
  inline ~ZtBitWindow() { if (m_data) ::free(m_data); }

  inline ZtBitWindow(ZtBitWindow &&q) :
      m_data(q.m_data), m_size(q.m_size),
      m_head(q.m_head), m_offset(q.m_offset) {
    q.null();
  }
  inline ZtBitWindow &operator =(ZtBitWindow &&q) {
    if (m_data) ::free(m_data);
    m_data = q.m_data;
    m_size = q.m_size;
    m_head = q.m_head;
    q.null();
    return *this;
  }

  inline void null() {
    if (m_data) ::free(m_data);
    m_data = nullptr;
    m_size = 0;
    m_head = 0;
    m_offset = 0;
  }

private:
  inline uint64_t ensure(uint64_t i) {
    if (ZuUnlikely(i < m_head)) {
      uint64_t j = ((m_head - i) + (IndexMul - 1)) / IndexMul;
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      memset(data, 0, j<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data + j, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + j + tailOffset, m_data, m_offset<<3);
      ::free(m_data);
      m_data = data;
      m_size += j;
      m_head -= j * IndexMul;
      m_offset = 0;
      return (i - m_head) / IndexMul;
    }
    i -= m_head;
    uint64_t size = m_size * IndexMul;
    if (ZuUnlikely(i >= size)) {
      uint64_t j = (((i + 1) - size) + (IndexMul - 1)) / IndexMul;
      if (j < (m_size>>3)) j = m_size>>3; // grow by at least 12.5%
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + tailOffset, m_data, m_offset<<3);
      memset(data + m_size, 0, j<<3);
      ::free(m_data);
      m_data = data;
      m_size += j;
      m_offset = 0;
      return i / IndexMul;
    }
    return index(i / IndexMul);
  }
  ZuInline uint64_t index(uint64_t i) const {
    i += m_offset;
    if (i >= m_size) i -= m_size;
    return i;
  }

public:
  inline void set(uint64_t i) {
    uint64_t j = ensure(i);
    m_data[j] |= (((uint64_t)Mask)<<((i % IndexMul) * Bits));
  }
  inline void set(uint64_t i, uint64_t v) {
    uint64_t j = ensure(i);
    m_data[j] |= (v<<((i % IndexMul) * Bits));
  }
  inline void clr(uint64_t i) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= m_size * IndexMul) return;
    if (m_data[index(i / IndexMul)] &=
	~(((uint64_t)Mask)<<((i % IndexMul) * Bits))) return;
    if (i < IndexMul) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += j * IndexMul;
      }
    }
  }
  inline void clr(uint64_t i, uint64_t v) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= m_size * IndexMul) return;
    if (m_data[index(i / IndexMul)] &= ~(v<<((i % IndexMul) * Bits))) return;
    if (i < IndexMul) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += j * IndexMul;
      }
    }
  }

  inline unsigned val(unsigned i) const {
    if (i < m_head) return 0;
    i -= m_head;
    if (i >= m_size * IndexMul) return 0;
    return (m_data[index(i / IndexMul)]>>((i % IndexMul) * Bits)) & Mask;
  }

  inline unsigned head() const { return m_head; }
  inline unsigned tail() const { return m_head + m_size * IndexMul; }
  inline unsigned size() const { return m_size * IndexMul; }

  // l(unsigned index, unsigned value) -> uintptr_t
  template <typename L>
  inline uintptr_t all(L l) {
    for (unsigned i = 0, k = 0, n = m_size; i < n; i++) {
      uint64_t w = m_data[index(i)];
      uint64_t m = Mask;
      for (unsigned j = 0, z = 0; j < IndexMul; j++, k++, z += Bits) {
	if (uint64_t v = (w & m))
	  if (uintptr_t r = l(m_head + k, v>>z))
	    return r;
	m <<= Bits;
      }
    }
    return 0;
  }

private:
  uint64_t	*m_data;
  uint64_t	m_size;
  uint64_t	m_head;
  uint64_t	m_offset;
};

template <> class ZtBitWindow<64U, true, true> {
  ZtBitWindow(const ZtBitWindow &) = delete;
  ZtBitWindow &operator =(const ZtBitWindow &) = delete;

public:
  enum { Bits = 64 };

public:
  inline ZtBitWindow() : m_data(nullptr), m_size(0), m_head(0), m_offset(0) { }
  inline ~ZtBitWindow() { if (m_data) ::free(m_data); }

  inline ZtBitWindow(ZtBitWindow &&q) :
      m_data(q.m_data), m_size(q.m_size),
      m_head(q.m_head), m_offset(q.m_offset) {
    q.null();
  }
  inline ZtBitWindow &operator =(ZtBitWindow &&q) {
    if (m_data) ::free(m_data);
    m_data = q.m_data;
    m_size = q.m_size;
    m_head = q.m_head;
    q.null();
    return *this;
  }

  inline void null() {
    if (m_data) ::free(m_data);
    m_data = nullptr;
    m_size = 0;
    m_head = 0;
    m_offset = 0;
  }

private:
  inline uint64_t ensure(uint64_t i) {
    if (ZuUnlikely(i < m_head)) {
      uint64_t j = m_head - i;
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      memset(data, 0, j<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data + j, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + j + tailOffset, m_data, m_offset<<3);
      if (m_data) ::free(m_data);
      m_data = data;
      m_size += j;
      m_head -= j;
      m_offset = 0;
      return i - m_head;
    }
    i -= m_head;
    if (ZuUnlikely(i >= m_size)) {
      uint64_t j = (i + 1) - m_size;
      if (j < (m_size>>3)) j = m_size>>3; // grow by at least 12.5%
      uint64_t *data = (uint64_t *)::malloc((j + m_size)<<3);
      uint64_t tailOffset = m_size - m_offset;
      if (tailOffset) memcpy(data, m_data + m_offset, tailOffset<<3);
      if (m_offset) memcpy(data + tailOffset, m_data, m_offset<<3);
      memset(data + m_size, 0, j<<3);
      if (m_data) ::free(m_data);
      m_data = data;
      m_size += j;
      m_offset = 0;
      return i;
    }
    return index(i);
  }
  ZuInline uint64_t index(uint64_t i) const {
    i += m_offset;
    if (i >= m_size) i -= m_size;
    return i;
  }

public:
  inline void set(uint64_t i) {
    uint64_t j = ensure(i);
    m_data[j] = ~((uint64_t)0);
  }
  inline void set(uint64_t i, uint64_t v) {
    uint64_t j = ensure(i);
    m_data[j] = v;
  }
  inline void clr(uint64_t i) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= m_size) return;
    m_data[index(i)] = 0;
    if (!i) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += j;
      }
    }
  }
  inline void clr(uint64_t i, uint64_t v) {
    if (i < m_head) return;
    i -= m_head;
    if (i >= m_size) return;
    m_data[index(i)] = 0;
    if (!i) {
      uint64_t j;
      for (j = 0; j < m_size && !m_data[index(j)]; j++);
      if (j) {
	if (j < m_size)
	  if ((m_offset += j) >= m_size) m_offset -= m_size;
	m_head += j;
      }
    }
  }

  inline uint64_t val(unsigned i) const {
    if (i < m_head) return 0;
    i -= m_head;
    if (i >= m_size) return 0;
    return m_data[index(i)];
  }

  inline unsigned head() const { return m_head; }
  inline unsigned tail() const { return m_head + m_size; }
  inline unsigned size() const { return m_size; }

  // l(unsigned index, unsigned value) -> uintptr_t
  template <typename L>
  inline uintptr_t all(L l) {
    for (unsigned i = 0, n = m_size; i < n; i++) {
      uint64_t v = m_data[index(i)];
      if (!v) continue;
      if (uintptr_t r = l(m_head + i, v)) return r;
    }
    return 0;
  }

private:
  uint64_t	*m_data;
  uint64_t	m_size;
  uint64_t	m_head;
  uint64_t	m_offset;
};

#endif /* ZtBitWindow_HPP */
