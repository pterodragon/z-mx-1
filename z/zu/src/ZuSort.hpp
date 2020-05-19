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
// - no STL iterator cruft
// - optimized three-way comparison
// - optimized array memory operations
// - median-of-three pivot
// - fallback to insertion sort for small partitions of N items or less
// - recurse smaller partition, iterate larger partition
// - minimized stack usage during recursion
// - bundled binary search

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
void ZuSort(T *data, unsigned n) {
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
void ZuSort(T *data, unsigned n, Cmp cmp) {
  using Fn = ZuSort_Fn<T, Cmp, N>;
  if (n > N)
    Fn::qsort_(data, n, cmp);
  else if (n > 1)
    Fn::isort_(data, n, cmp);
}

bool ZuSearchFound(unsigned i) { return i & 1; }
unsigned ZuSearchPos(unsigned i) { return i>>1; }

// binary search in sorted data (i.e. following ZuSort)
// - returns insertion position if not found
template <bool Match = true, typename T, typename Cmp>
unsigned ZuSearch(T *data, unsigned n, const T &item, Cmp cmp) {
  if (!n) return 0;
  unsigned o = 0;
loop:
  unsigned m = n>>1;
  int v = cmp(item, data[m]);
  if constexpr (Match) if (!v) return ((o + m)<<1) | 1;
  if (!m) return (o + (v >= 0))<<1;
  if (v < 0) { n = m; goto loop; }
  data += m;
  o += m;
  n -= m;
  goto loop;
}
template <bool Match = true, typename T>
unsigned ZuSearch(T *data, unsigned n, const T &item) {
  return ZuSearch<Match>(data, n, item, 
      [](const T &v1, const T &v2) { return ZuCmp<T>::cmp(v1, v2); });
}

// interpolation search in sorted data (i.e. following ZuSort)
// - returns insertion position if not found
template <bool Match = true, typename T, typename Cmp>
unsigned ZuInterSearch(T *data, unsigned n, const T &item, Cmp cmp) {
  if (!n) return 0;
  unsigned o = 0;
  int v1, v2;
  if (n <= 2) {
small1:
    v1 = cmp(item, data[0]);
    if (n == 2) v2 = cmp(item, data[1]);
small2:
    if (v1 < 0) return o<<1;
    if constexpr (Match) if (!v1) return (o<<1) | 1;
    ++o;
    if (n == 1) return o<<1;
    if constexpr (Match) if (!v2) return (o<<1) | 1;
    return (o + (v2 >= 0))<<1;
  }
  v1 = cmp(item, data[0]);
  v2 = cmp(item, data[n - 1]);
loop:
  if (v1 < 0) return 0;
  if constexpr (Match) {
    if (!v1) return (o<<1) | 1;
    if (!v2) return ((o + n - 1)<<1) | 1;
  }
  if (v2 >= 0) return (o + n)<<1;
  if (n <= 4) {
    ++data;
    ++o;
    n -= 2;
    goto small1;
  }
  unsigned m = ((n - 2) * v1) / (v1 - v2) + 1;
  int v3 = cmp(item, data[m]);
  if constexpr (Match) if (!v3) return ((o + m)<<1) | 1;
  if (v3 < 0) {
    ++data;
    ++o;
    if (m <= 1) return (o + m)<<1;
    n = m - 1;
    v2 = v3;
    if (n <= 2) goto small2;
    goto loop;
  }
  data += m + 1;
  o += m + 1;
  n -= m + 2;
  if (!n) return o<<1;
  v1 = v3;
  if (n <= 2) goto small2;
  goto loop;
}
template <bool Match = true, typename T>
unsigned ZuInterSearch(T *data, unsigned n, const T &item) {
  return ZuInterSearch<Match>(data, n, item, 
      [](const T &v1, const T &v2) { return ZuCmp<T>::cmp(v1, v2); });
}

#endif /* ZuSort_HPP */
