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

// "good ole' quick sort", modern C++ version
// - no STL range/iterator cruft
// - optimized three-way comparison
// - optimized array memory operations
// - median-of-three pivot
// - fallback to insertion sort for small partitions of N items or less
// - recurse smaller partition, iterate larger partition
// - minimized stack usage during recursion
// - bundled with binary and interpolation search of sorted data

// template <typename T, typename Cmp = ZuCmp<T>, unsigned N = ZuSort_::isort_N>
// ZuSort(T *data, unsigned n)
//
// data - pointer to T * data to be sorted
// n - number of contiguous elements of type T
//
// Cmp - three-way comparison - defaults to ZuCmp<T>
// N - insertion sort is used below this partition size threshold

#ifndef ZuSort_HPP
#define ZuSort_HPP

#ifndef ZuLib_HPP
#include <zlib/ZuLib.hpp>
#endif

#ifdef _MSC_VER
#pragma once
#endif

#include <zlib/ZuCmp.hpp>
#include <zlib/ZuLambdaFn.hpp>
#include <zlib/ZuArrayFn.hpp>

template <typename T, typename Cmp, unsigned N>
struct ZuSort_Fn {
  using Fn = ZuArrayFn<T>;

  static void isort_(T *base, unsigned n, Cmp &cmp) {
    T *end = base + n; // past end
    T *minimum = base, *ptr = base;
    // find minimum
    while (++ptr < end)
      if (cmp(*minimum, *ptr) > 0)
	minimum = ptr;
    // swap minimum into base
    if (minimum != base) {
      ZuSwap(*minimum, *base);
      minimum = base;
    }
    // standard insertion sort
    while ((ptr = ++minimum) < end) {
      while (cmp(*--ptr, *minimum) > 0);
      if (++ptr != minimum) {
	T tmp = ZuMv(*minimum);
	Fn::moveItems(ptr + 1, ptr, minimum - ptr);
	*ptr = ZuMv(tmp);
      }
    }
  }

  static void qsort_(T *base, unsigned n, Cmp &cmp) {
  loop:
    T *middle = base + (n>>1);
    {
      T *end = base + (n - 1); // at end (not past)
      // find pivot - median of base, middle, end
      T *ptr;
      if (cmp(*base, *middle) >= 0)
	ptr = base;
      else
	ptr = middle, middle = base;
      if (cmp(*ptr, *end) > 0)
	ptr = cmp(*middle, *end) >= 0 ? middle : end;
      // swap pivot into base (gets swapped back later)
      if (ptr != base) {
	ZuSwap(*ptr, *base);
	ptr = base;
      }
      // standard quicksort & check for flat partition
      middle = base;
      unsigned j = 1;
      int k;
      while (++ptr <= end) {
	if (!(k = cmp(*ptr, *base)))
	  j++;
	if (k < 0) {
	  if (++middle != ptr) {
	    ZuSwap(*middle, *ptr);
	  }
	}
      }
      if (j == n) return; // exit if flat
    }
    if (base != middle) ZuSwap(*base, *middle); // swap back pivot
    unsigned i = middle - base;
    if (i < (n>>1)) {
      // recurse smaller partition
      if (i > N)
	qsort_(base, i, cmp);
      else if (i > 1)
	isort_(base, i, cmp);
      ++middle;
      i = n - i - 1;
      // iterate larger partition (unless insertion sort)
      if (i > N) {
	base = middle;
	n = i;
	goto loop;
      }
    } else {
      // recurse smaller partition
      ++middle;
      i = n - i - 1;
      if (i > N)
	qsort_(middle, i, cmp);
      else if (i > 1)
	isort_(middle, i, cmp);
      middle = base;
      i = n - i - 1;
      // iterate larger partition (unless insertion sort)
      if (i > N) {
	n = i;
	goto loop;
      }
    }
    if (i > 1)
      isort_(middle, i, cmp);
  }
};

template <unsigned N = 8, typename T>
inline void ZuSort(T *data, unsigned n) {
  auto cmp = [](const T &v1, const T &v2) {
    return ZuCmp<T>::cmp(v1, v2);
  };
  using Cmp = decltype(cmp);
  using Fn = ZuSort_Fn<T, Cmp, N>;
  if (n > N)
    Fn::qsort_(data, n, cmp);
  else if (n > 1)
    Fn::isort_(data, n, cmp);
}

template <unsigned N = 8, typename T, typename Cmp>
inline void ZuSort(T *data, unsigned n, Cmp cmp) {
  using Fn = ZuSort_Fn<T, Cmp, N>;
  if (n > N)
    Fn::qsort_(data, n, cmp);
  else if (n > 1)
    Fn::isort_(data, n, cmp);
}

inline bool ZuSearchFound(unsigned i) { return i & 1; }
inline unsigned ZuSearchPos(unsigned i) { return i>>1; }

// binary search in sorted data (i.e. following ZuSort)
// - returns insertion position if not found
template <typename, typename> struct ZuSearch_OK;
template <typename R> struct ZuSearch_OK<R, int> { using T = R; }; 
template <bool Match = true, typename T, typename Cmp>
inline auto ZuSearch(T *data, unsigned n, Cmp cmp) ->
typename ZuSearch_OK<unsigned,
	 decltype(ZuDeclVal<Cmp>()(ZuDeclVal<const T &>()))>::T {
  if (!n) return 0;
  unsigned o = 0;
  int m = n>>1;
  int v1 = cmp(data[m]);
  while (n > 2) {
    n -= m;
    if (v1 > 0) {
      data += m;
      o += m;
    }
    v1 = cmp(data[m = n>>1]);
  }
  int v2;
  if (n == 2) {
    if (v1 > 0) return (o + 2)<<1;
    v2 = v1;
    v1 = cmp(data[0]);
  }
  if constexpr (Match) if (!v1) return (o<<1) | 1;
  if (v1 <= 0) return o<<1;
  if (n == 1) return (o + 1)<<1;
  if constexpr (Match) if (!v2) return ((o + 1)<<1) | 1;
  return (o + 1)<<1;
}
template <bool Match = true, typename T>
inline unsigned ZuSearch(T *data, unsigned n, const T &item) {
  return ZuSearch<Match>(data, n,
      [&item](const T &v) { return ZuCmp<T>::cmp(item, v); });
}

// interpolation search in sorted data (i.e. following ZuSort)
// - returns insertion position if not found
template <bool Match = true, typename T, typename Cmp>
auto ZuInterSearch(T *data, unsigned n, Cmp cmp) ->
typename ZuSearch_OK<unsigned,
	 decltype(ZuDeclVal<Cmp>()(ZuDeclVal<const T &>()))>::T {
  if (!n) return 0;
  if (n <= 2) { // special case for small arrays
    int v1 = cmp(data[0]);
    if constexpr (Match) if (!v1) return 1;
    if (v1 <= 0) return 0;
    if (n == 1) return 2;
    int v2 = cmp(data[1]);
    if constexpr (Match) if (!v2) return 3;
    if (v2 <= 0) return 2;
    return 4;
  }
  unsigned o = 0;
  int v1 = cmp(data[0]);
  int v2 = cmp(data[n - 1]);
  if constexpr (Match) if (!v1) return 1;
  if (v1 <= 0) return 0;
  if (v2 > 0) return n<<1;
  if (n == 3) {
    v2 = cmp(data[1]);
  } else {
    do {
      unsigned m = ((n - 3) * v1) / (v1 - v2) + 1;
      int v3 = cmp(data[m]);
      n -= m;
      if (v3 <= 0) {
	v2 = v3;
      } else {
	data += m;
	o += m;
	v1 = v3;
      }
    } while (n > 2);
    if constexpr (Match) if (!v1) return (o<<1) | 1;
    if (v1 <= 0) return o<<1;
    if (n == 1) return (o + 1)<<1;
  }
  if constexpr (Match) if (!v2) return ((o + 1)<<1) | 1;
  if (v2 <= 0) return (o + 1)<<1;
  return (o + 2)<<1;
}
template <bool Match = true, typename T>
inline typename ZuIsIntegral<T, unsigned>::T
ZuInterSearch(T *data, unsigned n, T v) {
  return ZuInterSearch<Match>(data, n,
      [v](const T &w) { return ZuCmp<T>::delta(v, w); });
}

#endif /* ZuSort_HPP */
